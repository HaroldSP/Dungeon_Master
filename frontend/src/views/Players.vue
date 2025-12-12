<template>
  <div>
    <v-app-bar
      app
      color="surface"
      elevation="1"
      class="players-toolbar"
    >
      <v-app-bar-title>
        <span class="text-h6">Players</span>
      </v-app-bar-title>
      <v-spacer></v-spacer>
      <v-tabs
        v-model="activeTab"
        class="d-flex"
      >
        <v-tab value="manage">Manage Players</v-tab>
        <v-tab value="add">Add New</v-tab>
      </v-tabs>
    </v-app-bar>

    <v-container
      fluid
      class="players-view pa-4"
    >
      <v-window v-model="activeTab">
        <v-window-item value="manage">
          <PlayerGrid :show-header="false" />
        </v-window-item>
        <v-window-item value="add">
          <AddNewPlayer @player-added="onPlayerAdded" />
        </v-window-item>
      </v-window>
    </v-container>
  </div>
</template>

<script setup>
  import { ref } from 'vue';
  import PlayerGrid from '../components/PlayerGrid.vue';
  import AddNewPlayer from '../components/AddNewPlayer.vue';

  const activeTab = ref('manage');

  function onPlayerAdded() {
    activeTab.value = 'manage';
  }
</script>

<style scoped>
  .players-view {
    min-height: 100vh;
    overflow: visible !important;
  }

  :deep(.v-window) {
    overflow: visible !important;
  }

  :deep(.v-window-item) {
    overflow: visible !important;
  }
</style>

