<template>
  <v-card
    v-if="tower"
    elevation="2"
    class="pa-4"
  >
    <v-card-title class="text-h6">
      Camera Controls - {{ tower.name }}
    </v-card-title>
    <v-card-text>
      <v-row>
        <v-col cols="12" md="6">
          <v-slider
            v-model="settings.brightness"
            label="Brightness"
            min="-2"
            max="2"
            step="1"
            thumb-label
            @end="updateSettings"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-slider
            v-model="settings.contrast"
            label="Contrast"
            min="-2"
            max="2"
            step="1"
            thumb-label
            @end="updateSettings"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-slider
            v-model="settings.saturation"
            label="Saturation"
            min="-2"
            max="2"
            step="1"
            thumb-label
            @end="updateSettings"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-slider
            v-model="settings.ae_level"
            label="Exposure Level"
            min="-2"
            max="2"
            step="1"
            thumb-label
            @end="updateSettings"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-slider
            v-model="settings.aec_value"
            label="Exposure Value"
            min="0"
            max="1200"
            step="10"
            thumb-label
            @end="updateSettings"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-slider
            v-model="settings.agc_gain"
            label="AGC Gain"
            min="0"
            max="30"
            step="1"
            thumb-label
            @end="updateSettings"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-slider
            v-model="settings.gainceiling"
            label="Gain Ceiling"
            min="0"
            max="6"
            step="1"
            thumb-label
            @end="updateSettings"
          />
        </v-col>
        <v-col cols="12" md="6">
          <v-slider
            v-model="settings.stream_delay_ms"
            label="Stream Latency (ms)"
            min="0"
            max="100"
            step="1"
            thumb-label
            @end="updateSettings"
          />
        </v-col>
        <v-col cols="12">
          <v-select
            v-model="settings.special_effect"
            label="Special Effect"
            :items="specialEffects"
            @update:model-value="updateSettings"
          />
        </v-col>
        <v-col cols="12">
          <v-btn
            color="primary"
            @click="loadSettings"
            :loading="loading"
          >
            Refresh Settings
          </v-btn>
          <v-btn
            color="secondary"
            class="ml-2"
            @click="resetSettings"
            :loading="loading"
          >
            Reset to Defaults
          </v-btn>
        </v-col>
      </v-row>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { ref, watch } from 'vue';

const props = defineProps({
  tower: {
    type: Object,
    required: true
  }
});

const loading = ref(false);
const settings = ref({
  brightness: 2,
  contrast: 1,
  saturation: 1,
  special_effect: 2,
  ae_level: 2,
  aec_value: 600,
  agc_gain: 0,
  gainceiling: 2,
  stream_delay_ms: 1
});

const specialEffects = [
  { title: 'None', value: 0 },
  { title: 'Negative', value: 1 },
  { title: 'Grayscale', value: 2 },
  { title: 'Red Tint', value: 3 },
  { title: 'Green Tint', value: 4 },
  { title: 'Blue Tint', value: 5 },
  { title: 'Sepia', value: 6 }
];

async function loadSettings() {
  if (!props.tower?.url) return;
  
  loading.value = true;
  try {
    const res = await fetch(`${props.tower.url}/camera/settings`, {
      cache: 'no-store'
    });
    
    if (res.ok) {
      const data = await res.json();
      if (data.ok) {
        Object.assign(settings.value, data);
      }
    }
  } catch (e) {
    console.error('Failed to load camera settings:', e);
  } finally {
    loading.value = false;
  }
}

async function updateSettings() {
  if (!props.tower?.url) return;
  
  loading.value = true;
  try {
    const res = await fetch(`${props.tower.url}/camera/settings`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(settings.value),
      cache: 'no-store'
    });
    
    if (!res.ok) {
      console.error('Failed to update camera settings');
    }
  } catch (e) {
    console.error('Failed to update camera settings:', e);
  } finally {
    loading.value = false;
  }
}

function resetSettings() {
  settings.value = {
    brightness: 2,
    contrast: 1,
    saturation: 1,
    special_effect: 2,
    ae_level: 2,
    aec_value: 600,
    agc_gain: 0,
    gainceiling: 2,
    stream_delay_ms: 1
  };
  updateSettings();
}

// Load settings when tower changes
watch(() => props.tower, () => {
  if (props.tower) {
    loadSettings();
  }
}, { immediate: true });
</script>

