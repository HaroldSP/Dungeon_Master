<template>
  <div class="player-grid-root">
    <v-container
      fluid
      class="pa-0"
    >
      <v-row
        v-if="showHeader"
        class="mb-4"
        align="center"
      >
        <v-col
          cols="12"
          md="8"
        >
          <div class="text-h6">Players</div>
          <div class="text-body-2 text-medium-emphasis">
            Manage existing players. Use "Add New" tab to create a player.
          </div>
        </v-col>
      </v-row>

      <v-row v-if="players.length === 0">
        <v-col cols="12">
          <v-alert
            type="info"
            variant="tonal"
            >No players yet. Use the Add New tab to create one.</v-alert
          >
        </v-col>
      </v-row>

      <v-row>
        <v-col
          v-for="(player, index) in players"
          :key="index"
          cols="12"
        >
          <v-card
            elevation="2"
            class="pa-3 player-card"
          >
            <div class="d-flex justify-space-between align-center mb-2">
              <div>
                <div class="text-subtitle-1">
                  {{ player.name || 'Unnamed Player' }}
                </div>
                <div class="text-body-2 text-medium-emphasis">
                  {{ player.class }}
                  {{ player.level ? `Level ${player.level}` : '' }}
                </div>
              </div>
            </div>

            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-chip
                size="small"
                variant="outlined"
                v-if="player.race"
                >{{ player.race }}</v-chip
              >
              <v-chip
                size="small"
                variant="outlined"
                v-if="player.hp"
                >HP: {{ player.hp }}</v-chip
              >
              <v-chip
                size="small"
                variant="outlined"
                v-if="player.ac"
                >AC: {{ player.ac }}</v-chip
              >
            </div>

            <v-divider class="my-2" />

            <div class="d-flex flex-wrap gap-2">
              <v-btn
                size="small"
                variant="outlined"
                color="primary"
                @click="toggleExpand(index)"
              >
                {{ expanded[index] ? 'Hide' : 'Show' }} Details
              </v-btn>
            </div>

            <div
              v-if="expanded[index]"
              class="player-details-wrapper"
            >
              <PlayerDetails :player="player" />
            </div>
          </v-card>
        </v-col>
      </v-row>
    </v-container>
  </div>
</template>

<script setup>
  import { ref, reactive, onMounted } from 'vue';
  import PlayerDetails from './PlayerDetails.vue';

  const props = defineProps({
    showHeader: {
      type: Boolean,
      default: true,
    },
  });

  const players = ref([]);
  const expanded = ref({});

  function toggleExpand(index) {
    expanded.value = {
      ...expanded.value,
      [index]: !expanded.value[index],
    };
  }

  async function loadPlayers() {
    try {
      // Use Vite's import.meta.glob to load all JSON files from players folder and subfolders
      const playerModules = import.meta.glob('/src/data/players/**/*.json', {
        eager: true,
      });
      const loadedPlayers = [];

      for (const [path, module] of Object.entries(playerModules)) {
        try {
          const data = module.default || module;
          const parsedData =
            typeof data.data === 'string' ? JSON.parse(data.data) : data.data;

          loadedPlayers.push({
            id: path,
            name: parsedData?.name?.value || 'Unnamed',
            class: parsedData?.info?.charClass?.value || '',
            subclass: parsedData?.info?.charSubclass?.value || '',
            level: parsedData?.info?.level?.value || null,
            race: parsedData?.info?.race?.value || '',
            background: parsedData?.info?.background?.value || '',
            alignment: parsedData?.info?.alignment?.value || '',
            playerName: parsedData?.info?.playerName?.value || '',
            hp:
              parsedData?.vitality?.['hp-max']?.value ||
              parsedData?.vitality?.hpMax?.value ||
              null,
            ac: parsedData?.vitality?.ac?.value || null,
            speed: parsedData?.vitality?.speed?.value || null,
            hitDie: parsedData?.vitality?.['hit-die']?.value || null,
            stats: parsedData?.stats || {},
            saves: parsedData?.saves || {},
            skills: parsedData?.skills || {},
            spells: parsedData?.spells || {},
            profBonus: parsedData?.proficiency || null,
            raw: data,
          });
        } catch (e) {
          console.error(`Error parsing player file ${path}:`, e);
        }
      }

      players.value = loadedPlayers;
    } catch (e) {
      console.error('Error loading players:', e);
    }
  }

  onMounted(() => {
    loadPlayers();
  });
</script>

<style scoped>
  .player-grid-root {
    overflow: visible;
  }

  .gap-2 {
    gap: 8px;
  }
  .stat-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(70px, 1fr));
    gap: 8px;
  }
  .stat {
    background: rgba(var(--v-theme-surface-variant), 0.4);
    border-radius: 6px;
    padding: 6px;
    text-align: center;
  }
  .skills-list {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(140px, 1fr));
    gap: 6px;
  }

  .player-details-wrapper {
    margin-top: 12px;
    animation: expand 0.3s ease-out;
    max-height: 600px;
    overflow-y: auto;
    overflow-x: hidden;
    width: 100%;
    max-width: 100%;
    padding-left: 2px;
    padding-right: 12px;
  }

  :deep(.v-container) {
    overflow: visible !important;
  }

  :deep(.v-row) {
    overflow: visible !important;
  }

  :deep(.v-col) {
    overflow: visible !important;
  }

  .player-card {
    overflow: visible !important;
    max-height: none !important;
  }

  :deep(.v-card) {
    overflow: visible !important;
    max-height: none !important;
  }

  :deep(.v-card__content) {
    overflow: visible !important;
  }

  @keyframes expand {
    from {
      opacity: 0;
      max-height: 0;
      overflow: hidden;
    }
    to {
      opacity: 1;
      max-height: 2000px;
    }
  }
</style>
