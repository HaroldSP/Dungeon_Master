import { defineStore } from 'pinia';
import { ref, computed, watch } from 'vue';

export const useTowerStore = defineStore('towers', () => {
  const towers = ref([]); // [{ id, name, url, lastSeenAt, online }]

  const count = computed(() => towers.value.length);

  // Auto-load from localStorage on store creation
  load();

  function addOrUpdateTower(partial) {
    const id = partial.id || partial.url || Date.now().toString();
    const existing = towers.value.find(
      t => t.id === id || t.url === partial.url
    );
    const nowIso = new Date().toISOString();
    if (existing) {
      Object.assign(existing, partial);
      existing.lastSeenAt = nowIso;
      return existing;
    }
    const item = {
      id,
      name: partial.name || `Tower ${count.value + 1}`,
      url: partial.url || '',
      lastSeenAt: nowIso,
      online: partial.online || false,
    };
    towers.value.push(item);
    return item;
  }

  function removeTower(id) {
    const i = towers.value.findIndex(t => t.id === id);
    if (i > -1) towers.value.splice(i, 1);
  }

  function cleanInvalidTowers() {
    const validTowers = towers.value.filter(
      tower =>
        tower.url &&
        tower.url.trim() !== '' &&
        (tower.url.startsWith('http://') || tower.url.startsWith('https://'))
    );
    towers.value = validTowers;
  }

  function load() {
    try {
      const raw = localStorage.getItem('dice-tower-list');
      if (raw) {
        const parsed = JSON.parse(raw);
        // Filter out towers with empty or invalid URLs
        const validTowers = parsed.filter(
          tower =>
            tower.url &&
            tower.url.trim() !== '' &&
            (tower.url.startsWith('http://') ||
              tower.url.startsWith('https://'))
        );
        towers.value = validTowers;
      } else {
        towers.value = [];
      }
    } catch (e) {
      console.error('towerStore load error', e);
      towers.value = [];
    }
  }

  function save() {
    try {
      localStorage.setItem('dice-tower-list', JSON.stringify(towers.value));
    } catch (e) {
      console.error('towerStore save error', e);
    }
  }

  watch(towers, save, { deep: true });

  return {
    towers,
    count,
    addOrUpdateTower,
    removeTower,
    load,
    save,
    cleanInvalidTowers,
  };
});
