<template>
  <div class="tower-details">
    <div class="stream-and-stats d-flex flex-wrap align-start ga-3">
      <div class="stream-column">
        <div class="stream-wrapper">
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
      <div class="stats-column">
        <div class="abilities-grid">
          <div
            v-for="ability in displayedAbilities"
            :key="ability.key"
            class="ability-box"
          >
            <div class="ability-header">
              <div class="ability-name">{{ ability.label }}</div>
            </div>
            <div class="ability-stats">
              <div class="modifier-circle">—</div>
              <div class="score-box">
                <div class="score-label">ЗНАЧЕНИЕ</div>
                <div class="score-value">—</div>
              </div>
            </div>
            <div class="skill-row save-row">
              <span class="skill-modifier">—</span>
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
                :key="skill"
                class="skill-row"
              >
                <span class="skill-modifier">—</span>
                <span class="skill-name">{{ skill }}</span>
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
    statusLoading: {
      type: Boolean,
      default: false,
    },
    detectLoading: {
      type: Boolean,
      default: false,
    },
  });

  const displayedAbilities = computed(() => {
    const src = props.abilityPlaceholders || [];
    // When there are exactly 2 columns, use column-major order (1,4),(2,5),(3,6)
    // Else, keep natural order.
    // We approximate by number of items: if <=4, keep natural (likely 2 cols or less); if 6 and layout likely 3 cols, keep natural.
    if (src.length === 6) {
      return src;
    }
    if (src.length > 0) {
      return src;
    }
    return [];
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
    width: 320px;
    max-width: 320px;
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
    min-width: 360px;
    padding: 0;
  }

  .abilities-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
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

  .skill-modifier {
    min-width: 28px;
    text-align: right;
    font-weight: 500;
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
