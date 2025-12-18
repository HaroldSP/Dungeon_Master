<template>
  <div
    class="player-screen"
    @dblclick="onFullscreenToggle"
    @touchend="onFullscreenTouchEnd"
  >
    <!-- Connection status indicator (visible when server URL exists, auto-hides after connected) -->
    <div
      v-if="!serverError && !statusHidden"
      class="connection-status"
      :class="{ connected: isConnected, 'fade-out': statusFading }"
    >
      <span class="status-dot"></span>
      <span class="status-text">{{
        isConnected ? 'Connected' : connectionError || 'Connecting...'
      }}</span>
    </div>

    <!-- Server URL error -->
    <div
      v-if="serverError"
      class="server-error"
    >
      <div class="error-icon">⚠️</div>
      <div class="error-text">{{ serverError }}</div>
      <div class="error-hint">Example: /player-screen?s=192.168.0.102:8003</div>
    </div>

    <!-- Idle state - static D20, no text -->
    <div
      v-else-if="!currentRoll"
      class="idle-state"
    >
      <div class="d20-wrapper idle">
        <div class="d20-glow"></div>
        <svg
          viewBox="0 0 100 115"
          class="d20-svg"
        >
          <!-- Outer shadow border (inset slightly so stroke isn't clipped by viewBox) -->
          <polygon
            class="d20-shadow"
            points="50,4 96,27 96,88 50,111 4,88 4,27"
          />
          <!-- Outer silhouette (full D20 contour) -->
          <polygon
            class="d20-bg"
            points="50,8 92,32 92,83 50,107 8,83 8,32"
          />
          <!-- Top triangle face (sharp side up, main front face) -->
          <polygon
            class="d20-face"
            points="50,30 16,78 84,78"
          />
          <!-- Triangle-to-contour connections (each triangle corner -> three contour corners) -->
          <!-- Top vertex connections -->
          <line
            class="d20-line"
            x1="50"
            y1="29"
            x2="50"
            y2="9"
          />
          <line
            class="d20-line"
            x1="50"
            y1="29"
            x2="90"
            y2="32"
          />
          <line
            class="d20-line"
            x1="50"
            y1="29"
            x2="9"
            y2="33"
          />
          <!-- Bottom-left vertex connections -->
          <line
            class="d20-line"
            x1="14"
            y1="79"
            x2="9"
            y2="33"
          />
          <line
            class="d20-line"
            x1="14"
            y1="79"
            x2="9"
            y2="82"
          />
          <line
            class="d20-line"
            x1="14"
            y1="79"
            x2="50"
            y2="106"
          />
          <!-- Bottom-right vertex connections -->
          <line
            class="d20-line"
            x1="86"
            y1="79"
            x2="91"
            y2="32"
          />
          <line
            class="d20-line"
            x1="86"
            y1="79"
            x2="91"
            y2="82"
          />
          <line
            class="d20-line"
            x1="86"
            y1="79"
            x2="50"
            y2="106"
          />
          <text
            x="50"
            y="67"
            class="d20-number"
          >
            20
          </text>
        </svg>
      </div>
    </div>

    <!-- Rolling / Result state -->
    <div
      v-else
      class="roll-container"
    >
      <!-- Header with player context -->
      <div class="roll-header">
        <div
          class="player-context"
          v-if="currentRoll.playerName"
        >
          {{ currentRoll.playerName }}
        </div>
      </div>

      <!-- Check type -->
      <div class="check-type">
        <div class="check-title">{{ checkTitle }}</div>
        <div class="check-subtitle">{{ checkSubtitle }}</div>
      </div>

      <!-- Difficulty Class placeholder -->
      <div class="dc-section">
        <div class="dc-label">DIFFICULTY<br />CLASS</div>
        <div class="dc-value">—</div>
      </div>

      <!-- Dice + bonuses row -->
      <div class="dice-bonus-row">
        <!-- Dice area -->
        <div class="dice-area">
          <div class="dice-row">
            <!-- First die (or only die for normal) -->
            <div
              class="d20-wrapper"
              :class="getDiceClass(0)"
            >
              <div
                class="d20-glow"
                :class="getDiceGlowClass(0)"
              ></div>
              <svg
                viewBox="0 0 100 115"
                class="d20-svg"
              >
                <!-- Outer shadow border (inset slightly so stroke isn't clipped by viewBox) -->
                <polygon
                  class="d20-shadow"
                  points="50,4 96,27 96,88 50,111 4,88 4,27"
                />
                <!-- Outer silhouette (full D20 contour) -->
                <polygon
                  class="d20-bg"
                  points="50,8 92,32 92,83 50,107 8,83 8,32"
                />
                <!-- Top triangle face (sharp side up, main front face) -->
                <polygon
                  class="d20-face"
                  :class="getDiceFaceClass(0)"
                  points="50,30 16,78 84,78"
                />
                <!-- Triangle-to-contour connections (each triangle corner -> three contour corners) -->
                <!-- Top vertex connections -->
                <line
                  class="d20-line"
                  x1="50"
                  y1="29"
                  x2="50"
                  y2="9"
                />
                <line
                  class="d20-line"
                  x1="50"
                  y1="29"
                  x2="90"
                  y2="32"
                />
                <line
                  class="d20-line"
                  x1="50"
                  y1="29"
                  x2="9"
                  y2="33"
                />
                <!-- Bottom-left vertex connections -->
                <line
                  class="d20-line"
                  x1="14"
                  y1="79"
                  x2="9"
                  y2="33"
                />
                <line
                  class="d20-line"
                  x1="14"
                  y1="79"
                  x2="9"
                  y2="82"
                />
                <line
                  class="d20-line"
                  x1="14"
                  y1="79"
                  x2="50"
                  y2="106"
                />
                <!-- Bottom-right vertex connections -->
                <line
                  class="d20-line"
                  x1="86"
                  y1="79"
                  x2="91"
                  y2="32"
                />
                <line
                  class="d20-line"
                  x1="86"
                  y1="79"
                  x2="91"
                  y2="82"
                />
                <line
                  class="d20-line"
                  x1="86"
                  y1="79"
                  x2="50"
                  y2="106"
                />
                <text
                  x="50"
                  y="67"
                  class="d20-number"
                  :class="getDiceTextClass(0)"
                >
                  {{ getDiceValue(0) }}
                </text>
              </svg>
            </div>

            <!-- Second die for advantage/disadvantage -->
            <div
              v-if="isAdvDis"
              class="d20-wrapper"
              :class="getDiceClass(1)"
            >
              <div
                class="d20-glow"
                :class="getDiceGlowClass(1)"
              ></div>
              <svg
                viewBox="0 0 100 115"
                class="d20-svg"
              >
                <!-- Outer shadow border (inset slightly so stroke isn't clipped by viewBox) -->
                <polygon
                  class="d20-shadow"
                  points="50,4 96,27 96,88 50,111 4,88 4,27"
                />
                <!-- Outer silhouette (full D20 contour) -->
                <polygon
                  class="d20-bg"
                  points="50,8 92,32 92,83 50,107 8,83 8,32"
                />
                <!-- Top triangle face (sharp side up, main front face) -->
                <polygon
                  class="d20-face"
                  :class="getDiceFaceClass(1)"
                  points="50,30 16,78 84,78"
                />
                <!-- Triangle-to-contour connections (each triangle corner -> three contour corners) -->
                <!-- Top vertex connections -->
                <line
                  class="d20-line"
                  x1="50"
                  y1="29"
                  x2="50"
                  y2="9"
                />
                <line
                  class="d20-line"
                  x1="50"
                  y1="29"
                  x2="90"
                  y2="32"
                />
                <line
                  class="d20-line"
                  x1="50"
                  y1="29"
                  x2="9"
                  y2="33"
                />
                <!-- Bottom-left vertex connections -->
                <line
                  class="d20-line"
                  x1="14"
                  y1="79"
                  x2="9"
                  y2="33"
                />
                <line
                  class="d20-line"
                  x1="14"
                  y1="79"
                  x2="9"
                  y2="82"
                />
                <line
                  class="d20-line"
                  x1="14"
                  y1="79"
                  x2="50"
                  y2="106"
                />
                <!-- Bottom-right vertex connections -->
                <line
                  class="d20-line"
                  x1="86"
                  y1="79"
                  x2="91"
                  y2="32"
                />
                <line
                  class="d20-line"
                  x1="86"
                  y1="79"
                  x2="91"
                  y2="82"
                />
                <line
                  class="d20-line"
                  x1="86"
                  y1="79"
                  x2="50"
                  y2="106"
                />
                <text
                  x="50"
                  y="67"
                  class="d20-number"
                  :class="getDiceTextClass(1)"
                >
                  {{ getDiceValue(1) }}
                </text>
              </svg>
            </div>
          </div>
          <div
            v-if="currentRoll.status === 'rolling'"
            class="roll-prompt"
          >
            Бросок кубика...
          </div>
          <div
            v-else
            class="mode-badge"
            :class="currentRoll.mode"
          >
            {{ modeLabel }}
          </div>
        </div>

        <!-- Bonuses breakdown (visible for any active roll) -->
        <div
          v-if="currentRoll"
          class="bonuses-section"
        >
          <div class="bonus-item">
            <div class="bonus-icon ability-icon">
              <span class="icon-symbol">⬡</span>
            </div>
            <div class="bonus-value">
              {{ currentRoll.modifier >= 0 ? '+' : ''
              }}{{ currentRoll.modifier }}
            </div>
            <div class="bonus-label">Модификатор</div>
          </div>

          <div
            v-if="isAdvDis"
            class="bonus-item"
          >
            <div
              class="bonus-icon"
              :class="
                currentRoll.mode === 'advantage' ? 'adv-icon' : 'dis-icon'
              "
            >
              <span class="icon-symbol">{{
                currentRoll.mode === 'advantage' ? '▲' : '▼'
              }}</span>
            </div>
            <div class="bonus-value situational">{{ modeLabel }}</div>
            <div class="bonus-label">Ситуативный</div>
          </div>
        </div>
      </div>

      <!-- Total (reserved space, value appears when result is ready) -->
      <div class="total-section">
        <div class="total-label">Итого</div>
        <div
          class="total-value"
          :class="totalClass"
        >
          {{ totalDisplay }}
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
  import { ref, computed, watch, onMounted, onBeforeUnmount } from 'vue';
  import { useRoute } from 'vue-router';
  import { useRollBroadcastStore } from '../stores/rollBroadcastStore';
  import { storeToRefs } from 'pinia';

  const route = useRoute();
  const rollStore = useRollBroadcastStore();
  const { currentRoll, isConnected, connectionError } = storeToRefs(rollStore);

  // Get server URL from query param: /player-screen?s=192.168.0.102:8003
  // Supports short form (?s=IP:PORT) or full form (?s=http://IP:PORT)
  const serverUrl = computed(() => {
    // Support both ?s= (short) and ?server= (legacy)
    let url = route.query.s || route.query.server || '';
    if (url && !url.startsWith('http')) {
      url = `http://${url}`;
    }
    return url;
  });
  const serverError = ref('');

  // Fullscreen toggle helpers
  const isFullscreenActive = () => {
    return !!(
      document.fullscreenElement ||
      document.webkitFullscreenElement ||
      document.msFullscreenElement
    );
  };

  const enterFullscreen = () => {
    const elem = document.documentElement;
    if (elem.requestFullscreen) {
      elem.requestFullscreen().catch(err => {
        console.log('[PlayerScreen] Fullscreen request failed:', err);
      });
    } else if (elem.webkitRequestFullscreen) {
      // Safari
      elem.webkitRequestFullscreen();
    } else if (elem.msRequestFullscreen) {
      // IE/Edge
      elem.msRequestFullscreen();
    }
  };

  const exitFullscreen = () => {
    if (document.exitFullscreen) {
      document.exitFullscreen().catch(err => {
        console.log('[PlayerScreen] Exit fullscreen failed:', err);
      });
    } else if (document.webkitExitFullscreen) {
      document.webkitExitFullscreen();
    } else if (document.msExitFullscreen) {
      document.msExitFullscreen();
    }
  };

  const onFullscreenToggle = () => {
    if (isFullscreenActive()) {
      exitFullscreen();
    } else {
      enterFullscreen();
    }
  };

  // Double-tap detection for touch devices
  let lastTouchTime = 0;
  const onFullscreenTouchEnd = event => {
    const now = Date.now();
    if (now - lastTouchTime < 350) {
      // Double-tap detected
      lastTouchTime = 0;
      event.preventDefault();
      onFullscreenToggle();
    } else {
      lastTouchTime = now;
    }
  };

  onMounted(() => {
    if (serverUrl.value) {
      console.log('[PlayerScreen] Connecting WebSocket to:', serverUrl.value);
      rollStore.connectWebSocket(serverUrl.value);
      serverError.value = '';
    } else {
      serverError.value = 'No server URL. Add ?s=YOUR_IP:8003 to the URL';
      console.warn('[PlayerScreen]', serverError.value);
    }
  });

  const animatedValue1 = ref(20);
  const animatedValue2 = ref(20);
  let animationInterval = null;

  // Connection status auto-hide
  const statusHidden = ref(false);
  const statusFading = ref(false);
  let statusHideTimeout = null;

  const isAdvDis = computed(
    () =>
      currentRoll.value?.mode === 'advantage' ||
      currentRoll.value?.mode === 'disadvantage'
  );

  const modeLabel = computed(() => {
    switch (currentRoll.value?.mode) {
      case 'advantage':
        return 'Преимущество';
      case 'disadvantage':
        return 'Помеха';
      default:
        return 'Бросок';
    }
  });

  const checkTitle = computed(() => {
    // Extract check type from label if possible
    const label = currentRoll.value?.label || '';
    if (label.includes('Спасбросок')) return 'Спасбросок';
    if (label.includes('проверка')) return 'Проверка';
    return 'Проверка';
  });

  const checkSubtitle = computed(() => {
    const label = currentRoll.value?.label || '';
    // For saving throws like "ИНТЕЛЛЕКТ Спасбросок" - extract just the ability name
    if (label.includes('Спасбросок')) {
      // Remove "Спасбросок" and clean up
      return label.replace(/\s*Спасбросок\s*/i, '').trim();
    }
    // For skills like "Атлетика проверка" - extract skill name
    if (label.includes('проверка')) {
      return label.replace(/\s*проверка\s*/i, '').trim();
    }
    return label;
  });

  const totalClass = computed(() => {
    if (currentRoll.value?.isNat20) return 'total-crit';
    if (currentRoll.value?.isNat1) return 'total-fail';
    return '';
  });

  const totalDisplay = computed(() => {
    if (currentRoll.value?.status === 'result') {
      return currentRoll.value.total;
    }
    return '—';
  });

  // Get dice value for display
  function getDiceValue(index) {
    if (currentRoll.value?.status === 'rolling') {
      return index === 0 ? animatedValue1.value : animatedValue2.value;
    }
    if (currentRoll.value?.status === 'result') {
      if (currentRoll.value.dice && currentRoll.value.dice.length > index) {
        return currentRoll.value.dice[index];
      }
      return currentRoll.value.value;
    }
    return 20;
  }

  // Get dice wrapper class
  function getDiceClass(index) {
    if (currentRoll.value?.status === 'rolling') return 'rolling';
    if (currentRoll.value?.status === 'result') {
      if (isAdvDis.value && currentRoll.value.dice) {
        const isUsed =
          currentRoll.value.chosenValue === currentRoll.value.dice[index];
        return isUsed ? 'result used' : 'result unused';
      }
      return 'result';
    }
    return '';
  }

  // Get glow class for dice
  function getDiceGlowClass(index) {
    if (currentRoll.value?.status === 'rolling') return 'glow-rolling';
    if (currentRoll.value?.status === 'result') {
      const val = getDiceValue(index);
      if (val === 20) return 'glow-crit';
      if (val === 1) return 'glow-fail';
      if (isAdvDis.value && currentRoll.value.chosenValue === val)
        return 'glow-used';
    }
    return '';
  }

  // Get dice face class
  function getDiceFaceClass(index) {
    if (currentRoll.value?.status === 'result') {
      const val = getDiceValue(index);
      if (val === 20) return 'face-crit';
      if (val === 1) return 'face-fail';
    }
    return '';
  }

  // Get dice text class
  function getDiceTextClass(index) {
    if (currentRoll.value?.status === 'result') {
      const val = getDiceValue(index);
      if (val === 20) return 'text-crit';
      if (val === 1) return 'text-fail';
    }
    return '';
  }

  // Animation
  function startAnimation() {
    if (animationInterval) clearInterval(animationInterval);
    animationInterval = setInterval(() => {
      animatedValue1.value = Math.floor(Math.random() * 20) + 1;
      animatedValue2.value = Math.floor(Math.random() * 20) + 1;
    }, 50);
  }

  function stopAnimation() {
    if (animationInterval) {
      clearInterval(animationInterval);
      animationInterval = null;
    }
  }

  watch(
    () => currentRoll.value?.status,
    (newStatus, oldStatus) => {
      const statusStr = newStatus ?? 'idle';
      const oldStr = oldStatus ?? 'idle';
      console.log(`[PlayerScreen] Status: ${oldStr} → ${statusStr}`);

      if (newStatus === 'rolling') {
        startAnimation();
      } else {
        stopAnimation();
      }
    },
    { immediate: true }
  );

  // Auto-hide connection status after 5 seconds when connected
  watch(
    isConnected,
    connected => {
      if (statusHideTimeout) {
        clearTimeout(statusHideTimeout);
        statusHideTimeout = null;
      }
      if (connected) {
        // Start fade out after 4 seconds, hide at 5 seconds
        statusHideTimeout = setTimeout(() => {
          statusFading.value = true;
          setTimeout(() => {
            statusHidden.value = true;
          }, 1000); // 1s for fade animation
        }, 4000);
      } else {
        // Show status again if disconnected
        statusHidden.value = false;
        statusFading.value = false;
      }
    },
    { immediate: true }
  );

  watch(
    currentRoll,
    (newVal, oldVal) => {
      if (newVal === oldVal) return;
      console.log('[PlayerScreen] Roll data updated:', {
        status: newVal?.status ?? 'none',
        player: newVal?.playerName ?? 'none',
        label: newVal?.label ?? 'none',
      });
    },
    { deep: true }
  );

  onBeforeUnmount(() => {
    stopAnimation();
    rollStore.disconnectWebSocket();
    if (statusHideTimeout) {
      clearTimeout(statusHideTimeout);
    }
  });
</script>

<style scoped>
  @import url('https://fonts.googleapis.com/css2?family=Cinzel:wght@400;600;700&family=Crimson+Text:ital,wght@0,400;0,600;1,400&display=swap');

  .player-screen {
    min-height: 100vh;
    width: 100%;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    background: radial-gradient(
        ellipse at center,
        rgba(30, 25, 35, 0.9) 0%,
        rgba(15, 12, 20, 1) 100%
      ),
      url("data:image/svg+xml,%3Csvg viewBox='0 0 100 100' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='noise'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.8' numOctaves='4' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23noise)' opacity='0.05'/%3E%3C/svg%3E");
    color: #e8dcc4;
    font-family: 'Crimson Text', Georgia, serif;
    overflow: hidden;
  }

  /* === SERVER ERROR === */
  .server-error {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    padding: 32px;
    background: rgba(50, 30, 30, 0.8);
    border: 1px solid rgba(200, 80, 80, 0.4);
    border-radius: 12px;
  }

  .error-icon {
    font-size: 48px;
  }

  .error-text {
    font-family: 'Crimson Text', serif;
    font-size: 18px;
    color: #de6a6a;
    text-align: center;
  }

  .error-hint {
    font-family: monospace;
    font-size: 12px;
    color: #8a6a6a;
    background: rgba(0, 0, 0, 0.3);
    padding: 8px 16px;
    border-radius: 4px;
  }

  /* === CONNECTION STATUS === */
  .connection-status {
    position: fixed;
    top: 12px;
    right: 12px;
    display: flex;
    align-items: center;
    gap: 6px;
    padding: 4px 10px;
    background: rgba(0, 0, 0, 0.5);
    border-radius: 12px;
    font-size: 11px;
    color: #888;
    opacity: 0.7;
    transition: opacity 0.3s;
  }

  .connection-status:hover {
    opacity: 1;
  }

  .connection-status .status-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: #f44;
    animation: pulse 1.5s infinite;
  }

  .connection-status.connected .status-dot {
    background: #4c4;
    animation: none;
  }

  .connection-status.connected .status-text {
    color: #6a6;
  }

  .connection-status.fade-out {
    opacity: 0;
    transition: opacity 1s ease-out;
  }

  @keyframes pulse {
    0%,
    100% {
      opacity: 1;
    }
    50% {
      opacity: 0.4;
    }
  }

  /* === IDLE STATE === */
  .idle-state {
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .idle-state .d20-wrapper {
    opacity: 0.4;
    transform: scale(1.6); /* idle die larger than active ones */
  }

  /* === ROLL CONTAINER === */
  .roll-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 24px;
    padding: 32px;
    max-width: 520px;
  }

  /* === HEADER === */
  .roll-header {
    text-align: center;
  }

  .player-context {
    font-family: 'Crimson Text', serif;
    font-size: 16px;
    font-style: italic;
    color: #b8a88a;
    opacity: 0.9;
  }

  /* === CHECK TYPE === */
  .check-type {
    text-align: center;
    margin-bottom: 8px;
  }

  .check-title {
    font-family: 'Cinzel', serif;
    font-size: 32px;
    font-weight: 600;
    color: #f4e4c1;
    text-transform: uppercase;
    letter-spacing: 3px;
    text-shadow: 0 2px 4px rgba(0, 0, 0, 0.5);
  }

  .check-subtitle {
    font-family: 'Crimson Text', serif;
    font-size: 20px;
    color: #b8a88a;
    margin-top: 4px;
  }

  /* === DIFFICULTY CLASS === */
  .dc-section {
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 12px 24px;
    background: linear-gradient(
      180deg,
      rgba(40, 35, 45, 0.8) 0%,
      rgba(25, 22, 30, 0.9) 100%
    );
    border: 1px solid rgba(180, 160, 120, 0.3);
    border-radius: 8px;
    margin-bottom: 16px;
  }

  .dc-label {
    font-family: 'Cinzel', serif;
    font-size: 10px;
    color: #8a7a6a;
    text-transform: uppercase;
    letter-spacing: 2px;
    text-align: center;
    line-height: 1.3;
  }

  .dc-value {
    font-family: 'Cinzel', serif;
    font-size: 36px;
    font-weight: 700;
    color: #f4e4c1;
    margin-top: 4px;
  }

  /* === DICE AREA === */
  .dice-bonus-row {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    width: 100%;
  }

  .dice-area {
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 32px;
    background: linear-gradient(
      180deg,
      rgba(50, 40, 55, 0.6) 0%,
      rgba(30, 25, 35, 0.8) 100%
    );
    border: 1px solid rgba(180, 160, 120, 0.2);
    border-radius: 12px;
    /* Reserve height so chosen die animation doesn't grow the box */
    min-height: 285px;
    position: relative;
  }

  /* Landscape: put dice and bonuses in one row (tablet / desktop) */
  @media (orientation: landscape) and (min-width: 768px) {
    .dice-bonus-row {
      flex-direction: row;
      justify-content: center;
      align-items: stretch;
      gap: 32px;
    }

    .dice-area,
    .bonuses-section {
      flex: 0 0 390px;
      max-width: 390px;
      height: 285px; /* match dice box height */
    }

    .bonuses-section {
      margin-top: 0;
      justify-content: center;
      align-items: center;
    }
  }

  .dice-area::before {
    content: '';
    position: absolute;
    top: -1px;
    left: 20%;
    right: 20%;
    height: 2px;
    background: linear-gradient(
      90deg,
      transparent,
      rgba(200, 170, 100, 0.5),
      transparent
    );
  }

  /* Same golden shine for bonuses box */
  .bonuses-section::before {
    content: '';
    position: absolute;
    top: -1px;
    left: 20%;
    right: 20%;
    height: 2px;
    background: linear-gradient(
      90deg,
      transparent,
      rgba(200, 170, 100, 0.7),
      transparent
    );
  }

  .dice-row {
    display: flex;
    gap: 24px;
    margin-bottom: 16px;
  }

  /* === D20 DICE === */
  .d20-wrapper {
    width: 160px;
    height: 182px;
    position: relative;
    transition: all 0.3s ease;
  }

  .d20-glow {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 135px;
    height: 135px;
    transform: translate(-50%, -50%);
    border-radius: 50%;
    background: radial-gradient(
      circle,
      rgba(200, 170, 100, 0.3) 0%,
      transparent 70%
    );
    opacity: 0;
    transition: opacity 0.3s ease;
  }

  .d20-glow.glow-rolling {
    opacity: 1;
    background: radial-gradient(
      circle,
      rgba(200, 170, 100, 0.5) 0%,
      transparent 70%
    );
    animation: pulse-glow 0.3s infinite alternate;
  }

  .d20-glow.glow-crit {
    opacity: 1;
    background: radial-gradient(
      circle,
      rgba(100, 200, 100, 0.6) 0%,
      transparent 70%
    );
  }

  .d20-glow.glow-fail {
    opacity: 1;
    background: radial-gradient(
      circle,
      rgba(200, 80, 80, 0.6) 0%,
      transparent 70%
    );
  }

  .d20-glow.glow-used {
    opacity: 1;
    background: radial-gradient(
      circle,
      rgba(200, 170, 100, 0.4) 0%,
      transparent 70%
    );
  }

  @keyframes pulse-glow {
    from {
      transform: translate(-50%, -50%) scale(1);
      opacity: 0.5;
    }
    to {
      transform: translate(-50%, -50%) scale(1.1);
      opacity: 1;
    }
  }

  .d20-svg {
    width: 100%;
    height: 100%;
    filter: drop-shadow(0 8px 16px rgba(0, 0, 0, 0.5));
  }

  .d20-bg {
    fill: #1a1520;
    stroke: rgba(200, 170, 100, 0.4);
    stroke-width: 1.5;
  }

  .d20-shadow {
    fill: none;
    stroke: #3a3040;
    stroke-width: 3;
  }

  .d20-face {
    fill: linear-gradient(135deg, #2a2530 0%, #1a1520 100%);
    fill: #252030;
    stroke: rgba(200, 170, 100, 0.4);
    stroke-width: 1.5;
    transition: all 0.3s ease;
  }

  .d20-face.face-crit {
    fill: #1a2a1a;
    stroke: rgba(100, 200, 100, 0.8);
    stroke-width: 2;
  }

  .d20-face.face-fail {
    fill: #2a1a1a;
    stroke: rgba(200, 80, 80, 0.8);
    stroke-width: 2;
  }

  .d20-line {
    stroke: rgba(200, 170, 100, 0.18);
    stroke-width: 0.8;
  }

  .d20-number {
    fill: #c8aa64;
    font-family: 'Cinzel', serif;
    font-size: 28px;
    font-weight: 700;
    text-anchor: middle;
    dominant-baseline: middle;
  }

  .d20-number.text-crit {
    fill: #6ade6a;
  }

  .d20-number.text-fail {
    fill: #de6a6a;
  }

  /* Dice states */
  .d20-wrapper.rolling {
    animation: dice-shake 0.1s infinite;
  }

  .d20-wrapper.rolling .d20-face {
    stroke: rgba(200, 170, 100, 0.7);
  }

  .d20-wrapper.result {
    animation: dice-pop 0.4s ease-out;
  }

  .d20-wrapper.unused {
    opacity: 0.4;
    transform: scale(0.85);
  }

  .d20-wrapper.used {
    transform: scale(1.05);
  }

  @keyframes dice-shake {
    0%,
    100% {
      transform: rotate(-4deg);
    }
    50% {
      transform: rotate(4deg);
    }
  }

  @keyframes dice-pop {
    0% {
      transform: scale(0.8);
      opacity: 0;
    }
    50% {
      transform: scale(1.1);
    }
    100% {
      transform: scale(1);
      opacity: 1;
    }
  }

  /* === ROLL PROMPT === */
  .roll-prompt {
    font-family: 'Crimson Text', serif;
    font-size: 14px;
    color: #8a7a6a;
    text-transform: uppercase;
    letter-spacing: 2px;
    animation: prompt-pulse 1s infinite;
  }

  @keyframes prompt-pulse {
    0%,
    100% {
      opacity: 0.6;
    }
    50% {
      opacity: 1;
    }
  }

  /* === MODE BADGE === */
  .mode-badge {
    font-family: 'Cinzel', serif;
    font-size: 14px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 2px;
    padding: 6px 16px;
    border-radius: 4px;
    background: rgba(200, 170, 100, 0.1);
    border: 1px solid rgba(200, 170, 100, 0.3);
    color: #c8aa64;
  }

  .mode-badge.advantage {
    background: rgba(100, 200, 100, 0.1);
    border-color: rgba(100, 200, 100, 0.4);
    color: #6ade6a;
  }

  .mode-badge.disadvantage {
    background: rgba(200, 80, 80, 0.1);
    border-color: rgba(200, 80, 80, 0.4);
    color: #de6a6a;
  }

  /* === BONUSES SECTION === */
  .bonuses-section {
    display: flex;
    gap: 40px;
    padding: 22px 32px;
    /* Match dice box background */
    background: linear-gradient(
      180deg,
      rgba(50, 40, 55, 0.6) 0%,
      rgba(30, 25, 35, 0.8) 100%
    );
    border-radius: 8px;
    border: 1px solid rgba(180, 160, 120, 0.2); /* same as dice-area */
    align-items: stretch; /* make all bonus columns same height */
    position: relative; /* enable ::before shine inside box */
    overflow: hidden; /* clip shine to rounded corners */
  }

  .bonus-item {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center; /* center icon/value/label vertically */
    gap: 6px;
    flex: 1 1 0; /* equal width columns so labels line up */
  }

  .bonus-icon {
    width: 50px;
    height: 50px;
    display: flex;
    align-items: center;
    justify-content: center;
    background: linear-gradient(
      180deg,
      rgba(60, 50, 70, 0.8) 0%,
      rgba(40, 35, 50, 0.9) 100%
    );
    border: 1px solid rgba(200, 170, 100, 0.3);
    border-radius: 6px;
  }

  .icon-symbol {
    font-size: 22px;
    color: #c8aa64;
  }

  .adv-icon {
    border-color: rgba(100, 200, 100, 0.4);
  }

  .adv-icon .icon-symbol {
    color: #6ade6a;
  }

  .dis-icon {
    border-color: rgba(200, 80, 80, 0.4);
  }

  .dis-icon .icon-symbol {
    color: #de6a6a;
  }

  .bonus-value {
    font-family: 'Cinzel', serif;
    font-size: 22px;
    font-weight: 600;
    color: #f4e4c1;
    line-height: 1.2;
    min-height: 1.4em; /* ensure same vertical box for value text in both columns */
  }

  .bonus-value.situational {
    font-size: 22px; /* same visual size as numeric modifier */
    color: #b8a88a;
  }

  .bonus-label {
    font-family: 'Crimson Text', serif;
    font-size: 14px;
    color: #6a5a4a;
    text-transform: uppercase;
    letter-spacing: 1px;
  }

  /* === TOTAL SECTION === */
  .total-section {
    display: flex;
    align-items: center;
    gap: 16px;
    margin-top: 16px;
    min-height: 80px;
    padding: 12px 32px;
    background: linear-gradient(
      180deg,
      rgba(50, 45, 55, 0.7) 0%,
      rgba(35, 30, 40, 0.9) 100%
    );
    border: 1px solid rgba(200, 170, 100, 0.3);
    border-radius: 8px;
  }

  .total-label {
    font-family: 'Cinzel', serif;
    font-size: 14px;
    color: #8a7a6a;
    text-transform: uppercase;
    letter-spacing: 2px;
  }

  .total-value {
    font-family: 'Cinzel', serif;
    font-size: 40px;
    font-weight: 700;
    color: #f4e4c1;
    text-shadow: 0 2px 8px rgba(200, 170, 100, 0.3);
  }

  .total-value.total-crit {
    color: #6ade6a;
    text-shadow: 0 0 20px rgba(100, 200, 100, 0.5);
  }

  .total-value.total-fail {
    color: #de6a6a;
    text-shadow: 0 0 20px rgba(200, 80, 80, 0.5);
  }
</style>
