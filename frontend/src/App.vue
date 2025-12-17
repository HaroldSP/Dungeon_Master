<script setup>
import { computed } from 'vue'
import { useRoute } from 'vue-router'
import AppHeader from './components/layout/AppHeader.vue'
import SideNavigation from './components/layout/SideNavigation.vue'

const route = useRoute()
const hideNav = computed(() => route.meta?.hideNav === true)
</script>

<template>
  <v-app>
    <!-- Fullscreen mode for player screen -->
    <template v-if="hideNav">
      <router-view />
    </template>
    <!-- Normal app with navigation -->
    <template v-else>
      <v-layout class="app-layout">
        <AppHeader />
        <SideNavigation />
        
        <v-main class="scrollable-main" style="overflow-y: auto; height: 100vh">
          <router-view />
        </v-main>
      </v-layout>
    </template>
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
