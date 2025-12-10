import { defineStore } from 'pinia';
import { ref, computed, watch } from 'vue';

export const useTowerStore = defineStore('towers', () => {
  // tower: { id, playerName, towerName, apiBase, streamUrl, online, lastSeenAt, lastStatus, lastDetection }
  const towers = ref([]);
  const count = computed(() => towers.value.length);

  load();

  function addOrUpdateTower(partial) {
    const id = partial.id || partial.apiBase || Date.now().toString();
    const existing = towers.value.find(
      t => t.id === id || (partial.apiBase && t.apiBase === partial.apiBase)
    );
    const nowIso = new Date().toISOString();
    if (existing) {
      Object.assign(existing, partial);
      existing.lastSeenAt = nowIso;
      return existing;
    }
    const item = {
      id,
      playerName: partial.playerName || 'Player',
      towerName: partial.towerName || `Tower ${count.value + 1}`,
      apiBase: partial.apiBase || '',
      streamUrl: partial.streamUrl || '',
      online: partial.online || false,
      lastSeenAt: nowIso,
      lastStatus: null,
      lastDetection: null,
    };
    towers.value.push(item);
    return item;
  }

  function setStatus(id, status) {
    const t = towers.value.find(t => t.id === id);
    if (!t) return;
    t.online = !!status?.online;
    t.lastSeenAt = new Date().toISOString();
    t.lastStatus = status || null;
    if (status?.raw) {
      const staIp = status.raw?.wifi?.sta?.ip || status.raw?.sta_ip;
      const apIp = status.raw?.ap_ip;
      if (staIp) t.lastStaIp = staIp;
      if (apIp) t.lastApIp = apIp;
    }
    if (status?.dice) {
      t.lastDetection = status.dice;
    }
  }

  function removeTower(id) {
    const i = towers.value.findIndex(t => t.id === id);
    if (i > -1) towers.value.splice(i, 1);
  }

  function cleanInvalidTowers() {
    towers.value = towers.value.filter(
      t =>
        t.apiBase &&
        t.apiBase.trim() !== '' &&
        (t.apiBase.startsWith('http://') || t.apiBase.startsWith('https://'))
    );
  }

  function load() {
    try {
      const raw = localStorage.getItem('dice-tower-list');
      if (raw) {
        const parsed = JSON.parse(raw);
        towers.value = Array.isArray(parsed) ? parsed : [];
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
    cleanInvalidTowers,
    setStatus,
    load,
    save,
  };
});
