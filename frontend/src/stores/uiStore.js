import { defineStore } from 'pinia';

const STORAGE_KEY = 'ui.drawerMinimized';

const loadDrawerState = () => {
  if (typeof localStorage === 'undefined') return false;
  const raw = localStorage.getItem(STORAGE_KEY);
  return raw === 'true';
};

const persistDrawerState = value => {
  if (typeof localStorage === 'undefined') return;
  localStorage.setItem(STORAGE_KEY, value ? 'true' : 'false');
};

export const useUiStore = defineStore('ui', {
  state: () => ({
    drawerMinimized: loadDrawerState(),
  }),
  actions: {
    setDrawerMinimized(value) {
      this.drawerMinimized = value;
      persistDrawerState(value);
    },
    toggleDrawer() {
      this.setDrawerMinimized(!this.drawerMinimized);
    },
  },
});

