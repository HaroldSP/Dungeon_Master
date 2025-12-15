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
              <div class="modifier-circle">
                {{ ability.mod ?? '—' }}
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
                  title="Disadvantage"
                  >D</span
                >
                <span
                  class="throw-pill"
                  title="Normal"
                  >N</span
                >
                <span
                  class="throw-pill"
                  title="Advantage"
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
                    title="Disadvantage"
                    >D</span
                  >
                  <span
                    class="throw-pill"
                    title="Normal"
                    >N</span
                  >
                  <span
                    class="throw-pill"
                    title="Advantage"
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
  import { computed } from 'vue';
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

  defineEmits(['refresh', 'status', 'whoami', 'detect', 'stream-load']);
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
    justify-content: space-between;
    align-items: center;
    gap: 8px;
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
  }
</style>
