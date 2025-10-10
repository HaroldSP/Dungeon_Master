<template>
  <v-app-bar
    app
    color="surface"
    elevation="1"
    class="monster-toolbar"
  >
    <v-app-bar-title>
      <span class="text-h6">Управление монстрами</span>
    </v-app-bar-title>

    <div class="d-flex align-center ga-2">
      <v-chip
        color="primary"
        variant="elevated"
        size="small"
      >
        👹 {{ monsterCount }} монстров
      </v-chip>
      <v-chip
        color="secondary"
        variant="elevated"
        size="small"
      >
        🎯 {{ selectedCount }} выбрано
      </v-chip>
    </div>

    <v-spacer></v-spacer>
    <v-tabs
      v-model="activeTab"
      class="d-flex"
    >
      <v-tab value="list">Список монстров</v-tab>
      <v-tab value="actions">Действия</v-tab>
    </v-tabs>
  </v-app-bar>

  <v-container
    fluid
    class="monster-manager-view pa-4"
  >
    <v-window v-model="activeTab">
      <v-window-item value="list">
        <v-row>
          <v-col cols="12">
            <div
              v-if="monsters.length === 0"
              class="text-center py-8"
            >
              <v-icon
                size="64"
                color="grey"
                >mdi-skull</v-icon
              >
              <p class="text-h6 mt-4">Нет монстров</p>
              <p class="text-body-1">
                Добавьте первого монстра или импортируйте из JSON
              </p>
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
      </v-window-item>

      <v-window-item value="actions">
        <v-row>
          <v-col cols="12">
            <v-card class="mb-4">
              <v-card-title class="text-h6">Действия с монстрами</v-card-title>
              <v-card-text>
                <div class="d-flex flex-column gap-3">
                  <v-btn
                    color="success"
                    @click="showAddForm"
                    prepend-icon="mdi-plus"
                    text="Добавить монстра"
                    size="large"
                  />
                  <v-btn
                    color="primary"
                    @click="showImportDialog"
                    prepend-icon="mdi-import"
                    text="Импорт JSON"
                    size="large"
                  />
                  <v-btn
                    color="secondary"
                    @click="exportMonsters"
                    prepend-icon="mdi-export"
                    text="Экспорт JSON"
                    size="large"
                  />
                  <v-btn
                    color="warning"
                    @click="loadSampleMonsters"
                    prepend-icon="mdi-book"
                    text="Загрузить примеры"
                    size="large"
                  />
                  <v-btn
                    v-if="selectedCount > 0"
                    color="error"
                    @click="clearSelected"
                    prepend-icon="mdi-close"
                    :text="`Очистить выбор (${selectedCount})`"
                    size="large"
                  />
                </div>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </v-window-item>
    </v-window>

    <!-- Import Dialog -->
    <v-dialog
      v-model="showImport"
      max-width="600"
    >
      <v-card>
        <v-card-title class="d-flex justify-space-between align-center">
          <span>Импорт монстров из JSON</span>
          <v-btn
            icon="mdi-close"
            variant="text"
            @click="closeImportDialog"
          />
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
          <v-btn
            variant="text"
            text="Отмена"
            @click="closeImportDialog"
          />
          <v-btn
            color="primary"
            text="Импортировать"
            @click="importMonsters"
          />
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
  import { ref, computed, onMounted } from 'vue';
  import { useMonsterStore } from '../stores/monsterStore';
  import MonsterCard from '../components/MonsterCard.vue';
  import MonsterForm from '../components/MonsterForm.vue';
  import sampleMonsters from '../data/sampleMonsters.json';

  const monsterStore = useMonsterStore();
  const {
    monsters,
    monsterCount,
    selectedCount,
    clearSelected,
    loadFromLocalStorage,
  } = monsterStore;

  const activeTab = ref('list');
  const showForm = ref(false);
  const editingMonster = ref(null);
  const showImport = ref(false);
  const importText = ref('');

  function showAddForm() {
    editingMonster.value = null;
    showForm.value = true;
  }

  function editMonster(monster) {
    editingMonster.value = monster;
    showForm.value = true;
  }

  function closeForm() {
    showForm.value = false;
    editingMonster.value = null;
  }

  function handleFormSubmit(monsterData) {
    if (editingMonster.value) {
      monsterStore.updateMonster(editingMonster.value.id, monsterData);
    } else {
      monsterStore.addMonster(monsterData);
    }
  }

  function onMonsterDelete(monster) {
    // Monster is already deleted in the card component
    console.log('Monster deleted:', monster.name);
  }

  function showImportDialog() {
    showImport.value = true;
    importText.value = '';
  }

  function closeImportDialog() {
    showImport.value = false;
    importText.value = '';
  }

  function importMonsters() {
    const result = monsterStore.importMonsters(importText.value);
    if (result.success) {
      alert(`Успешно импортировано ${result.count} монстров`);
      closeImportDialog();
    } else {
      alert(`Ошибка импорта: ${result.error}`);
    }
  }

  function exportMonsters() {
    const data = monsterStore.exportMonsters();
    const blob = new Blob([data], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'monsters.json';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  function loadSampleMonsters() {
    const result = monsterStore.importMonsters(sampleMonsters);
    if (result.success) {
      alert(`Загружено ${result.count} примеров монстров`);
    } else {
      alert(`Ошибка загрузки примеров: ${result.error}`);
    }
  }

  onMounted(() => {
    loadFromLocalStorage();
    // Only load samples if no monsters are saved
    if (monsterStore.monsterCount === 0) {
      loadSampleMonsters();
    }
  });
</script>

<style scoped>
  .monster-manager-view {
    height: 100vh;
    overflow-y: auto;
  }
</style>
