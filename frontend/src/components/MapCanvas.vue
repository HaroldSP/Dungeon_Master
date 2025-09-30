<template>
  <div class="map-container">
    <v-toolbar density="comfortable" color="surface">
      <v-btn variant="tonal" :color="mapSettings.showGrid ? 'primary' : undefined" @click="toggleGrid" prepend-icon="mdi-grid">
        Сетка
      </v-btn>
      <v-btn variant="tonal" color="error" @click="clearAllTokens" prepend-icon="mdi-delete">
        Очистить токены
      </v-btn>
      <v-spacer></v-spacer>
      <input 
        type="file" 
        @change="loadMap" 
        accept="image/*"
        ref="fileInput"
        style="display: none"
      >
      <v-btn variant="flat" color="primary" @click="() => $refs.fileInput.click()" prepend-icon="mdi-image-plus">
        Загрузить карту
      </v-btn>
    </v-toolbar>

    <div 
      class="map-canvas" 
      ref="mapCanvas"
      @click="handleCanvasClick"
      @dragover.prevent
      @drop="handleDrop"
    >
      <div class="map-content">
        <template v-if="currentMap">
          <img 
            :src="currentMap.url" 
            :alt="currentMap.name"
            class="map-image"
            @load="onMapLoad"
          >
        </template>

        <!-- Grid overlay (always render grid; if no map, show blank grid area) -->
        <svg 
          v-if="mapSettings.showGrid" 
          class="grid-overlay"
          :width="canvasSize.width"
          :height="canvasSize.height"
        >
          <defs>
            <pattern 
              id="grid" 
              :width="mapSettings.gridSize" 
              :height="mapSettings.gridSize" 
              patternUnits="userSpaceOnUse"
            >
              <path 
                :d="`M ${mapSettings.gridSize} 0 L 0 0 0 ${mapSettings.gridSize}`" 
                fill="none" 
                stroke="#fff" 
                :stroke-opacity="mapSettings.gridOpacity"
                stroke-width="1"
              />
            </pattern>
          </defs>
          <rect width="100%" height="100%" fill="url(#grid)" />
        </svg>

        <!-- Tokens -->
        <div
          v-for="token in tokens"
          :key="token.id"
          class="token"
          :style="{
            left: token.position.x + 'px',
            top: token.position.y + 'px',
            width: token.size.width + 'px',
            height: token.size.height + 'px',
            backgroundColor: token.color || '#8B4513'
          }"
          @mousedown="startDrag(token, $event)"
          @click.stop
        >
          <span class="token-label">{{ token.name }}</span>
        </div>

        <div v-if="!currentMap" class="no-map">
          <p>Пустая карта. Загрузите изображение или используйте сетку.</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { storeToRefs } from 'pinia'
import { useMapStore } from '../stores/mapStore'

const mapStore = useMapStore()
const { currentMap, tokens, mapSettings } = storeToRefs(mapStore)
const { loadFromLocalStorage } = mapStore

const mapCanvas = ref(null)
const canvasSize = ref({ width: 1200, height: 800 })
const isDragging = ref(false)
const dragToken = ref(null)
const dragOffset = ref({ x: 0, y: 0 })

function toggleGrid() {
  mapStore.updateMapSettings({ showGrid: !mapSettings.value.showGrid })
}

function clearAllTokens() {
  if (confirm('Удалить все токены?')) {
    mapStore.clearTokens()
  }
}

function loadMap(event) {
  const file = event.target.files[0]
  if (file) {
    const reader = new FileReader()
    reader.onload = (e) => {
      const mapData = {
        name: file.name,
        url: e.target.result,
        file: file
      }
      mapStore.setMap(mapData)
    }
    reader.readAsDataURL(file)
  }
}

function onMapLoad(event) {
  const img = event.target
  canvasSize.value = {
    width: img.naturalWidth,
    height: img.naturalHeight
  }
}

function handleCanvasClick(event) {
  if (event.target === mapCanvas.value || event.target.classList.contains('map-image')) {
    // Add new token at click position
    const rect = mapCanvas.value.getBoundingClientRect()
    const x = event.clientX - rect.left
    const y = event.clientY - rect.top
    
    const newToken = mapStore.addToken({
      name: 'Новый токен',
      color: '#8B4513'
    })
    
    mapStore.updateTokenPosition(newToken.id, { x, y })
  }
}

