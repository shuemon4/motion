import { useState, useEffect, useRef } from 'react'
import { useWebRTC } from '@/hooks/useWebRTC'

interface WebRTCStreamProps {
  cameraId: number
  className?: string
  onFallback?: () => void
}

/**
 * Connection state badge shown as an overlay on the video.
 * Fades out 3 seconds after a successful connection.
 */
function ConnectionBadge({
  state,
}: {
  state: 'connecting' | 'connected' | 'disconnected' | 'failed'
}) {
  const [visible, setVisible] = useState(true)
  const fadeTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null)

  // Show badge whenever state changes (render-time adjustment pattern)
  const [prevState, setPrevState] = useState(state)
  if (prevState !== state) {
    setPrevState(state)
    setVisible(true)
  }

  // Auto-hide after 3 s when connected
  useEffect(() => {
    if (fadeTimerRef.current) {
      clearTimeout(fadeTimerRef.current)
      fadeTimerRef.current = null
    }

    if (state === 'connected') {
      fadeTimerRef.current = setTimeout(() => setVisible(false), 3000)
    }

    return () => {
      if (fadeTimerRef.current) {
        clearTimeout(fadeTimerRef.current)
      }
    }
  }, [state])

  if (!visible) return null

  let dotColor: string
  let label: string

  switch (state) {
    case 'connecting':
      dotColor = 'bg-yellow-400'
      label = 'Connecting...'
      break
    case 'connected':
      dotColor = 'bg-green-500'
      label = 'WebRTC'
      break
    case 'disconnected':
      dotColor = 'bg-gray-400'
      label = 'Disconnected'
      break
    case 'failed':
      dotColor = 'bg-red-500'
      label = 'Stream unavailable'
      break
  }

  return (
    <div
      className={`absolute top-2 right-2 z-10 flex items-center gap-1.5 px-2 py-1 rounded-full bg-black/60 text-xs text-white transition-opacity duration-500 ${
        state === 'connected' ? 'opacity-80' : 'opacity-100'
      }`}
      role="status"
      aria-live="polite"
    >
      {state === 'connecting' ? (
        <svg
          className="w-3 h-3 animate-spin text-yellow-400"
          fill="none"
          viewBox="0 0 24 24"
          aria-hidden="true"
        >
          <circle
            className="opacity-25"
            cx="12"
            cy="12"
            r="10"
            stroke="currentColor"
            strokeWidth="4"
          />
          <path
            className="opacity-75"
            fill="currentColor"
            d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"
          />
        </svg>
      ) : (
        <span className={`w-2 h-2 rounded-full ${dotColor}`} aria-hidden="true" />
      )}
      <span>{label}</span>
    </div>
  )
}

/**
 * WebRTC video player component.
 *
 * Renders a <video> element driven by the useWebRTC hook.
 * Shows a connection state badge overlay and calls onFallback
 * when the WebRTC connection fails (so CameraStream can switch
 * to MJPEG mode).
 */
export function WebRTCStream({
  cameraId,
  className = '',
  onFallback,
}: WebRTCStreamProps) {
  const { videoRef, connectionState } = useWebRTC({
    cameraId,
    enabled: true,
    onFallback,
  })

  return (
    <div className={`relative w-full ${className}`}>
      <video
        ref={videoRef}
        autoPlay
        playsInline
        muted
        className="absolute inset-0 w-full h-full object-contain bg-black"
        aria-label={`Camera ${cameraId} WebRTC stream`}
      />
      <ConnectionBadge state={connectionState} />
    </div>
  )
}
