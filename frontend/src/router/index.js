import { createRouter, createWebHistory } from 'vue-router';
import MapView from '../views/MapView.vue';
import MonsterManager from '../views/MonsterManager.vue';
import DiceTowers from '../views/DiceTowers.vue';
import Players from '../views/Players.vue';
import SpellMapper from '../views/SpellMapper.vue';

const routes = [
  {
    path: '/',
    name: 'Map',
    component: MapView,
  },
  {
    path: '/monsters',
    name: 'Monsters',
    component: MonsterManager,
  },
  {
    path: '/towers',
    name: 'DiceTowers',
    component: DiceTowers,
  },
  {
    path: '/players',
    name: 'Players',
    component: Players,
  },
  {
    path: '/spell-mapper',
    name: 'SpellMapper',
    component: SpellMapper,
  },
];

const router = createRouter({
  history: createWebHistory(),
  routes,
});

export default router;
