<script setup>
import AppHeader from './components/layout/AppHeader.vue'
import SideNavigation from './components/layout/SideNavigation.vue'
</script>

<template>
  <v-app>
    <v-layout class="app-layout">
      <AppHeader />
      <SideNavigation />
      
      <v-main class="scrollable-main" style="overflow-y: auto; height: 100vh">
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

/* Hide scrollbar but keep scrolling functionality */
.scrollable-main {
  scrollbar-width: none; /* Firefox */
  -ms-overflow-style: none; /* IE and Edge */
}

.scrollable-main::-webkit-scrollbar {
  display: none; /* Chrome, Safari, Opera */
}
</style>
