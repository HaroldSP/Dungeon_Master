<template>
  <div class="player-screen">
    <!-- Idle state - static D20, no text -->
    <div
      v-if="!currentRoll"
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
        <div class="check-subtitle">{{ currentRoll.label }}</div>
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
  import { ref, computed, watch, onBeforeUnmount } from 'vue';
  import { useRollBroadcastStore } from '../stores/rollBroadcastStore';
  import { storeToRefs } from 'pinia';

  const rollStore = useRollBroadcastStore();
  const { currentRoll } = storeToRefs(rollStore);

  const animatedValue1 = ref(20);
  const animatedValue2 = ref(20);
  let animationInterval = null;

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
</style>
