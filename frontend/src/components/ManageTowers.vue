<template>
  <v-row>
    <v-col cols="12">
      <v-card
        elevation="2"
        class="pa-4 mb-4"
      >
        <v-card-title class="text-h6">Known Towers</v-card-title>
        <v-card-text>
          <div v-if="towersToUse.length === 0">No towers saved yet.</div>
          <v-list v-else>
            <v-list-item
              v-for="t in towersToUse"
              :key="t.id"
            >
              <template #prepend>
                <div class="d-flex align-center">
                  <v-icon
                    :icon="t.online ? 'mdi-lan-connect' : 'mdi-lan-disconnect'"
                    :color="t.online ? 'success' : 'error'"
                    class="mr-2"
                  />
                  <v-chip
                    :color="t.online ? 'success' : 'error'"
                    size="x-small"
                    variant="elevated"
                  >
                    {{ t.online ? 'ONLINE' : 'OFFLINE' }}
                  </v-chip>
                </div>
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
                  :loading="loading[`${t.id}-status`]"
                  @click="testTower(t)"
                />
                <v-btn
                  class="mr-2"
                  color="primary"
                  size="small"
                  text="On"
                  :loading="loading[`${t.id}-on`]"
                  @click="callOn(t, '/strip/on')"
                />
                <v-btn
                  class="mr-2"
                  color="secondary"
                  size="small"
                  text="Off"
                  :loading="loading[`${t.id}-off`]"
                  @click="callOn(t, '/strip/off')"
                />
                <v-btn
                  class="mr-2"
                  variant="tonal"
                  size="small"
                  text="Mode"
                  :loading="loading[`${t.id}-mode`]"
                  @click="callOn(t, '/strip/mode/toggle')"
                />
                <v-btn
                  variant="outlined"
                  size="small"
                  text="Check Name"
                  :loading="loading[`${t.id}-name`]"
                  @click="checkTowerName(t)"
                />
                <v-btn
                  class="mr-2"
                  color="success"
                  size="small"
                  text="Roll Dice"
                  :loading="loading[`${t.id}-dice`]"
                  @click="captureDice(t)"
                />
                <v-btn
                  class="mr-2"
                  color="info"
                  size="small"
                  text="Camera"
                  @click="showCameraControls(t)"
                />
              </template>
            </v-list-item>
            <v-list-item
              v-if="diceResults[t.id]"
              class="mt-2"
            >
              <v-alert
                type="success"
                variant="tonal"
                class="w-100"
              >
                <div class="d-flex align-center">
                  <v-icon
                    icon="mdi-dice-multiple"
                    class="mr-2"
                    size="large"
                  />
                  <div>
                    <strong>Dice Roll Result:</strong>
                    {{ diceResults[t.id].value }}
                    <div
                      v-if="diceResults[t.id].timestamp"
                      class="text-caption"
                    >
                      Rolled at
                      {{ formatTimestamp(diceResults[t.id].timestamp) }}
                    </div>
                  </div>
                </div>
              </v-alert>
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
      
      <!-- Camera Controls Dialog -->
      <v-dialog
        v-model="showCameraDialog"
        max-width="800px"
      >
        <v-card>
          <v-card-title>
            <span class="text-h6">Camera Controls</span>
            <v-spacer></v-spacer>
            <v-btn
              icon="mdi-close"
              variant="text"
              @click="showCameraDialog = false"
            />
          </v-card-title>
          <v-card-text>
            <CameraControls
              v-if="selectedTower"
              :tower="selectedTower"
            />
          </v-card-text>
        </v-card>
      </v-dialog>
    </v-col>
  </v-row>
</template>

