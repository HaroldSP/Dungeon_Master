<template>
  <div class="tower-grid-root">
    <v-container
      fluid
      class="pa-0"
    >
      <v-row
        v-if="showHeader"
        class="mb-4"
        align="center"
      >
        <v-col
          cols="12"
          md="8"
        >
          <div class="text-h6">Dice Towers</div>
          <div class="text-body-2 text-medium-emphasis">
            Manage existing towers. Use "Add New" tab to create a tower.
          </div>
        </v-col>
      </v-row>

      <v-row v-if="towers.length === 0">
        <v-col cols="12">
          <v-alert
            type="info"
            variant="tonal"
            >No towers yet. Use the Add New tab to create one.</v-alert
          >
        </v-col>
      </v-row>

      <v-row>
        <v-col
          v-for="(t, index) in towers"
          :key="t.id"
          cols="12"
        >
          <v-card
            elevation="2"
            class="pa-3"
          >
            <div class="d-flex justify-space-between align-center mb-2">
              <div>
                <div class="text-subtitle-1">
                  Tower {{ index + 1 }} — {{ t.towerName || 'Tower' }}
                </div>
                <div class="text-body-2 text-medium-emphasis">
                  Player: {{ t.playerName || 'Player' }}
                </div>
              </div>
              <v-chip
                :color="t.online ? 'success' : 'error'"
                size="small"
                variant="flat"
              >
                {{ t.online ? 'ONLINE' : 'OFFLINE' }}
              </v-chip>
            </div>
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-chip
                size="small"
                variant="outlined"
                v-if="t.lastStaIp"
                >STA {{ t.lastStaIp }}</v-chip
              >
              <v-chip
                size="small"
                variant="outlined"
                v-if="t.lastApIp"
                >AP {{ t.lastApIp }}</v-chip
              >
            </div>
            <div class="text-body-2 mb-1">
              ESP32-CAM API: {{ t.apiBase || '—' }}
            </div>
            <div class="text-body-2 mb-1">Stream: {{ t.streamUrl || '—' }}</div>
            <div class="text-body-2 mb-1">
              NodeMCU API: {{ t.nodemcuApiBase || '—' }}
            </div>
            <div class="text-caption text-medium-emphasis mb-2">
              Last seen:
              {{
                t.lastSeenAt ? new Date(t.lastSeenAt).toLocaleTimeString() : '—'
              }}
            </div>

            <v-divider class="my-2" />

            <div class="d-flex flex-wrap gap-2">
              <v-btn
                size="small"
                @click="pingStatus(t)"
                :loading="loading[`${t.id}-status`]"
                text="Status"
              />
              <v-btn
                size="small"
                color="success"
                @click="captureOnce(t)"
                :loading="loading[`${t.id}-capture`]"
                text="Capture"
              />
              <v-btn
                size="small"
                variant="outlined"
                color="primary"
                @click="openEdit(t)"
                text="Edit"
              />
              <v-btn
                size="small"
                variant="outlined"
                color="error"
                @click="remove(t.id)"
                text="Remove"
              />
            </div>

            <v-expand-transition>
              <div class="mt-3">
                <div class="text-body-2 mb-1">Live Stream</div>
                <div class="stream-wrapper">
                  <img
                    v-if="streamSrc[t.id]"
                    :key="streamSrc[t.id]"
                    :src="streamSrc[t.id]"
                    @load="onStreamLoad(t, $event)"
                    alt="Stream"
                    class="stream-image"
                  />
                  <div
                    v-if="getPyBox(t)"
                    class="bbox-overlay"
                    :style="getPyBox(t)"
                  >
                    <div class="bbox-label">
                      {{ getPyTopDet(pythonResults[t.id])?.class }}
                      ({{
                        getPyTopDet(pythonResults[t.id])?.confidence?.toFixed?.(
                          2
                        ) ?? '—'
                      }})
                    </div>
                  </div>
                </div>
                <div class="d-flex gap-2 mt-2">
                  <v-btn
                    size="small"
                    color="primary"
                    @click="refreshStream(t)"
                    text="Refresh"
                  />
                  <v-btn
                    size="small"
                    variant="tonal"
                    @click="pingStatus(t)"
                    text="Status"
                  />
                  <v-btn
                    size="small"
                    variant="outlined"
                    @click="whoamiEsp(t)"
                    text="Who am I"
                  />
                  <v-btn
                    size="small"
                    color="success"
                    variant="tonal"
                    @click="detectViaPython(t)"
                    :loading="loading[`${t.id}-py`]"
                    text="Detect (Python)"
                  />
                  <v-switch
                    v-model="streamEnabled[t.id]"
                    inset
                    hide-details
                    density="compact"
                    class="ml-2"
                    :label="streamEnabled[t.id] ? 'Stream on' : 'Stream off'"
                    @change="onStreamToggle(t)"
                  />
                </div>

                <v-divider class="my-3" />
                <div class="text-body-2 mb-1">
                  LED / NodeMCU controls (optional)
                </div>
                <div class="d-flex flex-wrap gap-2">
                  <v-btn
                    size="small"
                    color="primary"
                    @click="callNode(t, '/strip/on')"
                    text="On"
                  />
                  <v-btn
                    size="small"
                    color="secondary"
                    @click="callNode(t, '/strip/off')"
                    text="Off"
                  />
                  <v-btn
                    size="small"
                    variant="tonal"
                    @click="callNode(t, '/strip/mode/toggle')"
                    text="Mode"
                  />
                  <v-btn
                    size="small"
                    variant="outlined"
                    @click="whoamiNode(t)"
                    text="Who am I"
                  />
                </div>

                <v-alert
                  v-if="t.lastDetection"
                  class="mt-2"
                  density="compact"
                  variant="tonal"
                  type="success"
                >
                  Last roll: {{ t.lastDetection.value }} (conf
                  {{ t.lastDetection.confidence?.toFixed?.(2) ?? '—' }})
                </v-alert>
                <v-alert
                  v-if="pythonResults[t.id]"
                  class="mt-2"
                  density="compact"
                  variant="tonal"
                  :type="pythonResults[t.id].error ? 'error' : 'info'"
                >
                  <div v-if="pythonResults[t.id].error">
                    {{ pythonResults[t.id].error }}
                  </div>
                  <div v-else-if="getPyTopDet(pythonResults[t.id])">
                    Python:
                    {{ getPyTopDet(pythonResults[t.id]).class }} (conf
                    {{
                      getPyTopDet(pythonResults[t.id]).confidence?.toFixed?.(
                        2
                      ) ?? '—'
                    }})
                  </div>
                  <div v-else>Python: no detections</div>
                </v-alert>
              </div>
            </v-expand-transition>
          </v-card>
        </v-col>
      </v-row>
    </v-container>

    <v-dialog
      v-model="editDialog"
      max-width="700"
    >
      <v-card>
        <v-card-title class="text-h6">Edit Tower</v-card-title>
        <v-card-text>
          <v-row>
            <v-col
              cols="12"
              md="6"
            >
              <v-text-field
                v-model="editForm.playerName"
                label="Player name"
                variant="outlined"
                density="comfortable"
                class="mb-2"
              />
            </v-col>
            <v-col
              cols="12"
              md="6"
            >
              <v-text-field
                v-model="editForm.towerName"
                label="Tower name"
                variant="outlined"
                density="comfortable"
                class="mb-2"
              />
            </v-col>
          </v-row>

          <v-card
            elevation="1"
            class="pa-3 mb-3"
          >
            <div class="text-subtitle-1 mb-2">Step 2: ESP32-CAM</div>
            <p class="text-body-2 mb-2">
              Connect via AP (<code>DiceTower-XXYY</code>, pass
              <code>dungeon123</code>) or use STA fields below to edit
              endpoints.
            </p>
            <v-text-field
              v-model="editForm.apiBase"
              label="ESP32-CAM API (STA)"
              variant="outlined"
              density="comfortable"
              class="mb-2"
            />
            <v-text-field
              v-model="editForm.streamUrl"
              label="ESP32-CAM Stream"
              variant="outlined"
              density="comfortable"
              class="mb-2"
            />
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                size="small"
                variant="tonal"
                @click="pingStatus(currentEditTower)"
                >Status</v-btn
              >
              <v-btn
                size="small"
                color="success"
                variant="tonal"
                @click="captureOnce(currentEditTower)"
                >Capture</v-btn
              >
              <v-btn
                size="small"
                variant="outlined"
                @click="whoamiEsp(currentEditTower)"
                >Who am I</v-btn
              >
              <v-btn
                size="small"
                class="mb-2"
                color="success"
                variant="tonal"
                @click="detectViaPython(currentEditTower)"
                :loading="loading[`${currentEditTower?.id}-py`]"
                >Detect (Python)</v-btn
              >
            </div>
            <v-text-field
              v-model="editForm.pyServerUrl"
              label="Python server URL"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              :placeholder="pyDefault"
            />
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                size="small"
                variant="outlined"
                @click="pyHealthEdit"
                >Server health</v-btn
              >
              <v-btn
                size="small"
                color="success"
                variant="tonal"
                @click="pyTestDetectEdit"
                :loading="loading[`${currentEditTower?.id || 'edit'}-py`]"
                >Test detection</v-btn
              >
              <v-btn
                v-if="editPyChanged"
                size="small"
                color="primary"
                variant="outlined"
                @click="saveEditPyAddress"
              >
                Save new address
              </v-btn>
            </div>
            <v-alert
              v-if="editPyStatus"
              type="info"
              variant="tonal"
              class="mb-2"
            >
              {{ editPyStatus }}
            </v-alert>
          </v-card>

          <v-card
            elevation="1"
            class="pa-3 mb-3"
          >
            <div class="text-subtitle-1 mb-2">Step 3: NodeMCU</div>
            <v-text-field
              v-model="editForm.nodemcuApiBase"
              label="NodeMCU API"
              variant="outlined"
              density="comfortable"
              class="mb-2"
            />
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                size="small"
                variant="tonal"
                @click="pingNodeStatus(currentEditTower)"
                >Status</v-btn
              >
              <v-btn
                size="small"
                color="primary"
                variant="tonal"
                @click="callNode(currentEditTower, '/strip/on')"
                >LED On</v-btn
              >
              <v-btn
                size="small"
                color="secondary"
                variant="tonal"
                @click="callNode(currentEditTower, '/strip/off')"
                >LED Off</v-btn
              >
              <v-btn
                size="small"
                variant="outlined"
                @click="whoamiNode(currentEditTower)"
                >Who am I</v-btn
              >
            </div>
          </v-card>
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn
            variant="text"
            @click="editDialog = false"
            >Cancel</v-btn
          >
          <v-btn
            color="primary"
            @click="saveEdit"
            >Save</v-btn
          >
        </v-card-actions>
      </v-card>
    </v-dialog>
  </div>
