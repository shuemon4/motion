import { useState, useCallback, useEffect, useRef } from 'react'
import { useMjpegStream } from '@/hooks/useMjpegStream'
import { useMpegTsStream, isMseSupported } from '@/hooks/useMpegTsStream'
import { getStoredRestartTimestamp } from '@/lib/cameraRestart'
import { PtzControls } from '@/components/PtzControls'
import { useStreamStats } from '@/api/queries'

interface CameraStreamProps {
  cameraId: number
  className?: string
  onStreamFpsChange?: (fps: number) => void // Callback for streaming FPS updates
}

// Custom event name for camera restart notification
export const CAMERA_RESTARTED_EVENT = 'camera-restarted'

type StreamMode = 'mjpeg' | 'mpegts'

function loadStreamMode(): StreamMode {
  try {
    const stored = localStorage.getItem('motion-ui-preferences')
    if (stored) {
      const parsed = JSON.parse(stored)
      if (parsed.streamMode === 'mpegts') return 'mpegts'
    }
  } catch {
    // Ignore parse errors
  }
  return 'mjpeg'
}

function loadShowStreamStats(): boolean {
  try {
    const stored = localStorage.getItem('motion-ui-preferences')
    if (stored) {
      const parsed = JSON.parse(stored)
      if (typeof parsed.showStreamStats === 'boolean') return parsed.showStreamStats
    }
  } catch {
    // Ignore parse errors
  }
  return false
}

// ─── MJPEG sub-component ────────────────────────────────────────────────────

interface MjpegViewProps {
  cameraId: number
  streamKey: number
  onFpsChange: (fps: number) => void
  onError: (err: string | null) => void
  onConnected: (connected: boolean) => void
}

function MjpegView({ cameraId, streamKey, onFpsChange, onError, onConnected }: MjpegViewProps) {
  const { imageUrl, streamFps, isConnected, error } = useMjpegStream(cameraId, streamKey)

  // Stable callbacks — these must not change identity on each render to
  // prevent infinite re-render loops from the effects below.
  const onFpsChangeRef = useRef(onFpsChange)
  const onErrorRef = useRef(onError)
  const onConnectedRef = useRef(onConnected)
  useEffect(() => { onFpsChangeRef.current = onFpsChange })
  useEffect(() => { onErrorRef.current = onError })
  useEffect(() => { onConnectedRef.current = onConnected })

  useEffect(() => { onFpsChangeRef.current(streamFps) }, [streamFps])
  useEffect(() => { onErrorRef.current(error) }, [error])
  useEffect(() => { onConnectedRef.current(isConnected) }, [isConnected])

  if (!imageUrl) return null

  return (
    <img
      src={imageUrl}
      alt={`Camera ${cameraId} stream`}
      className="absolute inset-0 w-full h-full object-contain"
    />
  )
}

// ─── MPEG-TS sub-component ──────────────────────────────────────────────────

interface MpegTsViewProps {
  cameraId: number
  streamKey: number
  onFpsChange: (fps: number) => void
  onError: (err: string | null) => void
  onConnected: (connected: boolean) => void
}

function MpegTsView({ cameraId, streamKey, onFpsChange, onError, onConnected }: MpegTsViewProps) {
  const videoRef = useRef<HTMLVideoElement>(null)
  const { isConnected, error, streamFps } = useMpegTsStream(cameraId, streamKey, videoRef)

  const onFpsChangeRef = useRef(onFpsChange)
  const onErrorRef = useRef(onError)
  const onConnectedRef = useRef(onConnected)
  useEffect(() => { onFpsChangeRef.current = onFpsChange })
  useEffect(() => { onErrorRef.current = onError })
  useEffect(() => { onConnectedRef.current = onConnected })

  useEffect(() => { onFpsChangeRef.current(streamFps) }, [streamFps])
  useEffect(() => { onErrorRef.current(error) }, [error])
  useEffect(() => { onConnectedRef.current(isConnected) }, [isConnected])

  return (
    <video
      ref={videoRef}
      autoPlay
      muted
      playsInline
      disableRemotePlayback
      className="absolute inset-0 w-full h-full object-contain"
      aria-label={`Camera ${cameraId} stream`}
    />
  )
}

// ─── Main CameraStream component ────────────────────────────────────────────

