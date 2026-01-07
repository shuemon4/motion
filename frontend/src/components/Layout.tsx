import { useState } from 'react'
import { Outlet, Link } from 'react-router-dom'
import { useSystemStatus } from '@/api/queries'
import { useAuthContext } from '@/contexts/AuthContext'

export function Layout() {
  const { data: status } = useSystemStatus()
  const { isAuthenticated, role, showLoginModal } = useAuthContext()
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false)

  const formatBytes = (bytes: number) => {
    if (bytes < 1024) return `${bytes} B`
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(0)} KB`
    if (bytes < 1024 * 1024 * 1024) return `${(bytes / (1024 * 1024)).toFixed(0)} MB`
    return `${(bytes / (1024 * 1024 * 1024)).toFixed(1)} GB`
  }

  const getTempColor = (celsius: number) => {
    if (celsius >= 80) return 'text-red-500'
    if (celsius >= 70) return 'text-yellow-500'
    return 'text-green-500'
  }

  return (
    <div className="min-h-screen bg-surface">
      <header className="bg-surface-elevated border-b border-gray-800 sticky top-0 z-50">
        <div className="container mx-auto px-4 py-3 md:py-4">
          <nav className="flex items-center justify-between">
            {/* Logo and version */}
            <div className="flex items-center gap-2 md:gap-4">
              <h1 className="text-xl md:text-2xl font-bold">Motion</h1>
              {status?.version && (
                <span className="text-xs text-gray-500 hidden sm:inline">v{status.version}</span>
              )}
            </div>

            {/* Desktop navigation */}
            <div className="hidden md:flex items-center gap-6">
              <div className="flex items-center gap-4">
                <Link to="/" className="hover:text-primary">Dashboard</Link>
                {role === 'admin' && (
                  <Link to="/settings" className="hover:text-primary">Settings</Link>
                )}
                <Link to="/media" className="hover:text-primary">Media</Link>
                <button
                  onClick={showLoginModal}
                  className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg hover:bg-surface-elevated transition-colors"
                  title={isAuthenticated ? 'Logged in' : 'Login'}
                >
                  <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" />
                  </svg>
                  <span className={`text-xs ${isAuthenticated ? 'text-green-500' : 'text-gray-400'}`}>
                    {isAuthenticated ? 'Admin' : 'Login'}
                  </span>
                </button>
              </div>
              {status && (
                <div className="flex items-center gap-3 text-xs border-l border-gray-700 pl-4">
                  {status.temperature && (
                    <div className="flex items-center gap-1">
                      <svg className="w-4 h-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z" />
                      </svg>
                      <span className={getTempColor(status.temperature.celsius)}>
                        {status.temperature.celsius.toFixed(1)}°C
                      </span>
                    </div>
                  )}
                  {status.memory && (
                    <div className="flex items-center gap-1">
                      <span className="text-gray-400">RAM:</span>
                      <span>{status.memory.percent.toFixed(0)}%</span>
                    </div>
                  )}
                  {status.disk && (
                    <div className="flex items-center gap-1">
                      <span className="text-gray-400">Disk:</span>
                      <span>{formatBytes(status.disk.used)} / {formatBytes(status.disk.total)}</span>
                    </div>
                  )}
                </div>
              )}
            </div>

            {/* Mobile menu button */}
            <div className="flex items-center gap-2 md:hidden">
              {/* Compact system stats for mobile */}
              {status?.temperature && (
                <span className={`text-xs ${getTempColor(status.temperature.celsius)}`}>
                  {status.temperature.celsius.toFixed(0)}°C
                </span>
              )}
              <button
                onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
                className="p-2 rounded-lg hover:bg-surface transition-colors"
                aria-label="Toggle menu"
              >
                {mobileMenuOpen ? (
                  <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M6 18L18 6M6 6l12 12" />
                  </svg>
                ) : (
                  <svg className="w-6 h-6" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M4 6h16M4 12h16M4 18h16" />
                  </svg>
                )}
              </button>
            </div>
          </nav>

          {/* Mobile menu dropdown */}
          {mobileMenuOpen && (
            <div className="md:hidden mt-3 pt-3 border-t border-gray-800">
              <div className="flex flex-col gap-2">
                <Link
                  to="/"
                  onClick={() => setMobileMenuOpen(false)}
                  className="px-3 py-2 rounded-lg hover:bg-surface transition-colors"
                >
                  Dashboard
                </Link>
                {role === 'admin' && (
                  <Link
                    to="/settings"
                    onClick={() => setMobileMenuOpen(false)}
                    className="px-3 py-2 rounded-lg hover:bg-surface transition-colors"
                  >
                    Settings
                  </Link>
                )}
                <Link
                  to="/media"
                  onClick={() => setMobileMenuOpen(false)}
                  className="px-3 py-2 rounded-lg hover:bg-surface transition-colors"
                >
                  Media
                </Link>
                <button
                  onClick={() => {
                    showLoginModal()
                    setMobileMenuOpen(false)
                  }}
                  className="flex items-center gap-2 px-3 py-2 rounded-lg hover:bg-surface transition-colors text-left"
                >
                  <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" />
                  </svg>
                  <span className={isAuthenticated ? 'text-green-500' : 'text-gray-400'}>
                    {isAuthenticated ? 'Logged in as Admin' : 'Login'}
                  </span>
                </button>

                {/* Mobile system stats */}
                {status && (
                  <div className="flex flex-wrap gap-3 px-3 py-2 text-xs text-gray-400 border-t border-gray-800 mt-2 pt-3">
                    {status.temperature && (
                      <span className={getTempColor(status.temperature.celsius)}>
                        Temp: {status.temperature.celsius.toFixed(1)}°C
                      </span>
                    )}
                    {status.memory && (
                      <span>RAM: {status.memory.percent.toFixed(0)}%</span>
                    )}
                    {status.disk && (
                      <span>Disk: {formatBytes(status.disk.used)} / {formatBytes(status.disk.total)}</span>
                    )}
                  </div>
                )}
              </div>
            </div>
          )}
        </div>
      </header>

      <main className="container mx-auto px-4 py-4 md:py-8">
        <Outlet />
      </main>
    </div>
  )
}
