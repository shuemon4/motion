import { useState, useEffect } from 'react'
import { getSessionToken } from '@/api/session'

export function useCameraStream(cameraId: number) {
  const [streamUrl, setStreamUrl] = useState<string>('')
  const [isLoading, setIsLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    // MJPEG stream URL - Motion uses /camId/mjpg/stream
    // Include session token as query param for authentication
    // (img tags can't send custom headers)
    const token = getSessionToken()
    const baseUrl = `/${cameraId}/mjpg/stream`
    const url = token ? `${baseUrl}?token=${encodeURIComponent(token)}` : baseUrl

    // Test if stream is available
    const img = new Image()
    img.onload = () => {
      setStreamUrl(url)
      setIsLoading(false)
    }
    img.onerror = () => {
      setError('Stream not available')
      setIsLoading(false)
    }
    img.src = url

    return () => {
      img.onload = null
      img.onerror = null
    }
  }, [cameraId])

  return { streamUrl, isLoading, error }
}