<script setup>
  import { ref, onMounted, watch, nextTick } from 'vue';
  import { useTowerStore } from '../stores/towerStore';
  import { storeToRefs } from 'pinia';
  import CameraControls from './CameraControls.vue';

  const towerStore = useTowerStore();
  const { towers: towersToUse } = storeToRefs(towerStore);

  // Clean invalid towers on component mount
  towerStore.cleanInvalidTowers();
  const manageStatus = ref('');
  const editingName = ref(null);
  const editingNameValue = ref('');
  const loading = ref({});
  const diceResults = ref({}); // { towerId: { value, timestamp, detected } }
  let statusCheckTimeout = null;
  const showCameraDialog = ref(false);
  const selectedTower = ref(null);

  // Initialize loading states for all possible tower actions
  function initializeLoadingStates() {
    if (towersToUse.value && Array.isArray(towersToUse.value)) {
      towersToUse.value.forEach(tower => {
        const actions = ['status', 'on', 'off', 'mode', 'name', 'dice'];
        actions.forEach(action => {
          const key = `${tower.id}-${action}`;
          if (!(key in loading.value)) {
            loading.value[key] = false;
          }
        });
      });
    }
  }

  // Debounced status check function
  function debouncedStatusCheck() {
    if (statusCheckTimeout) {
      clearTimeout(statusCheckTimeout);
    }
    statusCheckTimeout = setTimeout(() => {
      checkAllTowerStatusAndNames();
    }, 100);
  }

  // Watch for towers changes and initialize loading states when towers are loaded
  watch(
    towersToUse,
    newTowers => {
      if (newTowers && newTowers.length > 0) {
        initializeLoadingStates();
        debouncedStatusCheck();
      }
    },
    { immediate: true }
  );

  async function checkAllTowerStatusAndNames() {
    if (!towersToUse.value || !Array.isArray(towersToUse.value)) {
      return;
    }
    for (const tower of towersToUse.value) {
      // Set loading state for status button
      const loadingKey = `${tower.id}-status`;
      loading.value[loadingKey] = true;
      await checkTowerStatusAndName(tower);
      // Clear loading state
      loading.value[loadingKey] = false;
    }
  }

  async function checkTowerStatusAndName(tower) {
    console.log(
      `🔄 ManageTowers: checkTowerStatusAndName called for ${tower.name}`
    );
    try {
      // Check status first
      console.log(`🔄 ManageTowers: Fetching status from ${tower.url}/status`);
      const statusRes = await fetch(`${tower.url}/status`, {
        cache: 'no-store',
      });

      if (statusRes.ok) {
        const statusText = await statusRes.text();
        console.log(
          `🔄 ManageTowers: Status OK for ${tower.name}: ${statusText}`
        );

        // Tower is online, now get the name
        try {
          console.log(
            `🔄 ManageTowers: Fetching name from ${tower.url}/name?action=get`
          );
          const nameRes = await fetch(`${tower.url}/name?action=get`, {
            cache: 'no-store',
          });
          const nameText = await nameRes.text();
          const nameData = JSON.parse(nameText);
          console.log(
            `🔄 ManageTowers: Name data for ${tower.name}:`,
            nameData
          );

          // Update both online status and name
          towerStore.addOrUpdateTower({
            id: tower.id,
            online: true,
            name: nameData.name || tower.name,
          });
          console.log(
            `🔄 ManageTowers: Updated ${tower.name} to online with name: ${
              nameData.name || tower.name
            }`
          );
        } catch (e) {
          // Name check failed, but status was OK
          console.log(
            `🔄 ManageTowers: Name check failed for ${tower.name}:`,
            e
          );
          towerStore.addOrUpdateTower({ id: tower.id, online: true });
        }
      } else {
        console.log(
          `🔄 ManageTowers: Status failed for ${tower.name}: ${statusRes.status}`
        );
        towerStore.addOrUpdateTower({ id: tower.id, online: false });
      }
    } catch (e) {
      console.log(`🔄 ManageTowers: Error checking ${tower.name}:`, e);
      towerStore.addOrUpdateTower({ id: tower.id, online: false });
    }
  }

  async function testTower(tower) {
    // Set loading state for status button
    const loadingKey = `${tower.id}-status`;
    loading.value[loadingKey] = true;

    try {
      // Check status and name together
      await checkTowerStatusAndName(tower);
    } finally {
      // Clear loading state
      loading.value[loadingKey] = false;
    }
  }

  async function callOn(tower, path) {
    const action = path.split('/').pop(); // Extract action from path (e.g., 'on', 'off', 'toggle', 'status')
    const loadingKey = `${tower.id}-${action}`;

    try {
      loading.value[loadingKey] = true;
      const res = await fetch(`${tower.url}${path}`, {
        cache: 'no-store',
      });

      const text = await res.text();
      manageStatus.value = `${tower.name} ${path} → ${res.status}: ${text}`;
      // Mark as online since command executed successfully
      towerStore.addOrUpdateTower({
        id: tower.id,
        url: tower.url,
        online: true,
      });
    } catch (e) {
      manageStatus.value = `${tower.name} ${path} error: ${e}`;
      // Mark as offline on error
      towerStore.addOrUpdateTower({ id: tower.id, online: false });
    } finally {
      loading.value[loadingKey] = false;
    }
  }

  async function checkTowerName(tower) {
    const loadingKey = `${tower.id}-name`;

    try {
      loading.value[loadingKey] = true;
      const res = await fetch(`${tower.url}/name?action=get`, {
        cache: 'no-store',
      });

      const text = await res.text();
      const data = JSON.parse(text);
      if (data.name) {
        // Update the tower name if it's different
        if (data.name !== tower.name) {
          towerStore.addOrUpdateTower({
            id: tower.id,
            name: data.name,
            online: true, // Mark as online since we got a response
          });
          manageStatus.value = `${tower.name} → Updated name to: "${data.name}"`;
        } else {
          manageStatus.value = `${tower.name} → Name confirmed: "${data.name}"`;
        }
      } else {
        manageStatus.value = `${tower.name} → No name saved on device`;
      }
    } catch (e) {
      manageStatus.value = `${tower.name} name check error: ${e}`;
    } finally {
      loading.value[loadingKey] = false;
    }
  }

  async function captureDice(tower) {
    const loadingKey = `${tower.id}-dice`;

    try {
      loading.value[loadingKey] = true;
      const res = await fetch(`${tower.url}/dice/capture`, {
        cache: 'no-store',
      });

      if (!res.ok) {
        const errorText = await res.text();
        manageStatus.value = `${tower.name} dice capture failed: ${errorText}`;
        return;
      }

      const text = await res.text();
      const data = JSON.parse(text);

      if (data.ok && data.detected) {
        diceResults.value[tower.id] = {
          value: data.value,
          timestamp: data.timestamp,
          detected: data.detected,
        };
        manageStatus.value = `${tower.name} → Dice rolled: ${data.value}!`;
        // Mark tower as online
        towerStore.addOrUpdateTower({
          id: tower.id,
          online: true,
        });
      } else {
        manageStatus.value = `${tower.name} → No dice detected. Try rolling again.`;
        diceResults.value[tower.id] = {
          value: 0,
          timestamp: data.timestamp || Date.now(),
          detected: false,
        };
      }
    } catch (e) {
      manageStatus.value = `${tower.name} dice capture error: ${e}`;
      console.error('Dice capture error:', e);
    } finally {
      loading.value[loadingKey] = false;
    }
  }

  function formatTimestamp(timestamp) {
    if (!timestamp) return '';
    const date = new Date(timestamp);
    return date.toLocaleTimeString();
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
      const res = await fetch(
        `${tower.url}/name?action=set&name=${encodeURIComponent(
          editingNameValue.value
        )}`,
        {
          cache: 'no-store',
        }
      );

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

  function showCameraControls(tower) {
    selectedTower.value = tower;
    showCameraDialog.value = true;
  }

  // Expose the check function for parent component
  defineExpose({
    checkAllTowerStatusAndNames: debouncedStatusCheck,
  });
</script>
