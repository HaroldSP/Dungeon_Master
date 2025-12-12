<template>
  <div class="add-new-player">
    <v-card
      elevation="2"
      class="pa-4 mb-4"
    >
      <div class="text-h6 mb-4">Add New Player</div>
      <div class="text-body-2 text-medium-emphasis mb-4">
        Place a JSON character file in the <code>frontend/src/data/players/</code> folder.
        The player will appear automatically in the "Manage Players" tab.
      </div>
      <v-alert
        type="info"
        variant="tonal"
        density="compact"
      >
        <div class="text-body-2">
          <strong>Note:</strong> Player files are loaded automatically from the players folder.
          Make sure your JSON file follows the D&D character sheet format.
        </div>
      </v-alert>
    </v-card>
  </div>
</template>

<script setup>
  defineEmits(['player-added']);
</script>

<style scoped>
  code {
    background: rgba(var(--v-theme-surface), 0.1);
    padding: 2px 6px;
    border-radius: 4px;
    font-family: monospace;
  }
</style>

