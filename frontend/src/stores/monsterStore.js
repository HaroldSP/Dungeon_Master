import { defineStore } from 'pinia'
import { ref, computed, watch } from 'vue'

export const useMonsterStore = defineStore('monster', () => {
  // State
  const monsters = ref([])
  const selectedMonsters = ref([])

  // Getters
  const monsterCount = computed(() => monsters.value.length)
  const selectedCount = computed(() => selectedMonsters.value.length)

  // Actions
  function addMonster(monster) {
    const newMonster = {
      id: Date.now().toString(),
      ...monster,
      createdAt: new Date().toISOString()
    }
    monsters.value.push(newMonster)
    return newMonster
  }

  function removeMonster(id) {
    const index = monsters.value.findIndex(m => m.id === id)
    if (index > -1) {
      monsters.value.splice(index, 1)
    }
    // Also remove from selected if present
    const selectedIndex = selectedMonsters.value.findIndex(m => m.id === id)
    if (selectedIndex > -1) {
      selectedMonsters.value.splice(selectedIndex, 1)
    }
  }

  function updateMonster(id, updates) {
    const monster = monsters.value.find(m => m.id === id)
    if (monster) {
      Object.assign(monster, updates)
    }
  }

  function selectMonster(monster) {
    if (!selectedMonsters.value.find(m => m.id === monster.id)) {
      selectedMonsters.value.push(monster)
    }
  }

  function deselectMonster(id) {
    const index = selectedMonsters.value.findIndex(m => m.id === id)
    if (index > -1) {
      selectedMonsters.value.splice(index, 1)
    }
  }

  function clearSelected() {
    selectedMonsters.value = []
  }

  function importMonsters(monsterData) {
    try {
      const data = typeof monsterData === 'string' ? JSON.parse(monsterData) : monsterData
      const monstersArray = Array.isArray(data) ? data : [data]
      
      monstersArray.forEach(monster => {
        addMonster(monster)
      })
      
      return { success: true, count: monstersArray.length }
    } catch (error) {
      return { success: false, error: error.message }
    }
  }

  function exportMonsters() {
    return JSON.stringify(monsters.value, null, 2)
  }

  function saveToLocalStorage() {
    try {
      localStorage.setItem('vue-master-screen-monsters', JSON.stringify(monsters.value))
    } catch (error) {
      console.error('Error saving monsters to localStorage:', error)
    }
  }

  function loadFromLocalStorage() {
    try {
      const saved = localStorage.getItem('vue-master-screen-monsters')
      if (saved) {
        const parsedMonsters = JSON.parse(saved)
        monsters.value = parsedMonsters
      }
    } catch (error) {
      console.error('Error loading monsters from localStorage:', error)
    }
  }

  // Auto-save when monsters change
  watch(monsters, saveToLocalStorage, { deep: true })

  return {
    // State
    monsters,
    selectedMonsters,
    // Getters
    monsterCount,
    selectedCount,
    // Actions
    addMonster,
    removeMonster,
    updateMonster,
    selectMonster,
    deselectMonster,
    clearSelected,
    importMonsters,
    exportMonsters,
    saveToLocalStorage,
    loadFromLocalStorage
  }
})
