<template>
  <div class="tower-details">
    <div class="stream-and-stats d-flex flex-wrap align-start ga-3">
      <div
        class="stream-column"
        :style="
          isMobile?.value
            ? {
                flex: '1 1 100%',
                maxWidth: '100%',
                minWidth: '0',
                padding: '0',
              }
            : null
        "
      >
        <div
          class="stream-wrapper"
          :style="isMobile?.value ? { width: '100%', maxWidth: '100%' } : null"
        >
          <img
            v-if="streamSrc"
            :key="streamSrc"
            :src="streamSrc"
            @load="$emit('stream-load', $event)"
            alt="Stream"
            class="stream-image"
          />
          <div
            v-if="bboxStyle"
            class="bbox-overlay"
            :style="bboxStyle"
          >
            <div class="bbox-label">
              {{ topDetection?.class }}
              ({{ topDetection?.confidence?.toFixed?.(2) ?? '—' }})
            </div>
          </div>
        </div>
        <div
          class="roll-mini-display"
          v-if="primaryRoll"
        >
          <div class="roll-line">
            <span
              class="roll-value"
              :class="rollColorClass(primaryRoll.value)"
            >
              {{ primaryRoll.value }}
            </span>
            <span class="roll-plus">
              +{{ primaryRoll.mod >= 0 ? primaryRoll.mod : primaryRoll.mod }}
            </span>
            <span class="roll-total"> (= {{ primaryRoll.total }}) </span>
          </div>
          <div
            v-if="currentSelection?.label"
            class="roll-label"
          >
            {{ currentSelection.label }}
          </div>
        </div>
      </div>
      <div
        class="stats-column"
        :style="
          isMobile?.value
            ? {
                flex: '1 1 100%',
                minWidth: '0',
                maxWidth: '100%',
                padding: '0',
              }
            : null
        "
      >
        <div
          class="abilities-grid"
          :style="
            isMobile?.value
              ? { width: '100%', 'grid-template-columns': '1fr', gap: '8px' }
              : { width: '100%' }
          "
        >
          <div
            v-for="ability in displayedAbilities"
            :key="ability.key"
            class="ability-box"
            :style="isMobile?.value ? { padding: '8px' } : null"
          >
            <div class="ability-header">
              <div class="ability-name">{{ ability.label }}</div>
            </div>
            <div
              class="ability-stats"
              :style="isMobile?.value ? { gap: '6px' } : null"
            >
              <div class="modifier-block">
                <div class="modifier-circle">
                  {{ ability.mod ?? '—' }}
                </div>
                <div class="ability-throws">
                  <span
                    class="throw-pill ability-throw-pill"
                    :class="{
                      'throw-pill--active': isThrowActive(
                        'disadvantage',
                        ability,
                        { name: '__ability__', isAbilityCheck: true }
                      ),
                    }"
                    @click="
                      onThrowClick('disadvantage', ability, {
                        name: '__ability__',
                        isAbilityCheck: true,
                        mod: ability.mod,
                      })
                    "
                  ></span>
                  <span
                    class="throw-pill ability-throw-pill"
                    :class="{
                      'throw-pill--active': isThrowActive('normal', ability, {
                        name: '__ability__',
                        isAbilityCheck: true,
                      }),
                    }"
                    @click="
                      onThrowClick('normal', ability, {
                        name: '__ability__',
                        isAbilityCheck: true,
                        mod: ability.mod,
                      })
                    "
                  ></span>
                  <span
                    class="throw-pill ability-throw-pill"
                    :class="{
                      'throw-pill--active': isThrowActive(
                        'advantage',
                        ability,
                        { name: '__ability__', isAbilityCheck: true }
                      ),
                    }"
                    @click="
                      onThrowClick('advantage', ability, {
                        name: '__ability__',
                        isAbilityCheck: true,
                        mod: ability.mod,
                      })
                    "
                  ></span>
                </div>
              </div>
              <div class="score-box">
                <div class="score-label">ЗНАЧЕНИЕ</div>
                <div class="score-value">{{ ability.score ?? '—' }}</div>
              </div>
            </div>
            <div class="skill-row save-row">
              <span
                class="prof-marker"
                :class="{
                  'prof-filled': ability.saveProf,
                }"
              >
                <span
                  v-if="ability.saveProf"
                  class="prof-icon prof-dot"
                ></span>
              </span>
              <span
                class="skill-modifier"
                :class="{ 'skill-mod-custom': ability.saveCustom }"
              >
                {{ ability.save ?? '—' }}
              </span>
              <span class="skill-name">Спасбросок</span>
              <div class="throws-group">
                <span
                  class="throw-pill"
                  :class="{
                    'throw-pill--active': isThrowActive(
                      'disadvantage',
                      ability,
                      null
                    ),
                  }"
                  title="Disadvantage"
                  @click="onThrowClick('disadvantage', ability, null)"
                  >D</span
                >
                <span
                  class="throw-pill"
                  :class="{
                    'throw-pill--active': isThrowActive(
                      'normal',
                      ability,
                      null
                    ),
                  }"
                  title="Normal"
                  @click="onThrowClick('normal', ability, null)"
                  >N</span
                >
                <span
                  class="throw-pill"
                  :class="{
                    'throw-pill--active': isThrowActive(
                      'advantage',
                      ability,
                      null
                    ),
                  }"
                  title="Advantage"
                  @click="onThrowClick('advantage', ability, null)"
                  >A</span
                >
              </div>
            </div>
            <div
              v-if="ability.skills?.length"
              class="skills-list"
            >
              <div
                v-for="skill in ability.skills"
                :key="skill.name || skill"
                class="skill-row"
              >
                <span
                  class="prof-marker"
                  :class="{
                    'prof-filled': skill.isProf && !skill.isExpert,
                    'prof-expert': skill.isExpert,
                  }"
                >
                  <span
                    v-if="skill.isExpert"
                    class="prof-icon prof-star"
                  >
                    ★
                  </span>
                  <span
                    v-else-if="skill.isProf"
                    class="prof-icon prof-dot"
                  ></span>
                </span>
                <span
                  class="skill-modifier"
                  :class="{ 'skill-mod-custom': skill.isCustom }"
                >
                  {{ skill.mod ?? '—' }}
                </span>
                <span class="skill-name">{{ skill.name || skill }}</span>
                <div class="throws-group">
                  <span
                    class="throw-pill"
                    :class="{
                      'throw-pill--active': isThrowActive(
                        'disadvantage',
                        ability,
                        skill
                      ),
                    }"
                    title="Disadvantage"
                    @click="onThrowClick('disadvantage', ability, skill)"
                    >D</span
                  >
                  <span
                    class="throw-pill"
                    :class="{
                      'throw-pill--active': isThrowActive(
                        'normal',
                        ability,
                        skill
                      ),
                    }"
                    title="Normal"
                    @click="onThrowClick('normal', ability, skill)"
                    >N</span
                  >
                  <span
                    class="throw-pill"
                    :class="{
                      'throw-pill--active': isThrowActive(
                        'advantage',
                        ability,
                        skill
                      ),
                    }"
                    title="Advantage"
                    @click="onThrowClick('advantage', ability, skill)"
                    >A</span
                  >
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
    <div class="actions d-flex flex-wrap gap-2 mt-3">
      <v-btn
        size="small"
        color="primary"
        @click="$emit('refresh')"
        text="Refresh"
      />
      <v-btn
        size="small"
        variant="tonal"
        @click="$emit('status')"
        :loading="statusLoading"
        text="Status"
      />
      <v-btn
        size="small"
        variant="outlined"
        @click="$emit('whoami')"
        text="Who am I"
      />
      <v-btn
        size="small"
        color="success"
        variant="tonal"
        @click="$emit('detect')"
        :loading="detectLoading"
        text="Detect (Python)"
      />
    </div>
  </div>
