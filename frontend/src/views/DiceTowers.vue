<template>
  <v-container class="pa-4">
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
                @click="callAp('/blink/on', 'apOn')"
                >Blink ON</v-btn
              >
              <v-btn
                color="secondary"
                :loading="loading.apOff"
                @click="callAp('/blink/off', 'apOff')"
                >Blink OFF</v-btn
              >
              <v-btn
                variant="tonal"
                :loading="loading.apToggle"
                @click="callAp('/blink/toggle', 'apToggle')"
                >Toggle</v-btn
              >
              <v-btn
                variant="text"
                :loading="loading.apStatus"
                @click="callAp('/status', 'apStatus')"
                >Status</v-btn
              >
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
          class="pa-4 mb-4"
        >
          <v-card-title class="text-h6">3) Provision Wi‑Fi</v-card-title>
          <v-card-text>
            <v-text-field
              v-model="homeSsid"
              label="Your Wi‑Fi SSID"
              variant="outlined"
              density="comfortable"
            />
            <v-text-field
              v-model="homePass"
              label="Your Wi‑Fi Password"
              type="password"
              variant="outlined"
              density="comfortable"
            />
            <v-btn
              color="primary"
              :loading="loading.provision"
              @click="provision"
              >Send Credentials</v-btn
            >
            <v-alert
              v-if="provisionResult"
              type="info"
              variant="tonal"
              class="mt-3"
              >{{ provisionResult }}</v-alert
            >
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
          <v-card-title class="text-h6">4) Control (STA mode)</v-card-title>
          <v-card-text>
            <p>
              After provisioning, reconnect your PC to your home Wi‑Fi. The
              tower will join and get an IP (shown in the provision response).
              Enter it below and control the LED.
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
                @click="call('/blink/on', 'on')"
                >Blink ON</v-btn
              >
              <v-btn
                color="secondary"
                :loading="loading.off"
                @click="call('/blink/off', 'off')"
                >Blink OFF</v-btn
              >
              <v-btn
                variant="tonal"
                :loading="loading.toggle"
                @click="call('/blink/toggle', 'toggle')"
                >Toggle</v-btn
              >
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
                Your Wi‑Fi credentials are stored on the device flash, not in
                this app.
              </li>
              <li>
                To re-provision, hold the device button (if added) or erase
                creds via API (coming soon).
              </li>
            </ul>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
  import { ref, watch } from 'vue';

  const apUrl = ref(
    localStorage.getItem('diceTowerApUrl') || 'http://192.168.4.1'
  );
  const deviceUrl = ref(localStorage.getItem('diceTowerStaUrl') || '');
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
      )}&pass=${encodeURIComponent(homePass.value)}`;
      const res = await fetch(url);
      const text = await res.text();
      provisionResult.value = `Provision → ${res.status}: ${text}`;
    } catch (e) {
      provisionResult.value = `Provision error: ${e}`;
    } finally {
      loading.value.provision = false;
    }
  }

  async function call(path, which) {
    try {
      loading.value[which] = true;
      const url = `${deviceUrl.value}${path}`;
      const res = await fetch(url);
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
      const res = await fetch(url);
      const text = await res.text();
      apStatusText.value = `AP ${path} → ${res.status}: ${text}`;
    } catch (e) {
      apStatusText.value = `AP error ${path}: ${e}`;
    } finally {
      loading.value[which] = false;
    }
  }
</script>

<style scoped>
  ul {
    padding-left: 18px;
  }
</style>
