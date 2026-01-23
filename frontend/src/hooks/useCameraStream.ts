import { useState, useEffect } from 'react'
import { getSessionToken } from '@/api/session'

export function useCameraStream(cameraId: number) {
  const [streamUrl, setStreamUrl] = useState<string>('')
  const [isLoading, setIsLoading] = useState(true)

  useEffect(() => {
    // MJPEG stream URL - Motion uses /camId/mjpg/stream
    // Include session token as query param for authentication
    // (img tags can't send custom headers)
    const token = getSessionToken()
    const baseUrl = `/${cameraId}/mjpg/stream`
    const url = token ? `${baseUrl}?token=${encodeURIComponent(token)}` : baseUrl

    // Set URL directly - MJPEG streams don't work with Image.onload testing
    // The CameraStream component handles errors via onError
    setStreamUrl(url)
    setIsLoading(false)

    return () => {
      // Cleanup - clear URL to stop stream
      setStreamUrl('')
    }
  }, [cameraId])

  return { streamUrl, isLoading, error: null }
}