</template>

<script setup>
  import { ref, computed, watch } from 'vue';
  import { useDisplay } from 'vuetify';

  const props = defineProps({
    streamSrc: {
      type: String,
      default: '',
    },
    bboxStyle: {
      type: Object,
      default: null,
    },
    topDetection: {
      type: Object,
      default: null,
    },
    abilityPlaceholders: {
      type: Array,
      default: () => [],
    },
    playerStats: {
      type: Array,
      default: null,
    },
    profBonus: {
      type: Number,
      default: null,
    },
    rollResetKey: {
      type: Number,
      default: 0,
    },
    statusLoading: {
      type: Boolean,
      default: false,
    },
    detectLoading: {
      type: Boolean,
      default: false,
    },
  });

  const display = useDisplay();
  const thresholds = display.thresholds;
  const width = display.width;
  const isMobile = display.mobile;

  const currentSelection = ref(null);
  const activeThrowKey = ref(null);

  const emit = defineEmits([
    'refresh',
    'status',
    'whoami',
    'detect',
    'stream-load',
    'start-roll',
    'stop-roll',
  ]);

  function selectModifierFromSave(ability) {
    if (!ability) return;
    currentSelection.value = {
      label: `${ability.label} Спасбросок`,
      mod: Number(ability.save ?? 0) || 0,
    };
  }

  function selectModifierFromSkill(ability, skill) {
    if (!skill) return;
    currentSelection.value = {
      label: skill.isAbilityCheck
        ? `${ability.label} проверка`
        : skill.name || ability.label || '',
      mod: Number(skill.mod ?? 0) || 0,
    };
  }

  function makeThrowKey(mode, ability, skill) {
    const base = ability?.key || ability?.label || 'ability';
    const skillPart = skill?.isAbilityCheck ? '__ability__' : skill?.name || '';
    return `${base}::${skillPart}::${mode}`;
  }

  function isThrowActive(mode, ability, skill) {
    return activeThrowKey.value === makeThrowKey(mode, ability, skill);
  }

  function onThrowClick(mode, ability, skill) {
    const key = makeThrowKey(mode, ability, skill);
    if (activeThrowKey.value === key) {
      // Toggle off
      activeThrowKey.value = null;
      currentSelection.value = null;
      emit('stop-roll');
      return;
    }

    activeThrowKey.value = key;

    if (skill) {
      selectModifierFromSkill(ability, skill);
    } else {
      selectModifierFromSave(ability);
    }
    emit('start-roll', {
      mode,
      abilityKey: ability?.key || null,
      abilityLabel: ability?.label || '',
      skillName: skill?.name || null,
    });
  }

  function rollColorClass(value) {
    if (value === 1) return 'roll-value--fail';
    if (value === 20) return 'roll-value--crit';
    return '';
  }

  const primaryRoll = computed(() => {
    if (!activeThrowKey.value) return null;
    if (!props.topDetection) return null;
    const raw = props.topDetection.value ?? props.topDetection.class ?? null;
    const value = Number(raw);
    if (!Number.isFinite(value)) return null;
    const mod = currentSelection.value?.mod ?? 0;
    const total = value + mod;
    return { value, mod, total };
  });

  watch(
    () => props.rollResetKey,
    () => {
      activeThrowKey.value = null;
      currentSelection.value = null;
    }
  );

  const displayedAbilities = computed(() => {
    const src =
      (props.playerStats && props.playerStats.length
        ? props.playerStats
        : props.abilityPlaceholders) || [];

    // 4 view types based on Vuetify breakpoints (one-col included):
    // < md: 1-col (natural)
    // md–lg: 2-col (1,4),(2,5),(3,6)
    // lg–xl: 3-col (1,2,3),(4,5,6)
    // >= xl: natural (1..6)
    if (isMobile?.value) return src; // force 1-col on mobile

    const w = width?.value ?? 1920;
    const md = thresholds?.value?.md ?? 960;
    const lg = thresholds?.value?.lg ?? 1280;
    const xl = thresholds?.value?.xl ?? 1920;

    if (w < md) {
      return src; // 1-col natural
    }
    if (w >= md && w < lg) {
      const order = [0, 3, 1, 4, 2, 5]; // 2-col
      return order.map(i => src[i]).filter(Boolean);
    }
    if (w >= lg && w < xl) {
      return src; // 3-col natural
    }
    return src; // 4+ cols natural
  });
