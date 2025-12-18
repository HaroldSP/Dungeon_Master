import { defineStore } from 'pinia';
import { ref, computed } from 'vue';

/**
 * Store for broadcasting roll results to the player screen.
 * DM uses HTTP POST, Player Screen uses WebSocket for instant updates.
 */
export const useRollBroadcastStore = defineStore('rollBroadcast', () => {
  const currentRoll = ref(null);
  const serverUrl = ref(''); // Will be set from tower's pyServerUrl
  const isConnected = ref(false);
  const connectionError = ref(null);
  let ws = null;
  let reconnectTimeout = null;

  // Compute the base URL (strip /detect or /detect/best if present)
  const baseUrl = computed(() => {
    let url = serverUrl.value || '';
    url = url.replace(/\/detect(\/best)?$/, '');
    return url.replace(/\/$/, '');
  });

  // Compute WebSocket URL from HTTP URL
  const wsUrl = computed(() => {
    if (!baseUrl.value) return '';
    // Replace http:// with ws:// or https:// with wss://
    return baseUrl.value.replace(/^http/, 'ws') + '/ws/roll';
  });

  // === HTTP API (for DM posting) ===
  
  async function postRoll(data) {
    if (!baseUrl.value) {
      console.warn('[RollBroadcast] No server URL configured');
      return;
    }
    try {
      const url = `${baseUrl.value}/roll`;
      console.log('[RollBroadcast] POST', url, data?.status);
      const res = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data),
      });
      if (!res.ok) {
        console.error('[RollBroadcast] POST failed:', res.status);
      }
    } catch (e) {
      console.error('[RollBroadcast] POST error:', e);
    }
  }

  async function deleteRoll() {
    if (!baseUrl.value) return;
    try {
      const url = `${baseUrl.value}/roll`;
      await fetch(url, { method: 'DELETE' });
      console.log('[RollBroadcast] DELETE sent');
    } catch (e) {
      console.error('[RollBroadcast] DELETE error:', e);
    }
  }

  // === WebSocket (for Player Screen) ===

  function connectWebSocket(url) {
    if (url) {
      serverUrl.value = url;
    }
    if (!wsUrl.value) {
      console.warn('[RollBroadcast] Cannot connect: no server URL');
      connectionError.value = 'No server URL';
      return;
    }

    // Close existing connection
    if (ws) {
      ws.close();
      ws = null;
    }

    console.log('[RollBroadcast] Connecting WebSocket:', wsUrl.value);
    connectionError.value = null;

    try {
      ws = new WebSocket(wsUrl.value);

      ws.onopen = () => {
        console.log('[RollBroadcast] WebSocket connected');
        isConnected.value = true;
        connectionError.value = null;
      };

      ws.onmessage = (event) => {
        try {
          const msg = JSON.parse(event.data);
          console.log('[RollBroadcast] WS message:', msg.type);
          
          if (msg.type === 'roll' && msg.data) {
            currentRoll.value = msg.data;
          } else if (msg.type === 'clear') {
            currentRoll.value = null;
          }
        } catch (e) {
          console.error('[RollBroadcast] WS parse error:', e);
        }
      };

      ws.onclose = (event) => {
        console.log('[RollBroadcast] WebSocket closed:', event.code);
        isConnected.value = false;
        ws = null;
        
        // Auto-reconnect after 2 seconds (unless intentionally closed)
        if (serverUrl.value && event.code !== 1000) {
          reconnectTimeout = setTimeout(() => {
            console.log('[RollBroadcast] Reconnecting...');
            connectWebSocket();
          }, 2000);
        }
      };

      ws.onerror = (error) => {
        console.error('[RollBroadcast] WebSocket error:', error);
        connectionError.value = 'Connection failed';
      };

    } catch (e) {
      console.error('[RollBroadcast] WebSocket create error:', e);
      connectionError.value = e.message;
    }
  }

  function disconnectWebSocket() {
    if (reconnectTimeout) {
      clearTimeout(reconnectTimeout);
      reconnectTimeout = null;
    }
    if (ws) {
      ws.close(1000, 'Manual disconnect');
      ws = null;
    }
    isConnected.value = false;
    console.log('[RollBroadcast] WebSocket disconnected');
  }

  // === Public API (called by DM) ===

  function setServerUrl(url) {
    serverUrl.value = url || '';
    console.log('[RollBroadcast] Server URL set:', baseUrl.value);
  }

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
      difficultyClass: payload.difficultyClass ?? null,
    };
    currentRoll.value = data;
    postRoll(data);
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
      difficultyClass: payload.difficultyClass ?? currentRoll.value?.difficultyClass ?? null,
    };
    currentRoll.value = data;
    postRoll(data);
  }

  function clearRoll() {
    currentRoll.value = null;
    deleteRoll();
  }

  return {
    currentRoll,
    serverUrl,
    isConnected,
    connectionError,
    baseUrl,
    wsUrl,
    setServerUrl,
    startRolling,
    showResult,
    clearRoll,
    connectWebSocket,
    disconnectWebSocket,
  };
});