function handleDrop(event) {
  event.preventDefault()
  // Handle dropping monsters from monster list
  const monsterData = event.dataTransfer.getData('application/json')
  if (monsterData) {
    try {
      const monster = JSON.parse(monsterData)
      const rect = mapCanvas.value.getBoundingClientRect()
      const x = event.clientX - rect.left
      const y = event.clientY - rect.top
      
      const newToken = mapStore.addToken({
        name: monster.name,
        color: monster.color || '#8B4513',
        monsterId: monster.id
      })
      
      mapStore.updateTokenPosition(newToken.id, { x, y })
    } catch (error) {
      console.error('Error handling drop:', error)
    }
  }
}

function startDrag(token, event) {
  isDragging.value = true
  dragToken.value = token
  const rect = mapCanvas.value.getBoundingClientRect()
  dragOffset.value = {
    x: event.clientX - rect.left - token.position.x,
    y: event.clientY - rect.top - token.position.y
  }
  
  document.addEventListener('mousemove', handleDrag)
  document.addEventListener('mouseup', stopDrag)
}

function handleDrag(event) {
  if (isDragging.value && dragToken.value) {
    const rect = mapCanvas.value.getBoundingClientRect()
    const newX = event.clientX - rect.left - dragOffset.value.x
    const newY = event.clientY - rect.top - dragOffset.value.y
    
    mapStore.updateTokenPosition(dragToken.value.id, { x: newX, y: newY })
  }
}

function stopDrag() {
  isDragging.value = false
  dragToken.value = null
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)
}

onMounted(() => {
  // Load saved map data
  loadFromLocalStorage()
  
  // Initialize canvas size
  if (mapCanvas.value) {
    const rect = mapCanvas.value.getBoundingClientRect()
    if (!currentMap.value) {
      canvasSize.value = { width: Math.max(rect.width, 1200), height: Math.max(rect.height, 800) }
    }
  }
})

// Keep grid responsive when container resizes (debounced)
let resizeObserver
onMounted(() => {
  resizeObserver = new ResizeObserver(() => {
    if (!currentMap.value && mapCanvas.value) {
      const rect = mapCanvas.value.getBoundingClientRect()
      canvasSize.value = { width: Math.max(rect.width, 800), height: Math.max(rect.height, 600) }
    }
  })
  if (mapCanvas.value) resizeObserver.observe(mapCanvas.value)
})

onUnmounted(() => {
  document.removeEventListener('mousemove', handleDrag)
  document.removeEventListener('mouseup', stopDrag)
  if (resizeObserver && mapCanvas.value) resizeObserver.unobserve(mapCanvas.value)
})
</script>

<style scoped>
.map-container {
  display: flex;
  flex-direction: column;
  height: 100vh;
}

.map-controls {
  display: flex;
  gap: 10px;
  padding: 10px;
  background: #2c3e50;
  border-bottom: 1px solid #34495e;
}

.map-controls button {
  padding: 8px 16px;
  background: #34495e;
  color: white;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  transition: background 0.2s;
}

.map-controls button:hover {
  background: #4a5f7a;
}

.map-controls button.active {
  background: #3498db;
}

.map-canvas {
  flex: 1;
  position: relative;
  overflow: auto;
  background: #1a1a1a;
  display: flex;
  align-items: center;
  justify-content: center;
}

.no-map {
  color: #7f8c8d;
  font-size: 18px;
}

.map-content {
  position: relative;
  display: inline-block;
}

.map-image {
  max-width: 100%;
  max-height: 100%;
  display: block;
}

.grid-overlay {
  position: absolute;
  top: 0;
  left: 0;
  pointer-events: none;
}

.token {
  position: absolute;
  border: 2px solid #fff;
  border-radius: 50%;
  cursor: move;
  display: flex;
  align-items: center;
  justify-content: center;
  user-select: none;
  box-shadow: 0 2px 8px rgba(0,0,0,0.3);
  transition: transform 0.1s;
}

.token:hover {
  transform: scale(1.1);
}

.token-label {
  color: white;
  font-size: 12px;
  font-weight: bold;
  text-shadow: 1px 1px 2px rgba(0,0,0,0.8);
  text-align: center;
  word-break: break-word;
  padding: 2px;
}
</style>
