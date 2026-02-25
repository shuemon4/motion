import { useState, useCallback, useEffect, useRef } from 'react'
import { getStoredRestartTimestamp } from '@/lib/cameraRestart'
import { PtzControls } from '@/components/PtzControls'
import { useSnapshotPolling } from '@/hooks/useSnapshotPolling'

interface CameraStreamProps {
  cameraId: number
  className?: string
  mode?: 'live' | 'snapshot' | 'substream'
  snapshotInterval?: number
  onStreamFpsChange?: (fps: number) => void
}

// Custom event name for camera restart notification
export const CAMERA_RESTARTED_EVENT = 'camera-restarted'

export function CameraStream({
  cameraId,
  className = '',
  mode = 'live',
  snapshotInterval = 1000,
}: CameraStreamProps) {
  // Track the last known restart timestamp to detect changes
  const lastKnownRestartRef = useRef<number>(getStoredRestartTimestamp(cameraId))

  // Track retry attempts for exponential backoff
  const retryCountRef = useRef<number>(0)

  // Initialize streamKey from stored restart timestamp
  const [streamKey, setStreamKey] = useState(() => getStoredRestartTimestamp(cameraId))

  const [isConnected, setIsConnected] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [hasEverConnected, setHasEverConnected] = useState(false)

  const { snapshotUrl } = useSnapshotPolling(cameraId, snapshotInterval, mode === 'snapshot')

  // Force stream reconnection by changing key with exponential backoff
  const handleReconnect = useCallback(() => {
    const delay = Math.min(2000 * Math.pow(2, retryCountRef.current), 30000)
    retryCountRef.current++
    setTimeout(() => {
      setStreamKey((k) => k + 1)
      setIsConnected(false)
    }, delay)
  }, [])

  // Listen for camera restart events to force reconnection (same-page scenario)
  useEffect(() => {
    const handleCameraRestarted = (event: CustomEvent<{ cameraId?: number }>) => {
      const eventCamId = event.detail?.cameraId
      if (!eventCamId || eventCamId === 0 || eventCamId === cameraId) {
        const newTimestamp = Date.now()
        lastKnownRestartRef.current = newTimestamp
        setStreamKey(newTimestamp)
        setIsConnected(false)
      }
    }

    window.addEventListener(CAMERA_RESTARTED_EVENT, handleCameraRestarted as EventListener)
    return () => {
      window.removeEventListener(CAMERA_RESTARTED_EVENT, handleCameraRestarted as EventListener)
    }
  }, [cameraId])

  // Check for restart timestamp changes on mount and periodically.
  useEffect(() => {
    const checkForRestart = () => {
      const storedTimestamp = getStoredRestartTimestamp(cameraId)
      const globalTimestamp = cameraId !== 0 ? getStoredRestartTimestamp(0) : 0
      const latestTimestamp = Math.max(storedTimestamp, globalTimestamp)

      if (latestTimestamp > lastKnownRestartRef.current) {
        lastKnownRestartRef.current = latestTimestamp
        setStreamKey(latestTimestamp)
        setIsConnected(false)
      }
    }

    checkForRestart()
    const intervalId = setInterval(checkForRestart, 5000)
    return () => clearInterval(intervalId)
  }, [cameraId])

  // Auto-retry on error — live and substream modes
  useEffect(() => {
    if ((mode === 'live' || mode === 'substream') && error && !isConnected) {
      handleReconnect()
    }
  }, [mode, error, isConnected, handleReconnect])

  // Reset connection state when mode changes
  useEffect(() => {
    setIsConnected(false)
    setError(null)
    // Don't reset hasEverConnected — avoids loading overlay flash on mode switch
  }, [mode])

  const streamUrl = `/${cameraId}/mjpg/stream?k=${streamKey}`
  const substreamUrl = `/${cameraId}/mjpg/substream?k=${streamKey}`

  return (
    <div className={`w-full ${className}`}>
      <div className="relative aspect-video bg-black rounded-lg overflow-hidden">
        {mode === 'substream' ? (
          <img
            key={streamKey}
            src={substreamUrl}
            alt={`Camera ${cameraId} substream`}
            className="absolute inset-0 w-full h-full object-contain"
            onLoad={() => {
              setIsConnected(true)
              setHasEverConnected(true)
              setError(null)
              retryCountRef.current = 0
            }}
            onError={() => {
              setIsConnected(false)
              setError('Stream unavailable')
            }}
          />
        ) : mode === 'live' ? (
          <img
            key={streamKey}
            src={streamUrl}
            alt={`Camera ${cameraId} live`}
            className="absolute inset-0 w-full h-full object-contain"
            onLoad={() => {
              setIsConnected(true)
              setHasEverConnected(true)
              setError(null)
              retryCountRef.current = 0
            }}
            onError={() => {
              setIsConnected(false)
              setError('Stream unavailable')
            }}
          />
        ) : (
          <img
            src={snapshotUrl}
            alt={`Camera ${cameraId} snapshot`}
            className="absolute inset-0 w-full h-full object-contain"
            onLoad={() => {
              setIsConnected(true)
              setHasEverConnected(true)
              setError(null)
            }}
            onError={() => {
              // Don't set error state for transient snapshot failures — next poll will retry.
              // Only surface error if we've never connected (initial load failure).
              if (!hasEverConnected) {
                setError('Camera unavailable')
              }
            }}
          />
        )}

        {/* Error overlay — only shown before first successful connection */}
        {error && !hasEverConnected && (
          <div className="absolute inset-0 z-20 flex items-center justify-center bg-gray-900">
            <div className="text-center p-4">
              <svg className="w-12 h-12 mx-auto text-red-500 mb-2" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
              </svg>
              <p className="text-red-500 text-sm">{error}</p>
            </div>
          </div>
        )}

        {/* Loading overlay — shown only during initial connection */}
        {!isConnected && !error && !hasEverConnected && (
          <div className="absolute inset-0 z-10 bg-gray-900">
            <div className="absolute top-4 right-4">
              <svg className="w-8 h-8 text-gray-600 animate-spin" fill="none" viewBox="0 0 24 24">
                <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
                <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
              </svg>
            </div>
          </div>
        )}

        <PtzControls cameraId={cameraId} />
      </div>
    </div>
  )
}
