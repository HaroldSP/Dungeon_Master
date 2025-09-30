<template>
  <v-navigation-drawer 
    app 
    :rail="isMinimized" 
    :width="isMinimized ? SIDEBAR_WIDTHS.minimized : SIDEBAR_WIDTHS.expanded"
    permanent
    style="transition: width 0.3s ease;"
  >
    <div class="pa-4">
      <div style="width: 100%; text-align: center;">
        <div class="d-flex align-center">
          <v-btn 
            :icon="toggleIcon"
            size="small" 
            variant="text"
            @click="toggleMinimize"
            :title="toggleTitle"
            class="px-5 ml-n2 mt-n2 mb-2"
          />
          <h2 
            v-show="!isMinimized" 
            class="ml-6 mt-n2"
            style="margin: 8px 0; font-size: 16px; white-space: nowrap; overflow: hidden;"
          >
            Vue Master Screen
          </h2>
        </div>
      </div>
    </div>

    <v-list density="comfortable" class="pt-0 mt-n4">

      <v-list-item
        v-for="item in NAVIGATION_ITEMS"
        :key="item.path"
        :active="$route.path === item.path"
        :title="isMinimized ? '' : item.title"
        :prepend-icon="item.icon"
        @click="$router.push(item.path)"
      />
    </v-list>

    <template #append>
      <div class="pa-4" v-if="!isMinimized" style="transition: all 0.3s ease;">
        <div class="d-flex ga-2 justify-space-between">
          <v-chip color="primary" variant="elevated" size="small" class="flex-1-1">
            👹 {{ monsterCount }}
          </v-chip>
          <v-chip color="secondary" variant="elevated" size="small" class="flex-1-1">
            🎯 {{ tokenCount }}
          </v-chip>
        </div>
      </div>
      <div class="pa-2" v-else style="transition: all 0.3s ease;">
        <div class="d-flex flex-column ga-1 align-center">
          <v-chip color="primary" variant="elevated" size="x-small">
            👹 {{ monsterCount }}
          </v-chip>
          <v-chip color="secondary" variant="elevated" size="x-small">
            🎯 {{ tokenCount }}
          </v-chip>
        </div>
      </div>
    </template>
  </v-navigation-drawer>
</template>

<script setup>
import { ref, computed } from 'vue'
import { storeToRefs } from 'pinia'
import { useMonsterStore } from '../../stores/monsterStore'
import { useMapStore } from '../../stores/mapStore'

// Constants
const SIDEBAR_WIDTHS = {
  expanded: 260,
  minimized: 60
}

const NAVIGATION_ITEMS = [
  { path: '/', title: 'Карта', icon: 'mdi-map' },
  { path: '/monsters', title: 'Монстры', icon: 'mdi-skull' }
]

// Stores
const monsterStore = useMonsterStore()
const mapStore = useMapStore()

// Reactive data
const isMinimized = ref(false)

// Computed
const { tokens } = storeToRefs(mapStore)
const monsterCount = computed(() => monsterStore.monsterCount)
const tokenCount = computed(() => tokens.value.length)

const toggleIcon = computed(() => isMinimized.value ? 'mdi-chevron-right' : 'mdi-chevron-left')
const toggleTitle = computed(() => isMinimized.value ? 'Развернуть' : 'Свернуть')

// Methods
const toggleMinimize = () => {
  isMinimized.value = !isMinimized.value
}
</script>

