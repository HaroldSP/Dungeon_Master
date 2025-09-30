import { defineStore } from 'pinia'
import { ref, watch } from 'vue'

export const useMapStore = defineStore('map', () => {
  // State
  const currentMap = ref(null)
  const tokens = ref([])
  const mapSettings = ref({
    gridSize: 50,
    showGrid: true,
    gridOpacity: 0.3
  })

  // Actions
  function setMap(mapData) {
    currentMap.value = mapData
  }

  function addToken(token) {
    const newToken = {
      id: Date.now().toString(),
      ...token,
      position: { x: 0, y: 0 },
      size: { width: 40, height: 40 }
    }
    tokens.value.push(newToken)
    return newToken
  }

  function removeToken(id) {
    const index = tokens.value.findIndex(t => t.id === id)
    if (index > -1) {
      tokens.value.splice(index, 1)
    }
  }

  function updateTokenPosition(id, position) {
    const token = tokens.value.find(t => t.id === id)
    if (token) {
      token.position = position
    }
  }

  function updateTokenSize(id, size) {
    const token = tokens.value.find(t => t.id === id)
    if (token) {
      token.size = size
    }
  }

  function clearTokens() {
    tokens.value = []
  }

  function updateMapSettings(settings) {
    Object.assign(mapSettings.value, settings)
  }

  function saveToLocalStorage() {
    try {
      const mapData = {
        currentMap: currentMap.value,
        tokens: tokens.value,
        mapSettings: mapSettings.value
      }
      localStorage.setItem('vue-master-screen-map', JSON.stringify(mapData))
    } catch (error) {
      console.error('Error saving map to localStorage:', error)
    }
  }

  function loadFromLocalStorage() {
    try {
      const saved = localStorage.getItem('vue-master-screen-map')
      if (saved) {
        const parsedData = JSON.parse(saved)
        currentMap.value = parsedData.currentMap
        tokens.value = parsedData.tokens || []
        mapSettings.value = parsedData.mapSettings || {
          gridSize: 50,
          showGrid: true,
          gridOpacity: 0.3
        }
      }
    } catch (error) {
      console.error('Error loading map from localStorage:', error)
    }
  }

  // Auto-save when map data changes
  watch([currentMap, tokens, mapSettings], saveToLocalStorage, { deep: true })

  return {
    // State
    currentMap,
    tokens,
    mapSettings,
    // Actions
    setMap,
    addToken,
    removeToken,
    updateTokenPosition,
    updateTokenSize,
    clearTokens,
    updateMapSettings,
    saveToLocalStorage,
    loadFromLocalStorage
  }
})
