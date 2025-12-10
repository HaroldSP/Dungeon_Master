<template>
<v-container fluid class="pa-0">
  <v-row v-if="showHeader" class="mb-4" align="center">
    <v-col cols="12" md="8">
      <div class="text-h6">Dice Towers</div>
      <div class="text-body-2 text-medium-emphasis">
        Manage existing towers. Use "Add New" tab to create a tower.
      </div>
    </v-col>
  </v-row>

    <v-row v-if="towers.length === 0">
      <v-col cols="12">
        <v-alert type="info" variant="tonal">No towers yet. Use the Add New tab to create one.</v-alert>
      </v-col>
    </v-row>

    <v-row>
      <v-col v-for="(t, index) in towers" :key="t.id" cols="12">
        <v-card elevation="2" class="pa-3">
          <div class="d-flex justify-space-between align-center mb-2">
            <div>
              <div class="text-subtitle-1">Tower {{ index + 1 }} — {{ t.towerName || 'Tower' }}</div>
              <div class="text-body-2 text-medium-emphasis">Player: {{ t.playerName || 'Player' }}</div>
            </div>
            <v-chip :color="t.online ? 'success' : 'error'" size="small" variant="flat">
              {{ t.online ? 'ONLINE' : 'OFFLINE' }}
            </v-chip>
          </div>
          <div class="d-flex flex-wrap gap-2 mb-2">
            <v-chip size="small" variant="outlined" v-if="t.lastStaIp">STA {{ t.lastStaIp }}</v-chip>
            <v-chip size="small" variant="outlined" v-if="t.lastApIp">AP {{ t.lastApIp }}</v-chip>
          </div>
          <div class="text-body-2 mb-1">ESP32-CAM API: {{ t.apiBase || '—' }}</div>
          <div class="text-body-2 mb-1">Stream: {{ t.streamUrl || '—' }}</div>
          <div class="text-body-2 mb-1">NodeMCU API: {{ t.nodemcuApiBase || '—' }}</div>
          <div class="text-caption text-medium-emphasis mb-2">
            Last seen: {{ t.lastSeenAt ? new Date(t.lastSeenAt).toLocaleTimeString() : '—' }}
          </div>

          <v-divider class="my-2" />

          <div class="d-flex flex-wrap gap-2">
            <v-btn size="small" @click="pingStatus(t)" :loading="loading[`${t.id}-status`]" text="Status" />
            <v-btn size="small" color="success" @click="captureOnce(t)" :loading="loading[`${t.id}-capture`]" text="Capture" />
            <v-btn size="small" variant="tonal" @click="toggleExpand(t.id)" text="Details" />
            <v-btn size="small" variant="outlined" color="error" @click="remove(t.id)" text="Remove" />
          </div>

          <v-expand-transition>
            <div v-show="expanded[t.id]" class="mt-3">
              <div class="text-body-2 mb-1">Live Stream</div>
              <div class="stream-wrapper">
                <img
                  v-if="t.streamUrl"
                  :key="`${t.id}-${streamKey}`"
                  :src="`${t.streamUrl}?t=${Date.now()}`"
                  alt="Stream"
                  class="stream-image"
                />
                <div v-else class="text-caption text-medium-emphasis">No stream URL set</div>
              </div>
              <div class="d-flex gap-2 mt-2">
                <v-btn size="small" color="primary" @click="refreshStream" text="Refresh" />
                <v-btn size="small" variant="tonal" @click="pingStatus(t)" text="Status" />
                <v-btn size="small" variant="outlined" @click="whoamiEsp(t)" text="Who am I" />
              </div>

              <v-divider class="my-3" />
              <div class="text-body-2 mb-1">LED / NodeMCU controls (optional)</div>
              <div class="d-flex flex-wrap gap-2">
                <v-btn size="small" color="primary" @click="callNode(t, '/strip/on')" text="On" />
                <v-btn size="small" color="secondary" @click="callNode(t, '/strip/off')" text="Off" />
                <v-btn size="small" variant="tonal" @click="callNode(t, '/strip/mode/toggle')" text="Mode" />
                <v-btn size="small" variant="outlined" @click="whoamiNode(t)" text="Who am I" />
              </div>

              <v-alert v-if="t.lastDetection" class="mt-2" density="compact" variant="tonal" type="success">
                Last roll: {{ t.lastDetection.value }} (conf {{ t.lastDetection.confidence?.toFixed?.(3) ?? '—' }})
              </v-alert>
            </div>
          </v-expand-transition>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
import { ref } from 'vue';
import { useTowerStore } from '../stores/towerStore';
import { storeToRefs } from 'pinia';

const props = defineProps({
  showHeader: {
    type: Boolean,
    default: true,
  },
});

const towerStore = useTowerStore();
const { towers } = storeToRefs(towerStore);

const loading = ref({});
const streamKey = ref(0);
const expanded = ref({});

async function pingStatus(tower) {
  if (!tower?.apiBase) return;
  loading.value[`${tower.id}-status`] = true;
  try {
    const res = await fetch(`${tower.apiBase}/status`, { cache: 'no-store' });
    const ok = res.ok;
    let data = null;
    if (ok) data = await res.json();
    towerStore.setStatus(tower.id, {
      online: ok,
      dice: data?.dice,
      raw: data,
    });
  } catch (e) {
    towerStore.setStatus(tower.id, { online: false });
    console.error('status error', e);
  } finally {
    loading.value[`${tower.id}-status`] = false;
  }
}

async function captureOnce(tower) {
  if (!tower?.apiBase) return;
  loading.value[`${tower.id}-capture`] = true;
  try {
    const res = await fetch(`${tower.apiBase}/dice/capture`, { cache: 'no-store' });
    if (res.ok) {
      const data = await res.json();
      towerStore.setStatus(tower.id, {
        online: true,
        dice: data,
      });
    }
  } catch (e) {
    console.error('capture error', e);
  } finally {
    loading.value[`${tower.id}-capture`] = false;
  }
}

function refreshStream() {
  streamKey.value += 1;
}

function remove(id) {
  towerStore.removeTower(id);
}

function toggleExpand(id) {
  expanded.value[id] = !expanded.value[id];
}

async function callNode(tower, path) {
  if (!tower?.nodemcuApiBase) return;
  try {
    await fetch(`${tower.nodemcuApiBase}${path}`, { cache: 'no-store' });
  } catch (e) {
    console.error('node call error', e);
  }
}

async function whoamiEsp(tower) {
  if (!tower?.apiBase) return;
  try {
    const res = await fetch(`${tower.apiBase}/whoami`, { cache: 'no-store' });
    if (!res.ok) return;
    const data = await res.json();
    towerStore.setStatus(tower.id, {
      online: true,
      raw: data,
    });
  } catch (e) {
    console.error('whoami esp error', e);
  }
}

async function whoamiNode(tower) {
  if (!tower?.nodemcuApiBase) return;
  try {
    const res = await fetch(`${tower.nodemcuApiBase}/whoami`, { cache: 'no-store' });
    if (!res.ok) return;
    const data = await res.json();
    towerStore.setStatus(tower.id, {
      online: true,
      raw: data,
    });
  } catch (e) {
    console.error('whoami node error', e);
  }
}
</script>

<style scoped>
.stream-wrapper {
  width: 100%;
  background: #111;
  border-radius: 8px;
  min-height: 240px;
  display: flex;
  align-items: center;
  justify-content: center;
}
.stream-image {
  max-width: 100%;
  border-radius: 6px;
}
.gap-2 {
  gap: 8px;
}
</style>

