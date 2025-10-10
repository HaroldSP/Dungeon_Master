<template>
  <div>
    <v-app-bar
      app
      color="surface"
      elevation="1"
      class="dice-towers-toolbar"
    >
      <v-app-bar-title>
        <span class="text-h6">Dice Towers</span>
      </v-app-bar-title>
      <v-spacer></v-spacer>
      <v-tabs
        v-model="activeTab"
        class="d-flex"
      >
        <v-tab value="manage">Manage Towers</v-tab>
        <v-tab value="add">Add New</v-tab>
      </v-tabs>
    </v-app-bar>

    <v-container
      fluid
      class="dice-towers-view pa-4"
    >
      <v-window v-model="activeTab">
        <v-window-item value="manage">
          <v-row>
            <v-col cols="12">
              <v-card
                elevation="2"
                class="pa-4 mb-4"
              >
                <v-card-title class="text-h6">Known Towers</v-card-title>
                <v-card-text>
                  <div v-if="towers.length === 0">No towers saved yet.</div>
                  <v-list v-else>
                    <v-list-item
                      v-for="t in towers"
                      :key="t.id"
                    >
                      <template #prepend>
                        <v-icon icon="mdi-lan-connect" />
                      </template>
                      <v-list-item-title>
                        <v-text-field
                          v-if="editingName === t.id"
                          v-model="editingNameValue"
                          density="compact"
                          variant="outlined"
                          @blur="saveName(t)"
                          @keyup.enter="saveName(t)"
                          @keyup.escape="cancelEdit()"
                          autofocus
                        />
                        <span
                          v-else
                          @click="startEdit(t)"
                          >{{ t.name }}</span
                        >
                      </v-list-item-title>
                      <v-list-item-subtitle>{{ t.url }}</v-list-item-subtitle>
                      <template #append>
                        <v-btn
                          class="mr-2"
                          size="small"
                          text="Status"
                          @click="testTower(t)"
                        />
                        <v-btn
                          class="mr-2"
                          color="primary"
                          size="small"
                          text="On"
                          @click="callOn(t, '/blink/on')"
                        />
                        <v-btn
                          class="mr-2"
                          color="secondary"
                          size="small"
                          text="Off"
                          @click="callOn(t, '/blink/off')"
                        />
                        <v-btn
                          variant="tonal"
                          size="small"
                          text="Toggle"
                          @click="callOn(t, '/blink/toggle')"
                        />
                      </template>
                    </v-list-item>
                  </v-list>
                  <v-alert
                    v-if="manageStatus"
                    type="info"
                    variant="tonal"
                    class="mt-3"
                    >{{ manageStatus }}</v-alert
                  >
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-window-item>
        <v-window-item value="add">
          <v-row>
            <v-col
              cols="12"
              md="6"
            >
              <v-card
                elevation="2"
                class="pa-4 mb-4"
              >
                <v-card-title class="text-h6"
                  >1) Connect to Tower AP</v-card-title
                >
                <v-card-text>
                  <p>
                    Power a new dice tower. It will create a Wi‑Fi AP named like
                    <code>DiceTower-XXYY</code> (password
                    <code>dungeon123</code>). Temporarily connect your PC/phone
                    to that network.
                  </p>
                </v-card-text>
              </v-card>

              <v-card
                elevation="2"
                class="pa-4 mb-4"
                min-height="300"
              >
                <v-card-title class="text-h6"
                  >2) Control via AP (sanity check)</v-card-title
                >
                <v-card-text>
                  <v-text-field
                    v-model="apUrl"
                    label="Tower URL (AP mode)"
                    placeholder="http://192.168.4.1"
                    variant="outlined"
                    density="comfortable"
                  />
                  <div class="d-flex ga-2 mb-2">
                    <v-btn
                      color="primary"
                      :loading="loading.apOn"
                      text="Blink ON"
                      @click="callAp('/blink/on', 'apOn')"
                    />
                    <v-btn
                      color="secondary"
                      :loading="loading.apOff"
                      text="Blink OFF"
                      @click="callAp('/blink/off', 'apOff')"
                    />
                    <v-btn
                      variant="tonal"
                      :loading="loading.apToggle"
                      text="Toggle"
                      @click="callAp('/blink/toggle', 'apToggle')"
                    />
                    <v-btn
                      variant="text"
                      :loading="loading.apStatus"
                      text="Status"
                      @click="callAp('/status', 'apStatus')"
                    />
                  </div>
                  <v-alert
                    v-if="apStatusText"
                    type="info"
                    variant="tonal"
                    >{{ apStatusText }}</v-alert
                  >
                </v-card-text>
              </v-card>

              <v-card
                elevation="2"
                class="pa-4 mb-14"
                style="height: 500px; display: flex; flex-direction: column"
              >
                <v-card-title class="text-h6 pa-4"
                  >3) Provision Wi‑Fi</v-card-title
                >
                <div style="flex: 1; overflow-y: auto; padding: 16px">
                  <v-text-field
                    v-model="towerName"
                    label="Tower Name (optional)"
                    placeholder="e.g., Player 1 Tower"
                    variant="outlined"
                    density="comfortable"
                    class="mb-2"
                  />
                  <v-text-field
                    v-model="homeSsid"
                    label="Your Wi‑Fi SSID"
                    variant="outlined"
                    density="comfortable"
                    class="mb-2"
                  />
                  <v-text-field
                    v-model="homePass"
                    label="Your Wi‑Fi Password"
                    type="password"
                    variant="outlined"
                    density="comfortable"
                    class="mb-2"
                  />
                  <v-btn
                    color="primary"
                    :loading="loading.provision"
                    text="Send Credentials"
                    @click="provision"
                    class="mb-3"
                  />
                  <v-alert
                    v-if="provisionResult"
                    type="info"
                    variant="tonal"
                    >{{ provisionResult }}</v-alert
                  >
                </div>
              </v-card>
            </v-col>

            <v-col
              cols="12"
              md="6"
            >
              <v-card
                elevation="2"
                class="pa-4 mb-4"
                min-height="300"
              >
                <v-card-title class="text-h6"
                  >4) Control (STA mode)</v-card-title
                >
                <v-card-text>
                  <p class="mb-4">
                    After provisioning, reconnect your PC to your home Wi‑Fi.
                    The tower will join and get an IP (shown in the provision
                    response). Enter it below and control the LED.
                  </p>
                  <v-text-field
                    v-model="deviceUrl"
                    label="Tower URL (Home Wi‑Fi)"
                    placeholder="http://192.168.1.50"
                    variant="outlined"
                    density="comfortable"
                  />
                  <div class="d-flex ga-2 mb-2">
                    <v-btn
                      color="primary"
                      :loading="loading.on"
                      text="Blink ON"
                      @click="call('/blink/on', 'on')"
                    />
                    <v-btn
                      color="secondary"
                      :loading="loading.off"
                      text="Blink OFF"
                      @click="call('/blink/off', 'off')"
                    />
                    <v-btn
                      variant="tonal"
                      :loading="loading.toggle"
                      text="Toggle"
                      @click="call('/blink/toggle', 'toggle')"
                    />
                  </div>
                  <v-alert
                    v-if="statusText"
                    type="info"
                    variant="tonal"
                    >{{ statusText }}</v-alert
                  >
                </v-card-text>
              </v-card>

              <v-card
                elevation="2"
                class="pa-4"
              >
                <v-card-title class="text-h6">Tips</v-card-title>
                <v-card-text>
                  <ul>
                    <li>
                      Store multiple tower URLs in bookmarks; mDNS discovery can
                      be added later.
                    </li>
                    <li>
                      Your Wi‑Fi credentials are stored on the device flash, not
                      in this app.
                    </li>
                    <li>
                      To re-provision, hold the device button (if added) or
                      erase creds via API (coming soon).
                    </li>
                  </ul>
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-window-item>
      </v-window>
    </v-container>
  </div>
