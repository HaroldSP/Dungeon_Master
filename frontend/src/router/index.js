import { createRouter, createWebHistory } from 'vue-router'
import MapView from '../views/MapView.vue'
import MonsterManager from '../views/MonsterManager.vue'

const routes = [
  {
    path: '/',
    name: 'Map',
    component: MapView
  },
  {
    path: '/monsters',
    name: 'Monsters',
    component: MonsterManager
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

export default router

