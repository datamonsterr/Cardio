import { resolve } from 'path'
import { defineConfig, loadEnv } from 'electron-vite'
import react from '@vitejs/plugin-react'

export default defineConfig(({ mode }) => {
  // Load env file based on mode
  const envDir = process.cwd()
  const env = loadEnv(mode, envDir, '')
  
  console.log('🔧 electron-vite mode:', mode)
  console.log('🔧 envDir:', envDir)
  console.log('🔧 VITE_SERVER_ADDRESS:', env.VITE_SERVER_ADDRESS)
  console.log('🔧 VITE_SERVER_PORT:', env.VITE_SERVER_PORT)
  
  return {
    main: {},
    preload: {},
    renderer: {
      envDir,
      envPrefix: ['VITE_', 'SERVER_', 'PORT'],
      resolve: {
        alias: {
          '@renderer': resolve('src/renderer/src')
        }
      },
      plugins: [react()],
      define: {
        // Explicitly define environment variables for both dev and production
        'import.meta.env.VITE_SERVER_ADDRESS': JSON.stringify(env.VITE_SERVER_ADDRESS || 'localhost'),
        'import.meta.env.VITE_SERVER_PORT': JSON.stringify(env.VITE_SERVER_PORT || '8080'),
        'import.meta.env.SERVER_ADDRESS': JSON.stringify(env.SERVER_ADDRESS || ''),
        'import.meta.env.PORT': JSON.stringify(env.PORT || ''),
      }
    }
  }
})
