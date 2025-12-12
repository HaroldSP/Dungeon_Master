<template>
  <v-container class="scrollable-content" style="height: 100%; overflow-y: auto">
    <v-row>
      <v-col cols="12">
        <h1 class="text-h4 mb-4">Spell ID Mapper</h1>
        <p class="text-body-1 mb-4">
          Match spell IDs from the character JSON to spell names from the PDF.
          Select a player, scan their PDF, then fill in any missing spell names.
        </p>
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12">
        <v-card class="mb-4">
          <v-card-title>Player Selection</v-card-title>
          <v-card-text>
            <v-row>
              <v-col cols="12" md="6">
                <v-select
                  v-model="selectedPlayer"
                  :items="playerOptions"
                  label="Select Player"
                  variant="outlined"
                  density="comfortable"
                  @update:model-value="onPlayerSelected"
                >
                  <template #item="{ item, props: itemProps }">
                    <v-list-item v-bind="itemProps">
                      <template #prepend>
                        <v-icon>mdi-account</v-icon>
                      </template>
                      <v-list-item-title>{{ item.title }}</v-list-item-title>
                      <v-list-item-subtitle>
                        {{ item.subtitle }}
                      </v-list-item-subtitle>
                    </v-list-item>
                  </template>
                </v-select>
              </v-col>
              <v-col cols="12" md="6">
                <v-btn
                  v-if="selectedPlayer"
                  color="primary"
                  block
                  :loading="scanningPDF"
                  @click="scanPDF"
                >
                  <v-icon start>mdi-file-pdf-box</v-icon>
                  Scan PDF
                </v-btn>
              </v-col>
            </v-row>

            <v-alert
              v-if="scanResult"
              :type="scanResult.success ? 'success' : 'error'"
              variant="tonal"
              density="compact"
              class="mt-3"
            >
              {{ scanResult.message }}
            </v-alert>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12">
        <v-card style="height: 80vh; display: flex; flex-direction: column">
          <v-card-title>Spell Mapper</v-card-title>
          <v-card-text
            class="scrollable-content"
            style="
              flex: 1 1 auto;
              overflow-y: scroll;
              overflow-x: hidden;
              padding: 16px;
            "
          >
            <v-row>
              <v-col cols="12" md="8">
                <h3 class="text-h6 mb-4">
                  Spell Mappings
                  <span v-if="spellMappings.length > 0" class="text-caption text-medium-emphasis">
                    ({{ spellMappings.length }} spells)
                  </span>
                </h3>
                <v-alert
                  v-if="!selectedPlayer"
                  type="info"
                  variant="tonal"
                  density="compact"
                  class="mb-4"
                >
                  Please select a player above to load their spells.
                </v-alert>
                <v-alert
                  v-else-if="spellMappings.length === 0"
                  type="warning"
                  variant="tonal"
                  density="compact"
                  class="mb-4"
                >
                  No prepared spells found for this player.
                </v-alert>
                <div
                  v-for="(spell, index) in spellMappings"
                  v-else
                  :key="spell.id"
                  class="mb-4"
                >
                  <v-text-field
                    v-model="spell.name"
                    :label="`Spell ${index + 1} (ID: ${spell.id.substring(0, 8)}...)`"
                    :placeholder="'Enter spell name (e.g., Искусство друидов [Druidcraft])'"
                    variant="outlined"
                    density="comfortable"
                    class="mb-2"
                  >
                    <template #prepend-inner>
                      <span class="text-caption text-medium-emphasis mr-2">
                        {{ index + 1 }}.
                      </span>
                    </template>
                  </v-text-field>

                  <v-row dense>
                    <v-col cols="6">
                      <v-select
                        v-model="spell.level"
                        :items="levelOptions"
                        label="Level"
                        variant="outlined"
                        density="compact"
                      ></v-select>
                    </v-col>
                    <v-col cols="6">
                      <v-select
                        v-model="spell.school"
                        :items="schoolOptions"
                        label="School (optional)"
                        variant="outlined"
                        density="compact"
                        clearable
                      ></v-select>
                    </v-col>
                  </v-row>
                </div>
              </v-col>

              <v-col cols="12" md="4">
                <h3 class="text-h6 mb-4">Generated JSON</h3>
                <v-textarea
                  :model-value="generatedJSON"
                  readonly
                  variant="outlined"
                  rows="15"
                  class="font-monospace mb-2"
                  style="font-size: 12px"
                ></v-textarea>
                <v-btn
                  color="primary"
                  block
                  class="mb-2"
                  @click="copyToClipboard"
                >
                  <v-icon start>mdi-content-copy</v-icon>
                  Copy JSON
                </v-btn>
                <v-alert
                  v-if="copySuccess"
                  type="success"
                  density="compact"
                  class="mb-2"
                >
                  Copied to clipboard!
                </v-alert>
                <v-alert
                  type="info"
                  density="compact"
                  variant="tonal"
                >
                  <small>
                    <strong>Note:</strong> The "Copy JSON" button copies to your clipboard.
                    To save to a file, paste the JSON into a text editor and save it.
                  </small>
                </v-alert>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12">
        <v-alert type="info" variant="tonal">
          <strong>Instructions:</strong>
          <ol class="mt-2">
            <li>Select a player from the dropdown</li>
            <li>Click "Scan PDF" to extract spells from the player's PDF file (requires backend API)</li>
            <li>Review and fill in any missing spell names</li>
            <li>Select spell levels and schools</li>
            <li>Click "Copy JSON" to copy the generated spell data</li>
            <li>
              <strong>Paste the copied JSON into <code>frontend/src/data/spells.json</code></strong>
              <br />
              <small class="text-medium-emphasis">
                The file should be an array. Merge the new spells into the existing array, avoiding duplicates by checking the <code>_id</code> field.
              </small>
            </li>
          </ol>
        </v-alert>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue';

