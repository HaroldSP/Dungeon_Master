<template>
  <div class="tower-grid-root">
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
          <div class="text-h6">Dice Towers</div>
          <div class="text-body-2 text-medium-emphasis">
            Manage existing towers. Use "Add New" tab to create a tower.
          </div>
        </v-col>
      </v-row>

      <v-row v-if="towers.length === 0">
        <v-col cols="12">
          <v-alert
            type="info"
            variant="tonal"
            >No towers yet. Use the Add New tab to create one.</v-alert
          >
        </v-col>
      </v-row>

      <v-row>
        <v-col
          v-for="(t, index) in towers"
          :key="t.id"
          cols="12"
        >
          <v-card
            elevation="2"
            class="pa-3"
          >
            <div class="d-flex justify-space-between align-center mb-2">
              <div>
                <div class="text-subtitle-1">
                  Tower {{ index + 1 }} — {{ t.towerName || 'Tower' }}
                </div>
                <div class="text-body-2 text-medium-emphasis">
                  Player: {{ t.playerName || 'Player' }}
                </div>
              </div>
              <v-chip
                :color="t.online ? 'success' : 'error'"
                size="small"
                variant="flat"
              >
                {{ t.online ? 'ONLINE' : 'OFFLINE' }}
              </v-chip>
            </div>
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-chip
                size="small"
                variant="outlined"
                v-if="t.lastStaIp"
                >STA {{ t.lastStaIp }}</v-chip
              >
              <v-chip
                size="small"
                variant="outlined"
                v-if="t.lastApIp"
                >AP {{ t.lastApIp }}</v-chip
              >
            </div>
            <div class="text-body-2 mb-1">
              ESP32-CAM API: {{ t.apiBase || '—' }}
            </div>
            <div class="text-body-2 mb-1">Stream: {{ t.streamUrl || '—' }}</div>
            <div class="text-body-2 mb-1">
              NodeMCU API: {{ t.nodemcuApiBase || '—' }}
            </div>
            <div class="text-caption text-medium-emphasis mb-2">
              Last seen:
              {{
                t.lastSeenAt ? new Date(t.lastSeenAt).toLocaleTimeString() : '—'
              }}
            </div>

            <v-divider class="my-2" />

            <div class="d-flex flex-wrap gap-2">
              <v-btn
                size="small"
                @click="pingStatus(t)"
                :loading="loading[`${t.id}-status`]"
                text="Status"
              />
              <v-btn
                size="small"
                color="success"
                @click="captureOnce(t)"
                :loading="loading[`${t.id}-capture`]"
                text="Capture"
              />
              <v-btn
                size="small"
                variant="outlined"
                color="primary"
                @click="openEdit(t)"
                text="Edit"
              />
              <v-btn
                size="small"
                variant="outlined"
                color="error"
                @click="remove(t.id)"
                text="Remove"
              />
              <v-btn
                size="small"
                variant="outlined"
                color="primary"
                @click="toggleDetails(t)"
                :text="
                  uiStore.towerDetailsExpanded[t.id]
                    ? 'Hide details'
                    : 'Details'
                "
              />
            </div>

            <v-expand-transition>
              <div
                v-if="uiStore.towerDetailsExpanded[t.id]"
                class="mt-3"
              >
                <div class="d-flex align-center justify-space-between mb-1">
                  <div class="text-body-2">Live Stream</div>
                  <div class="d-flex align-center text-caption ga-3">
                    <div class="d-flex align-center">
                      <span class="mr-2">Difficulty Class</span>
                      <v-switch
                        :model-value="
                          uiStore.difficultyClassEnabled[t.id] === true
                        "
                        @update:model-value="
                          val => uiStore.setDifficultyClassEnabled(t.id, val)
                        "
                        hide-details
                        inset
                        density="compact"
                        class="dc-enabled-switch"
                      />
                      <v-text-field
                        v-if="uiStore.difficultyClassEnabled[t.id] === true"
                        :model-value="uiStore.difficultyClassValue[t.id] || ''"
                        @update:model-value="
                          val => {
                            console.log('[TowerGrid] DC value changed:', {
                              towerId: t.id,
                              value: val,
                              type: typeof val,
                            });
                            uiStore.setDifficultyClassValue(t.id, val);
                          }
                        "
                        @blur="
                          () => {
                            const val = uiStore.difficultyClassValue[t.id];
                            console.log('[TowerGrid] DC value on blur:', {
                              towerId: t.id,
                              value: val,
                            });
                          }
                        "
                        type="number"
                        hide-details
                        density="compact"
                        variant="outlined"
                        class="dc-value-input ml-2"
                        style="max-width: 80px"
                        min="1"
                        max="30"
                      />
                    </div>
                    <div class="d-flex align-center">
                      <span class="mr-2">Results auto clear</span>
                      <v-switch
                        :model-value="uiStore.resultsAutoClear[t.id] !== false"
                        @update:model-value="
                          val => uiStore.setResultsAutoClear(t.id, val)
                        "
                        hide-details
                        inset
                        density="compact"
                        class="results-auto-clear-switch"
                      />
                    </div>
                  </div>
                </div>
                <TowerDetails
                  :stream-src="streamSrc[t.id]"
                  :bbox-style="getPyBox(t)"
                  :all-boxes="getAllPyBoxes(t)"
                  :top-detection="getPyTopDet(pythonResults[t.id])"
                  :ability-placeholders="abilityPlaceholders"
                  :player-stats="getPlayerStatsForTower(t)"
                  :prof-bonus="getPlayerProfBonusForTower(t)"
                  :roll-reset-key="rollResetKey[t.id] || 0"
                  :roll-mode="rollSessions[t.id]?.mode || 'normal'"
                  :dice-values="rollSessions[t.id]?.diceValues || null"
                  :one-shot-value="oneShotValues[t.id] ?? null"
                  :status-loading="loading[`${t.id}-status`]"
                  :detect-loading="loading[`${t.id}-py`]"
                  @stream-load="onStreamLoad(t, $event)"
                  @refresh="refreshStream(t)"
                  @status="pingStatus(t)"
                  @whoami="whoamiEsp(t)"
                  @detect="detectViaPython(t)"
                  @start-roll="startRoll(t, $event)"
                  @stop-roll="stopRoll(t)"
                />

                <v-divider class="my-3" />
                <div class="text-body-2 mb-1">
                  LED / NodeMCU controls (optional)
                </div>
                <div class="d-flex flex-wrap gap-2">
                  <v-btn
                    size="small"
                    color="primary"
                    @click="callNode(t, '/strip/on')"
                    text="Strip On"
                  />
                  <v-btn
                    size="small"
                    color="secondary"
                    @click="callNode(t, '/strip/off')"
                    text="Strip Off"
                  />
                  <v-btn
                    size="small"
                    variant="tonal"
                    @click="callNode(t, '/strip/mode/toggle')"
                    text="Mode"
                  />
                  <v-btn
                    size="small"
                    color="info"
                    @click="callNode(t, '/led/on')"
                    text="LED On"
                  />
                  <v-btn
                    size="small"
                    color="info"
                    variant="outlined"
                    @click="callNode(t, '/led/off')"
                    text="LED Off"
                  />
                  <v-btn
                    size="small"
                    variant="outlined"
                    @click="whoamiNode(t)"
                    text="Who am I"
                  />
                </div>

                <v-alert
                  v-if="t.lastDetection"
                  class="mt-2"
                  density="compact"
                  variant="tonal"
                  type="success"
                >
                  Last roll: {{ t.lastDetection.value }} (conf
                  {{ t.lastDetection.confidence?.toFixed?.(2) ?? '—' }})
                </v-alert>
                <v-alert
                  v-if="pythonResults[t.id]"
                  class="mt-2"
                  density="compact"
                  variant="tonal"
                  :type="pythonResults[t.id].error ? 'error' : 'info'"
                >
                  <div v-if="pythonResults[t.id].error">
                    {{ pythonResults[t.id].error }}
                  </div>
                  <div v-else-if="getPyTopDet(pythonResults[t.id])">
                    Python:
                    {{ getPyTopDet(pythonResults[t.id]).class }} (conf
                    {{
                      getPyTopDet(pythonResults[t.id]).confidence?.toFixed?.(
                        2
                      ) ?? '—'
                    }})
                  </div>
                  <div v-else>Python: no detections</div>
                </v-alert>
              </div>
            </v-expand-transition>
          </v-card>
        </v-col>
      </v-row>
    </v-container>

    <v-dialog
      v-model="editDialog"
      max-width="700"
    >
      <v-card>
        <v-card-title class="text-h6">Edit Tower</v-card-title>
        <v-card-text>
          <v-row>
            <v-col
              cols="12"
              md="6"
            >
              <v-select
                v-model="editSelectedPlayer"
                :items="players"
                item-title="title"
                item-value="value"
                label="Player"
                variant="outlined"
                density="comfortable"
                return-object
                clearable
                hint="Select a player JSON"
                persistent-hint
                class="mb-2"
              />
            </v-col>
            <v-col
              cols="12"
              md="6"
            >
              <v-text-field
                v-model="editForm.towerName"
                label="Tower name"
                variant="outlined"
                density="comfortable"
                class="mb-2"
              />
            </v-col>
          </v-row>

          <v-card
            elevation="1"
            class="pa-3 mb-3"
          >
            <div class="text-subtitle-1 mb-2">Step 2: ESP32-CAM</div>
            <p class="text-body-2 mb-2">
              Connect via AP (<code>DiceTower-XXYY</code>, pass
              <code>dungeon123</code>) or use STA fields below to edit
              endpoints.
            </p>
            <v-text-field
              v-model="editForm.apiBase"
              label="ESP32-CAM API (STA)"
              variant="outlined"
              density="comfortable"
              class="mb-2"
            />
            <v-text-field
              v-model="editForm.streamUrl"
              label="ESP32-CAM Stream"
              variant="outlined"
              density="comfortable"
              class="mb-2"
            />
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                size="small"
                variant="tonal"
                @click="pingStatus(currentEditTower)"
                >Status</v-btn
              >
              <v-btn
                size="small"
                color="success"
                variant="tonal"
                @click="captureOnce(currentEditTower)"
                >Capture</v-btn
              >
              <v-btn
                size="small"
                variant="outlined"
                @click="whoamiEsp(currentEditTower)"
                >Who am I</v-btn
              >
              <v-btn
                size="small"
                class="mb-2"
                color="success"
                variant="tonal"
                @click="detectViaPython(currentEditTower)"
                :loading="loading[`${currentEditTower?.id}-py`]"
                >Detect (Python)</v-btn
              >
            </div>
            <v-text-field
              v-model="editForm.pyServerUrl"
              label="Python server URL"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              :placeholder="pyDefault"
            />
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                size="small"
                variant="outlined"
                @click="pyHealthEdit"
                >Server health</v-btn
              >
              <v-btn
                size="small"
                color="success"
                variant="tonal"
                @click="pyTestDetectEdit"
                :loading="loading[`${currentEditTower?.id || 'edit'}-py`]"
                >Test detection</v-btn
              >
              <v-btn
                v-if="editPyChanged"
                size="small"
                color="primary"
                variant="outlined"
                @click="saveEditPyAddress"
              >
                Save new address
              </v-btn>
            </div>
            <v-alert
              v-if="editPyStatus"
              type="info"
              variant="tonal"
              class="mb-2"
            >
              {{ editPyStatus }}
            </v-alert>
          </v-card>

          <v-card
            elevation="1"
            class="pa-3 mb-3"
          >
            <div class="text-subtitle-1 mb-2">Step 3: NodeMCU</div>
            <v-text-field
              v-model="editForm.nodemcuApiBase"
              label="NodeMCU API (home/work IP)"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="http://192.168.110.54"
              persistent-placeholder
            />

            <!-- NodeMCU provisioning (for adding the LED node later) -->
            <p class="text-body-2 mb-2">
              Connect NodeMCU to your home Wi‑Fi (only needed once per device).
            </p>
            <v-text-field
              v-model="editNodeApUrl"
              label="NodeMCU AP URL (for provisioning)"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="http://192.168.4.1"
              persistent-placeholder
            />
            <v-text-field
              v-model="editNodeSsid"
              label="Home Wi‑Fi SSID"
              variant="outlined"
              density="comfortable"
              class="mb-2"
              placeholder="your-ssid"
              persistent-placeholder
            />
            <v-text-field
              v-model="editNodePass"
              label="Home Wi‑Fi Password"
              type="password"
              variant="outlined"
              density="comfortable"
              class="mb-3"
              placeholder="your-password"
              persistent-placeholder
            />
            <div class="d-flex flex-wrap gap-2 mb-2">
              <v-btn
                size="small"
                color="primary"
                variant="tonal"
                @click="provisionNodeForEdit(currentEditTower)"
              >
                Send Credentials
              </v-btn>
              <v-btn
                size="small"
                variant="tonal"
                @click="pingNodeStatus(currentEditTower)"
              >
                Status
              </v-btn>
              <v-btn
                size="small"
                color="primary"
                variant="tonal"
                @click="callNode(currentEditTower, '/strip/on')"
              >
                Strip On
              </v-btn>
              <v-btn
                size="small"
                color="secondary"
                variant="tonal"
                @click="callNode(currentEditTower, '/strip/off')"
              >
                Strip Off
              </v-btn>
              <v-btn
                size="small"
                variant="tonal"
                @click="callNode(currentEditTower, '/strip/mode/toggle')"
              >
                Mode
              </v-btn>
              <v-btn
                size="small"
                color="info"
                variant="tonal"
                @click="callNode(currentEditTower, '/led/on')"
              >
                LED On
              </v-btn>
              <v-btn
                size="small"
                color="info"
                variant="outlined"
                @click="callNode(currentEditTower, '/led/off')"
              >
                LED Off
              </v-btn>
              <v-btn
                size="small"
                variant="outlined"
                @click="whoamiNode(currentEditTower)"
              >
                Who am I
              </v-btn>
            </div>
            <v-alert
              v-if="editNodeStatusText"
              type="info"
              variant="tonal"
              class="mt-2"
            >
              {{ editNodeStatusText }}
            </v-alert>
          </v-card>
        </v-card-text>
        <v-card-actions>
          <v-spacer />
          <v-btn
            variant="text"
            @click="editDialog = false"
            >Cancel</v-btn
          >
          <v-btn
            color="primary"
            @click="saveEdit"
            >Save</v-btn
          >
        </v-card-actions>
      </v-card>
    </v-dialog>
  </div>
