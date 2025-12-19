<template>
  <div>
    <v-app-bar
      app
      color="surface"
      elevation="1"
      class="player-screen-toolbar"
    >
      <v-app-bar-title>
        <span class="text-h6">Player Screen</span>
      </v-app-bar-title>
      <v-spacer></v-spacer>
      <v-tabs
        v-model="activeTab"
        class="d-flex"
        @update:model-value="onTabChange"
      >
        <v-tab value="dice">Rolling Dice</v-tab>
        <v-tab value="browser">Browser (YouTube)</v-tab>
        <v-tab value="map">Map</v-tab>
      </v-tabs>
    </v-app-bar>

    <v-container
      fluid
      class="player-screen-view pa-4"
    >
      <v-window v-model="activeTab">
        <v-window-item value="dice">
          <div class="preview-container">
            <div class="text-h6 mb-4">Dice Rolling Mode</div>
            <div class="text-body-2 text-medium-emphasis mb-4">
              Players will see dice roll animations and results when you trigger ability checks.
            </div>
            <div class="preview-box">
              <div class="preview-label">Player Screen Preview</div>
              <div class="preview-content dice-preview">
                <div class="preview-d20">20</div>
                <div class="preview-text">Rolling Dice Mode</div>
              </div>
            </div>
          </div>
        </v-window-item>

        <v-window-item value="browser">
          <div class="preview-container">
            <div class="text-h6 mb-4">Browser (YouTube) Mode</div>
            <div class="text-body-2 text-medium-emphasis mb-4">
              Players will see a browser window (YouTube) on their screen.
            </div>
            <div class="preview-box">
              <div class="preview-label">Player Screen Preview & Control</div>
              <div
                v-if="embedUrl"
                class="preview-content browser-preview"
              >
                <div
                  :id="previewPlayerId"
                  class="youtube-preview-player"
                />
                <div class="preview-controls">
                  <v-btn
                    color="primary"
                    size="small"
                    @click="togglePlayback"
                  >
                    <v-icon>{{ isPlaying ? 'mdi-pause' : 'mdi-play' }}</v-icon>
                    {{ isPlaying ? 'Pause' : 'Play' }}
                  </v-btn>
                </div>
              </div>
              <div
                v-else
                class="preview-content browser-preview"
              >
                <div class="preview-icon">🌐</div>
                <div class="preview-text">Browser Mode</div>
                <div class="preview-hint">Enter a YouTube URL below to see preview</div>
              </div>
            </div>
            <v-text-field
              v-model="browserUrl"
              label="Browser URL"
              placeholder="https://www.youtube.com/..."
              class="mt-4"
              @update:model-value="onBrowserUrlChange"
            />
          </div>
        </v-window-item>

        <v-window-item value="map">
          <div class="preview-container">
            <div class="text-h6 mb-4">Map Mode</div>
            <div class="text-body-2 text-medium-emphasis mb-4">
              Players will see the game map on their screen.
            </div>
            <div class="preview-box">
              <div class="preview-label">Player Screen Preview</div>
              <div class="preview-content map-preview">
                <div class="preview-icon">🗺️</div>
                <div class="preview-text">Map Mode</div>
                <div class="preview-hint">Game map view</div>
              </div>
            </div>
          </div>
        </v-window-item>
      </v-window>
    </v-container>
  </div>
</template>

