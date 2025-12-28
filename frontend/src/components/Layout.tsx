import { Outlet, Link } from 'react-router-dom'

export function Layout() {
  return (
    <div className="min-h-screen bg-surface">
      <header className="bg-surface-elevated border-b border-gray-800">
        <div className="container mx-auto px-4 py-4">
          <nav className="flex items-center justify-between">
            <h1 className="text-2xl font-bold">Motion</h1>
            <div className="flex gap-4">
              <Link to="/" className="hover:text-primary">Dashboard</Link>
              <Link to="/settings" className="hover:text-primary">Settings</Link>
              <Link to="/media" className="hover:text-primary">Media</Link>
            </div>
          </nav>
        </div>
      </header>

      <main className="container mx-auto px-4 py-8">
        <Outlet />
      </main>
    </div>
  )
}
