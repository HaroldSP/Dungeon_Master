<template>
  <v-app-bar app color="surface" elevation="1">
    <v-app-bar-title>
      <span class="text-h6">D&D Master Screen</span>
    </v-app-bar-title>
    
    <v-spacer></v-spacer>
    
    <div class="d-flex align-center ga-2">
      <v-chip color="primary" variant="elevated" size="small">
        👹 {{ monsterCount }} монстров
      </v-chip>
      <v-chip color="secondary" variant="elevated" size="small">
        🎯 {{ tokenCount }} токенов
      </v-chip>
    </div>
  </v-app-bar>
</template>

<script setup>
import { computed } from 'vue'
import { storeToRefs } from 'pinia'
import { useMonsterStore } from '../../stores/monsterStore'
import { useMapStore } from '../../stores/mapStore'

const monsterStore = useMonsterStore()
const mapStore = useMapStore()

const { tokens } = storeToRefs(mapStore)
const monsterCount = computed(() => monsterStore.monsterCount)
const tokenCount = computed(() => tokens.value.length)
</script>

<style scoped>
/* Header-specific styles if needed */
</style>