</script>

<style scoped>
  .stream-wrapper {
    position: relative;
    display: inline-block;
    width: 320px;
    max-width: 320px;
    min-height: 240px; /* reserve space for stream when enabled */
    background: transparent;
    border-radius: 0;
  }
  .stream-image {
    width: 100%;
    max-width: 100%;
    max-height: 240px;
    height: auto;
    border-radius: 6px;
    display: block;
    object-fit: contain;
  }
  .bbox-overlay {
    position: absolute;
    border: 2px solid #e91e63;
    box-shadow: 0 0 6px rgba(233, 30, 99, 0.7);
    pointer-events: none;
  }
  .bbox-label {
    position: absolute;
    top: 0;
    left: 50%;
    background: rgba(233, 30, 99, 0.85);
    color: #fff;
    padding: 2px 6px;
    font-size: 11px;
    border-radius: 3px;
    white-space: nowrap;
    transform: translate(-50%, calc(-100% - 1px));
  }
  .tower-details {
    width: 100%;
  }

  .stream-and-stats {
    gap: 12px;
    width: 100%;
    display: flex;
    flex-wrap: wrap;
    align-items: flex-start;
  }

  .stream-column {
    flex: 0 0 340px;
    max-width: 340px;
    min-width: 320px;
  }

  .stats-column {
    flex: 1 1 0;
    min-width: 250px;
    padding: 0;
  }

  .abilities-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 12px;
  }

  .ability-box {
    border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
    border-radius: 4px;
    padding: 10px;
    background: rgba(var(--v-theme-surface), 0.5);
  }

  .ability-header {
    margin-bottom: 6px;
  }

  .ability-name {
    font-weight: 700;
    font-size: 0.85rem;
    text-align: center;
    white-space: nowrap;
  }

  .ability-stats {
    display: flex;
    justify-content: flex-start;
    align-items: stretch;
    gap: 8px;
  }

  .modifier-block {
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    align-items: center;
  }

  .modifier-circle {
    width: 36px;
    height: 36px;
    border: 2px solid rgba(var(--v-theme-on-surface), 0.3);
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    font-weight: bold;
    font-size: 0.85rem;
    flex-shrink: 0;
  }

  .ability-throws {
    display: flex;
    gap: 3px;
  }

  .score-box {
    flex: 1;
    min-width: 0;
    border: 1px solid rgba(var(--v-theme-on-surface), 0.3);
    border-radius: 4px;
    padding: 6px;
    text-align: center;
  }

  .score-label {
    font-size: 0.65rem;
    opacity: 0.7;
  }

  .score-value {
    font-weight: 700;
    font-size: 1rem;
  }

  .skills-list {
    margin-top: 8px;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .skill-row {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 0.78rem;
  }
  .save-row {
    margin-top: 8px;
  }

  .prof-marker {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: 14px;
    height: 14px;
    border-radius: 50%;
    border: 1px solid rgba(var(--v-theme-on-surface), 0.5);
    margin-right: 4px;
  }
  .prof-filled {
    border-color: rgba(var(--v-theme-on-surface), 0.9);
  }
  .prof-expert {
    border-color: rgba(var(--v-theme-primary), 0.95);
  }
  .prof-icon {
    display: inline-block;
    line-height: 1;
  }
  .prof-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: #fff;
  }
  .prof-star {
    font-size: 0.6rem;
    color: #fff;
  }

  .skill-modifier {
    min-width: 28px;
    text-align: right;
    font-weight: 500;
  }
  .skill-mod-custom {
    color: #6ec1ff;
  }

  .skill-name {
    flex: 1;
    min-width: 0;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .throws-group {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }

  .throw-pill {
    width: 20px;
    height: 20px;
    border: 1px solid rgba(var(--v-theme-on-surface), 0.6);
    border-radius: 50%;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    font-size: 0.7rem;
    font-weight: 600;
    color: rgba(var(--v-theme-on-surface), 0.8);
    background: rgba(var(--v-theme-surface), 0.4);
    cursor: pointer;
    transition: background-color 0.15s ease, border-color 0.15s ease,
      color 0.15s ease;
  }

  .ability-throw-pill {
    width: 14px;
    height: 14px;
    min-width: 14px;
    min-height: 14px;
    padding: 0;
    border-radius: 50%;
    border-width: 1px;
    background: transparent;
    font-size: 0;
  }
  .throw-pill--active {
    border-color: rgba(var(--v-theme-primary), 0.9);
    background: rgba(var(--v-theme-primary), 0.2);
    color: rgba(var(--v-theme-primary), 1);
  }

  .roll-mini-display {
    width: 320px;
    height: 120px;
    max-width: 320px;
    background: rgba(0, 0, 0, 0.7);
    padding: 8px 10px;
    border-radius: 4px;
    font-size: 13px;
    color: #fff;
    margin-top: 8px;
    box-sizing: border-box;
  }
  .roll-line {
    display: flex;
    align-items: baseline;
    gap: 4px;
  }
  .roll-value {
    font-weight: 700;
  }
  .roll-value--crit {
    color: #4caf50;
  }
  .roll-value--fail {
    color: #f44336;
  }
  .roll-plus,
  .roll-total {
    opacity: 0.9;
  }
  .roll-label {
    margin-top: 2px;
    font-size: 12px;
    opacity: 0.8;
  }
</style>
