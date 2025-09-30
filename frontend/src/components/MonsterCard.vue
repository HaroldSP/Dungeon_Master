<template>
  <v-card
    class="monster-card"
    :elevation="isSelected ? 8 : 2"
    @click="toggleSelection"
    draggable="true"
    @dragstart="handleDragStart"
  >
    <v-card-title class="d-flex justify-space-between align-center">
      <div class="text-truncate">{{ monster.name }}</div>
      <div class="d-flex ga-1">
        <v-btn size="small" icon="mdi-pencil" variant="text" @click.stop="editMonster" />
        <v-btn size="small" icon="mdi-delete" color="error" variant="text" @click.stop="deleteMonster" />
      </div>
    </v-card-title>

    <v-card-text>
      <div class="monster-stats">
        <div class="stat-row">
          <span class="stat-label">ХП:</span>
          <span class="stat-value">{{ monster.hitPoints || 'N/A' }}</span>
        </div>
        <div class="stat-row">
          <span class="stat-label">Класс доспеха:</span>
          <span class="stat-value">{{ monster.armorClass || 'N/A' }}</span>
        </div>
        <div class="stat-row">
          <span class="stat-label">Скорость:</span>
          <span class="stat-value">{{ monster.speed || 'N/A' }}</span>
        </div>
        <div class="stat-row" v-if="monster.challengeRating">
          <span class="stat-label">Уровень сложности:</span>
          <span class="stat-value">{{ monster.challengeRating }}</span>
        </div>
      </div>

      <div class="monster-description" v-if="monster.description">
        <p>{{ monster.description }}</p>
      </div>

      <div class="monster-abilities" v-if="monster.abilities && monster.abilities.length">
        <h4>Способности:</h4>
        <ul>
          <li v-for="ability in monster.abilities" :key="ability.name">
            <strong>{{ ability.name }}:</strong> {{ ability.description }}
          </li>
        </ul>
      </div>
    </v-card-text>
  </v-card>
</template>

<script setup>
import { computed } from 'vue'
import { useMonsterStore } from '../stores/monsterStore'

const props = defineProps({
  monster: {
    type: Object,
    required: true
  }
})

const emit = defineEmits(['edit', 'delete'])

const monsterStore = useMonsterStore()

const isSelected = computed(() => 
  monsterStore.selectedMonsters.some(m => m.id === props.monster.id)
)

function toggleSelection() {
  if (isSelected.value) {
    monsterStore.deselectMonster(props.monster.id)
  } else {
    monsterStore.selectMonster(props.monster)
  }
}

function editMonster() {
  emit('edit', props.monster)
}

function deleteMonster() {
  if (confirm(`Удалить монстра "${props.monster.name}"?`)) {
    monsterStore.removeMonster(props.monster.id)
    emit('delete', props.monster)
  }
}

function handleDragStart(event) {
  event.dataTransfer.setData('application/json', JSON.stringify(props.monster))
  event.dataTransfer.effectAllowed = 'copy'
}
</script>

<style scoped>
.monster-card { cursor: pointer; }
.monster-stats { margin-bottom: 12px; }
.stat-row { display: flex; justify-content: space-between; margin-bottom: 4px; }
.stat-label { opacity: .8; }
.monster-description { margin-top: 8px; }
.monster-abilities h4 { font-size: 14px; margin: 8px 0; }
.monster-abilities ul { margin: 0; padding-left: 16px; }
</style>

