import { lazy, Suspense, useEffect } from 'react'
import { Routes, Route, useLocation } from 'react-router-dom'
import { Layout } from './components/Layout'

// Lazy-load route components for code splitting
const Dashboard = lazy(() => import('./pages/Dashboard').then(m => ({ default: m.Dashboard })))
const Settings = lazy(() => import('./pages/Settings').then(m => ({ default: m.Settings })))
const Media = lazy(() => import('./pages/Media').then(m => ({ default: m.Media })))

// Loading skeleton for lazy-loaded routes
function PageSkeleton() {
  return (
    <div className="flex items-center justify-center h-64">
      <div className="animate-pulse text-text-secondary">Loading...</div>
    </div>
  )
}

function App() {
  const location = useLocation()

  // DEBUG: Track route changes at App level
  console.log('[App] render at:', location.pathname)

  useEffect(() => {
    console.log('[App] location changed to:', location.pathname)
  }, [location.pathname])

  return (
    <Suspense fallback={<PageSkeleton />}>
      <Routes>
        <Route path="/" element={<Layout />}>
          <Route index element={<Dashboard />} />
          <Route path="settings" element={<Settings />} />
          <Route path="media" element={<Media />} />
        </Route>
      </Routes>
    </Suspense>
  )
}

export default App