const selectedPlayer = ref(null);
const players = ref([]);
const scanningPDF = ref(false);
const scanResult = ref(null);
const spellMappings = ref([]);

const levelOptions = [
  { title: 'Cantrip (0)', value: 0 },
  { title: 'Level 1', value: 1 },
  { title: 'Level 2', value: 2 },
  { title: 'Level 3', value: 3 },
  { title: 'Level 4', value: 4 },
  { title: 'Level 5', value: 5 },
];

const schoolOptions = [
  { title: 'Abjuration (Ограждение)', value: 'abj' },
  { title: 'Conjuration (Вызов)', value: 'con' },
  { title: 'Divination (Прорицание)', value: 'div' },
  { title: 'Enchantment (Очарование)', value: 'enc' },
  { title: 'Evocation (Воплощение)', value: 'evo' },
  { title: 'Illusion (Иллюзия)', value: 'ill' },
  { title: 'Necromancy (Некромантия)', value: 'nec' },
  { title: 'Transmutation (Преобразование)', value: 'trs' },
];

const copySuccess = ref(false);

const playerOptions = computed(() => {
  return players.value.map((player) => ({
    title: player.name || 'Unnamed',
    value: player.id,
    subtitle: `${player.class}${player.level ? ` Level ${player.level}` : ''}`,
  }));
});

