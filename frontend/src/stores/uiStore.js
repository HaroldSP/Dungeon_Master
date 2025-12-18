import { defineStore } from 'pinia';

const STORAGE_KEY_DRAWER = 'ui.drawerMinimized';
const STORAGE_KEY_AUTO_CLEAR = 'ui.resultsAutoClear';
const STORAGE_KEY_DETAILS_EXPANDED = 'ui.towerDetailsExpanded';
const STORAGE_KEY_DC_ENABLED = 'ui.difficultyClassEnabled';
const STORAGE_KEY_DC_VALUE = 'ui.difficultyClassValue';

const loadDrawerState = () => {
  if (typeof localStorage === 'undefined') return false;
  const raw = localStorage.getItem(STORAGE_KEY_DRAWER);
  return raw === 'true';
};

const persistDrawerState = value => {
  if (typeof localStorage === 'undefined') return;
  localStorage.setItem(STORAGE_KEY_DRAWER, value ? 'true' : 'false');
};

const loadAutoClearState = () => {
  if (typeof localStorage === 'undefined') return {};
  try {
    const raw = localStorage.getItem(STORAGE_KEY_AUTO_CLEAR);
    if (!raw) return {};
    const parsed = JSON.parse(raw);
    return typeof parsed === 'object' && parsed !== null ? parsed : {};
  } catch {
    return {};
  }
};

const persistAutoClearState = map => {
  if (typeof localStorage === 'undefined') return;
  try {
    localStorage.setItem(STORAGE_KEY_AUTO_CLEAR, JSON.stringify(map || {}));
  } catch {
    // ignore
  }
};

const loadDetailsExpandedState = () => {
  if (typeof localStorage === 'undefined') return {};
  try {
    const raw = localStorage.getItem(STORAGE_KEY_DETAILS_EXPANDED);
    if (!raw) return {};
    const parsed = JSON.parse(raw);
    return typeof parsed === 'object' && parsed !== null ? parsed : {};
  } catch {
    return {};
  }
};

const persistDetailsExpandedState = map => {
  if (typeof localStorage === 'undefined') return;
  try {
    localStorage.setItem(
      STORAGE_KEY_DETAILS_EXPANDED,
      JSON.stringify(map || {})
    );
  } catch {
    // ignore
  }
};

const loadDcEnabledState = () => {
  if (typeof localStorage === 'undefined') return {};
  try {
    const raw = localStorage.getItem(STORAGE_KEY_DC_ENABLED);
    if (!raw) return {};
    const parsed = JSON.parse(raw);
    return typeof parsed === 'object' && parsed !== null ? parsed : {};
  } catch {
    return {};
  }
};

const persistDcEnabledState = map => {
  if (typeof localStorage === 'undefined') return;
  try {
    localStorage.setItem(STORAGE_KEY_DC_ENABLED, JSON.stringify(map || {}));
  } catch {
    // ignore
  }
};

const loadDcValueState = () => {
  if (typeof localStorage === 'undefined') return {};
  try {
    const raw = localStorage.getItem(STORAGE_KEY_DC_VALUE);
    if (!raw) return {};
    const parsed = JSON.parse(raw);
    return typeof parsed === 'object' && parsed !== null ? parsed : {};
  } catch {
    return {};
  }
};

const persistDcValueState = map => {
  if (typeof localStorage === 'undefined') return;
  try {
    localStorage.setItem(STORAGE_KEY_DC_VALUE, JSON.stringify(map || {}));
  } catch {
    // ignore
  }
};

export const useUiStore = defineStore('ui', {
  state: () => ({
    drawerMinimized: loadDrawerState(),
    // { [towerId]: boolean } – true / undefined → auto clear ON, false → OFF
    resultsAutoClear: loadAutoClearState(),
    // { [towerId]: boolean } – true → details expanded, false/undefined → collapsed
    towerDetailsExpanded: loadDetailsExpandedState(),
    // { [towerId]: boolean } – true → stream is currently active/connected (not persisted)
    towerStreamActive: {},
    // { [towerId]: boolean } – true → difficulty class enabled, false/undefined → disabled
    difficultyClassEnabled: loadDcEnabledState(),
    // { [towerId]: number } – difficulty class value per tower
    difficultyClassValue: loadDcValueState(),
  }),
  actions: {
    setDrawerMinimized(value) {
      this.drawerMinimized = value;
      persistDrawerState(value);
    },
    toggleDrawer() {
      this.setDrawerMinimized(!this.drawerMinimized);
    },
    setResultsAutoClear(towerId, value) {
      if (!towerId && towerId !== 0) return;
      this.resultsAutoClear = {
        ...this.resultsAutoClear,
        [towerId]: !!value,
      };
      persistAutoClearState(this.resultsAutoClear);
    },
    setTowerDetailsExpanded(towerId, value) {
      if (!towerId && towerId !== 0) return;
      this.towerDetailsExpanded = {
        ...this.towerDetailsExpanded,
        [towerId]: !!value,
      };
      persistDetailsExpandedState(this.towerDetailsExpanded);
    },
    setTowerStreamActive(towerId, value) {
      if (!towerId && towerId !== 0) return;
      this.towerStreamActive = {
        ...this.towerStreamActive,
        [towerId]: !!value,
      };
    },
    setDifficultyClassEnabled(towerId, value) {
      if (!towerId && towerId !== 0) return;
      this.difficultyClassEnabled = {
        ...this.difficultyClassEnabled,
        [towerId]: !!value,
      };
      persistDcEnabledState(this.difficultyClassEnabled);
    },
    setDifficultyClassValue(towerId, value) {
      if (!towerId && towerId !== 0) return;
      const numValue =
        value === '' || value === null || value === undefined
          ? null
          : Number(value);
      console.log('[uiStore] setDifficultyClassValue:', {
        towerId,
        inputValue: value,
        numValue,
        isNaN: isNaN(numValue),
      });
      this.difficultyClassValue = {
        ...this.difficultyClassValue,
        [towerId]: numValue,
      };
      persistDcValueState(this.difficultyClassValue);
      console.log('[uiStore] DC values after save:', this.difficultyClassValue);
    },
  },
});
