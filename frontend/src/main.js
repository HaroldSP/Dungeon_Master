import { createApp } from 'vue'
import { createPinia } from 'pinia'
import router from './router'
import './style.css'
import App from './App.vue'
// Vuetify
import 'vuetify/styles'
import { createVuetify } from 'vuetify'
import '@mdi/font/css/materialdesignicons.css'
import * as components from 'vuetify/components'
import * as directives from 'vuetify/directives'

const app = createApp(App)
const pinia = createPinia()
const vuetify = createVuetify({
  components,
  directives,
  defaults: {
    global: {
      ripple: true
    }
  },
  theme: {
    defaultTheme: 'dark'
  }
})

app.use(pinia)
app.use(router)
app.use(vuetify)
app.mount('#app')
