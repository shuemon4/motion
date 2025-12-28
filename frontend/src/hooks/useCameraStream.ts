import { useState, useEffect } from 'react'

export function useCameraStream(cameraId: number) {
  const [streamUrl, setStreamUrl] = useState<string>('')
  const [isLoading, setIsLoading] = useState(true)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    // MJPEG stream URL
    const url = `/${cameraId}/stream`

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
