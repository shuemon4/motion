import { useRef, useState, useCallback, useEffect } from 'react'
import { useCameraStream } from '@/hooks/useCameraStream'

interface CameraStreamProps {
  cameraId: number
  className?: string
}

// Custom event name for camera restart notification
export const CAMERA_RESTARTED_EVENT = 'camera-restarted'

export function CameraStream({ cameraId, className = '' }: CameraStreamProps) {
  const { streamUrl, isLoading, error } = useCameraStream(cameraId)
  const imgRef = useRef<HTMLImageElement>(null)
  const [streamKey, setStreamKey] = useState(0)

  // Force stream reconnection by changing key
  const handleStreamError = useCallback(() => {
    // Add small delay before retry to avoid hammering
    setTimeout(() => {
      setStreamKey((k) => k + 1)
    }, 2000)
  }, [])

  // Listen for camera restart events to force reconnection
  useEffect(() => {
    const handleCameraRestarted = (event: CustomEvent<{ cameraId?: number }>) => {
      // Reconnect if event is for this camera or all cameras (cameraId undefined or 0)
      const eventCamId = event.detail?.cameraId
      if (!eventCamId || eventCamId === 0 || eventCamId === cameraId) {
        // Force new connection by incrementing key
        setStreamKey((k) => k + 1)
      }
    }

    window.addEventListener(CAMERA_RESTARTED_EVENT, handleCameraRestarted as EventListener)
    return () => {
      window.removeEventListener(CAMERA_RESTARTED_EVENT, handleCameraRestarted as EventListener)
    }
  }, [cameraId])

  // Reset stream key when camera changes
  useEffect(() => {
    setStreamKey(0)
  }, [cameraId])

  if (error) {
    return (
      <div className={`w-full ${className}`}>
        <div className="aspect-video flex items-center justify-center bg-gray-900 rounded-lg">
          <div className="text-center p-4">
            <svg className="w-12 h-12 mx-auto text-red-500 mb-2" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
            </svg>
            <p className="text-red-500 text-sm">{error}</p>
          </div>
        </div>
      </div>
    )
  }

  if (isLoading) {
    return (
      <div className={`w-full ${className}`}>
        <div className="relative aspect-video bg-gray-900 animate-pulse rounded-lg">
          {/* Loading spinner in top-right corner */}
          <div className="absolute top-4 right-4">
            <svg className="w-8 h-8 text-gray-600 animate-spin" fill="none" viewBox="0 0 24 24">
              <circle className="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" strokeWidth="4"></circle>
              <path className="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8V0C5.373 0 0 5.373 0 12h4zm2 5.291A7.962 7.962 0 014 12H0c0 3.042 1.135 5.824 3 7.938l3-2.647z"></path>
            </svg>
          </div>
        </div>
      </div>
    )
  }

  // Add cache buster to stream URL when key changes (forces reconnection)
  const streamUrlWithKey = streamUrl
    ? `${streamUrl}${streamUrl.includes('?') ? '&' : '?'}_k=${streamKey}`
    : ''

  return (
    <div className={`w-full ${className}`}>
      <div className="relative aspect-video bg-black rounded-lg overflow-hidden">
        <img
          ref={imgRef}
          src={streamUrlWithKey}
          alt={`Camera ${cameraId} stream`}
          className="absolute inset-0 w-full h-full object-contain"
          onError={handleStreamError}
        />
      </div>
    </div>
  )
}