<script setup>
  import { ref, onMounted, onBeforeUnmount, watch, computed } from 'vue';
  import { useUiStore } from '../stores/uiStore';
  import { useRollBroadcastStore } from '../stores/rollBroadcastStore';
  import { useTowerStore } from '../stores/towerStore';

  const uiStore = useUiStore();
  const rollBroadcast = useRollBroadcastStore();
  const towerStore = useTowerStore();

  const activeTab = ref(uiStore.playerScreenMode || 'dice');
  const browserUrl = ref(uiStore.playerScreenBrowserUrl || '');

  // YouTube iframe API
  const previewPlayerId = 'youtube-preview-player';
  let previewPlayer = null;
  let ytApiReady = false;
  const isPlaying = ref(false);
  let lastKnownPosition = 0;
  let seekDebounceTimeout = null;

  // Convert YouTube URL to embed format
  const getEmbedUrl = url => {
    if (!url) return '';
    try {
      const urlObj = new URL(url);
      if (
        urlObj.hostname.includes('youtube.com') ||
        urlObj.hostname.includes('youtu.be')
      ) {
        if (urlObj.pathname.includes('/embed/')) {
          return url;
        }
        let videoId = null;
        if (urlObj.searchParams.has('v')) {
          videoId = urlObj.searchParams.get('v');
        } else if (urlObj.hostname.includes('youtu.be')) {
          videoId = urlObj.pathname.slice(1);
        } else if (urlObj.pathname && urlObj.pathname !== '/') {
          const pathParts = urlObj.pathname.split('/').filter(p => p);
          if (pathParts.length > 0 && pathParts[0] !== 'watch') {
            videoId = pathParts[pathParts.length - 1];
          }
        }
        if (videoId) {
          return `https://www.youtube.com/embed/${videoId}?enablejsapi=1`;
        }
        return '';
      }
      return url;
    } catch (e) {
      console.warn('[PlayerScreenSettings] Invalid URL:', url, e);
      return '';
    }
  };

  const embedUrl = computed(() => getEmbedUrl(browserUrl.value));

  // Get server URL from first tower with pyServerUrl, or use a default
  const serverUrl = computed(() => {
    const towers = towerStore.towers;
    const towerWithServer = towers.find(t => t.pyServerUrl);
    return towerWithServer?.pyServerUrl || '';
  });

  const broadcastMode = (mode, url = '') => {
    if (serverUrl.value) {
      rollBroadcast.setServerUrl(serverUrl.value);
      rollBroadcast.broadcastModeChange(mode, url);
    } else {
      console.warn('[PlayerScreenSettings] No server URL available. Configure a tower with Python server URL first.');
    }
  };

  const onTabChange = (newTab) => {
    if (newTab && newTab !== uiStore.playerScreenMode) {
      uiStore.setPlayerScreenMode(newTab);
      // Broadcast mode change to player screens via WebSocket
      broadcastMode(newTab, browserUrl.value);
    }
  };

  const onBrowserUrlChange = (url) => {
    uiStore.setPlayerScreenBrowserUrl(url);
    if (activeTab.value === 'browser') {
      broadcastMode('browser', url);
    }
  };

  // Watch for external mode changes
  watch(
    () => uiStore.playerScreenMode,
    (newMode) => {
      if (newMode && newMode !== activeTab.value) {
        activeTab.value = newMode;
      }
    }
  );

  // Load YouTube iframe API
  const loadYouTubeAPI = () => {
    if (window.YT && window.YT.Player) {
      ytApiReady = true;
      initPreviewPlayer();
      return;
    }
    if (document.getElementById('youtube-iframe-api')) {
      // Already loading
      return;
    }
    const tag = document.createElement('script');
    tag.id = 'youtube-iframe-api';
    tag.src = 'https://www.youtube.com/iframe_api';
    const firstScriptTag = document.getElementsByTagName('script')[0];
    firstScriptTag.parentNode.insertBefore(tag, firstScriptTag);
    window.onYouTubeIframeAPIReady = () => {
      ytApiReady = true;
      initPreviewPlayer();
    };
  };

  // Initialize preview player
  const initPreviewPlayer = () => {
    if (!ytApiReady || !embedUrl.value) return;
    if (previewPlayer) {
      if (seekDebounceTimeout) {
        clearTimeout(seekDebounceTimeout);
        seekDebounceTimeout = null;
      }
      if (seekMonitorInterval) {
        clearInterval(seekMonitorInterval);
        seekMonitorInterval = null;
      }
      previewPlayer.destroy();
      previewPlayer = null;
    }
    try {
      const videoId = embedUrl.value.match(/embed\/([^?]+)/)?.[1] || '';
      if (!videoId) return;
      previewPlayer = new window.YT.Player(previewPlayerId, {
        videoId,
        playerVars: {
          enablejsapi: 1,
          modestbranding: 1,
          rel: 0,
        },
        events: {
          onReady: () => {
            console.log('[PlayerScreenSettings] Preview player ready');
            // Initialize last known position
            try {
              lastKnownPosition = previewPlayer.getCurrentTime?.() || 0;
            } catch (e) {
              // Ignore
            }
            // Start monitoring for seeks
            startSeekMonitoring();
          },
          onStateChange: event => {
            // YT.PlayerState.PLAYING = 1, YT.PlayerState.PAUSED = 2, YT.PlayerState.ENDED = 0
            isPlaying.value = event.data === 1; // 1 = playing
            
            // When state changes, check if it was a seek (user dragged slider)
            // Use a small delay to ensure position is updated after seek
            detectAndSendSeek();
          },
        },
      });
    } catch (e) {
      console.error('[PlayerScreenSettings] Failed to init preview player:', e);
    }
  };

  // Toggle playback (play/pause)
  const togglePlayback = () => {
    if (!previewPlayer) return;
    try {
      const playerState = previewPlayer.getPlayerState?.();
      // YT.PlayerState.PLAYING = 1, YT.PlayerState.PAUSED = 2
      if (playerState === 1) {
        // Currently playing, so pause
        previewPlayer.pauseVideo();
        broadcastPlaybackCommand('pause');
      } else {
        // Currently paused or stopped, so play
        previewPlayer.playVideo();
        broadcastPlaybackCommand('play');
      }
    } catch (e) {
      console.error('[PlayerScreenSettings] Failed to toggle playback:', e);
    }
  };

  // Monitor for seeks by checking position periodically (only to detect slider drags)
  let seekMonitorInterval = null;
  let lastMonitoredPosition = 0;
  let lastMonitorTime = Date.now();
  
  const startSeekMonitoring = () => {
    if (seekMonitorInterval) {
      clearInterval(seekMonitorInterval);
    }
    
    lastMonitorTime = Date.now();
    try {
      lastMonitoredPosition = previewPlayer.getCurrentTime?.() || 0;
    } catch (e) {
      // Ignore
    }
    
    seekMonitorInterval = setInterval(() => {
      if (!previewPlayer) {
        if (seekMonitorInterval) {
          clearInterval(seekMonitorInterval);
          seekMonitorInterval = null;
        }
        return;
      }
      try {
        const currentTime = previewPlayer.getCurrentTime?.();
        if (currentTime === undefined || currentTime === null) return;
        
        const now = Date.now();
        const timeElapsed = (now - lastMonitorTime) / 1000; // seconds
        lastMonitorTime = now;
        
        const playerState = previewPlayer.getPlayerState?.();
        const isPlayingState = playerState === 1;
        
        // Calculate expected position based on playback
        const expectedPosition = isPlayingState 
          ? lastMonitoredPosition + timeElapsed 
          : lastMonitoredPosition;
        
        // If actual position differs significantly from expected, it's a seek (user dragged slider)
        const positionDiff = Math.abs(currentTime - expectedPosition);
        
        // Threshold: if difference > 1.0 second, it's definitely a seek (user dragged slider)
        if (positionDiff > 1.0) {
          lastKnownPosition = currentTime;
          lastMonitoredPosition = currentTime;
          broadcastPosition(currentTime);
        } else {
          // Update tracked position normally
          lastMonitoredPosition = currentTime;
        }
      } catch (e) {
        // Silently handle errors
      }
    }, 500); // Check every 500ms - less frequent, only to detect seeks
  };

  // Detect seek and send position update (called on state changes)
  const detectAndSendSeek = () => {
    if (!previewPlayer || !serverUrl.value) return;
    
    // Clear any pending debounce
    if (seekDebounceTimeout) {
      clearTimeout(seekDebounceTimeout);
    }
    
    // Debounce to catch when user releases the slider
    seekDebounceTimeout = setTimeout(() => {
      try {
        const currentTime = previewPlayer.getCurrentTime?.();
        if (currentTime === undefined || currentTime === null) return;
        
        // If position changed significantly from last known, it's a seek
        const positionDiff = Math.abs(currentTime - lastKnownPosition);
        
        // Threshold: if difference > 0.5 seconds, it's definitely a seek
        if (positionDiff > 0.5) {
          lastKnownPosition = currentTime;
          lastMonitoredPosition = currentTime;
          broadcastPosition(currentTime);
        } else {
          // Update last known position
          lastKnownPosition = currentTime;
          lastMonitoredPosition = currentTime;
        }
      } catch (e) {
        // Silently handle errors
      }
    }, 200); // Wait 200ms after state change to catch slider release
  };

  // Broadcast playback command to player screen
  const broadcastPlaybackCommand = async (command) => {
    if (!serverUrl.value) return;
    try {
      const baseUrl = serverUrl.value.replace(/\/detect(\/best)?$/, '').replace(/\/$/, '');
      await fetch(`${baseUrl}/youtube-playback`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command }),
      });
      // Reduced logging
    } catch (e) {
      console.error('[PlayerScreenSettings] Failed to broadcast playback command:', e);
    }
  };

  // Broadcast position to player screen (only called when seek is detected)
  const broadcastPosition = async (position) => {
    if (!serverUrl.value) return;
    try {
      const baseUrl = serverUrl.value.replace(/\/detect(\/best)?$/, '').replace(/\/$/, '');
      await fetch(`${baseUrl}/youtube-playback`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command: 'seek', position }),
      });
      console.log('[PlayerScreenSettings] Seek detected, broadcasting position:', position.toFixed(2));
    } catch (e) {
      console.error('[PlayerScreenSettings] Failed to broadcast position:', e);
    }
  };


  // Watch embedUrl to reinitialize player
  watch(embedUrl, (newUrl) => {
    if (newUrl && activeTab.value === 'browser') {
      if (ytApiReady) {
        initPreviewPlayer();
      } else {
        loadYouTubeAPI();
      }
    }
  });

  // Watch activeTab to load API when browser tab is active
  watch(activeTab, (newTab) => {
    if (newTab === 'browser' && embedUrl.value) {
      loadYouTubeAPI();
    }
  });

  onMounted(() => {
    activeTab.value = uiStore.playerScreenMode || 'dice';
    browserUrl.value = uiStore.playerScreenBrowserUrl || '';
    if (activeTab.value === 'browser' && embedUrl.value) {
      loadYouTubeAPI();
    }
  });

  onBeforeUnmount(() => {
    if (seekDebounceTimeout) {
      clearTimeout(seekDebounceTimeout);
      seekDebounceTimeout = null;
    }
    if (seekMonitorInterval) {
      clearInterval(seekMonitorInterval);
      seekMonitorInterval = null;
    }
    if (previewPlayer) {
      previewPlayer.destroy();
      previewPlayer = null;
    }
  });
