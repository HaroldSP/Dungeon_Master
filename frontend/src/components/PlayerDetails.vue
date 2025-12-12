<template>
  <div class="player-details mt-3">
    <!-- Basics and Vitals (first row) -->
    <v-row
      dense
      class="mb-2"
    >
      <v-col
        cols="12"
        md="6"
      >
        <div class="ability-box">
          <div class="ability-header">
            <div class="ability-name">ОСНОВНОЕ</div>
          </div>
          <div class="info-section">
            <div class="info-row">
              <span class="info-label">Класс/Подкласс:</span>
              <span class="info-value"
                >{{ player.class || '—'
                }}<span v-if="player.subclass">
                  / {{ player.subclass }}</span
                ></span
              >
            </div>
            <div class="info-row">
              <span class="info-label">Уровень:</span>
              <span class="info-value">{{ player.level ?? '—' }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">Раса:</span>
              <span class="info-value">{{ player.race || '—' }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">Предыстория:</span>
              <span class="info-value">{{ player.background || '—' }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">Мировоззрение:</span>
              <span class="info-value">{{ player.alignment || '—' }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">Игрок:</span>
              <span class="info-value">{{ player.playerName || '—' }}</span>
            </div>
          </div>
        </div>
      </v-col>
      <v-col
        cols="12"
        md="6"
      >
        <div class="ability-box">
          <div class="ability-header">
            <div class="ability-name">ЖИЗНЕННЫЕ ПОКАЗАТЕЛИ</div>
          </div>
          <div class="info-section">
            <div class="info-row">
              <span class="info-label">ХП:</span>
              <span class="info-value">{{ player.hp ?? '—' }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">КД:</span>
              <span class="info-value">{{ player.ac ?? '—' }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">Скорость:</span>
              <span class="info-value">{{ player.speed ?? '—' }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">Кость хитов:</span>
              <span class="info-value">{{ player.hitDie || '—' }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">Мастерство:</span>
              <span class="info-value">+{{ player.profBonus ?? '—' }}</span>
            </div>
            <div class="info-row">
              <span class="info-label">Грузоподъёмность:</span>
              <span class="info-value">{{ getCarryingCapacity() }}</span>
            </div>
          </div>
        </div>
      </v-col>
    </v-row>

    <!-- Abilities with Skills and Saves (second row) -->
    <v-row
      dense
      class="mb-2"
    >
      <v-col cols="12">
        <div class="abilities-grid">
          <div
            v-for="abilityKey in abilityOrder"
            :key="abilityKey"
            class="ability-box"
          >
            <div class="ability-header">
              <div class="ability-name">{{ getAbilityName(abilityKey) }}</div>
            </div>

            <div class="ability-stats">
              <div class="modifier-circle">
                {{ formatModifier(getAbilityModifier(abilityKey)) }}
              </div>
              <div class="score-box">
                <div class="score-label">ЗНАЧЕНИЕ</div>
                <div class="score-value">
                  {{ player.stats?.[abilityKey]?.score ?? '—' }}
                </div>
              </div>
            </div>

            <div class="modifier-section">
              <div class="modifier-label">МОДИФИКАТОР</div>

              <!-- Saving Throw -->
              <div class="skill-row">
                <span class="skill-modifier">
                  {{ formatModifier(getSaveModifier(abilityKey)) }}
                </span>
                <span class="skill-name">Спасбросок</span>
                <span
                  class="proficiency-checkbox"
                  :class="{ checked: isSaveProficient(abilityKey) }"
                >
                  {{ isSaveProficient(abilityKey) ? '✓' : '○' }}
                </span>
              </div>

              <!-- Skills for this ability -->
              <div
                v-for="(skill, skillKey) in getSkillsForAbility(abilityKey)"
                :key="skillKey"
                class="skill-row"
              >
                <span class="skill-modifier">
                  {{ formatModifier(getSkillModifier(skillKey, skill)) }}
                </span>
                <span class="skill-name">{{
                  getSkillDisplayName(skillKey)
                }}</span>
                <span
                  class="proficiency-checkbox"
                  :class="{ checked: isSkillProficient(skill) }"
                >
                  {{ isSkillProficient(skill) ? '✓' : '○' }}
                </span>
              </div>
            </div>
          </div>
        </div>
      </v-col>
    </v-row>

    <!-- Spell Slots (full width below) -->
    <v-row
      dense
      class="mt-2"
    >
      <v-col cols="12">
        <div class="ability-box">
          <div class="ability-header">
            <div class="ability-name">ЯЧЕЙКИ ЗАКЛИНАНИЙ</div>
          </div>
          <div class="spell-slots-section">
            <template v-if="player.spells">
              <template v-for="(slot, key) in player.spells">
                <div
                  v-if="
                    key && typeof key === 'string' && key.startsWith('slots-')
                  "
                  :key="key"
                  class="spell-slot-row"
                >
                  <div class="spell-slot-label">
                    {{ key.replace('slots-', 'Уровень ') }}
                  </div>
                  <div class="spell-slot-dots">
                    <button
                      v-for="i in slot?.value ?? 0"
                      :key="i"
                      class="spell-slot-dot"
                      :class="{ used: i <= getUsedSlots(key) }"
                      @click="toggleSpellSlot(key, i)"
                    ></button>
                  </div>
                  <div class="spell-slot-count">
                    {{ getUsedSlots(key) }} / {{ slot?.value ?? 0 }}
                  </div>
                </div>
              </template>
            </template>
            <div
              v-else
              class="text-caption text-medium-emphasis"
            >
              Нет данных о ячейках заклинаний
            </div>
          </div>
        </div>
      </v-col>
    </v-row>

    <!-- Spells List (full width below) -->
    <v-row
      dense
      class="mt-2"
    >
      <v-col cols="12">
        <div class="ability-box">
          <div class="ability-header">
            <div class="ability-name">ЗАКЛИНАНИЯ</div>
          </div>
          <div class="spells-list-section">
            <template v-if="getPreparedSpells().length > 0">
              <div class="spells-table">
                <div class="spells-header">
                  <div class="spell-col-level">Ур.</div>
                  <div class="spell-col-name">Название</div>
                  <div class="spell-col-time">Время</div>
                  <div class="spell-col-range">Дистанция</div>
                  <div class="spell-col-components">Компоненты</div>
                  <div class="spell-col-indicators">
                    <span class="component-indicator-header">
                      <span class="component-diamond"></span>
                      <span class="component-letter">K</span>
                    </span>
                    <span class="component-indicator-header">
                      <span class="component-diamond"></span>
                      <span class="component-letter">P</span>
                    </span>
                    <span class="component-indicator-header">
                      <span class="component-diamond"></span>
                      <span class="component-letter">M</span>
                    </span>
                  </div>
                </div>
                <div
                  v-for="(spell, index) in getPreparedSpells()"
                  :key="spell.id || index"
                  class="spell-row"
                  :class="{ 'spell-row-unmapped': spell.needsMapping }"
                >
                  <div class="spell-col-level">
                    <span
                      v-if="spell.level !== null && spell.level !== undefined"
                      class="spell-level-badge"
                    >
                      {{ spell.level }}
                    </span>
                    <span
                      v-else
                      class="spell-info-value"
                    >
                      —
                    </span>
                  </div>
                  <div class="spell-col-name">
                    <span class="spell-name-text">
                      {{ spell.name }}
                    </span>
                    <span
                      v-if="spell.needsMapping"
                      class="spell-mapper-hint text-caption text-medium-emphasis"
                    >
                      (нужен маппер)
                    </span>
                  </div>
                  <div class="spell-col-time">
                    <span class="spell-info-value">
                      {{
                        spell.activation
                          ? formatCastingTime(spell.activation)
                          : '—'
                      }}
                    </span>
                  </div>
                  <div class="spell-col-range">
                    <span class="spell-info-value">
                      {{ spell.range ? formatRange(spell.range) : '—' }}
                    </span>
                  </div>
                  <div class="spell-col-components">
                    <span class="spell-info-value">
                      {{
                        spell.components
                          ? formatComponents(spell.components)
                          : '—'
                      }}
                    </span>
                  </div>
                  <div class="spell-col-indicators">
                    <span
                      class="component-indicator"
                      :class="{ filled: spell.components?.concentration }"
                      title="Концентрация"
                    >
                      <span class="component-diamond"></span>
                      <span class="component-letter">K</span>
                    </span>
                    <span
                      class="component-indicator"
                      :class="{ filled: spell.components?.ritual }"
                      title="Ритуал"
                    >
                      <span class="component-diamond"></span>
                      <span class="component-letter">P</span>
                    </span>
                    <span
                      class="component-indicator"
                      :class="{ filled: spell.components?.material }"
                      title="Материальный компонент"
                    >
                      <span class="component-diamond"></span>
                      <span class="component-letter">M</span>
                    </span>
                  </div>
                </div>
              </div>
            </template>
            <div
              v-else
              class="text-caption text-medium-emphasis"
            >
              Нет подготовленных заклинаний
            </div>
          </div>
        </div>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
  import { computed, ref, watch, onMounted } from 'vue';
  import spellsData from '../data/spells.json';
  import allSpellsData from '../data/allSpells.json';

  const props = defineProps({
    player: {
      type: Object,
      required: true,
    },
  });

  // Cache for fetched spells from API
  const fetchedSpellsCache = ref(new Map());

  // Track used spell slots (initialize from player data)
  const usedSlots = ref({});

  function initializeUsedSlots() {
    usedSlots.value = {};
    if (props.player.spells) {
      for (const [key, slot] of Object.entries(props.player.spells)) {
        if (key && typeof key === 'string' && key.startsWith('slots-')) {
          usedSlots.value[key] = slot?.filled ?? 0;
        }
      }
    }
  }

  // Initialize on mount and watch for player changes
  onMounted(() => {
    initializeUsedSlots();
  });

  watch(
    () => props.player.spells,
    () => {
      initializeUsedSlots();
    },
    { deep: true }
  );

  function toggleSpellSlot(slotKey, slotIndex) {
    if (!usedSlots.value[slotKey]) {
      usedSlots.value[slotKey] = 0;
    }

    const slot = props.player.spells?.[slotKey];
    const maxSlots = slot?.value ?? 0;

    if (slotIndex <= usedSlots.value[slotKey]) {
      // Unuse this slot and all slots after it
      usedSlots.value[slotKey] = slotIndex - 1;
    } else {
      // Use this slot and all slots before it
      usedSlots.value[slotKey] = Math.min(slotIndex, maxSlots);
    }

    // Update the player data if it exists
    if (slot) {
      slot.filled = usedSlots.value[slotKey];
    }
  }

  function getUsedSlots(slotKey) {
    return (
      usedSlots.value[slotKey] ?? props.player.spells?.[slotKey]?.filled ?? 0
    );
  }

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

  function getAbilityName(key) {
    return abilityNames[key] || key.toUpperCase();
  }

  function getSkillDisplayName(key) {
    return skillNames[key] || key;
  }

  function getAbilityModifier(abilityKey) {
    const score = props.player.stats?.[abilityKey]?.score;
    if (!score) return 0;
    return Math.floor((score - 10) / 2);
  }

  function formatModifier(mod) {
    if (mod === 0) return '+0';
    return mod > 0 ? `+${mod}` : `${mod}`;
  }

  function isSaveProficient(abilityKey) {
    return (
      props.player.saves?.[abilityKey]?.isProf === true ||
      props.player.saves?.[abilityKey]?.isProf === 1
    );
  }

  function getSaveModifier(abilityKey) {
    const baseMod = getAbilityModifier(abilityKey);
    if (isSaveProficient(abilityKey)) {
      return baseMod + (props.player.profBonus || 0);
    }
    return baseMod;
  }

  function isSkillProficient(skill) {
    return skill?.isProf === true || skill?.isProf === 1;
  }

  function getSkillModifier(skillKey, skill) {
    const baseStat = skill?.baseStat;
    if (!baseStat) return 0;

    const baseMod = getAbilityModifier(baseStat);
    let totalMod = baseMod;

    // Add proficiency if proficient
    if (isSkillProficient(skill)) {
      totalMod += props.player.profBonus || 0;
    }

    // Add custom modifier if present
    if (skill?.customModifier !== undefined && skill?.customModifier !== null) {
      totalMod = skill.customModifier;
    }

    return totalMod;
  }

  function getSkillsForAbility(abilityKey) {
    if (!props.player.skills) return {};
    const result = {};
    for (const [skillKey, skill] of Object.entries(props.player.skills)) {
      if (skill?.baseStat === abilityKey) {
        result[skillKey] = skill;
      }
    }
    return result;
  }

  function getCarryingCapacity() {
    const strScore = props.player.stats?.str?.score;
    if (!strScore) return '—';
    // D&D 5e: Carrying capacity = Strength score × 15 pounds
    // Push/drag/lift = Strength score × 30 pounds
    const capacity = strScore * 15;
    const dragCapacity = strScore * 30;
    return `${capacity}/${dragCapacity} фнт`;
  }

  /**
   * Attempts to fetch spell data from a D&D 5e API.
   * Note: The character JSON uses MongoDB ObjectIds which are internal to the character sheet system.
   * These don't match D&D API spell indices, so we'd need a mapping table or the character sheet's API.
   *
   * @param {string} spellId - The spell ID from the character JSON
   * @returns {Promise<Object|null>} Spell data or null if not found
   */
  async function fetchSpellFromAPI(spellId) {
    // Check cache first
    if (fetchedSpellsCache.value.has(spellId)) {
      return fetchedSpellsCache.value.get(spellId);
    }

    // TODO: If you know the character sheet system's API endpoint, add it here
    // For example, if it's from a specific service, you could do:
    // const response = await fetch(`https://api.example.com/spells/${spellId}`);

    // For now, we can't fetch because:
    // 1. MongoDB ObjectIds don't match D&D 5e API spell indices
    // 2. We don't have spell names to search by
    // 3. We'd need the character sheet system's API endpoint

    return null;
  }

  // Flatten allSpells.json (it's a nested array: [[...]])
  const allSpellsFlat = computed(() => {
    if (!Array.isArray(allSpellsData)) return [];
    // If it's a nested array, flatten it
    if (Array.isArray(allSpellsData[0])) {
      return allSpellsData.flat();
    }
    return allSpellsData;
  });

  function getPreparedSpells() {
    // Check if player has raw data
    if (!props.player.raw) return [];

    try {
      // The raw data might be a string or already parsed
      let rawData;
      if (typeof props.player.raw === 'string') {
        rawData = JSON.parse(props.player.raw);
      } else {
        rawData = props.player.raw;
      }

      // Access spells from the top level (not from data.spells which is slots)
      // The prepared spells are at the top level spells.prepared
      const topLevelSpells = rawData?.spells;
      if (!topLevelSpells || !topLevelSpells.prepared) return [];

      // Match prepared spell IDs with spells from databases
      return topLevelSpells.prepared
        .map(id => {
          // 1. First try allSpells.json (main database)
          const allSpellsSpell = allSpellsFlat.value.find(s => s._id === id);
          if (allSpellsSpell) {
            return {
              id,
              name: allSpellsSpell.name,
              level: allSpellsSpell.system?.level ?? null,
              school: allSpellsSpell.system?.school || null,
              activation: allSpellsSpell.system?.activation || null,
              range: allSpellsSpell.system?.range || null,
              components: allSpellsSpell.system?.components || null,
              materials: allSpellsSpell.system?.materials || null,
              duration: allSpellsSpell.system?.duration || null,
            };
          }

          // 2. Then try spells.json (custom spells)
          const spell = spellsData.find(s => s._id === id);
          if (spell) {
            return {
              id,
              name: spell.name,
              level: spell.system?.level ?? null,
              school: spell.system?.school || null,
              activation: spell.system?.activation || null,
              range: spell.system?.range || null,
              components: spell.system?.components || null,
              materials: spell.system?.materials || null,
              duration: spell.system?.duration || null,
            };
          }

          // 3. If not found locally, check cache
          const cachedSpell = fetchedSpellsCache.value.get(id);
          if (cachedSpell) {
            return {
              id,
              name: cachedSpell.name,
              level: cachedSpell.level ?? null,
              school: cachedSpell.school || null,
              activation: cachedSpell.activation || null,
              range: cachedSpell.range || null,
              components: cachedSpell.components || null,
              materials: cachedSpell.materials || null,
              duration: cachedSpell.duration || null,
            };
          }

          // 4. Fallback: show shortened ID with hint to use mapper
          return {
            id,
            name: `Неизвестное заклинание (ID: ${id.substring(0, 8)}...)`,
            level: null,
            school: null,
            needsMapping: true,
          };
        })
        .sort((a, b) => {
          // Sort by level first (null/undefined levels go to the end)
          const levelA = a.level ?? 999;
          const levelB = b.level ?? 999;
          if (levelA !== levelB) {
            return levelA - levelB;
          }
          // If same level, sort alphabetically by name
          return (a.name || '').localeCompare(b.name || '');
        });
    } catch (e) {
      console.error('Error parsing spells:', e);
      return [];
    }
  }

  function formatCastingTime(activation) {
    if (!activation) return '—';
    const type = activation.type || '';
    const cost = activation.cost || 1;

    const typeMap = {
      action: 'Действие',
      bonus: 'Бонусное действие',
      reaction: 'Реакция',
      special: 'Особое',
    };

    const typeName = typeMap[type] || type;
    return cost === 1 ? typeName : `${typeName} (${cost})`;
  }

  function formatRange(range) {
    if (!range) return '—';
    if (range.units === 'self') return 'На себя';
    if (range.units === 'touch') return 'Касание';
    if (range.units === 'sight') return 'В пределах видимости';
    if (range.value && range.units) {
      return `${range.value} ${range.units === 'ft' ? 'фт' : range.units}`;
    }
    return '—';
  }

  function formatComponents(components) {
    if (!components) return '—';
    const parts = [];
    if (components.vocal) parts.push('В');
    if (components.somatic) parts.push('С');
    if (components.material) parts.push('М');
    if (components.concentration) parts.push('К');
    if (components.ritual) parts.push('Р');
    return parts.length > 0 ? parts.join(', ') : '—';
  }

  function formatDuration(duration) {
    if (!duration) return '—';
    if (duration.units === 'inst') return 'Мгновенно';
    if (duration.units === 'spec') return 'Особое';
    if (duration.value && duration.units) {
      const unitMap = {
        round: 'раунд',
        minute: 'минута',
        hour: 'час',
        day: 'день',
      };
      const unit = unitMap[duration.units] || duration.units;
      const plural =
        duration.value > 1
          ? unit === 'минута'
            ? 'минуты'
            : unit === 'час'
            ? 'часа'
            : unit + 'ы'
          : unit;
      return `${duration.value} ${plural}`;
    }
    return '—';
  }
</script>

<style scoped>
  .player-details {
    margin-top: 12px;
    width: 100%;
    max-width: 100%;
    overflow: visible;
    box-sizing: border-box;
  }

  .abilities-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 12px;
    width: 100%;
    max-width: 100%;
    box-sizing: border-box;
  }

  .ability-box {
    border: 1px solid rgba(var(--v-border-color), var(--v-border-opacity));
    border-radius: 4px;
    padding: 12px;
    background: rgba(var(--v-theme-surface), 0.5);
    min-width: 0;
    box-sizing: border-box;
    overflow-wrap: break-word;
    word-wrap: break-word;
  }

  .ability-header {
    margin-bottom: 8px;
  }

  .ability-name {
    font-weight: bold;
    font-size: 0.875rem;
    text-align: center;
    margin-bottom: 8px;
  }

  .ability-stats {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;
    gap: 8px;
    min-width: 0;
  }

  .modifier-circle {
    width: 40px;
    height: 40px;
    border: 2px solid rgba(var(--v-theme-on-surface), 0.3);
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    font-weight: bold;
    font-size: 0.875rem;
    flex-shrink: 0;
  }

  .score-box {
    flex: 1;
    min-width: 0;
    border: 1px solid rgba(var(--v-theme-on-surface), 0.3);
    border-radius: 4px;
    padding: 6px;
    text-align: center;
    box-sizing: border-box;
  }

  .score-label {
    font-size: 0.65rem;
    opacity: 0.7;
    margin-bottom: 2px;
  }

  .score-value {
    font-weight: bold;
    font-size: 1rem;
  }

  .modifier-section {
    margin-top: 8px;
  }

  .modifier-label {
    font-size: 0.7rem;
    font-weight: 500;
    margin-bottom: 6px;
    display: block;
  }

  .skill-row {
    display: flex;
    align-items: center;
    gap: 6px;
    margin-bottom: 4px;
    font-size: 0.75rem;
    min-width: 0;
  }

  .skill-modifier {
    min-width: 32px;
    font-weight: 500;
    text-align: right;
  }

  .skill-name {
    flex: 1;
    min-width: 0;
    overflow-wrap: break-word;
    word-wrap: break-word;
  }

  .proficiency-checkbox {
    width: 16px;
    height: 16px;
    border: 1px solid rgba(var(--v-theme-on-surface), 0.5);
    border-radius: 50%;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    font-size: 0.7rem;
    flex-shrink: 0;
  }

  .proficiency-checkbox.checked {
    background: rgba(var(--v-theme-primary), 0.2);
    border-color: rgb(var(--v-theme-primary));
  }

  @media (max-width: 960px) {
    .abilities-grid {
      grid-template-columns: repeat(2, 1fr);
    }
  }

  .info-section {
    margin-top: 8px;
  }

  .info-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 6px;
    font-size: 0.75rem;
    min-width: 0;
  }

  .info-label {
    flex: 1;
    opacity: 0.7;
    text-align: left;
  }

  .info-value {
    font-weight: 500;
    text-align: right;
    min-width: 0;
    overflow-wrap: break-word;
    word-wrap: break-word;
  }

  .spell-slots-section {
    margin-top: 8px;
  }

  .spell-slot-row {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-bottom: 10px;
    font-size: 0.75rem;
  }

  .spell-slot-label {
    min-width: 80px;
    font-weight: 500;
  }

  .spell-slot-dots {
    display: flex;
    gap: 6px;
    flex: 1;
    flex-wrap: wrap;
  }

  .spell-slot-dot {
    width: 24px;
    height: 24px;
    border: 2px solid rgba(var(--v-theme-on-surface), 0.5);
    border-radius: 50%;
    background: transparent;
    cursor: pointer;
    padding: 0;
    flex-shrink: 0;
    transition: all 0.2s ease;
  }

  .spell-slot-dot:hover {
    border-color: rgba(var(--v-theme-primary), 0.7);
    transform: scale(1.1);
  }

  .spell-slot-dot.used {
    background: rgba(var(--v-theme-primary), 0.3);
    border-color: rgb(var(--v-theme-primary));
  }

  .spell-slot-count {
    min-width: 50px;
    text-align: right;
    font-weight: 500;
    font-size: 0.8rem;
  }

  .spells-list-section {
    margin-top: 8px;
  }

  .spells-table {
    display: flex;
    flex-direction: column;
    gap: 4px;
    width: 100%;
  }

  .spells-header {
    display: grid;
    grid-template-columns: 0.4fr 2fr 0.8fr 0.8fr 1fr 0.6fr;
    gap: 12px;
    padding: 8px 12px;
    background: rgba(var(--v-theme-surface), 0.5);
    border-bottom: 1px solid rgba(var(--v-theme-on-surface), 0.2);
    font-size: 0.75rem;
    font-weight: 600;
    color: rgba(var(--v-theme-on-surface), 0.8);
    align-items: center;
  }

  .spell-row {
    display: grid;
    grid-template-columns: 0.4fr 2fr 0.8fr 0.8fr 1fr 0.6fr;
    gap: 12px;
    padding: 10px 12px;
    border: 1px solid rgba(var(--v-theme-on-surface), 0.2);
    border-radius: 4px;
    background: rgba(var(--v-theme-surface), 0.3);
    transition: all 0.2s ease;
    align-items: center;
  }

  .spell-row:hover {
    background: rgba(var(--v-theme-surface), 0.5);
    border-color: rgba(var(--v-theme-primary), 0.5);
  }

  .spell-row-unmapped {
    border-color: rgba(var(--v-theme-warning), 0.5);
    background: rgba(var(--v-theme-warning), 0.1);
    opacity: 0.7;
  }

  .spell-col-level {
    font-size: 0.8rem;
    white-space: nowrap;
    text-align: center;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .spell-col-name {
    display: flex;
    align-items: center;
    gap: 8px;
    flex-wrap: wrap;
    min-width: 0;
  }

  .spell-col-time,
  .spell-col-range,
  .spell-col-components {
    font-size: 0.8rem;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .spell-col-indicators {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
  }

  .spell-name-text {
    font-size: 0.9rem;
    font-weight: 500;
    word-wrap: break-word;
    flex: 1;
    min-width: 0;
  }

  .spell-level-badge {
    font-size: 0.75rem;
    font-weight: 600;
    padding: 2px 6px;
    background: rgba(var(--v-theme-primary), 0.2);
    border-radius: 3px;
    flex-shrink: 0;
  }

  .spell-info-value {
    color: rgba(var(--v-theme-on-surface), 0.9);
    font-size: 0.8rem;
  }

  .component-indicator,
  .component-indicator-header {
    display: flex;
    align-items: center;
    gap: 4px;
    font-size: 0.7rem;
  }

  .component-indicator-header {
    color: rgba(var(--v-theme-on-surface), 0.8);
  }

  .component-diamond {
    width: 10px;
    height: 10px;
    border: 1px solid rgba(var(--v-theme-on-surface), 0.6);
    transform: rotate(45deg);
    display: inline-block;
    flex-shrink: 0;
    background: transparent;
  }

  .component-indicator.filled .component-diamond {
    background: rgba(var(--v-theme-on-surface), 0.9);
    border-color: rgba(var(--v-theme-on-surface), 0.9);
  }

  .component-letter {
    font-size: 0.7rem;
    font-weight: 500;
    color: rgba(var(--v-theme-on-surface), 0.9);
    margin-left: 2px;
  }

  .spell-mapper-hint {
    font-size: 0.65rem;
    font-style: italic;
    color: rgba(var(--v-theme-warning), 0.8);
  }

  .spell-name {
    font-size: 0.75rem;
    font-weight: 500;
    margin-bottom: 4px;
    word-wrap: break-word;
  }

  .spell-level {
    font-size: 0.65rem;
    opacity: 0.7;
    font-style: italic;
  }

  @media (max-width: 600px) {
    .abilities-grid {
      grid-template-columns: 1fr;
    }

    .player-details :deep(.v-row) {
      margin: 0;
    }

    .player-details :deep(.v-col) {
      padding: 8px;
    }

    .spell-slot-row {
      flex-wrap: wrap;
    }

    .spell-slot-label {
      width: 100%;
      margin-bottom: 4px;
    }
  }
</style>
