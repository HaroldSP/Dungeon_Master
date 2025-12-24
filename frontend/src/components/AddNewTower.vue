<template>
  <v-container
    fluid
    class="pa-0 add-tab-scroll"
  >
    <v-row>
      <v-col cols="12">
        <v-card
          elevation="2"
          class="pa-4 mb-4"
        >
          <v-card-title class="text-h6">Step 1: Tower basics</v-card-title>
          <v-card-text>
            <v-row>
              <v-col
                cols="12"
                md="4"
              >
                <v-text-field
                  v-model="towerNumber"
                  label="Tower #"
                  type="number"
                  variant="outlined"
                  density="comfortable"
                />
              </v-col>
              <v-col
                cols="12"
                md="8"
              >
                <v-select
                  v-model="selectedPlayer"
                  :items="players"
                  item-title="title"
                  item-value="value"
                  label="Player"
                  variant="outlined"
                  density="comfortable"
                  return-object
                  clearable
                  hint="Select a player JSON"
                  persistent-hint
                />
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row>
      <v-col
        cols="12"
        md="6"
      >
        <v-card
          elevation="2"
          class="pa-4 mb-4"
        >
          <v-card-title class="text-h6">Step 2: ESP32-CAM</v-card-title>
          <v-card-text>
            <p class="text-body-2 mb-2">
              Connect to tower AP (<code>DiceTower-ESP-XXYY</code>, password
              <code>dungeon123</code>), then send your home Wi‑Fi. The last 2
              letters (XXYY) come from the device MAC address.
            </p>
            <v-text-field
              v-model="apUrl"
              label="Tower URL (AP mode)"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="http://192.168.4.1"
              persistent-placeholder
            />

            <v-text-field
              v-model="homeSsid"
              label="Home Wi‑Fi SSID"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="your-ssid"
              persistent-placeholder
            />
            <v-text-field
              v-model="homePass"
              label="Home Wi‑Fi Password"
              type="password"
              variant="outlined"
              density="comfortable"
              class="mb-3"
              placeholder="your-password"
              persistent-placeholder
            />
            <div class="d-flex flex-wrap gap-2 mb-3">
              <v-btn
                size="small"
                color="primary"
                variant="tonal"
                :loading="loading.provision"
                text="Send Credentials"
                @click="provision"
              />
              <v-btn
                size="small"
                variant="outlined"
                @click="checkStaStatus"
              >
                Check if already connected (STA)
              </v-btn>
            </div>
            <v-alert
              v-if="lastProvisionIp"
              type="success"
              variant="tonal"
              class="mb-2"
            >
              ESP32-CAM connected. STA IP: {{ lastProvisionIp }}
            </v-alert>
            <v-alert
              v-if="provisionResult"
              type="info"
              variant="tonal"
              class="mb-2"
              >{{ provisionResult }}</v-alert
            >
            <v-alert
              v-if="staCheckResult"
              type="info"
              variant="tonal"
              class="mb-3"
              >{{ staCheckResult }}</v-alert
            >
            <v-divider class="my-3" />
            <div class="text-subtitle-2 mb-2">ESP32-CAM endpoints</div>
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                class="mb-2"
                size="small"
                variant="outlined"
                @click="pingEspWhoami"
                >Who am I (ESP)</v-btn
              >
            </div>
            <v-text-field
              v-model="espApiBase"
              label="ESP32-CAM API (STA)"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="http://192.168.110.53"
              persistent-placeholder
            />
            <v-text-field
              v-model="espStreamUrl"
              label="ESP32-CAM Stream"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="http://192.168.110.53:81/camera/stream"
              persistent-placeholder
            />
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                size="small"
                variant="tonal"
                @click="pingEspStatus"
                >Status</v-btn
              >
              <v-btn
                size="small"
                color="success"
                variant="tonal"
                class="mb-2"
                @click="testEspCapture"
                >Test capture</v-btn
              >
            </div>
            <v-text-field
              v-model="pyServerUrl"
              label="Python server URL"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="http://localhost:8003/detect"
              persistent-placeholder
            />
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                size="small"
                variant="outlined"
                @click="pyHealth"
                >Server health</v-btn
              >
              <v-btn
                size="small"
                color="success"
                variant="tonal"
                @click="pyTestDetect"
                >Test detection</v-btn
              >
              <v-btn
                v-if="pyChanged"
                size="small"
                color="primary"
                variant="outlined"
                @click="savePyServer"
              >
                Save new address
              </v-btn>
            </div>
            <v-alert
              v-if="pyStatusText"
              type="info"
              variant="tonal"
              class="mb-2"
            >
              {{ pyStatusText }}
            </v-alert>
            <v-alert
              v-if="espStatusText"
              type="info"
              variant="tonal"
              class="mb-2"
            >
              {{ espStatusText }}
            </v-alert>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col
        cols="12"
        md="6"
      >
        <v-card
          elevation="2"
          class="pa-4 mb-4"
        >
          <v-card-title class="text-h6">Step 3: NodeMCU</v-card-title>
          <v-card-text>
            <p class="text-body-2 mb-2">
              Connect to NodeMCU AP (<code>DiceTower-nodeMCU-XXYY</code>,
              password
              <code>dungeon123</code>) if needed, then send your home Wi‑Fi. The
              XXYY part is derived from the NodeMCU MAC address.
            </p>
            <v-text-field
              v-model="nodemcuApUrl"
              label="NodeMCU URL (AP mode)"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="http://192.168.4.1"
              persistent-placeholder
            />
            <v-text-field
              v-model="nodemcuProvisionSsid"
              label="Home Wi‑Fi SSID"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="your-ssid"
              persistent-placeholder
            />
            <v-text-field
              v-model="nodemcuProvisionPass"
              label="Home Wi‑Fi Password"
              type="password"
              variant="outlined"
              density="comfortable"
              class="mb-3"
              placeholder="your-password"
              persistent-placeholder
            />
            <div class="d-flex flex-wrap gap-2 mb-3">
              <v-btn
                size="small"
                color="primary"
                variant="tonal"
                @click="provisionNode"
                >Send Credentials</v-btn
              >
              <v-btn
                size="small"
                variant="outlined"
                @click="pingNodeStatus"
                >Status</v-btn
              >
              <v-btn
                size="small"
                color="primary"
                variant="tonal"
                @click="testNodeOn"
                >LED On</v-btn
              >
              <v-btn
                size="small"
                color="secondary"
                variant="tonal"
                @click="testNodeOff"
                >LED Off</v-btn
              >
            </div>
            <v-divider class="my-3" />
            <div class="text-subtitle-2 mb-2">NodeMCU endpoints</div>
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                class="mb-2"
                size="small"
                variant="outlined"
                @click="pingNodeWhoami"
                >Who am I (Node)</v-btn
              >
            </div>
            <v-text-field
              v-model="nodemcuApiBase"
              label="NodeMCU API"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="http://192.168.110.54"
              persistent-placeholder
            />
            <v-alert
              v-if="nodeProvisionResult"
              type="info"
              variant="tonal"
              class="mb-2"
            >
              {{ nodeProvisionResult }}
            </v-alert>
            <v-alert
              v-if="nodeStatusText"
              type="info"
              variant="tonal"
              class="mb-2"
            >
              {{ nodeStatusText }}
            </v-alert>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
    <v-row>
      <v-col
        cols="12"
        class="d-flex justify-center"
      >
        <div
          class="d-flex flex-column align-center"
          style="gap: 12px"
        >
          <v-btn
            color="success"
            @click="saveTower"
            class="px-6"
            >Save and Add Tower</v-btn
          >
          <v-alert
            v-if="statusText"
            type="info"
            variant="tonal"
            class="mb-0"
          >
            {{ statusText }}
          </v-alert>
        </div>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
  import { ref, watch, onMounted, computed } from 'vue';
  import { useTowerStore } from '../stores/towerStore';

  const towerStore = useTowerStore();
  const apUrl = ref(
    localStorage.getItem('diceTowerApUrl') || 'http://192.168.4.1'
  );
  const deviceUrl = ref(localStorage.getItem('diceTowerStaUrl') || '');
  const towerNumber = ref(1);
  const playerName = ref('');
  const towerName = ref('');
  const homeSsid = ref('');
  const homePass = ref('');
  const provisionResult = ref('');
  const statusText = ref('');
  const apStatusText = ref('');
  const lastProvisionIp = ref('');
  const storedPy = localStorage.getItem('pyServerUrl');
  const defaultEspApi =
    import.meta.env.VITE_API_BASE || 'http://192.168.110.53';
  const defaultEspStream =
    import.meta.env.VITE_STREAM_URL || 'http://192.168.110.53:81/camera/stream';
  const defaultNodeApi = 'http://192.168.110.54';
  const defaultPyServer =
    storedPy ||
    import.meta.env.VITE_PY_SERVER_URL ||
    'http://localhost:8003/detect';
  const espApiBase = ref(defaultEspApi);
  const espStreamUrl = ref(defaultEspStream);
  const nodemcuApiBase = ref(defaultNodeApi);
  const nodemcuProvisionSsid = ref('');
  const nodemcuProvisionPass = ref('');
  const nodemcuApUrl = ref('http://192.168.4.1');
  const staCheckResult = ref('');
  const espStatusText = ref('');
  const nodeStatusText = ref('');
  const nodeProvisionResult = ref('');
  const pyServerUrl = ref(defaultPyServer);
  const pyStatusText = ref('');
  const pyChanged = computed(
    () => pyServerUrl.value.trim() !== defaultPyServer.trim()
  );
  const players = ref([]);
  const selectedPlayer = ref(null);
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

  onMounted(async () => {
    try {
      const mods = import.meta.glob('/src/data/players/**/*.json', {
        eager: true,
      });
      players.value = Object.entries(mods).map(([path, mod], idx) => {
        const data = mod.default || mod;
        const parsed =
          typeof data.data === 'string'
            ? JSON.parse(data.data)
            : data.data || data;
        const name =
          parsed?.name?.value ||
          parsed?.info?.playerName?.value ||
          parsed?.info?.name?.value ||
          `Player ${idx + 1}`;
        return { value: path, title: name, raw: parsed };
      });
    } catch (e) {
      console.error('load players failed', e);
    }
  });

  watch(
    selectedPlayer,
    val => {
      if (val && !playerName.value) {
        playerName.value =
          val.title ||
          val.raw?.name?.value ||
          val.raw?.info?.playerName?.value ||
          val.raw?.info?.name?.value ||
          '';
      }
    },
    { immediate: false }
  );

  function getSelectedPlayerName() {
    const sel = selectedPlayer.value;
    if (!sel) return '';
    const raw = sel.raw;
    return (
      sel.title ||
      raw?.name?.value ||
      raw?.info?.playerName?.value ||
      raw?.info?.name?.value ||
      ''
    );
  }

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

          lastProvisionIp.value = parsed.ip;
          const apiBase = `http://${parsed.ip}`;
          const streamUrl = `${apiBase}:81/camera/stream`;

          const saved = towerStore.addOrUpdateTower({
            apiBase,
            streamUrl,
            towerName: parsed.name || towerName.value || 'Tower',
            playerName: playerName.value || getSelectedPlayerName() || 'Player',
            online: true, // Mark as online since provision was successful
          });

          // Update the towerName if we got a different one from the device
          if (parsed.name && parsed.name !== towerName.value) {
            towerStore.addOrUpdateTower({
              id: saved.id,
              towerName: parsed.name,
            });
          }
          deviceUrl.value = apiBase;
          espApiBase.value = apiBase;
          espStreamUrl.value = streamUrl;

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

  async function provisionNode() {
    nodeProvisionResult.value = '';
    const base = (nodemcuApiBase.value || defaultNodeApi)
      .trim()
      .replace(/\/+$/, '');
    if (!base) {
      nodeProvisionResult.value =
        'Enter NodeMCU API, e.g., http://192.168.110.54';
      return;
    }
    if (!nodemcuProvisionSsid.value) {
      nodeProvisionResult.value = 'Enter Wi‑Fi SSID for NodeMCU';
      return;
    }
    try {
      const url = `${base}/provision?ssid=${encodeURIComponent(
        nodemcuProvisionSsid.value
      )}&pass=${encodeURIComponent(nodemcuProvisionPass.value)}`;
      const res = await fetch(url, { cache: 'no-store' });
      const text = await res.text();
      nodeProvisionResult.value = `Node provision → ${res.status}: ${text}`;
    } catch (e) {
      nodeProvisionResult.value = `Node provision error: ${e}`;
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

  function saveTower() {
    const apiBase = espApiBase.value?.trim();
    const streamUrl = espStreamUrl.value?.trim();
    const nodeBase = nodemcuApiBase.value?.trim();
    const id = `${towerNumber.value || 1}-${Date.now()}`;
    const sel = selectedPlayer.value;
    const playerPath = sel?.value || '';
    towerStore.addOrUpdateTower({
      id,
      playerName:
        playerName.value ||
        getSelectedPlayerName() ||
        `Player ${towerNumber.value || 1}`,
      playerPath,
      towerName: towerName.value || `Tower ${towerNumber.value || 1}`,
      apiBase,
      streamUrl,
      nodemcuApiBase: nodeBase,
      online: false,
    });
    statusText.value = `Saved tower #${towerNumber.value || 1} with ESP API ${
      apiBase || 'n/a'
    }`;
    emit('provision-success');
  }

  async function checkStaStatus() {
    staCheckResult.value = '';
    const base = (
      deviceUrl.value ||
      espApiBase.value ||
      defaultEspApi ||
      ''
    ).trim();
    if (!base) {
      staCheckResult.value =
        'Enter the tower STA URL, e.g., http://192.168.x.x';
      return;
    }
    try {
      const normBase = base.replace(/\/+$/, '');
      const res = await fetch(`${normBase}/status`, { cache: 'no-store' });
      if (!res.ok) {
        staCheckResult.value = `Status failed: HTTP ${res.status}`;
        return;
      }
      const data = await res.json();
      const ip = data?.wifi?.sta?.ip || data?.ip || 'unknown';
      const connected = data?.wifi?.sta?.connected ?? data?.connected ?? false;
      staCheckResult.value = connected
        ? `Already connected. STA IP: ${ip}`
        : `Status OK but not connected. STA IP: ${ip}`;
      if (connected && ip && ip !== 'unknown') {
        lastProvisionIp.value = ip;
        espApiBase.value = `http://${ip}`;
        espStreamUrl.value = `http://${ip}:81/camera/stream`;
        deviceUrl.value = `http://${ip}`;
      }
    } catch (e) {
      staCheckResult.value = `Status error: ${e}`;
    }
  }

  async function pingEspStatus() {
    espStatusText.value = '';
    const base = (espApiBase.value || defaultEspApi || '')
      .trim()
      .replace(/\/+$/, '');
    if (!base) {
      espStatusText.value = 'Enter ESP32-CAM API, e.g., http://192.168.110.53';
      return;
    }
    try {
      const res = await fetch(`${base}/status`, { cache: 'no-store' });
      if (!res.ok) {
        espStatusText.value = `ESP status failed: HTTP ${res.status}`;
        return;
      }
      const data = await res.json();
      const ip = data?.wifi?.sta?.ip || data?.ip || 'unknown';
      espStatusText.value = `ESP status OK. STA IP: ${ip}`;
    } catch (e) {
      espStatusText.value = `ESP status error: ${e}`;
    }
  }

  async function testEspCapture() {
    espStatusText.value = '';
    const base = (espApiBase.value || defaultEspApi || '')
      .trim()
      .replace(/\/+$/, '');
    if (!base) {
      espStatusText.value = 'Enter ESP32-CAM API, e.g., http://192.168.110.53';
      return;
    }
    try {
      const res = await fetch(`${base}/dice/capture`, { cache: 'no-store' });
      const text = await res.text();
      espStatusText.value = `Capture → ${res.status}: ${text}`;
    } catch (e) {
      espStatusText.value = `Capture error: ${e}`;
    }
  }

  async function pyHealth() {
    pyStatusText.value = '';
    const url = (pyServerUrl.value || '').trim().replace(/\/+$/, '');
    if (!url) {
      pyStatusText.value = 'Enter Python server URL';
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
      pyStatusText.value = `Health → ${res.status}: ${text}`;
    } catch (e) {
      pyStatusText.value = `Health error: ${e}`;
    }
  }

  async function pyTestDetect() {
    pyStatusText.value = '';
    const url = (pyServerUrl.value || '').trim().replace(/\/+$/, '');
    if (!url) {
      pyStatusText.value = 'Enter Python server URL';
      return;
    }
    const snapBase = (espApiBase.value || defaultEspApi || '')
      .trim()
      .replace(/\/+$/, '');
    if (!snapBase) {
      pyStatusText.value = 'Enter ESP32-CAM API first';
      return;
    }
    try {
      const snapRes = await fetch(`${snapBase}/camera/snapshot`, {
        cache: 'no-store',
      });
      if (!snapRes.ok) {
        pyStatusText.value = `Snapshot failed ${snapRes.status}`;
        return;
      }
      const blob = await snapRes.blob();
      const fd = new FormData();
      fd.append('file', new File([blob], 'frame.jpg', { type: 'image/jpeg' }));
      const detectUrl = url;
      const res = await fetch(detectUrl, { method: 'POST', body: fd });
      const text = await res.text();
      pyStatusText.value = `Detect → ${res.status}: ${text}`;
    } catch (e) {
      pyStatusText.value = `Detect error: ${e}`;
    }
  }

  function savePyServer() {
    const val = pyServerUrl.value.trim();
    if (!val) return;
    localStorage.setItem('pyServerUrl', val);
    pyStatusText.value = 'Saved new address';
  }

  async function pingEspWhoami() {
    espStatusText.value = '';
    const base = (espApiBase.value || defaultEspApi || '')
      .trim()
      .replace(/\/+$/, '');
    if (!base) {
      espStatusText.value = 'Enter ESP32-CAM API, e.g., http://192.168.110.53';
      return;
    }
    try {
      const res = await fetch(`${base}/whoami`, { cache: 'no-store' });
      const text = await res.text();
      espStatusText.value = `ESP whoami → ${res.status}: ${text}`;
      try {
        const data = JSON.parse(text);
        const ip = data?.sta_ip || data?.wifi?.sta?.ip;
        if (ip) {
          lastProvisionIp.value = ip;
          espApiBase.value = `http://${ip}`;
          espStreamUrl.value = `http://${ip}:81/camera/stream`;
          deviceUrl.value = `http://${ip}`;
        }
      } catch (_) {}
    } catch (e) {
      espStatusText.value = `ESP whoami error: ${e}`;
    }
  }

  async function pingNodeStatus() {
    nodeStatusText.value = '';
    const base = (nodemcuApiBase.value || '').trim().replace(/\/+$/, '');
    if (!base) {
      nodeStatusText.value = 'Enter NodeMCU API, e.g., http://192.168.110.54';
      return;
    }
    try {
      const res = await fetch(`${base}/status`, { cache: 'no-store' });
      const text = await res.text();
      nodeStatusText.value = `Node status → ${res.status}: ${text}`;
    } catch (e) {
      nodeStatusText.value = `Node status error: ${e}`;
    }
  }

  async function testNodeOn() {
    await callNodeCmd('/strip/on', 'Node LED on');
  }
  async function testNodeOff() {
    await callNodeCmd('/strip/off', 'Node LED off');
  }

  async function callNodeCmd(path, label) {
    nodeStatusText.value = '';
    const base = (nodemcuApiBase.value || '').trim().replace(/\/+$/, '');
    if (!base) {
      nodeStatusText.value = 'Enter NodeMCU API, e.g., http://192.168.110.54';
      return;
    }
    try {
      const res = await fetch(`${base}${path}`, { cache: 'no-store' });
      const text = await res.text();
      nodeStatusText.value = `${label} → ${res.status}: ${text}`;
    } catch (e) {
      nodeStatusText.value = `${label} error: ${e}`;
    }
  }

  async function pingNodeWhoami() {
    nodeStatusText.value = '';
    const base = (nodemcuApiBase.value || defaultNodeApi)
      .trim()
      .replace(/\/+$/, '');
    if (!base) {
      nodeStatusText.value = 'Enter NodeMCU API, e.g., http://192.168.110.54';
      return;
    }
    try {
      const res = await fetch(`${base}/whoami`, { cache: 'no-store' });
      const text = await res.text();
      nodeStatusText.value = `Node whoami → ${res.status}: ${text}`;
      // optional parse if present
    } catch (e) {
      nodeStatusText.value = `Node whoami error: ${e}`;
    }
  }
</script>

<style scoped>
  .add-tab-scroll {
    max-height: calc(100vh - 120px);
    overflow-y: auto;
  }
</style>