</script>

<style scoped>
  .player-screen-view {
    height: 100vh;
    overflow-y: auto;
  }

  .preview-container {
    max-width: 1200px;
    margin: 0 auto;
  }

  .preview-box {
    border: 2px solid rgba(200, 170, 100, 0.3);
    border-radius: 8px;
    padding: 24px;
    background: linear-gradient(
      180deg,
      rgba(50, 45, 55, 0.7) 0%,
      rgba(35, 30, 40, 0.9) 100%
    );
  }

  .preview-label {
    font-family: 'Cinzel', serif;
    font-size: 12px;
    color: #8a7a6a;
    text-transform: uppercase;
    letter-spacing: 2px;
    margin-bottom: 16px;
  }

  .preview-content {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 400px;
    gap: 16px;
  }

  .dice-preview {
    background: rgba(26, 21, 32, 0.8);
  }

  .preview-d20 {
    font-family: 'Cinzel', serif;
    font-size: 120px;
    font-weight: 700;
    color: rgba(200, 170, 100, 0.3);
    text-shadow: 0 0 20px rgba(200, 170, 100, 0.2);
  }

  .browser-preview,
  .map-preview {
    background: rgba(26, 21, 32, 0.8);
  }

  .preview-icon {
    font-size: 80px;
    opacity: 0.5;
  }

  .preview-text {
    font-family: 'Cinzel', serif;
    font-size: 24px;
    color: #f4e4c1;
    text-align: center;
  }

  .preview-hint {
    font-family: 'Crimson Text', serif;
    font-size: 16px;
    color: #b8a88a;
    font-style: italic;
    text-align: center;
  }

  .youtube-preview-player {
    width: 100%;
    max-width: 640px;
    aspect-ratio: 16 / 9;
    margin: 0 auto;
  }

  .preview-controls {
    display: flex;
    gap: 12px;
    justify-content: center;
    margin-top: 16px;
  }
</style>

