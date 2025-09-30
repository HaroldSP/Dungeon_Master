<template>
  <v-dialog :model-value="show" @click:outside="closeForm" max-width="760">
    <v-card>
      <v-card-title class="d-flex justify-space-between align-center">
        <span>{{ isEditing ? 'Редактировать монстра' : 'Добавить монстра' }}</span>
        <v-btn icon="mdi-close" variant="text" @click="closeForm"/>
      </v-card-title>
      <v-card-text>
        <v-form @submit.prevent="submitForm">
          <v-container fluid>
            <v-row>
              <v-col cols="12" md="6">
                <v-text-field v-model="form.name" label="Имя" required placeholder="Название монстра" />
              </v-col>
              <v-col cols="12" md="3">
                <v-text-field v-model="form.hitPoints" type="number" label="Хиты" />
              </v-col>
              <v-col cols="12" md="3">
                <v-text-field v-model="form.armorClass" type="number" label="Класс доспеха" />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field v-model="form.speed" label="Скорость" placeholder="30 фт." />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field v-model="form.challengeRating" label="Уровень сложности" placeholder="1/2" />
              </v-col>
              <v-col cols="12" md="4">
                <v-text-field v-model="form.color" label="Цвет" placeholder="#8B4513" />
              </v-col>
              <v-col cols="12">
                <v-textarea v-model="form.description" rows="3" label="Описание" placeholder="Описание монстра, его особенности..." />
              </v-col>
            </v-row>

            <v-divider class="my-4" />

            <div class="d-flex align-center justify-space-between mb-2">
              <h3 class="text-subtitle-1">Способности</h3>
              <v-btn size="small" color="primary" variant="tonal" @click="addAbility" prepend-icon="mdi-plus">Добавить способность</v-btn>
            </div>

            <div v-for="(ability, index) in form.abilities" :key="index" class="mb-2">
              <v-row>
                <v-col cols="12" md="4">
                  <v-text-field v-model="ability.name" label="Название способности" />
                </v-col>
                <v-col cols="12" md="7">
                  <v-textarea v-model="ability.description" label="Описание способности" rows="2" auto-grow />
                </v-col>
                <v-col cols="12" md="1" class="d-flex align-center">
                  <v-btn icon="mdi-delete" color="error" variant="text" @click="removeAbility(index)" />
                </v-col>
              </v-row>
            </div>
          </v-container>
        </v-form>
      </v-card-text>
      <v-card-actions>
        <v-spacer></v-spacer>
        <v-btn variant="text" @click="closeForm">Отмена</v-btn>
        <v-btn color="primary" @click="submitForm">{{ isEditing ? 'Сохранить' : 'Добавить' }}</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup>
import { ref, watch, reactive } from 'vue'

const props = defineProps({
  show: {
    type: Boolean,
    default: false
  },
  monster: {
    type: Object,
    default: null
  }
})

const emit = defineEmits(['close', 'submit'])

const isEditing = ref(false)
const form = reactive({
  name: '',
  hitPoints: '',
  armorClass: '',
  speed: '',
  challengeRating: '',
  color: '#8B4513',
  description: '',
  abilities: []
})

function resetForm() {
  form.name = ''
  form.hitPoints = ''
  form.armorClass = ''
  form.speed = ''
  form.challengeRating = ''
  form.color = '#8B4513'
  form.description = ''
  form.abilities = []
}

function populateForm(monster) {
  form.name = monster.name || ''
  form.hitPoints = monster.hitPoints || ''
  form.armorClass = monster.armorClass || ''
  form.speed = monster.speed || ''
  form.challengeRating = monster.challengeRating || ''
  form.color = monster.color || '#8B4513'
  form.description = monster.description || ''
  form.abilities = monster.abilities ? [...monster.abilities] : []
}

watch(() => props.monster, (newMonster) => {
  if (newMonster) {
    isEditing.value = true
    populateForm(newMonster)
  } else {
    isEditing.value = false
    resetForm()
  }
}, { immediate: true })

function addAbility() {
  form.abilities.push({ name: '', description: '' })
}

function removeAbility(index) {
  form.abilities.splice(index, 1)
}

function submitForm() {
  const monsterData = {
    name: form.name,
    hitPoints: form.hitPoints ? parseInt(form.hitPoints) : null,
    armorClass: form.armorClass ? parseInt(form.armorClass) : null,
    speed: form.speed,
    challengeRating: form.challengeRating,
    color: form.color,
    description: form.description,
    abilities: form.abilities.filter(a => a.name.trim() || a.description.trim())
  }
  
  emit('submit', monsterData)
  closeForm()
}

function closeForm() {
  emit('close')
  resetForm()
}
</script>

<style scoped>
</style>