</template>

<script setup>
  import { ref, computed, onMounted, onBeforeUnmount, watch } from 'vue';
  import { useTowerStore } from '../stores/towerStore';
  import { useUiStore } from '../stores/uiStore';
  import { useRollBroadcastStore } from '../stores/rollBroadcastStore';
  import { storeToRefs } from 'pinia';
  import TowerDetails from './TowerDetails.vue';

  const pyServerUrl = import.meta.env.VITE_PY_SERVER_URL || '';

  const props = defineProps({
    showHeader: {
      type: Boolean,
      default: true,
    },
  });

  const towerStore = useTowerStore();
  const uiStore = useUiStore();
  const rollBroadcast = useRollBroadcastStore();
  const { towers } = storeToRefs(towerStore);

  const loading = ref({});
  const streamSrc = ref({});
  const streamSizes = ref({});
  const streamEnabled = ref({});
  const pythonResults = ref({});
  const rollSessions = ref({});
  const rollResetKey = ref({});
  const detectAutoClearTimers = ref({});
  const oneShotValues = ref({}); // { [towerId]: number | null } for Detect button
  const abilityPlaceholders = [
    { key: 'str', label: 'СИЛА', skills: ['Атлетика'] },
    {
      key: 'dex',
      label: 'ЛОВКОСТЬ',
      skills: ['Акробатика', 'Ловкость рук', 'Скрытность'],
    },
    { key: 'con', label: 'ТЕЛОСЛОЖЕНИЕ', skills: [] },
    {
      key: 'int',
      label: 'ИНТЕЛЛЕКТ',
      skills: ['Анализ', 'История', 'Магия', 'Природа', 'Религия'],
    },
    {
      key: 'wis',
      label: 'МУДРОСТЬ',
      skills: [
        'Проницательность',
        'Медицина',
        'Уход за животными',
        'Восприятие',
        'Выживание',
      ],
    },
    {
      key: 'cha',
      label: 'ХАРИЗМА',
      skills: ['Обман', 'Запугивание', 'Выступление', 'Убеждение'],
    },
  ];

  const abilityOrder = ['str', 'dex', 'con', 'int', 'wis', 'cha'];
  const abilityNames = {
    str: 'СИЛА',
    dex: 'ЛОВКОСТЬ',
    con: 'ТЕЛОСЛОЖЕНИЕ',
    int: 'ИНТЕЛЛЕКТ',
    wis: 'МУДРОСТЬ',
    cha: 'ХАРИЗМА',
  };
  const skillNames = {
    athletics: 'Атлетика',
    acrobatics: 'Акробатика',
    'sleight of hand': 'Ловкость рук',
    stealth: 'Скрытность',
    arcana: 'Магия',
    history: 'История',
    investigation: 'Анализ',
    nature: 'Природа',
    religion: 'Религия',
    'animal handling': 'Уход за животными',
    insight: 'Проницательность',
    medicine: 'Медицина',
    perception: 'Восприятие',
    survival: 'Выживание',
    deception: 'Обман',
    intimidation: 'Запугивание',
    performance: 'Выступление',
    persuasion: 'Убеждение',
  };
  const editDialog = ref(false);
  const editPyStatus = ref('');
  const editForm = ref({
    id: '',
    playerName: '',
    towerName: '',
    apiBase: '',
    streamUrl: '',
    nodemcuApiBase: '',
    pyServerUrl: '',
  });
  const editNodeSsid = ref('');
  const editNodePass = ref('');
  const editNodeApUrl = ref('http://192.168.4.1');
  const editNodeStatusText = ref('');
  const pyDefault =
    import.meta.env.VITE_PY_SERVER_URL || 'http://localhost:8003/detect';
  const players = ref([]);
  const editSelectedPlayer = ref(null);
  const editPyChanged = computed(() => {
    const base = editForm.value.pyServerUrl || '';
    const cur = currentEditTower?.value?.pyServerUrl || pyServerUrl || '';
    return base.trim() !== cur.trim();
  });
  const currentEditTower = computed(() =>
    towers.value.find(t => t.id === editForm.value.id)
  );

  function getAbilityModifier(player, abilityKey) {
    const score = player.stats?.[abilityKey]?.score;
    if (!score) return null;
    return Math.floor((score - 10) / 2);
  }

  function isSaveProficient(player, abilityKey) {
    return (
      player.saves?.[abilityKey]?.isProf === true ||
      player.saves?.[abilityKey]?.isProf === 1
    );
  }

  function getSaveModifier(player, abilityKey) {
    const saveData = player.saves?.[abilityKey];
    if (
      saveData &&
      saveData.customModifier !== undefined &&
      saveData.customModifier !== null
    ) {
      return saveData.customModifier;
    }
    const baseMod = getAbilityModifier(player, abilityKey) ?? 0;
    const profBonus = player.profBonus ?? player.proficiency ?? 0;
    if (isSaveProficient(player, abilityKey)) {
      return baseMod + profBonus;
    }
    return baseMod;
  }

  function isSkillProficient(skill) {
    const v = skill?.isProf;
    return v === true || v === 1 || v === 2 || v === '1' || v === '2';
  }

  function getSkillModifier(player, skill) {
    const baseStat = skill?.baseStat;
    if (!baseStat) return null;
    const baseMod = getAbilityModifier(player, baseStat) ?? 0;
    const profBonus = player.profBonus ?? player.proficiency ?? 0;

    // Custom modifier always wins
    if (skill?.customModifier !== undefined && skill?.customModifier !== null) {
      return skill.customModifier;
    }

    let totalMod = baseMod;
    const v = skill?.isProf;
    if (v === 2 || v === '2') {
      // Expertise: double proficiency
      totalMod += 2 * profBonus;
    } else if (v === 1 || v === '1' || v === true) {
      // Normal proficiency
      totalMod += profBonus;
    }
    return totalMod;
  }

  function getSkillsForAbility(player, abilityKey) {
    if (!player.skills) return {};
    const result = {};
    for (const [skillKey, skill] of Object.entries(player.skills)) {
      if (skill?.baseStat === abilityKey) {
        result[skillKey] = skill;
      }
    }
    return result;
  }

  function getPlayerStatsForTower(tower) {
    if (!tower?.playerPath) return null;
    const entry = players.value.find(p => p.value === tower.playerPath);
    const player = entry?.raw;
    if (!player) return null;

    return abilityOrder.map(key => {
      const label = abilityNames[key] || key.toUpperCase();
      const score = player.stats?.[key]?.score ?? null;
      const mod = getAbilityModifier(player, key);
      const save = getSaveModifier(player, key);
      const saveData = player.saves?.[key];
      const saveCustom =
        saveData &&
        saveData.customModifier !== undefined &&
        saveData.customModifier !== null;
      const skillsObj = getSkillsForAbility(player, key);
      const skills = Object.entries(skillsObj).map(([skillKey, skill]) => {
        const mod = getSkillModifier(player, skill);
        const rawProf = skill.isProf;
        const prof = isSkillProficient(skill);
        const isExpert = rawProf === 2 || rawProf === '2';
        const isCustom =
          skill.customModifier !== undefined && skill.customModifier !== null;
        return {
          name: skillNames[skillKey] || skillKey,
          mod,
          isProf: prof,
          isExpert,
          isCustom,
        };
      });
      return {
        key,
        label,
        score,
        mod,
        save,
        saveProf: isSaveProficient(player, key),
        saveCustom,
        skills,
      };
    });
  }

  function getPlayerProfBonusForTower(tower) {
    if (!tower?.playerPath) return null;
    const entry = players.value.find(p => p.value === tower.playerPath);
    if (!entry) return null;
    // Prefer normalized profBonus stored on entry; fall back to raw data if present.
    if (entry.profBonus !== undefined && entry.profBonus !== null) {
      return entry.profBonus;
    }
    const raw = entry.raw || {};
    return raw.proficiency ?? null;
  }

  onMounted(async () => {
    try {
      const mods = import.meta.glob('/src/data/players/**/*.json', {
        eager: true,
      });
      players.value = Object.entries(mods).map(([path, mod], idx) => {
        const data = mod.default || mod;
        const parsed =
          typeof data.data === 'string'
            ? JSON.parse(data.data)
            : data.data || data;
        const name =
          parsed?.name?.value ||
          parsed?.info?.playerName?.value ||
          parsed?.info?.name?.value ||
          `Player ${idx + 1}`;
        return {
          value: path,
          title: name,
          raw: parsed,
          profBonus: parsed?.proficiency ?? null,
        };
      });
    } catch (e) {
      console.error('load players failed', e);
    }
  });

  watch(
    editSelectedPlayer,
    val => {
      if (!val) return;
      editForm.value.playerName =
        val.title ||
        val.raw?.name?.value ||
        val.raw?.info?.playerName?.value ||
        val.raw?.info?.name?.value ||
        '';
    },
    { immediate: false }
  );

  async function pingStatus(tower) {
    if (!tower?.apiBase) return;
    loading.value[`${tower.id}-status`] = true;
    try {
      const res = await fetch(`${tower.apiBase}/status`, { cache: 'no-store' });
      const ok = res.ok;
      let data = null;
      if (ok) data = await res.json();
      towerStore.setStatus(tower.id, {
        online: ok,
        dice: data?.dice,
        raw: data,
      });
    } catch (e) {
      towerStore.setStatus(tower.id, { online: false });
      console.error('status error', e);
    } finally {
      loading.value[`${tower.id}-status`] = false;
    }
  }

  async function captureOnce(tower) {
    if (!tower?.apiBase) return;
    loading.value[`${tower.id}-capture`] = true;
    try {
      const res = await fetch(`${tower.apiBase}/dice/capture`, {
        cache: 'no-store',
      });
      if (res.ok) {
        const data = await res.json();
        towerStore.setStatus(tower.id, {
          online: true,
          dice: data,
        });
      }
    } catch (e) {
      console.error('capture error', e);
    } finally {
      loading.value[`${tower.id}-capture`] = false;
    }
  }

  function refreshStream(tower) {
    if (!tower?.streamUrl || !tower?.id) return;
    if (!streamEnabled.value[tower.id]) return;
    streamSrc.value[tower.id] = `${tower.streamUrl}?t=${Date.now()}`;
  }

  function onStreamLoad(tower, ev) {
    if (!tower?.id || !ev?.target) return;
    streamSizes.value[tower.id] = {
      w: ev.target.clientWidth || ev.target.naturalWidth || 0,
      h: ev.target.clientHeight || ev.target.naturalHeight || 0,
    };
  }

  function remove(id) {
    towerStore.removeTower(id);
  }

  async function stopStream(id) {
    if (!id && id !== 0) return;
    const tower = towers.value.find(t => t.id === id);
    if (tower?.apiBase) {
      // Call ESP32 stop endpoint to force-close the stream
      try {
        await fetch(`${tower.apiBase}/camera/stream/stop`, {
          method: 'POST',
          cache: 'no-store',
        });
      } catch (e) {
        // Ignore errors - we'll still clear the frontend state
      }
    }
    // Clear any pending auto-clear timer
    if (detectAutoClearTimers.value[id]) {
      clearTimeout(detectAutoClearTimers.value[id]);
      delete detectAutoClearTimers.value[id];
    }
    // Clear Python results immediately so overlay disappears right away
    const pyCopy = { ...pythonResults.value };
    delete pyCopy[id];
    pythonResults.value = pyCopy;
    // Clear src to close MJPEG quickly
    streamSrc.value[id] = '';
    setTimeout(() => {
      const copy = { ...streamSrc.value };
      delete copy[id];
      streamSrc.value = copy;
      const sizesCopy = { ...streamSizes.value };
      delete sizesCopy[id];
      streamSizes.value = sizesCopy;
    }, 150);
  }

  function onStreamToggle(tower) {
    if (!tower?.id) return;
    if (streamEnabled.value[tower.id]) {
      refreshStream(tower);
    } else {
      stopStream(tower.id);
    }
  }

  function toggleDetails(tower) {
    if (!tower?.id) return;
    const next = !uiStore.towerDetailsExpanded[tower.id];
    uiStore.setTowerDetailsExpanded(tower.id, next);
    if (next) {
      // Opening: enable stream and refresh
      streamEnabled.value[tower.id] = true;
      refreshStream(tower);
      uiStore.setTowerStreamActive(tower.id, true);
    } else {
      // Closing: turn off stream and stop on ESP
      streamEnabled.value[tower.id] = false;
      stopStream(tower.id);
      uiStore.setTowerStreamActive(tower.id, false);
    }
  }

  async function callNode(tower, path) {
    if (!tower?.nodemcuApiBase) return;
    try {
      await fetch(`${tower.nodemcuApiBase}${path}`, { cache: 'no-store' });
    } catch (e) {
      console.error('node call error', e);
    }
  }

  async function whoamiEsp(tower) {
    if (!tower?.apiBase) return;
    try {
      const res = await fetch(`${tower.apiBase}/whoami`, { cache: 'no-store' });
      if (!res.ok) return;
      const data = await res.json();
      towerStore.setStatus(tower.id, {
        online: true,
        raw: data,
      });
    } catch (e) {
      console.error('whoami esp error', e);
    }
  }

  async function whoamiNode(tower) {
    if (!tower?.nodemcuApiBase) return;
    try {
      const res = await fetch(`${tower.nodemcuApiBase}/whoami`, {
        cache: 'no-store',
      });
      if (!res.ok) return;
      const data = await res.json();
      towerStore.setStatus(tower.id, {
        online: true,
        raw: data,
      });
    } catch (e) {
      console.error('whoami node error', e);
    }
  }

  async function pingNodeStatus(tower) {
    editNodeStatusText.value = '';
    const base = (tower?.nodemcuApiBase || editForm.value.nodemcuApiBase || '')
      .trim()
      .replace(/\/+$/, '');
    if (!base) {
      editNodeStatusText.value =
        'Enter NodeMCU API for this tower, e.g. http://192.168.110.54';
      console.warn('[TowerGrid] pingNodeStatus: missing nodemcuApiBase');
      return;
    }
    try {
      const res = await fetch(`${base}/status`, { cache: 'no-store' });
      const text = await res.text();
      editNodeStatusText.value = `Node status → ${res.status}: ${text}`;
      console.log('[TowerGrid] Node status', {
        base,
        status: res.status,
        text,
      });
    } catch (e) {
      editNodeStatusText.value = `Node status error: ${e}`;
      console.error('[TowerGrid] node status error', e);
    }
  }

  async function provisionNodeForEdit(tower) {
    editNodeStatusText.value = '';
    // Use AP URL for provisioning (NodeMCU is in AP mode before connecting to Wi-Fi)
    const apUrl = editNodeApUrl.value.trim().replace(/\/+$/, '');
    if (!apUrl) {
      editNodeStatusText.value =
        'Enter NodeMCU AP URL for provisioning, e.g. http://192.168.4.1';
      return;
    }
    if (!editNodeSsid.value) {
      editNodeStatusText.value = 'Enter Wi‑Fi SSID for NodeMCU';
      return;
    }
    try {
      const url = `${apUrl}/provision?ssid=${encodeURIComponent(
        editNodeSsid.value
      )}&pass=${encodeURIComponent(editNodePass.value || '')}`;
      const res = await fetch(url, { cache: 'no-store' });
      const text = await res.text();
      editNodeStatusText.value = `Node provision → ${res.status}: ${text}`;
      console.log('[TowerGrid] Node provision', {
        apUrl,
        status: res.status,
        text,
      });
    } catch (e) {
      editNodeStatusText.value = `Node provision error: ${e}`;
      console.error('[TowerGrid] node provision error', e);
    }
  }

  async function detectViaPython(tower) {
    const detectUrl = (tower?.pyServerUrl || pyServerUrl || '')
      .toString()
      .trim();
    if (!detectUrl) return;
    const snapUrl = tower?.apiBase ? `${tower.apiBase}/camera/snapshot` : '';
    if (!snapUrl) return;
    const id = tower.id;

    // Clear any pending auto-clear timer for this tower
    if (detectAutoClearTimers.value[id]) {
      clearTimeout(detectAutoClearTimers.value[id]);
      delete detectAutoClearTimers.value[id];
    }

    // Clear previous one-shot value
    oneShotValues.value = { ...oneShotValues.value, [id]: null };

    loading.value[`${id}-py`] = true;
    try {
      const snapRes = await fetch(snapUrl, { cache: 'no-store' });
      if (!snapRes.ok) {
        pythonResults.value[id] = {
          error: `Snapshot failed ${snapRes.status}`,
        };
        return;
      }
      const blob = await snapRes.blob();
      const fd = new FormData();
      fd.append('file', new File([blob], 'frame.jpg', { type: 'image/jpeg' }));
      const res = await fetch(detectUrl, { method: 'POST', body: fd });
      if (!res.ok) {
        pythonResults.value[id] = {
          error: `Detect failed ${res.status}`,
        };
        return;
      }
      const data = await res.json();
      pythonResults.value[id] = normalizePythonResult(data);

      // Extract best value for one-shot display
      const det = getPyTopDet(pythonResults.value[id]);
      const rawVal = det?.value ?? det?.class;
      const num = Number(rawVal);
      if (Number.isFinite(num)) {
        oneShotValues.value = { ...oneShotValues.value, [id]: num };
      }

      // Schedule auto-clear after 3s if enabled
      if (uiStore.resultsAutoClear[id] !== false) {
        detectAutoClearTimers.value[id] = setTimeout(() => {
          // Clear python result (bbox + label)
          const pyCopy = { ...pythonResults.value };
          delete pyCopy[id];
          pythonResults.value = pyCopy;
          // Clear one-shot value
          const osCopy = { ...oneShotValues.value };
          delete osCopy[id];
          oneShotValues.value = osCopy;
          // Bump reset key so TowerDetails clears its state
          const rk = { ...rollResetKey.value };
          rk[id] = (rk[id] || 0) + 1;
          rollResetKey.value = rk;
          // Clean up timer ref
          delete detectAutoClearTimers.value[id];
        }, 3000);
      }
    } catch (e) {
      pythonResults.value[id] = { error: String(e) };
    } finally {
      loading.value[`${id}-py`] = false;
    }
  }

  onMounted(() => {
    towers.value.forEach(t => {
      pingStatus(t);
      // Restore stream for towers that were previously expanded
      if (uiStore.towerDetailsExpanded[t.id]) {
        // Only start stream if not already active (avoid double-open)
        if (!uiStore.towerStreamActive[t.id]) {
          streamEnabled.value[t.id] = true;
          refreshStream(t);
          uiStore.setTowerStreamActive(t.id, true);
        }
      }
    });
  });

  onBeforeUnmount(async () => {
    // Stop all active streams when leaving the tab
    for (const t of towers.value) {
      if (uiStore.towerStreamActive[t.id]) {
        await stopStream(t.id);
        uiStore.setTowerStreamActive(t.id, false);
      }
    }
  });

  function openEdit(tower) {
    if (!tower) return;
    editForm.value = {
      id: tower.id,
      playerName: tower.playerName || '',
      towerName: tower.towerName || '',
      apiBase: tower.apiBase || '',
      streamUrl: tower.streamUrl || '',
      nodemcuApiBase: tower.nodemcuApiBase || '',
      pyServerUrl:
        tower.pyServerUrl || pyServerUrl || 'http://localhost:8003/detect',
      playerPath: tower.playerPath || '',
    };
    editSelectedPlayer.value = null;
    editPyStatus.value = '';
    editDialog.value = true;
  }

  function saveEdit() {
    if (!editForm.value.id) {
      editDialog.value = false;
      return;
    }
    // Remember previous tower state before updating, to detect real changes
    const prevTower = currentEditTower.value;
    const prevStreamUrl = prevTower?.streamUrl || '';
    const selectedName =
      editSelectedPlayer.value?.title ||
      editSelectedPlayer.value?.raw?.name?.value ||
      editSelectedPlayer.value?.raw?.info?.playerName?.value ||
      editSelectedPlayer.value?.raw?.info?.name?.value ||
      '';
    towerStore.addOrUpdateTower({
      id: editForm.value.id,
      playerName: editForm.value.playerName || selectedName,
      playerPath:
        editSelectedPlayer.value?.value || editForm.value.playerPath || '',
      towerName: editForm.value.towerName,
      apiBase: editForm.value.apiBase,
      streamUrl: editForm.value.streamUrl,
      nodemcuApiBase: editForm.value.nodemcuApiBase,
      pyServerUrl: editForm.value.pyServerUrl,
    });
    // Only touch the stream if the URL actually changed; this avoids
    // reopening the MJPEG stream when we just change the player.
    const newStreamUrl = editForm.value.streamUrl || '';
    const streamChanged = newStreamUrl && newStreamUrl !== prevStreamUrl;
    if (uiStore.towerDetailsExpanded[editForm.value.id] && streamChanged) {
      streamSrc.value[editForm.value.id] = newStreamUrl;
    }
    editDialog.value = false;
  }

  function saveEditPyAddress() {
    if (!editForm.value.id) return;
    const url = (editForm.value.pyServerUrl || '').trim();
    if (!url) return;
    towerStore.addOrUpdateTower({
      id: editForm.value.id,
      pyServerUrl: url,
    });
    editPyStatus.value = 'Saved new address';
  }

  function normalizePythonResult(data) {
    // If /detect/best returned { result: {...} } convert to detected_numbers array
    if (data && data.result && !data.detected_numbers) {
      data.detected_numbers = [data.result];
    }
    return data;
  }

  function getPyTopDet(res) {
    if (!res) return null;
    if (res.detected_numbers && res.detected_numbers.length) {
      return res.detected_numbers[0];
    }
    if (res.result) return res.result;
    return null;
  }

  // Get all detected dice values as numbers (sorted by confidence desc)
  function getAllDiceValues(res) {
    if (!res) return [];
    const nums = res.detected_numbers || (res.result ? [res.result] : []);
    return nums
      .map(d => {
        const raw = d?.value ?? d?.class;
        const num = Number(raw);
        return Number.isFinite(num)
          ? { value: num, confidence: d?.confidence ?? 0, det: d }
          : null;
      })
      .filter(Boolean)
      .sort((a, b) => b.confidence - a.confidence);
  }

  // Get the two highest-confidence dice for advantage/disadvantage
  function getAdvDisDice(res) {
    const all = getAllDiceValues(res);
    if (all.length < 2) return null;
    // Return [die1, die2] sorted by confidence
    return [all[0], all[1]];
  }

  function getPyBox(tower) {
    const res = pythonResults.value[tower.id];
    const det = getPyTopDet(res);
    if (!det || !det.bbox) return null;
    const imgSize = res?.image_size ||
      det.image_size || {
        width: 320,
        height: 240,
      };
    if (!imgSize.width || !imgSize.height) {
      imgSize.width = Math.max(det.bbox.x2 || 0, imgSize.width || 0, 320);
      imgSize.height = Math.max(det.bbox.y2 || 0, imgSize.height || 0, 240);
    }
    const disp = streamSizes.value[tower.id];
    if (!imgSize || !imgSize.width || !imgSize.height || !disp) return null;
    const scaleX = disp.w / imgSize.width;
    const scaleY = disp.h / imgSize.height;
    return {
      left: `${det.bbox.x1 * scaleX}px`,
      top: `${det.bbox.y1 * scaleY}px`,
      width: `${det.bbox.width * scaleX}px`,
      height: `${det.bbox.height * scaleY}px`,
    };
  }

  // Get bounding boxes for dice detection (max 2, highest confidence)
  function getAllPyBoxes(tower) {
    const res = pythonResults.value[tower.id];
    if (!res) return [];
    const detections = res.detected_numbers || (res.result ? [res.result] : []);
    if (!detections.length) return [];

    const imgSize = res?.image_size || { width: 320, height: 240 };
    const disp = streamSizes.value[tower.id];
    if (!disp) return [];

    const scaleX = disp.w / imgSize.width;
    const scaleY = disp.h / imgSize.height;

    // Sort by confidence and take only top 2 (we never need more than 2 dice)
    const topTwo = [...detections]
      .filter(det => det?.bbox)
      .sort((a, b) => (b.confidence || 0) - (a.confidence || 0))
      .slice(0, 2);

    return topTwo.map(det => ({
      style: {
        left: `${det.bbox.x1 * scaleX}px`,
        top: `${det.bbox.y1 * scaleY}px`,
        width: `${det.bbox.width * scaleX}px`,
        height: `${det.bbox.height * scaleY}px`,
      },
      label: det.class,
      confidence: det.confidence,
    }));
  }

  function clearRollSession(id) {
    const sessions = { ...rollSessions.value };
    const session = sessions[id];
    if (session?.timerId) {
      clearTimeout(session.timerId);
    }
    delete sessions[id];
    rollSessions.value = sessions;
  }

  function stopRoll(tower) {
    if (!tower?.id) return;
    const id = tower.id;
    clearRollSession(id);
    // Clear any pending auto-clear timer from pure detect
    if (detectAutoClearTimers.value[id]) {
      clearTimeout(detectAutoClearTimers.value[id]);
      delete detectAutoClearTimers.value[id];
    }
    // Clear last Python result so bbox and label disappear when user cancels roll
    const pyCopy = { ...pythonResults.value };
    delete pyCopy[id];
    pythonResults.value = pyCopy;
    // Also clear player screen when DM manually stops
    rollBroadcast.clearRoll();
    // Turn off LED when roll stops
    if (tower?.nodemcuApiBase) {
      callNode(tower, '/strip/off');
    }
  }

  async function runRollLoop(tower) {
    if (!tower?.id) return;
    const id = tower.id;
    const session = rollSessions.value[id];
    if (!session || !session.active) return;

    const detectUrl = (tower?.pyServerUrl || pyServerUrl || '')
      .toString()
      .trim();
    const snapUrl = tower?.apiBase ? `${tower.apiBase}/camera/snapshot` : '';
    if (!detectUrl || !snapUrl) {
      clearRollSession(id);
      return;
    }

    const isAdvDis =
      session.mode === 'advantage' || session.mode === 'disadvantage';

    try {
      const snapRes = await fetch(snapUrl, { cache: 'no-store' });
      if (!snapRes.ok) {
        clearRollSession(id);
        return;
      }
      const blob = await snapRes.blob();
      const fd = new FormData();
      fd.append('file', new File([blob], 'frame.jpg', { type: 'image/jpeg' }));
      const res = await fetch(detectUrl, { method: 'POST', body: fd });
      if (!res.ok) {
        clearRollSession(id);
        return;
      }
      const data = await res.json();
      pythonResults.value[id] = normalizePythonResult(data);

      const now = Date.now();

      if (isAdvDis) {
        // Advantage/Disadvantage: need 2 stable dice
        const twoDice = getAdvDisDice(pythonResults.value[id]);
        if (twoDice && twoDice.length === 2) {
          const vals = [twoDice[0].value, twoDice[1].value].sort(
            (a, b) => a - b
          );
          const stableKey = vals.join(','); // e.g. "2,10"

          let updated = { ...session };
          if (updated.stableKey === stableKey) {
            if (!updated.stableSince) {
              updated.stableSince = now;
            } else if (now - updated.stableSince >= 600) {
              // Both dice stable for 600ms → finalize
              updated.active = false;
              updated.diceValues = vals; // [low, high]
              rollSessions.value = { ...rollSessions.value, [id]: updated };

              // Broadcast result to player screen
              const chosenValue =
                session.mode === 'advantage' ? vals[1] : vals[0];
              const total = chosenValue + (session.modifier || 0);

              // Get difficulty class value if enabled
              const isDcEnabled = uiStore.difficultyClassEnabled[id] === true;
              const dcValueRaw = uiStore.difficultyClassValue[id];
              let dcValue = null;

              if (isDcEnabled && dcValueRaw != null) {
                const numValue = Number(dcValueRaw);
                if (!isNaN(numValue) && numValue >= 1 && numValue <= 30) {
                  dcValue = numValue;
                }
              }

              rollBroadcast.showResult({
                mode: session.mode,
                playerName: session.playerName,
                label: session.label,
                dice: vals,
                chosenValue,
                modifier: session.modifier || 0,
                total,
                isNat1: chosenValue === 1,
                isNat20: chosenValue === 20,
                difficultyClass: dcValue,
              });

              // Blink 3 times, then turn on solid
              if (tower?.nodemcuApiBase) {
                callNode(tower, '/strip/blink').then(() => {
                  callNode(tower, '/strip/solid');
                });
              }

              scheduleAutoClear(id);
              return;
            }
          } else {
            updated.stableKey = stableKey;
            updated.stableSince = now;
          }
          rollSessions.value = { ...rollSessions.value, [id]: updated };
        }
      } else {
        // Normal roll: single die
        const det = getPyTopDet(pythonResults.value[id]);
        const rawVal = det?.value ?? det?.class;
        const num = Number(rawVal);

        if (Number.isFinite(num)) {
          let updated = { ...session };
          if (updated.stableValue === num) {
            if (!updated.stableSince) {
              updated.stableSince = now;
            } else if (now - updated.stableSince >= 600) {
              // Stable for at least 600ms → finalize
              updated.active = false;
              updated.diceValues = [num];
              rollSessions.value = { ...rollSessions.value, [id]: updated };

              // Broadcast result to player screen
              const total = num + (session.modifier || 0);
              rollBroadcast.showResult({
                mode: 'normal',
                playerName: session.playerName,
                label: session.label,
                value: num,
                chosenValue: num,
                modifier: session.modifier || 0,
                total,
                isNat1: num === 1,
                isNat20: num === 20,
              });

              // Blink 3 times, then turn on solid
              if (tower?.nodemcuApiBase) {
                callNode(tower, '/strip/blink').then(() => {
                  callNode(tower, '/strip/solid');
                });
              }

              scheduleAutoClear(id);
              return;
            }
          } else {
            updated.stableValue = num;
            updated.stableSince = now;
          }
          rollSessions.value = { ...rollSessions.value, [id]: updated };
        }
      }
    } catch (e) {
      console.error('roll loop error', e);
      clearRollSession(id);
      return;
    }

    // Schedule next poll if still active
    const nextSession = rollSessions.value[id];
    if (nextSession && nextSession.active) {
      const timerId = setTimeout(() => runRollLoop(tower), 400);
      rollSessions.value = {
        ...rollSessions.value,
        [id]: { ...nextSession, timerId },
      };
    }
  }

  function scheduleAutoClear(id) {
    if (uiStore.resultsAutoClear[id] !== false) {
      setTimeout(() => {
        const towerCurrent = towers.value.find(t => t.id === id);
        if (towerCurrent) {
          stopRoll(towerCurrent);
          const rk = { ...rollResetKey.value };
          rk[id] = (rk[id] || 0) + 1;
          rollResetKey.value = rk;
        }
        // Clear player screen at the same time (stopRoll now handles this)
      }, 3000);
    }
  }

  function startRoll(tower, payload) {
    if (!tower?.id) return;
    const id = tower.id;
    clearRollSession(id);
    // Clear one-shot value when starting a roll
    const osCopy = { ...oneShotValues.value };
    delete osCopy[id];
    oneShotValues.value = osCopy;

    // Get player name for broadcast
    const playerData = players.value.find(p => p.value === tower.playerPath);
    const playerName = playerData?.title || tower.playerName || '';

    // Build label for broadcast
    const label =
      payload?.skillName === '__ability__'
        ? `${payload?.abilityLabel || ''} проверка`
        : payload?.skillName || `${payload?.abilityLabel || ''} Спасбросок`;

    const modifier = payload?.modifier ?? 0;

    const session = {
      mode: payload?.mode || 'normal',
      abilityKey: payload?.abilityKey || null,
      abilityLabel: payload?.abilityLabel || '',
      skillName: payload?.skillName || null,
      playerName,
      label,
      modifier,
      startedAt: Date.now(),
      stableValue: null,
      stableSince: null,
      active: true,
      timerId: null,
    };
    rollSessions.value = { ...rollSessions.value, [id]: session };

    // Broadcast rolling state to player screen
    const pyUrl = tower?.pyServerUrl || pyServerUrl || '';
    rollBroadcast.setServerUrl(pyUrl);

    // Get difficulty class value if enabled
    const isDcEnabled = uiStore.difficultyClassEnabled[tower.id] === true;
    const dcValueRaw = uiStore.difficultyClassValue[tower.id];
    let dcValue = null;

    if (isDcEnabled && dcValueRaw != null) {
      const numValue = Number(dcValueRaw);
      if (!isNaN(numValue) && numValue >= 1 && numValue <= 30) {
        dcValue = numValue;
      }
    }

    console.log('[TowerGrid] Broadcasting startRolling:', {
      mode: session.mode,
      playerName,
      label,
      modifier,
      difficultyClass: dcValue,
      isDcEnabled,
      dcValueRaw,
      towerId: tower.id,
      server: pyUrl,
    });

    rollBroadcast.startRolling({
      mode: session.mode,
      playerName,
      label,
      modifier,
      difficultyClass: dcValue,
    });

    // Start LED in breathe mode (GLOW)
    if (tower?.nodemcuApiBase) {
      callNode(tower, '/strip/mode/glow').then(() => {
        callNode(tower, '/strip/on');
      });
    }

    runRollLoop(tower);
  }

  async function pyHealthEdit() {
    editPyStatus.value = '';
    const url = (editForm.value.pyServerUrl || pyServerUrl || '')
      .toString()
      .trim()
      .replace(/\/+$/, '');
    if (!url) {
      editPyStatus.value = 'Enter Python server URL';
      return;
    }
    const healthUrl =
      url.endsWith('/health') ||
      url.endsWith('/detect') ||
      url.endsWith('/detect/best')
        ? url.replace(/\/detect(?:\/best)?$/, '/health')
        : `${url}/health`;
    try {
      const res = await fetch(healthUrl, { cache: 'no-store' });
      const text = await res.text();
      editPyStatus.value = `Health → ${res.status}: ${text}`;
    } catch (e) {
      editPyStatus.value = `Health error: ${e}`;
    }
  }

  async function pyTestDetectEdit() {
    editPyStatus.value = '';
    const detectUrl = (editForm.value.pyServerUrl || pyServerUrl || '')
      .toString()
      .trim()
      .replace(/\/+$/, '');
    if (!detectUrl) {
      editPyStatus.value = 'Enter Python server URL';
      return;
    }
    const snapBase = (editForm.value.apiBase || '')
      .toString()
      .trim()
      .replace(/\/+$/, '');
    if (!snapBase) {
      editPyStatus.value = 'Enter ESP32-CAM API';
      return;
    }
    const loadKey = `${editForm.value.id || 'edit'}-py`;
    loading.value[loadKey] = true;
    try {
      const snapRes = await fetch(`${snapBase}/camera/snapshot`, {
        cache: 'no-store',
      });
      if (!snapRes.ok) {
        editPyStatus.value = `Snapshot failed ${snapRes.status}`;
        return;
      }
      const blob = await snapRes.blob();
      const fd = new FormData();
      fd.append('file', new File([blob], 'frame.jpg', { type: 'image/jpeg' }));
      const res = await fetch(detectUrl, { method: 'POST', body: fd });
      const text = await res.text();
      editPyStatus.value = `Detect → ${res.status}: ${text}`;
    } catch (e) {
      editPyStatus.value = `Detect error: ${e}`;
    } finally {
      loading.value[loadKey] = false;
    }
  }
</script>

<style scoped>
  .stream-wrapper {
    position: relative;
    display: inline-block;
    width: auto;
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
  .gap-2 {
    gap: 8px;
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
    transform: translate(
      -50%,
      calc(-100% - 1px)
    ); /* center and lift above the box */
  }

  .stream-and-stats {
    gap: 8px;
  }

  .stream-column {
    flex: 0 0 auto;
  }

  .stats-column {
    flex: 1 1 0;
    min-width: 260px;
    padding: 0;
  }

  .abilities-grid {
    display: grid;
    grid-template-columns: repeat(3, minmax(140px, 1fr));
    gap: 10px;
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

  .skill-modifier {
    min-width: 28px;
    text-align: right;
    font-weight: 500;
  }

  .skill-name {
    flex: 1;
    min-width: 0;
  }

  .proficiency-checkbox {
    width: 16px;
    height: 16px;
    border: 1px solid rgba(var(--v-theme-on-surface), 0.4);
    display: inline-flex;
    align-items: center;
    justify-content: center;
    font-size: 0.7rem;
    border-radius: 50%;
    flex-shrink: 0;
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

  .results-auto-clear-switch {
    transform: scale(0.8);
    transform-origin: right center;
  }
</style>