</template>

<script setup>
  import { ref, computed, onMounted } from 'vue';
  import { useTowerStore } from '../stores/towerStore';
  import { storeToRefs } from 'pinia';

  const pyServerUrl = import.meta.env.VITE_PY_SERVER_URL || '';

  const props = defineProps({
    showHeader: {
      type: Boolean,
      default: true,
    },
  });

  const towerStore = useTowerStore();
  const { towers } = storeToRefs(towerStore);

  const loading = ref({});
  const streamSrc = ref({});
  const streamSizes = ref({});
  const expanded = ref({});
  const streamEnabled = ref({});
  const pythonResults = ref({});
  const editDialog = ref(false);
  const editPyStatus = ref('');
  const editForm = ref({
    id: '',
    playerName: '',
    towerName: '',
    apiBase: '',
    streamUrl: '',
    nodemcuApiBase: '',
    pyServerUrl: '',
  });
  const pyDefault =
    import.meta.env.VITE_PY_SERVER_URL || 'http://localhost:8003/detect';
  const editPyChanged = computed(() => {
    const base = editForm.value.pyServerUrl || '';
    const cur = currentEditTower?.value?.pyServerUrl || pyServerUrl || '';
    return base.trim() !== cur.trim();
  });
  const currentEditTower = computed(() =>
    towers.value.find(t => t.id === editForm.value.id)
  );

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
      const res = await fetch(`${tower.apiBase}/dice/capture`, {
        cache: 'no-store',
      });
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

  function refreshStream(tower) {
    if (!tower?.streamUrl || !tower?.id) return;
    if (!streamEnabled.value[tower.id]) return;
    streamSrc.value[tower.id] = `${tower.streamUrl}?t=${Date.now()}`;
  }

  function onStreamLoad(tower, ev) {
    if (!tower?.id || !ev?.target) return;
    streamSizes.value[tower.id] = {
      w: ev.target.clientWidth || ev.target.naturalWidth || 0,
      h: ev.target.clientHeight || ev.target.naturalHeight || 0,
    };
  }

  function remove(id) {
    towerStore.removeTower(id);
  }

  function toggleExpand(id) {
    // keep details always open
    expanded.value[id] = true;
  }

  async function stopStream(id) {
    if (!id && id !== 0) return;
    const tower = towers.value.find(t => t.id === id);
    if (tower?.apiBase) {
      // Call ESP32 stop endpoint to force-close the stream
      try {
        await fetch(`${tower.apiBase}/camera/stream/stop`, {
          method: 'POST',
          cache: 'no-store',
        });
      } catch (e) {
        // Ignore errors - we'll still clear the frontend state
      }
    }
    // Clear Python results immediately so overlay disappears right away
    const pyCopy = { ...pythonResults.value };
    delete pyCopy[id];
    pythonResults.value = pyCopy;
    // Clear src to close MJPEG quickly
    streamSrc.value[id] = '';
    setTimeout(() => {
      const copy = { ...streamSrc.value };
      delete copy[id];
      streamSrc.value = copy;
      const sizesCopy = { ...streamSizes.value };
      delete sizesCopy[id];
      streamSizes.value = sizesCopy;
    }, 150);
  }

  function onStreamToggle(tower) {
    if (!tower?.id) return;
    if (streamEnabled.value[tower.id]) {
      refreshStream(tower);
    } else {
      stopStream(tower.id);
    }
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
      const res = await fetch(`${tower.nodemcuApiBase}/whoami`, {
        cache: 'no-store',
      });
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

  async function detectViaPython(tower) {
    const detectUrl = (tower?.pyServerUrl || pyServerUrl || '')
      .toString()
      .trim();
    if (!detectUrl) return;
    const snapUrl = tower?.apiBase ? `${tower.apiBase}/camera/snapshot` : '';
    if (!snapUrl) return;
    loading.value[`${tower.id}-py`] = true;
    try {
      const snapRes = await fetch(snapUrl, { cache: 'no-store' });
      if (!snapRes.ok) {
        pythonResults.value[tower.id] = {
          error: `Snapshot failed ${snapRes.status}`,
        };
        return;
      }
      const blob = await snapRes.blob();
      const fd = new FormData();
      fd.append('file', new File([blob], 'frame.jpg', { type: 'image/jpeg' }));
      const res = await fetch(detectUrl, { method: 'POST', body: fd });
      if (!res.ok) {
        pythonResults.value[tower.id] = {
          error: `Detect failed ${res.status}`,
        };
        return;
      }
      const data = await res.json();
      pythonResults.value[tower.id] = normalizePythonResult(data);
    } catch (e) {
      pythonResults.value[tower.id] = { error: String(e) };
    } finally {
      loading.value[`${tower.id}-py`] = false;
    }
  }

  onMounted(() => {
    towers.value.forEach(t => {
      pingStatus(t);
    });
  });

  function openEdit(tower) {
    editForm.value = {
      id: tower.id,
      playerName: tower.playerName || '',
      towerName: tower.towerName || '',
      apiBase: tower.apiBase || '',
      streamUrl: tower.streamUrl || '',
      nodemcuApiBase: tower.nodemcuApiBase || '',
      pyServerUrl:
        tower.pyServerUrl || pyServerUrl || 'http://localhost:8003/detect',
    };
    editPyStatus.value = '';
    editDialog.value = true;
  }

  function saveEdit() {
    if (!editForm.value.id) {
      editDialog.value = false;
      return;
    }
    towerStore.addOrUpdateTower({
      id: editForm.value.id,
      playerName: editForm.value.playerName,
      towerName: editForm.value.towerName,
      apiBase: editForm.value.apiBase,
      streamUrl: editForm.value.streamUrl,
      nodemcuApiBase: editForm.value.nodemcuApiBase,
      pyServerUrl: editForm.value.pyServerUrl,
    });
    // If we are editing the currently expanded tower, refresh its stream URL
    if (expanded.value[editForm.value.id] && editForm.value.streamUrl) {
      streamSrc.value[editForm.value.id] = editForm.value.streamUrl;
    }
    editDialog.value = false;
  }

  function saveEditPyAddress() {
    if (!editForm.value.id) return;
    const url = (editForm.value.pyServerUrl || '').trim();
    if (!url) return;
    towerStore.addOrUpdateTower({
      id: editForm.value.id,
      pyServerUrl: url,
    });
    editPyStatus.value = 'Saved new address';
  }

  function normalizePythonResult(data) {
    // If /detect/best returned { result: {...} } convert to detected_numbers array
    if (data && data.result && !data.detected_numbers) {
      data.detected_numbers = [data.result];
    }
    return data;
  }

  function getPyTopDet(res) {
    if (!res) return null;
    if (res.detected_numbers && res.detected_numbers.length) {
      return res.detected_numbers[0];
    }
    if (res.result) return res.result;
    return null;
  }

  function getPyBox(tower) {
    const res = pythonResults.value[tower.id];
    const det = getPyTopDet(res);
    if (!det || !det.bbox) return null;
    const imgSize = res?.image_size ||
      det.image_size || {
        width: 320,
        height: 240,
      };
    if (!imgSize.width || !imgSize.height) {
      imgSize.width = Math.max(det.bbox.x2 || 0, imgSize.width || 0, 320);
      imgSize.height = Math.max(det.bbox.y2 || 0, imgSize.height || 0, 240);
    }
    const disp = streamSizes.value[tower.id];
    if (!imgSize || !imgSize.width || !imgSize.height || !disp) return null;
    const scaleX = disp.w / imgSize.width;
    const scaleY = disp.h / imgSize.height;
    return {
      left: `${det.bbox.x1 * scaleX}px`,
      top: `${det.bbox.y1 * scaleY}px`,
      width: `${det.bbox.width * scaleX}px`,
      height: `${det.bbox.height * scaleY}px`,
    };
  }

  async function pyHealthEdit() {
    editPyStatus.value = '';
    const url = (editForm.value.pyServerUrl || pyServerUrl || '')
      .toString()
      .trim()
      .replace(/\/+$/, '');
    if (!url) {
      editPyStatus.value = 'Enter Python server URL';
      return;
    }
    const healthUrl =
      url.endsWith('/health') ||
      url.endsWith('/detect') ||
      url.endsWith('/detect/best')
        ? url.replace(/\/detect(?:\/best)?$/, '/health')
        : `${url}/health`;
    try {
      const res = await fetch(healthUrl, { cache: 'no-store' });
      const text = await res.text();
      editPyStatus.value = `Health → ${res.status}: ${text}`;
    } catch (e) {
      editPyStatus.value = `Health error: ${e}`;
    }
  }

  async function pyTestDetectEdit() {
    editPyStatus.value = '';
    const detectUrl = (editForm.value.pyServerUrl || pyServerUrl || '')
      .toString()
      .trim()
      .replace(/\/+$/, '');
    if (!detectUrl) {
      editPyStatus.value = 'Enter Python server URL';
      return;
    }
    const snapBase = (editForm.value.apiBase || '')
      .toString()
      .trim()
      .replace(/\/+$/, '');
    if (!snapBase) {
      editPyStatus.value = 'Enter ESP32-CAM API';
      return;
    }
    const loadKey = `${editForm.value.id || 'edit'}-py`;
    loading.value[loadKey] = true;
    try {
      const snapRes = await fetch(`${snapBase}/camera/snapshot`, {
        cache: 'no-store',
      });
      if (!snapRes.ok) {
        editPyStatus.value = `Snapshot failed ${snapRes.status}`;
        return;
      }
      const blob = await snapRes.blob();
      const fd = new FormData();
      fd.append('file', new File([blob], 'frame.jpg', { type: 'image/jpeg' }));
      const res = await fetch(detectUrl, { method: 'POST', body: fd });
      const text = await res.text();
      editPyStatus.value = `Detect → ${res.status}: ${text}`;
    } catch (e) {
      editPyStatus.value = `Detect error: ${e}`;
    } finally {
      loading.value[loadKey] = false;
    }
  }
</script>

<style scoped>
  .stream-wrapper {
    position: relative;
    display: block;
    width: 100%;
    min-height: 240px; /* reserve space for stream when enabled */
    background: transparent;
    border-radius: 0;
  }
  .stream-image {
    max-width: 100%;
    max-height: 360px;
    height: auto;
    width: auto;
    border-radius: 6px;
    display: block;
  }
  .gap-2 {
    gap: 8px;
  }
  .bbox-overlay {
    position: absolute;
    border: 2px solid #e91e63;
    box-shadow: 0 0 6px rgba(233, 30, 99, 0.7);
    pointer-events: none;
  }
  .bbox-label {
    position: absolute;
    top: 0;
    left: 50%;
    background: rgba(233, 30, 99, 0.85);
    color: #fff;
    padding: 2px 6px;
    font-size: 11px;
    border-radius: 3px;
    white-space: nowrap;
    transform: translate(
      -50%,
      calc(-100% - 1px)
    ); /* center and lift above the box */
  }
</style>
