<template>
  <div class="player-screen">
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
          <polygon
            class="d20-bg"
            points="50,2 98,28 98,87 50,113 2,87 2,28"
          />
          <polygon
            class="d20-face"
            points="50,10 90,32 90,83 50,105 10,83 10,32"
          />
          <line
            class="d20-line"
            x1="50"
            y1="10"
            x2="50"
            y2="58"
          />
          <line
            class="d20-line"
            x1="50"
            y1="58"
            x2="10"
            y2="32"
          />
          <line
            class="d20-line"
            x1="50"
            y1="58"
            x2="90"
            y2="32"
          />
          <line
            class="d20-line"
            x1="50"
            y1="58"
            x2="10"
            y2="83"
          />
          <line
            class="d20-line"
            x1="50"
            y1="58"
            x2="90"
            y2="83"
          />
          <line
            class="d20-line"
            x1="50"
            y1="58"
            x2="50"
            y2="105"
          />
          <text
            x="50"
            y="48"
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
              <polygon
                class="d20-bg"
                points="50,2 98,28 98,87 50,113 2,87 2,28"
              />
              <polygon
                class="d20-face"
                :class="getDiceFaceClass(0)"
                points="50,10 90,32 90,83 50,105 10,83 10,32"
              />
              <line
                class="d20-line"
                x1="50"
                y1="10"
                x2="50"
                y2="58"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="10"
                y2="32"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="90"
                y2="32"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="10"
                y2="83"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="90"
                y2="83"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="50"
                y2="105"
              />
              <text
                x="50"
                y="48"
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
              <polygon
                class="d20-bg"
                points="50,2 98,28 98,87 50,113 2,87 2,28"
              />
              <polygon
                class="d20-face"
                :class="getDiceFaceClass(1)"
                points="50,10 90,32 90,83 50,105 10,83 10,32"
              />
              <line
                class="d20-line"
                x1="50"
                y1="10"
                x2="50"
                y2="58"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="10"
                y2="32"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="90"
                y2="32"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="10"
                y2="83"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="90"
                y2="83"
              />
              <line
                class="d20-line"
                x1="50"
                y1="58"
                x2="50"
                y2="105"
              />
              <text
                x="50"
                y="48"
                class="d20-number"
                :class="getDiceTextClass(1)"
              >
                {{ getDiceValue(1) }}
              </text>
            </svg>
          </div>
        </div>

        <!-- Roll prompt or mode badge -->
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

      <!-- Bonuses breakdown (only on result) -->
      <div
        v-if="currentRoll.status === 'result'"
        class="bonuses-section"
      >
        <div class="bonus-item">
          <div class="bonus-icon ability-icon">
            <span class="icon-symbol">⬡</span>
          </div>
          <div class="bonus-value">
            {{ currentRoll.modifier >= 0 ? '+' : '' }}{{ currentRoll.modifier }}
          </div>
          <div class="bonus-label">Модификатор</div>
        </div>

        <div
          v-if="isAdvDis"
          class="bonus-item"
        >
          <div
            class="bonus-icon"
            :class="currentRoll.mode === 'advantage' ? 'adv-icon' : 'dis-icon'"
          >
            <span class="icon-symbol">{{
              currentRoll.mode === 'advantage' ? '▲' : '▼'
            }}</span>
          </div>
          <div class="bonus-value situational">{{ modeLabel }}</div>
          <div class="bonus-label">Ситуативный</div>
        </div>
      </div>

      <!-- Total -->
      <div
        v-if="currentRoll.status === 'result'"
        class="total-section"
      >
        <div class="total-label">Итого</div>
        <div
          class="total-value"
          :class="totalClass"
        >
          {{ currentRoll.total }}
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
    transform: scale(1.2);
  }

  /* === ROLL CONTAINER === */
  .roll-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
    padding: 24px;
    max-width: 400px;
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
    font-size: 28px;
    font-weight: 600;
    color: #f4e4c1;
    text-transform: uppercase;
    letter-spacing: 3px;
    text-shadow: 0 2px 4px rgba(0, 0, 0, 0.5);
  }

  .check-subtitle {
    font-family: 'Crimson Text', serif;
    font-size: 18px;
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
    font-size: 32px;
    font-weight: 700;
    color: #f4e4c1;
    margin-top: 4px;
  }

  /* === DICE AREA === */
  .dice-area {
    display: flex;
    flex-direction: column;
    align-items: center;
    padding: 24px;
    background: linear-gradient(
      180deg,
      rgba(50, 40, 55, 0.6) 0%,
      rgba(30, 25, 35, 0.8) 100%
    );
    border: 1px solid rgba(180, 160, 120, 0.2);
    border-radius: 12px;
    position: relative;
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

  .dice-row {
    display: flex;
    gap: 24px;
    margin-bottom: 16px;
  }

  /* === D20 DICE === */
  .d20-wrapper {
    width: 120px;
    height: 138px;
    position: relative;
    transition: all 0.3s ease;
  }

  .d20-glow {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 100px;
    height: 100px;
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
    stroke: #3a3040;
    stroke-width: 2;
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
    stroke: rgba(200, 170, 100, 0.15);
    stroke-width: 0.5;
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
    gap: 32px;
    margin-top: 24px;
    padding: 16px 24px;
    background: linear-gradient(
      180deg,
      rgba(40, 35, 45, 0.5) 0%,
      rgba(25, 22, 30, 0.7) 100%
    );
    border-radius: 8px;
    border: 1px solid rgba(180, 160, 120, 0.15);
  }

  .bonus-item {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 4px;
  }

  .bonus-icon {
    width: 40px;
    height: 40px;
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
    font-size: 18px;
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
    font-size: 16px;
    font-weight: 600;
    color: #f4e4c1;
  }

  .bonus-value.situational {
    font-size: 12px;
    color: #b8a88a;
  }

  .bonus-label {
    font-family: 'Crimson Text', serif;
    font-size: 11px;
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
    font-size: 36px;
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

  /* === LANDSCAPE TABLET RESPONSIVE === */
  @media (orientation: landscape) and (max-height: 600px) {
    .player-screen {
      justify-content: flex-start;
      padding-top: 8px;
    }

    .roll-container {
      flex-direction: row;
      flex-wrap: wrap;
      justify-content: center;
      align-items: flex-start;
      gap: 12px;
      padding: 8px 16px;
      max-width: 100%;
      width: 100%;
    }

    /* Left column: player, check type */
    .roll-header,
    .check-type {
      flex: 0 0 auto;
      text-align: left;
    }

    .roll-header {
      order: 1;
      width: 100%;
    }

    .check-type {
      order: 2;
      flex: 1;
      min-width: 150px;
    }

    .check-title {
      font-size: 18px;
      letter-spacing: 1px;
    }

    .check-subtitle {
      font-size: 14px;
    }

    .player-context {
      font-size: 13px;
    }

    /* DC section */
    .dc-section {
      order: 3;
      padding: 8px 16px;
      margin-bottom: 0;
    }

    .dc-label {
      font-size: 8px;
    }

    .dc-value {
      font-size: 24px;
    }

    /* Dice area in center */
    .dice-area {
      order: 4;
      padding: 12px;
      flex: 0 0 auto;
    }

    .dice-row {
      gap: 16px;
    }

    .d20-wrapper {
      width: 80px;
      height: 92px;
    }

    .d20-svg {
      width: 80px;
      height: 92px;
    }

    .d20-number {
      font-size: 22px;
    }

    .roll-prompt,
    .mode-badge {
      font-size: 10px;
      padding: 3px 10px;
      margin-top: 6px;
    }

    /* Bonuses section */
    .bonuses-section {
      order: 5;
      gap: 12px;
      flex: 1;
      min-width: 120px;
    }

    .bonus-item {
      padding: 6px;
      gap: 2px;
    }

    .bonus-icon {
      width: 28px;
      height: 28px;
    }

    .icon-symbol {
      font-size: 14px;
    }

    .bonus-value {
      font-size: 14px;
    }

    .bonus-label {
      font-size: 9px;
    }

    /* Total section */
    .total-section {
      order: 6;
      margin-top: 0;
      padding: 8px 20px;
      gap: 10px;
    }

    .total-label {
      font-size: 11px;
    }

    .total-value {
      font-size: 28px;
    }

    /* Idle state */
    .idle-state .d20-wrapper {
      transform: scale(0.9);
    }
  }

  /* Extra small landscape (phone landscape) */
  @media (orientation: landscape) and (max-height: 420px) {
    .roll-container {
      gap: 8px;
      padding: 4px 12px;
    }

    .check-title {
      font-size: 16px;
    }

    .check-subtitle {
      font-size: 12px;
    }

    .d20-wrapper {
      width: 60px;
      height: 69px;
    }

    .d20-svg {
      width: 60px;
      height: 69px;
    }

    .d20-number {
      font-size: 18px;
    }

    .dice-area {
      padding: 8px;
    }

    .dice-row {
      gap: 12px;
    }

    .dc-section {
      padding: 6px 12px;
    }

    .dc-value {
      font-size: 20px;
    }

    .total-value {
      font-size: 24px;
    }

    .bonuses-section {
      gap: 8px;
    }

    .bonus-icon {
      width: 24px;
      height: 24px;
    }

    .icon-symbol {
      font-size: 12px;
    }
  }
</style>
