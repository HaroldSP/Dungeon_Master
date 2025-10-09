<template>
  <v-container
    fluid
    class="map-view pa-0"
  >
    <MapCanvas />
  </v-container>
</template>

<script setup>
  import MapCanvas from '../components/MapCanvas.vue';
</script>

<style scoped>
  .map-view {
    height: 100vh;
    overflow: hidden;
  }
</style>