async function loadPlayers() {
  try {
    // Use Vite's import.meta.glob to load all JSON files from players folder and subfolders
    const playerModules = import.meta.glob('/src/data/players/**/*.json', { eager: true });
    const loadedPlayers = [];

    for (const [path, module] of Object.entries(playerModules)) {
      try {
        const data = module.default || module;
        const parsedData = typeof data.data === 'string' ? JSON.parse(data.data) : data.data;
        
        // Extract folder name from path (e.g., /src/data/players/Lia/file.json -> Lia)
        const pathParts = path.split('/');
        const folderName = pathParts[pathParts.length - 2] || 'Unknown';
        
        // Get prepared spells from root level spells.prepared OR from parsed data
        const preparedSpells = data.spells?.prepared || parsedData?.spells?.prepared || [];
        
        loadedPlayers.push({
          id: path,
          folderName: folderName,
          name: parsedData?.name?.value || 'Unnamed',
          class: parsedData?.info?.charClass?.value || '',
          subclass: parsedData?.info?.charSubclass?.value || '',
          level: parsedData?.info?.level?.value || null,
          raw: data,
          preparedSpells: preparedSpells,
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

function onPlayerSelected() {
  if (!selectedPlayer.value) {
    spellMappings.value = [];
    return;
  }

  const player = players.value.find((p) => p.id === selectedPlayer.value);
  if (!player) {
    console.error('Player not found:', selectedPlayer.value);
    spellMappings.value = [];
    return;
  }

  console.log('Selected player:', player);
  console.log('Prepared spells:', player.preparedSpells);

  // Load spell IDs from player's prepared spells
  const preparedSpellIds = player.preparedSpells || [];
  console.log('Prepared spell IDs:', preparedSpellIds);
  
  spellMappings.value = preparedSpellIds.map((spellId) => ({
    id: spellId,
    name: '',
    level: null,
    school: null,
  }));

  console.log('Initial spell mappings:', spellMappings.value);

  // Pre-fill known spells (for Lia)
  if (player.folderName === 'Lia' || player.name?.includes('Лиа') || player.name?.includes('Lia')) {
    const knownSpells = {
      '65d3c16ff3d820fa1add46fe': { name: 'Искусство друидов [Druidcraft]', level: 0, school: 'trs' },
      '65d3c16bf3d820fa1add4418': { name: 'Сопротивление [Resistance]', level: 0, school: 'abj' },
      '65d3c169f3d820fa1add42d6': { name: 'Сотворение пламени [Produce Flame]', level: 0, school: 'con' },
      '65d3c168f3d820fa1add41ef': { name: 'Указание [Guidance]', level: 0, school: 'div' },
      '65d3c175f3d820fa1add49b3': { name: 'Ядовитые брызги [Poison Spray]', level: 0, school: 'con' },
      '65d3c172f3d820fa1add4871': { name: 'Лечащее слово [Healing Word]', level: 1, school: 'evo' },
      '65d3c16ff3d820fa1add4713': { name: 'Лечение ран [Cure Wounds]', level: 1, school: 'evo' },
      '65d3c16ff3d820fa1add4721': { name: 'Направляющий снаряд [Guiding Bolt]', level: 1, school: 'evo' },
      '67cfe212bad3f549390199d5': { name: 'Поглощение стихий [Absorb elements]', level: 1, school: 'abj' },
      '65d3c172f3d820fa1add488d': { name: 'Обнаружение магии [Detect Magic]', level: 1, school: 'div' },
      '65d3c174f3d820fa1add495f': { name: 'Разговор с животными [Speak with Animals]', level: 1, school: 'div' },
      '65d3c16cf3d820fa1add44b2': { name: 'Долгий шаг [Longstrider]', level: 1, school: 'trs' },
      '65d3c16bf3d820fa1add4457': { name: 'Поиск предмета [Locate Object]', level: 2, school: 'div' },
      '65d3c16ff3d820fa1add4705': { name: 'Поиск животных или растений [Locate Animals or Plants]', level: 2, school: 'div' },
    };

    spellMappings.value = spellMappings.value.map((spell) => {
      const known = knownSpells[spell.id];
      if (known) {
        return { ...spell, ...known };
      }
      return spell;
    });
    
    console.log('After pre-fill spell mappings:', spellMappings.value);
  }
}

async function scanPDF() {
  if (!selectedPlayer.value) {
    scanResult.value = { success: false, message: 'Please select a player first' };
    return;
  }

  const player = players.value.find((p) => p.id === selectedPlayer.value);
  if (!player) return;

  scanningPDF.value = true;
  scanResult.value = null;

  try {
    // TODO: This requires a backend API endpoint to scan PDFs
    // For now, we'll show a message explaining this
    // In the future, this should call: POST /api/scan-pdf with { playerFolder: player.folderName }
    
    // Simulate API call (replace with actual API call when backend is ready)
    await new Promise((resolve) => setTimeout(resolve, 1000));
    
    scanResult.value = {
      success: false,
      message: 'PDF scanning requires a backend API. This feature will be implemented when the backend endpoint is available.',
    };

    // When backend is ready, uncomment this:
    /*
    const response = await fetch('/api/scan-pdf', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ playerFolder: player.folderName }),
    });
    
    if (!response.ok) throw new Error('PDF scan failed');
    
    const data = await response.json();
    // Update spellMappings with extracted spell names
    scanResult.value = { success: true, message: `Found ${data.spells.length} spells in PDF` };
    */
  } catch (error) {
    console.error('PDF scan error:', error);
    scanResult.value = {
      success: false,
      message: `Error scanning PDF: ${error.message}`,
    };
  } finally {
    scanningPDF.value = false;
  }
}

const generatedJSON = computed(() => {
  const spells = spellMappings.value
    .filter((s) => s.name && s.level !== null)
    .map((spell) => {
      const spellObj = {
        name: spell.name,
        type: 'spell',
        system: {
          level: spell.level,
          school: spell.school || 'evo',
          components: {
            vocal: true,
            somatic: true,
          },
        },
        _id: spell.id,
        playerId: '676850ded05cc329905e8835', // TODO: Get from selected player
        createdAt: new Date().toISOString(),
        updatedAt: new Date().toISOString(),
        __v: 0,
      };
      return spellObj;
    });

  return JSON.stringify(spells, null, 2);
});

async function copyToClipboard() {
  try {
    await navigator.clipboard.writeText(generatedJSON.value);
    copySuccess.value = true;
    setTimeout(() => {
      copySuccess.value = false;
    }, 2000);
  } catch (err) {
    console.error('Failed to copy:', err);
  }
}

onMounted(() => {
  loadPlayers();
});
</script>

<style scoped>
.font-monospace {
  font-family: 'Courier New', monospace;
}

/* Hide scrollbar but keep scrolling functionality */
.scrollable-content {
  scrollbar-width: none; /* Firefox */
  -ms-overflow-style: none; /* IE and Edge */
}

.scrollable-content::-webkit-scrollbar {
  display: none; /* Chrome, Safari, Opera */
}
</style>
