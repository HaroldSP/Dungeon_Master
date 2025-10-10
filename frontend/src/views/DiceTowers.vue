<template>
  <div>
    <v-app-bar
      app
      color="surface"
      elevation="1"
      class="dice-towers-toolbar"
    >
      <v-app-bar-title>
        <span class="text-h6">Dice Towers</span>
      </v-app-bar-title>
      <v-spacer></v-spacer>
      <v-tabs
        v-model="activeTab"
        class="d-flex"
      >
        <v-tab value="manage">Manage Towers</v-tab>
        <v-tab value="add">Add New</v-tab>
      </v-tabs>
    </v-app-bar>

    <v-container
      fluid
      class="dice-towers-view pa-4"
    >
      <v-window v-model="activeTab">
        <v-window-item value="manage">
          <ManageTowers ref="manageTowersRef" />
        </v-window-item>
        <v-window-item value="add">
          <AddNewTower @provision-success="onProvisionSuccess" />
        </v-window-item>
      </v-window>
    </v-container>
  </div>
</template>

<script setup>
  import { ref, watch } from 'vue';
  import ManageTowers from '../components/ManageTowers.vue';
  import AddNewTower from '../components/AddNewTower.vue';

  const activeTab = ref('manage');
  const manageTowersRef = ref(null);

  // Watch for tab changes to check status when Manage Towers tab becomes active
  watch(activeTab, async newTab => {
    if (newTab === 'manage' && manageTowersRef.value) {
      await manageTowersRef.value.checkAllTowerStatusAndNames();
    }
  });

  function onProvisionSuccess() {
    activeTab.value = 'manage';
  }
</script>

<style scoped>
  .dice-towers-view {
    height: 100vh;
    overflow-y: auto;
  }

  ul {
    padding-left: 18px;
  }
</style>