export function CameraStream({ cameraId, className = '', onStreamFpsChange }: CameraStreamProps) {
  // Track the last known restart timestamp to detect changes
  const lastKnownRestartRef = useRef<number>(getStoredRestartTimestamp(cameraId))

  // Track retry attempts for exponential backoff
  const retryCountRef = useRef<number>(0)

  // Initialize streamKey from stored restart timestamp
  // This ensures fresh connections after navigation back from Settings
  const [streamKey, setStreamKey] = useState(() => getStoredRestartTimestamp(cameraId))

  // Stream connection state (lifted from sub-components via callbacks)
  const [isConnected, setIsConnected] = useState(false)
  const [error, setError] = useState<string | null>(null)
  // Track whether we've connected at least once — after that, errors don't
  // replace the video with a full error screen; they just trigger silent retry.
  const [hasEverConnected, setHasEverConnected] = useState(false)

  // Determine effective stream mode once at mount.
  // Reads from localStorage, falls back to MJPEG if MSE unavailable.
  const [effectiveMode] = useState<StreamMode>(() => {
    const preferred = loadStreamMode()
    if (preferred === 'mpegts' && !isMseSupported()) return 'mjpeg'
    return preferred
  })

  // Was MPEG-TS requested but MSE unavailable? Show a small indicator.
  const [showFallbackIndicator] = useState<boolean>(() => {
    return loadStreamMode() === 'mpegts' && !isMseSupported()
  })

  // Stream stats overlay (user preference)
  const [showStreamStats] = useState<boolean>(() => loadShowStreamStats())
  const { data: streamStats } = useStreamStats(cameraId, { enabled: showStreamStats })

  // Store callback in ref to avoid triggering effect when callback reference changes.
  // Prevents render loops when parent passes inline arrow functions.
  const onStreamFpsChangeRef = useRef(onStreamFpsChange)
  useEffect(() => {
    onStreamFpsChangeRef.current = onStreamFpsChange
  })

  const handleFpsChange = useCallback((fps: number) => {
    onStreamFpsChangeRef.current?.(fps)
  }, [])

  const handleConnected = useCallback((connected: boolean) => {
    setIsConnected(connected)
    if (connected) {
      setHasEverConnected(true)
      retryCountRef.current = 0
    }
  }, [])

  const handleError = useCallback((err: string | null) => {
    setError(err)
  }, [])

  // Force stream reconnection by changing key with exponential backoff
  const handleReconnect = useCallback(() => {
    // Exponential backoff: 2s → 4s → 8s → 16s → 30s (max)
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
  // Handles cross-navigation restarts, multi-tab scenarios, and edge cases.
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

  // Auto-retry on error
  useEffect(() => {
    if (error && !isConnected) {
      handleReconnect()
    }
  }, [error, isConnected, handleReconnect])

  return (
    <div className={`w-full ${className}`}>
      <div className="relative aspect-video bg-black rounded-lg overflow-hidden">
        {/*
         * Sub-components are always rendered so their hooks run and stream
         * data flows. Loading/error states are z-indexed overlays on top.
         */}
        {effectiveMode === 'mjpeg' ? (
          <MjpegView
            cameraId={cameraId}
            streamKey={streamKey}
            onFpsChange={handleFpsChange}
            onError={handleError}
            onConnected={handleConnected}
          />
        ) : (
          <MpegTsView
            cameraId={cameraId}
            streamKey={streamKey}
            onFpsChange={handleFpsChange}
            onError={handleError}
            onConnected={handleConnected}
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

        {/* Loading overlay — shown only during initial connection, not during reconnections.
         * Once video has played, the <video>/<img> element retains the last frame. */}
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

        {/* Stream stats overlay: shown when user enables "Show Stream Stats" preference */}
        {showStreamStats && streamStats && (
          <div className="absolute top-2 left-2 z-10 text-xs text-white bg-black/60 px-2 py-1 rounded font-mono leading-tight">
            <span>{streamStats.streams.norm.mjpeg.fps_actual.toFixed(1)} fps</span>
            <span className="mx-1 text-gray-400">·</span>
            <span>{streamStats.streams.norm.mjpeg.clients} client{streamStats.streams.norm.mjpeg.clients !== 1 ? 's' : ''}</span>
          </div>
        )}

        {/* Fallback indicator: preference was MPEG-TS but MSE unavailable */}
        {showFallbackIndicator && (
          <div className="absolute bottom-2 left-2 z-10 text-xs text-gray-400 bg-black/60 px-2 py-1 rounded">
            MJPEG (H.264 unavailable on this browser)
          </div>
        )}
      </div>
    </div>
  )
}
