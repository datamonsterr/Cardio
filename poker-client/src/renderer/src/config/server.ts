export interface ServerConfig {
  host: string;
  port: number;
}

function readEnv(key: string): string | undefined {
  // Renderer bundle (Vite)
  try {
    const maybe = (import.meta as any)?.env?.[key];
    if (typeof maybe === 'string' && maybe.length > 0) return maybe;
  } catch {
    // ignore
  }

  // Runtime (Electron renderer with nodeIntegration)
  try {
    const maybe = (globalThis as any)?.process?.env?.[key];
    if (typeof maybe === 'string' && maybe.length > 0) return maybe;
  } catch {
    // ignore
  }

  return undefined;
}

export function getServerConfig(): ServerConfig {
  const host =
    readEnv('VITE_SERVER_ADDRESS') ||
    readEnv('SERVER_ADDRESS') ||
    'localhost';

  const portRaw =
    readEnv('VITE_SERVER_PORT') ||
    readEnv('VITE_PORT') ||
    readEnv('PORT') ||
    '8080';

  const port = Number.parseInt(portRaw, 10);

  console.log('🔍 Server Config Debug:');
  console.log('  VITE_SERVER_ADDRESS:', readEnv('VITE_SERVER_ADDRESS'));
  console.log('  SERVER_ADDRESS:', readEnv('SERVER_ADDRESS'));
  console.log('  VITE_SERVER_PORT:', readEnv('VITE_SERVER_PORT'));
  console.log('  PORT:', readEnv('PORT'));
  console.log('  Final config:', { host, port });

  return {
    host,
    port: Number.isFinite(port) ? port : 8080
  };
}
