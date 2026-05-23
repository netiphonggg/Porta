import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react-swc'

// https://vite.dev/config/
export default defineConfig({
  plugins: [react()],
  server: {   // fix CORS
    proxy: {
      "/api":{
        target: "http://porta1-v01.local",
        "changeOrigin": true
      }
    }
  }
})
