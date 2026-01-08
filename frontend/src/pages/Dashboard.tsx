import { useState, useMemo, useEffect } from 'react'
import { useQuery } from '@tanstack/react-query'
import { CameraStream } from '@/components/CameraStream'
import { BottomSheet } from '@/components/BottomSheet'
import { QuickSettings } from '@/components/QuickSettings'
import { useCameras } from '@/api/queries'
import { apiGet } from '@/api/client'
import { setCsrfToken } from '@/api/csrf'
import { useAuthContext } from '@/contexts/AuthContext'

interface ConfigParam {
  value: string | number | boolean
  enabled: boolean
  category: number
  type: string
}

interface DashboardConfig {
  configuration: {
    default: Record<string, ConfigParam>
    [key: string]: Record<string, ConfigParam>
  }
  csrf_token?: string
}

export function Dashboard() {
  const { data: cameras, isLoading, error } = useCameras()
  const { role } = useAuthContext()
  const [sheetOpen, setSheetOpen] = useState(false)
  const [selectedCameraId, setSelectedCameraId] = useState<number | null>(null)
  const [cameraFps, setCameraFps] = useState<Record<number, number>>({})

  // Fetch config when sheet is open
  const { data: configData } = useQuery({
    queryKey: ['config-quick'],
    queryFn: async () => {
      const cfg = await apiGet<DashboardConfig>('/0/api/config')
      if (cfg.csrf_token) {
        setCsrfToken(cfg.csrf_token)
      }
      return cfg
    },
    enabled: sheetOpen, // Only fetch when sheet is open
    staleTime: 30000,
  })

  const openQuickSettings = (cameraId: number) => {
    setSelectedCameraId(cameraId)
    setSheetOpen(true)
  }

  const closeQuickSettings = () => {
    setSheetOpen(false)
  }

  const handleFpsChange = (cameraId: number, fps: number) => {
    setCameraFps((prev) => ({ ...prev, [cameraId]: fps }))
  }

  // Get camera name for sheet title
  const selectedCamera = cameras?.find((c) => c.id === selectedCameraId)
  const sheetTitle = selectedCamera
    ? `Quick Settings - ${selectedCamera.name}`
    : 'Quick Settings'

  // Build config for selected camera (merge camera-specific with defaults)
  const configForCamera = useMemo(() => {
    if (!configData || !selectedCameraId) return {}

    const defaultConfig = configData.configuration?.default || {}
    const cameraConfig = configData.configuration?.[`cam${selectedCameraId}`] || {}

    // Merge: camera-specific overrides global
    return { ...defaultConfig, ...cameraConfig }
  }, [configData, selectedCameraId])

  // Gear icon button component
  const SettingsButton = ({ cameraId }: { cameraId: number }) => (
    <button
      type="button"
      onClick={(e) => {
        e.stopPropagation()
        openQuickSettings(cameraId)
      }}
      className="p-1.5 hover:bg-surface rounded-full transition-colors"
      aria-label="Quick settings"
      title="Quick settings"
    >
      <svg
        className="w-5 h-5 text-gray-400 hover:text-gray-200"
        fill="none"
        stroke="currentColor"
        viewBox="0 0 24 24"
      >
        <path
          strokeLinecap="round"
          strokeLinejoin="round"
          strokeWidth={2}
          d="M10.325 4.317c.426-1.756 2.924-1.756 3.35 0a1.724 1.724 0 002.573 1.066c1.543-.94 3.31.826 2.37 2.37a1.724 1.724 0 001.065 2.572c1.756.426 1.756 2.924 0 3.35a1.724 1.724 0 00-1.066 2.573c.94 1.543-.826 3.31-2.37 2.37a1.724 1.724 0 00-2.572 1.065c-.426 1.756-2.924 1.756-3.35 0a1.724 1.724 0 00-2.573-1.066c-1.543.94-3.31-.826-2.37-2.37a1.724 1.724 0 00-1.065-2.572c-1.756-.426-1.756-2.924 0-3.35a1.724 1.724 0 001.066-2.573c-.94-1.543.826-3.31 2.37-2.37.996.608 2.296.07 2.572-1.065z"
        />
        <path
          strokeLinecap="round"
          strokeLinejoin="round"
          strokeWidth={2}
          d="M15 12a3 3 0 11-6 0 3 3 0 016 0z"
        />
      </svg>
    </button>
  )

  // Fullscreen button component
  const FullscreenButton = ({ cameraId }: { cameraId: number }) => {
    const [isFullscreen, setIsFullscreen] = useState(false)

    const toggleFullscreen = (e: React.MouseEvent) => {
      e.stopPropagation()
      // Find the camera stream container
      const container = document.querySelector(`[data-camera-id="${cameraId}"]`)
      if (!container) return

      if (!isFullscreen) {
        if (container.requestFullscreen) {
          container.requestFullscreen()
        }
      } else {
        if (document.exitFullscreen) {
          document.exitFullscreen()
        }
      }
    }

    // Listen for fullscreen changes
    useEffect(() => {
      const handleFullscreenChange = () => {
        setIsFullscreen(!!document.fullscreenElement)
      }

      document.addEventListener('fullscreenchange', handleFullscreenChange)
      return () => {
        document.removeEventListener('fullscreenchange', handleFullscreenChange)
      }
    }, [])

    return (
      <button
        type="button"
        onClick={toggleFullscreen}
        className="p-1.5 hover:bg-surface rounded-full transition-colors"
        aria-label="Toggle fullscreen"
        title="Toggle fullscreen"
      >
        <svg
          className="w-5 h-5 text-gray-400 hover:text-gray-200"
          fill="none"
          stroke="currentColor"
          viewBox="0 0 24 24"
        >
          {isFullscreen ? (
            <path
              strokeLinecap="round"
              strokeLinejoin="round"
              strokeWidth={2}
              d="M9 9V4.5M9 9H4.5M9 9L3.75 3.75M9 15v4.5M9 15H4.5M9 15l-5.25 5.25M15 9h4.5M15 9V4.5M15 9l5.25-5.25M15 15h4.5M15 15v4.5m0-4.5l5.25 5.25"
            />
          ) : (
            <path
              strokeLinecap="round"
              strokeLinejoin="round"
              strokeWidth={2}
              d="M4 8V4m0 0h4M4 4l5 5m11-1V4m0 0h-4m4 0l-5 5M4 16v4m0 0h4m-4 0l5-5m11 5l-5-5m5 5v-4m0 4h-4"
            />
          )}
        </svg>
      </button>
    )
  }

  if (isLoading) {
    return (
      <div className="p-4 sm:p-6">
        <h2 className="text-2xl sm:text-3xl font-bold mb-4 sm:mb-6">Camera Dashboard</h2>
        <div className="flex flex-col items-center gap-6">
          {[1].map((i) => (
            <div
              key={i}
              className="bg-surface-elevated rounded-lg p-4 animate-pulse w-full max-w-4xl"
            >
              <div className="h-6 bg-surface rounded w-1/3 mb-4"></div>
              <div className="aspect-video bg-surface rounded"></div>
            </div>
          ))}
        </div>
      </div>
    )
  }

  if (error) {
    return (
      <div className="p-4 sm:p-6">
        <h2 className="text-2xl sm:text-3xl font-bold mb-4 sm:mb-6">Camera Dashboard</h2>
        <div className="bg-danger/10 border border-danger rounded-lg p-4 max-w-2xl mx-auto">
          <p className="text-danger">
            Failed to load cameras: {error instanceof Error ? error.message : 'Unknown error'}
          </p>
          <button
            className="mt-2 text-sm text-primary hover:underline"
            onClick={() => window.location.reload()}
          >
            Retry
          </button>
        </div>
      </div>
    )
  }

  if (!cameras || cameras.length === 0) {
    return (
      <div className="p-4 sm:p-6">
        <h2 className="text-2xl sm:text-3xl font-bold mb-4 sm:mb-6">Camera Dashboard</h2>
        <div className="bg-surface-elevated rounded-lg p-8 text-center max-w-2xl mx-auto">
          <svg className="w-16 h-16 mx-auto text-gray-600 mb-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M15 10l4.553-2.276A1 1 0 0121 8.618v6.764a1 1 0 01-1.447.894L15 14M5 18h8a2 2 0 002-2V8a2 2 0 00-2-2H5a2 2 0 00-2 2v8a2 2 0 002 2z" />
          </svg>
          <p className="text-gray-400 text-lg">No cameras configured</p>
          <p className="text-sm text-gray-500 mt-2">
            Add cameras in Motion's configuration file
          </p>
        </div>
      </div>
    )
  }

  // Determine layout based on camera count
  const cameraCount = cameras.length

  // Single camera: streamlined layout without extra headers
  if (cameraCount === 1) {
    const camera = cameras[0]
    const fps = cameraFps[camera.id] || 0
    return (
      <div className="p-4 sm:p-6">
        <div className="max-w-5xl mx-auto">
          <div className="bg-surface-elevated rounded-lg overflow-hidden shadow-lg" data-camera-id={camera.id}>
            {/* Camera header */}
            <div className="px-4 py-3 border-b border-surface flex items-center justify-between">
              <div className="flex items-center gap-2">
                <div className="w-2 h-2 bg-green-500 rounded-full animate-pulse"></div>
                <h3 className="font-medium">{camera.name}</h3>
              </div>
              <div className="flex items-center gap-2">
                {camera.width && camera.height && (
                  <span className="text-xs text-gray-500">
                    {camera.width}x{camera.height}{fps > 0 && ` @ ${fps} fps`}
                  </span>
                )}
                <FullscreenButton cameraId={camera.id} />
                {role === 'admin' && <SettingsButton cameraId={camera.id} />}
              </div>
            </div>

            {/* Camera stream */}
            <CameraStream
              cameraId={camera.id}
              onFpsChange={(fps) => handleFpsChange(camera.id, fps)}
            />
          </div>
        </div>

        {/* Quick Settings Bottom Sheet */}
        <BottomSheet
          isOpen={sheetOpen}
          onClose={closeQuickSettings}
          title={sheetTitle}
        >
          {selectedCameraId && (
            <QuickSettings
              cameraId={selectedCameraId}
              config={configForCamera}
            />
          )}
        </BottomSheet>
      </div>
    )
  }

  // Multiple cameras: responsive grid
  const getGridClasses = () => {
    if (cameraCount === 2) {
      return 'grid grid-cols-1 lg:grid-cols-2 gap-4 sm:gap-6'
    } else if (cameraCount <= 4) {
      return 'grid grid-cols-1 md:grid-cols-2 gap-4 sm:gap-6'
    } else {
      return 'grid grid-cols-1 md:grid-cols-2 xl:grid-cols-3 gap-4 sm:gap-6'
    }
  }

  return (
    <div className="p-4 sm:p-6">
      <h2 className="text-2xl sm:text-3xl font-bold mb-4 sm:mb-6">
        Cameras ({cameraCount})
      </h2>

      <div className={getGridClasses()}>
        {cameras.map((camera) => {
          const fps = cameraFps[camera.id] || 0
          return (
            <div
              key={camera.id}
              className="bg-surface-elevated rounded-lg overflow-hidden shadow-lg"
              data-camera-id={camera.id}
            >
              {/* Camera header */}
              <div className="px-4 py-3 border-b border-surface flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <div className="w-2 h-2 bg-green-500 rounded-full animate-pulse"></div>
                  <h3 className="font-medium text-sm sm:text-base">{camera.name}</h3>
                </div>
                <div className="flex items-center gap-2">
                  {camera.width && camera.height && (
                    <span className="text-xs text-gray-500">
                      {camera.width}x{camera.height}{fps > 0 && ` @ ${fps} fps`}
                    </span>
                  )}
                  <FullscreenButton cameraId={camera.id} />
                  {role === 'admin' && <SettingsButton cameraId={camera.id} />}
                </div>
              </div>

              {/* Camera stream */}
              <CameraStream
                cameraId={camera.id}
                onFpsChange={(fps) => handleFpsChange(camera.id, fps)}
              />
            </div>
          )
        })}
      </div>

      <p className="text-center text-xs text-gray-500 mt-6">
        Click on a camera to view fullscreen
      </p>

      {/* Quick Settings Bottom Sheet */}
      <BottomSheet
        isOpen={sheetOpen}
        onClose={closeQuickSettings}
        title={sheetTitle}
      >
        {selectedCameraId && (
          <QuickSettings
            cameraId={selectedCameraId}
            config={configForCamera}
          />
        )}
      </BottomSheet>
    </div>
  )
}