</template>

<script setup>
  import { ref, watch, onMounted } from 'vue';
  import { useTowerStore } from '../stores/towerStore';

  const activeTab = ref('manage');
  const apUrl = ref(
    localStorage.getItem('diceTowerApUrl') || 'http://192.168.4.1'
  );
  const deviceUrl = ref(localStorage.getItem('diceTowerStaUrl') || '');
  const towerName = ref('');
  const homeSsid = ref('');
  const homePass = ref('');
  const provisionResult = ref('');
  const statusText = ref('');
  const apStatusText = ref('');
  const manageStatus = ref('');
  const editingName = ref(null);
  const editingNameValue = ref('');
  const loading = ref({
    provision: false,
    on: false,
    off: false,
    toggle: false,
    apOn: false,
    apOff: false,
    apToggle: false,
    apStatus: false,
  });

  watch(apUrl, v => localStorage.setItem('diceTowerApUrl', v));
  watch(deviceUrl, v => localStorage.setItem('diceTowerStaUrl', v));
  const towerStore = useTowerStore();
  const towers = towerStore.towers;

  async function provision() {
    try {
      loading.value.provision = true;
      const url = `${apUrl.value}/provision?ssid=${encodeURIComponent(
        homeSsid.value
      )}&pass=${encodeURIComponent(homePass.value)}&name=${encodeURIComponent(
        towerName.value
      )}`;

      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 10000);

      const res = await fetch(url, {
        signal: controller.signal,
        cache: 'no-store',
      });
      clearTimeout(timeoutId);

      const text = await res.text();
      provisionResult.value = `Provision → ${res.status}: ${text}`;

      try {
        const parsed = JSON.parse(text);
        if (parsed && parsed.connected && parsed.ip) {
          // Add micro-delay to handle AP disconnect
          await new Promise(resolve => setTimeout(resolve, 1000));

          const saved = towerStore.addOrUpdateTower({
            url: `http://${parsed.ip}`,
            name: parsed.name || towerName.value || 'Tower',
          });
          deviceUrl.value = saved.url;
          activeTab.value = 'manage';
        }
      } catch (_) {
        // non-JSON response; ignore
      }
    } catch (e) {
      if (e.name === 'AbortError') {
        provisionResult.value = `Provision timeout (10s) - check if device connected to Wi-Fi`;
      } else {
        provisionResult.value = `Provision error: ${e}`;
      }
    } finally {
      loading.value.provision = false;
    }
  }

  async function call(path, which) {
    try {
      loading.value[which] = true;
      const url = `${deviceUrl.value}${path}`;

      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 10000);

      const res = await fetch(url, {
        signal: controller.signal,
        cache: 'no-store',
      });
      clearTimeout(timeoutId);

      const text = await res.text();
      statusText.value = `GET ${path} → ${res.status}: ${text}`;
    } catch (e) {
      if (e.name === 'AbortError') {
        statusText.value = `Timeout calling ${path} (10s)`;
      } else {
        statusText.value = `Error calling ${path}: ${e}`;
      }
    } finally {
      loading.value[which] = false;
    }
  }

  async function callOn(tower, path) {
    try {
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 10000);

      const res = await fetch(`${tower.url}${path}`, {
        signal: controller.signal,
        cache: 'no-store',
      });
      clearTimeout(timeoutId);

      const text = await res.text();
      manageStatus.value = `${tower.name} ${path} → ${res.status}: ${text}`;
      towerStore.addOrUpdateTower({ id: tower.id, url: tower.url });
    } catch (e) {
      if (e.name === 'AbortError') {
        manageStatus.value = `${tower.name} ${path} timeout (10s)`;
      } else {
        manageStatus.value = `${tower.name} ${path} error: ${e}`;
      }
    }
  }

  async function testTower(tower) {
    await callOn(tower, '/status');
  }

  function startEdit(tower) {
    editingName.value = tower.id;
    editingNameValue.value = tower.name;
  }

  function cancelEdit() {
    editingName.value = null;
    editingNameValue.value = '';
  }

  async function saveName(tower) {
    if (editingNameValue.value.trim() === '') return;

    try {
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 10000);

      const res = await fetch(
        `${tower.url}/name?action=set&name=${encodeURIComponent(
          editingNameValue.value
        )}`,
        {
          signal: controller.signal,
          cache: 'no-store',
        }
      );
      clearTimeout(timeoutId);

      if (res.ok) {
        towerStore.addOrUpdateTower({
          id: tower.id,
          name: editingNameValue.value,
        });
        manageStatus.value = `Name updated to "${editingNameValue.value}"`;
      } else {
        manageStatus.value = `Failed to update name: ${res.status}`;
      }
    } catch (e) {
      manageStatus.value = `Error updating name: ${e}`;
    }

    cancelEdit();
  }

  async function callAp(path, which) {
    try {
      loading.value[which] = true;
      const url = `${apUrl.value}${path}`;

      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 10000);

      const res = await fetch(url, {
        signal: controller.signal,
        cache: 'no-store',
      });
      clearTimeout(timeoutId);

      const text = await res.text();
      apStatusText.value = `AP ${path} → ${res.status}: ${text}`;
    } catch (e) {
      if (e.name === 'AbortError') {
        apStatusText.value = `AP timeout ${path} (10s)`;
      } else {
        apStatusText.value = `AP error ${path}: ${e}`;
      }
    } finally {
      loading.value[which] = false;
    }
  }
</script>

<style scoped>
  .dice-towers-view {
    height: 100vh;
    overflow-y: auto;
  }

  ul {
    padding-left: 18px;
  }
</style>
