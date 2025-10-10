<template>
  <v-row>
    <v-col
      cols="12"
      md="6"
    >
      <v-card
        elevation="2"
        class="pa-4 mb-4"
      >
        <v-card-title class="text-h6">1) Connect to Tower AP</v-card-title>
        <v-card-text>
          <p>
            Power a new dice tower. It will create a Wi‑Fi AP named like
            <code>DiceTower-XXYY</code> (password <code>dungeon123</code>).
            Temporarily connect your PC/phone to that network.
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
        <v-card-title class="text-h6 pa-4">3) Provision Wi‑Fi</v-card-title>
        <div style="flex: 1; overflow-y: auto; padding: 16px">
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
          <v-text-field
            v-model="towerName"
            label="Tower Name (optional)"
            placeholder="e.g., Player 1 Tower"
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
        <v-card-title class="text-h6">4) Control (STA mode)</v-card-title>
        <v-card-text>
          <p class="mb-4">
            After provisioning, reconnect your PC to your home Wi‑Fi. The tower
            will join and get an IP (shown in the provision response). Enter it
            below and control the LED.
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
              Store multiple tower URLs in bookmarks; mDNS discovery can be
              added later.
            </li>
            <li>
              Your Wi‑Fi credentials are stored on the device flash, not in this
              app.
            </li>
            <li>
              To re-provision, hold the device button (if added) or erase creds
              via API (coming soon).
            </li>
          </ul>
        </v-card-text>
      </v-card>
    </v-col>
  </v-row>
</template>

<script setup>
  import { ref, watch } from 'vue';
  import { useTowerStore } from '../stores/towerStore';

  const towerStore = useTowerStore();
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

  async function provision() {
    try {
      loading.value.provision = true;
      const url = `${apUrl.value}/provision?ssid=${encodeURIComponent(
        homeSsid.value
      )}&pass=${encodeURIComponent(homePass.value)}&name=${encodeURIComponent(
        towerName.value
      )}`;

      const res = await fetch(url, {
        cache: 'no-store',
      });

      const text = await res.text();
      provisionResult.value = `Provision → ${res.status}: ${text}`;

      try {
        const parsed = JSON.parse(text);
        if (parsed && parsed.connected && parsed.ip) {
          // Add micro-delay to handle AP disconnect
          await new Promise(resolve => setTimeout(resolve, 2000));

          const saved = towerStore.addOrUpdateTower({
            url: `http://${parsed.ip}`,
            name: parsed.name || towerName.value || 'Tower',
            online: true, // Mark as online since provision was successful
          });

          // Update the name if we got a different one from the device
          if (parsed.name && parsed.name !== towerName.value) {
            towerStore.addOrUpdateTower({
              id: saved.id,
              name: parsed.name,
            });
          }
          deviceUrl.value = saved.url;

          // Emit event to switch to manage tab
          emit('provision-success');

          // Verify name was saved by checking the device
          setTimeout(async () => {
            try {
              const nameRes = await fetch(`${saved.url}/name?action=get`, {
                cache: 'no-store',
              });
              const nameText = await nameRes.text();
              const nameData = JSON.parse(nameText);
              if (nameData.name) {
                provisionResult.value += `\n✅ Name saved: "${nameData.name}"`;
              }
            } catch (e) {
              provisionResult.value += `\n⚠️ Could not verify name: ${e}`;
            }
          }, 3000);
        }
      } catch (_) {
        // non-JSON response; ignore
      }
    } catch (e) {
      if (
        e.message.includes('ERR_CONNECTION_RESET') ||
        e.message.includes('Failed to fetch')
      ) {
        provisionResult.value = `Provision completed (connection reset is normal when device switches to home Wi-Fi)`;
        // Still try to add the tower if we have the expected IP
        if (homeSsid.value) {
          provisionResult.value += `\n⚠️ Check if device connected to "${homeSsid.value}" and try Step 4`;
        }
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

      const res = await fetch(url, {
        cache: 'no-store',
      });

      const text = await res.text();
      statusText.value = `GET ${path} → ${res.status}: ${text}`;
    } catch (e) {
      statusText.value = `Error calling ${path}: ${e}`;
    } finally {
      loading.value[which] = false;
    }
  }

  async function callAp(path, which) {
    try {
      loading.value[which] = true;
      const url = `${apUrl.value}${path}`;

      const res = await fetch(url, {
        cache: 'no-store',
      });

      const text = await res.text();
      apStatusText.value = `AP ${path} → ${res.status}: ${text}`;
    } catch (e) {
      apStatusText.value = `AP error ${path}: ${e}`;
    } finally {
      loading.value[which] = false;
    }
  }

  const emit = defineEmits(['provision-success']);
</script>
