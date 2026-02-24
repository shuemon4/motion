import { useState, useEffect, useRef, useCallback } from 'react'

interface SnapshotPollingState {
  snapshotUrl: string
  isConnected: boolean
  error: string | null
}

export function useSnapshotPolling(
  cameraId: number,
  intervalMs: number = 1000,
  enabled: boolean = true
): SnapshotPollingState {
  const [timestamp, setTimestamp] = useState(Date.now())
  const [isConnected] = useState(false)
  const [error] = useState<string | null>(null)
  const intervalRef = useRef<ReturnType<typeof setInterval> | null>(null)

  const startPolling = useCallback(() => {
    if (intervalRef.current) return
    intervalRef.current = setInterval(() => {
      setTimestamp(Date.now())
    }, intervalMs)
  }, [intervalMs])

  const stopPolling = useCallback(() => {
    if (intervalRef.current) {
      clearInterval(intervalRef.current)
      intervalRef.current = null
    }
  }, [])

  useEffect(() => {
    if (!enabled) {
      stopPolling()
      return
    }

    startPolling()

    const handleVisibility = () => {
      if (document.hidden) {
        stopPolling()
      } else {
        setTimestamp(Date.now())
        startPolling()
      }
    }

    document.addEventListener('visibilitychange', handleVisibility)

    return () => {
      stopPolling()
      document.removeEventListener('visibilitychange', handleVisibility)
    }
  }, [enabled, startPolling, stopPolling])

  const snapshotUrl = `/${cameraId}/static/stream?_t=${timestamp}`

  return { snapshotUrl, isConnected, error }
}
