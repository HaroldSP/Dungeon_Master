<template>
  <v-container
    fluid
    class="map-view pa-0"
  >
    <v-row no-gutters>
      <v-col
        cols="12"
        md="8"
      >
        <MapCanvas />
      </v-col>
      <v-col
        cols="12"
        md="4"
        class="pa-4"
      >
        <v-card
          elevation="2"
          class="pa-4"
        >
          <v-card-title class="text-h6">Dice Tower Control</v-card-title>
          <v-card-text>
            <v-text-field
              v-model="deviceUrl"
              label="Device URL"
              placeholder="http://192.168.4.1"
              density="comfortable"
              variant="outlined"
            />
            <div class="d-flex ga-2 mb-2">
              <v-btn
                color="primary"
                @click="blinkOn"
                :loading="loading.on"
                >Blink ON</v-btn
              >
              <v-btn
                color="secondary"
                @click="blinkOff"
                :loading="loading.off"
                >Blink OFF</v-btn
              >
              <v-btn
                variant="tonal"
                @click="blinkToggle"
                :loading="loading.toggle"
                >Toggle</v-btn
              >
            </div>
            <v-alert
              v-if="statusText"
              type="info"
              density="comfortable"
              variant="tonal"
            >
              {{ statusText }}
            </v-alert>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
  import { ref } from 'vue';
  import MapCanvas from '../components/MapCanvas.vue';

  const deviceUrl = ref(
    localStorage.getItem('diceTowerUrl') || 'http://192.168.4.1'
  );
  const statusText = ref('');
  const loading = ref({ on: false, off: false, toggle: false });

  function saveUrl() {
    localStorage.setItem('diceTowerUrl', deviceUrl.value);
  }

  async function call(path, which) {
    try {
      loading.value[which] = true;
      saveUrl();
      const url = `${deviceUrl.value}${path}`;
      const res = await fetch(url, { method: 'GET' });
      const text = await res.text();
      statusText.value = `GET ${path} → ${res.status}: ${text}`;
    } catch (e) {
      statusText.value = `Error calling ${path}: ${e}`;
    } finally {
      loading.value[which] = false;
    }
  }

  const blinkOn = () => call('/blink/on', 'on');
  const blinkOff = () => call('/blink/off', 'off');
  const blinkToggle = () => call('/blink/toggle', 'toggle');
</script>

<style scoped>
  .map-view {
    height: 100vh;
    overflow: hidden;
  }
</style>
