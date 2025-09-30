<template>
  <v-container fluid class="monster-manager">
    <v-row>
      <v-col cols="12">
        <div class="d-flex justify-space-between align-center mb-4">
          <h1 class="text-h4">Управление монстрами</h1>
          <div class="d-flex gap-2 flex-wrap">
            <v-btn color="success" @click="showAddForm" prepend-icon="mdi-plus">
              Добавить монстра
            </v-btn>
            <v-btn color="primary" @click="showImportDialog" prepend-icon="mdi-import">
              Импорт JSON
            </v-btn>
            <v-btn color="secondary" @click="exportMonsters" prepend-icon="mdi-export">
              Экспорт JSON
            </v-btn>
            <v-btn color="warning" @click="loadSampleMonsters" prepend-icon="mdi-book">
              Загрузить примеры
            </v-btn>
            <v-btn 
              v-if="selectedCount > 0" 
              color="error" 
              @click="clearSelected" 
              prepend-icon="mdi-close"
            >
              Очистить выбор ({{ selectedCount }})
            </v-btn>
          </div>
        </div>
      </v-col>
    </v-row>
    
    <v-row>
      <v-col cols="12">
        <v-card class="mb-4">
          <v-card-text>
            <v-row>
              <v-col cols="6" md="3">
                <div class="text-center">
                  <div class="text-h6 text-primary">{{ monsterCount }}</div>
                  <div class="text-caption">Всего монстров</div>
                </div>
              </v-col>
              <v-col cols="6" md="3">
                <div class="text-center">
                  <div class="text-h6 text-secondary">{{ selectedCount }}</div>
                  <div class="text-caption">Выбрано</div>
                </div>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
    
    <v-row>
      <v-col cols="12">
        <div v-if="monsters.length === 0" class="text-center py-8">
          <v-icon size="64" color="grey">mdi-skull</v-icon>
          <p class="text-h6 mt-4">Нет монстров</p>
          <p class="text-body-1">Добавьте первого монстра или импортируйте из JSON</p>
        </div>
        <v-row v-else>
          <v-col 
            v-for="monster in monsters"
            :key="monster.id"
            cols="12" 
            sm="6" 
            md="4" 
            lg="3"
          >
            <MonsterCard
              :monster="monster"
              @edit="editMonster"
              @delete="onMonsterDelete"
            />
          </v-col>
        </v-row>
      </v-col>
    </v-row>
    
    <!-- Import Dialog -->
    <v-dialog v-model="showImport" max-width="600">
      <v-card>
        <v-card-title class="d-flex justify-space-between align-center">
          <span>Импорт монстров из JSON</span>
          <v-btn icon="mdi-close" variant="text" @click="closeImportDialog"/>
        </v-card-title>
        <v-card-text>
          <v-textarea 
            v-model="importText" 
            label="JSON данные монстров"
            placeholder="Вставьте JSON с данными монстров..."
            rows="10"
            variant="outlined"
          />
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn variant="text" @click="closeImportDialog">Отмена</v-btn>
          <v-btn color="primary" @click="importMonsters">Импортировать</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
    
    <!-- Monster Form -->
    <MonsterForm
      :show="showForm"
      :monster="editingMonster"
      @close="closeForm"
      @submit="handleFormSubmit"
    />
  </v-container>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useMonsterStore } from '../stores/monsterStore'
import MonsterCard from '../components/MonsterCard.vue'
import MonsterForm from '../components/MonsterForm.vue'
import sampleMonsters from '../data/sampleMonsters.json'

const monsterStore = useMonsterStore()
const { monsters, monsterCount, selectedCount, clearSelected, loadFromLocalStorage } = monsterStore

const showForm = ref(false)
const editingMonster = ref(null)
const showImport = ref(false)
const importText = ref('')

function showAddForm() {
  editingMonster.value = null
  showForm.value = true
}

function editMonster(monster) {
  editingMonster.value = monster
  showForm.value = true
}

function closeForm() {
  showForm.value = false
  editingMonster.value = null
}

function handleFormSubmit(monsterData) {
  if (editingMonster.value) {
    monsterStore.updateMonster(editingMonster.value.id, monsterData)
  } else {
    monsterStore.addMonster(monsterData)
  }
}

function onMonsterDelete(monster) {
  // Monster is already deleted in the card component
  console.log('Monster deleted:', monster.name)
}

function showImportDialog() {
  showImport.value = true
  importText.value = ''
}

function closeImportDialog() {
  showImport.value = false
  importText.value = ''
}

function importMonsters() {
  const result = monsterStore.importMonsters(importText.value)
  if (result.success) {
    alert(`Успешно импортировано ${result.count} монстров`)
    closeImportDialog()
  } else {
    alert(`Ошибка импорта: ${result.error}`)
  }
}

function exportMonsters() {
  const data = monsterStore.exportMonsters()
  const blob = new Blob([data], { type: 'application/json' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = 'monsters.json'
  document.body.appendChild(a)
  a.click()
  document.body.removeChild(a)
  URL.revokeObjectURL(url)
}

function loadSampleMonsters() {
  const result = monsterStore.importMonsters(sampleMonsters)
  if (result.success) {
    alert(`Загружено ${result.count} примеров монстров`)
  } else {
    alert(`Ошибка загрузки примеров: ${result.error}`)
  }
}

onMounted(() => {
  loadFromLocalStorage()
  // Only load samples if no monsters are saved
  if (monsterStore.monsterCount === 0) {
    loadSampleMonsters()
  }
})
</script>

<style scoped>
.monster-manager {
  min-height: 100vh;
}
</style>
