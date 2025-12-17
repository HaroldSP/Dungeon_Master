import { defineStore } from 'pinia';
import { ref, watch } from 'vue';

const STORAGE_KEY = 'dm_roll_broadcast';

/**
 * Store for broadcasting roll results to the player screen.
 * Uses localStorage + storage events for cross-tab communication.
 */
export const useRollBroadcastStore = defineStore('rollBroadcast', () => {
  const currentRoll = ref(null);

  // Write to localStorage (called by DM tab)
  function writeToStorage(data) {
    try {
      const payload = JSON.stringify({ ...data, _ts: Date.now() });
      localStorage.setItem(STORAGE_KEY, payload);
      console.log('[RollBroadcast] Written to storage:', data?.status || 'cleared');
    } catch (e) {
      console.error('[RollBroadcast] Failed to write:', e);
    }
  }

  // Read from localStorage
  function readFromStorage() {
    try {
      const raw = localStorage.getItem(STORAGE_KEY);
      if (!raw) return null;
      const parsed = JSON.parse(raw);
      // Ignore stale data (older than 60 seconds)
      if (parsed._ts && Date.now() - parsed._ts > 60000) {
        return null;
      }
      return parsed;
    } catch (e) {
      console.error('[RollBroadcast] Failed to read:', e);
      return null;
    }
  }

  // Listen for storage changes from OTHER tabs
  function setupStorageListener() {
    // Listen for ALL storage events first (debug)
    window.addEventListener('storage', (event) => {
      console.log('[RollBroadcast] ANY storage event:', event.key, event.newValue?.slice(0, 50));
      
      if (event.key !== STORAGE_KEY) return;
      
      console.log('[RollBroadcast] Our key changed!');
      
      if (!event.newValue) {
        console.log('[RollBroadcast] Storage cleared -> currentRoll = null');
        currentRoll.value = null;
        return;
      }

      try {
        const data = JSON.parse(event.newValue);
        console.log('[RollBroadcast] Parsed data:', data?.status, data?.playerName);
        currentRoll.value = data;
      } catch (e) {
        console.error('[RollBroadcast] Failed to parse storage event:', e);
      }
    });
    console.log('[RollBroadcast] Storage listener ready');
    console.log('[RollBroadcast] Listening for key:', STORAGE_KEY);
  }

  // === Public API ===

  function startRolling(payload) {
    const data = {
      id: `roll_${Date.now()}`,
      status: 'rolling',
      mode: payload.mode || 'normal',
      playerName: payload.playerName || '',
      label: payload.label || '',
      dice: null,
      value: null,
      chosenValue: null,
      modifier: payload.modifier ?? 0,
      total: null,
      isNat1: false,
      isNat20: false,
    };
    currentRoll.value = data;
    writeToStorage(data);
  }

  function showResult(payload) {
    const data = {
      id: currentRoll.value?.id || `roll_${Date.now()}`,
      status: 'result',
      mode: payload.mode || 'normal',
      playerName: payload.playerName || currentRoll.value?.playerName || '',
      label: payload.label || currentRoll.value?.label || '',
      dice: payload.dice || null,
      value: payload.value ?? null,
      chosenValue: payload.chosenValue ?? payload.value ?? null,
      modifier: payload.modifier ?? currentRoll.value?.modifier ?? 0,
      total: payload.total ?? null,
      isNat1: payload.isNat1 ?? false,
      isNat20: payload.isNat20 ?? false,
    };
    currentRoll.value = data;
    writeToStorage(data);
  }

  function clearRoll() {
    currentRoll.value = null;
    localStorage.removeItem(STORAGE_KEY);
    console.log('[RollBroadcast] Cleared');
  }

  // Load initial state and setup listener
  const initial = readFromStorage();
  if (initial) {
    currentRoll.value = initial;
    console.log('[RollBroadcast] Loaded initial state:', initial?.status || 'none');
  } else {
    console.log('[RollBroadcast] No initial state');
  }
  
  setupStorageListener();

  return {
    currentRoll,
    startRolling,
    showResult,
    clearRoll,
  };
});
