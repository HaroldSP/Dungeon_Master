<script setup>
import AppHeader from './components/layout/AppHeader.vue'
import SideNavigation from './components/layout/SideNavigation.vue'
</script>

<template>
  <v-app>
    <v-layout class="app-layout">
      <AppHeader />
      <SideNavigation />
      
      <v-main>
        <router-view />
      </v-main>
    </v-layout>
  </v-app>
</template>

<style>
/* App-specific styles */
.app-layout { 
  height: 100vh; 
}
</style>
