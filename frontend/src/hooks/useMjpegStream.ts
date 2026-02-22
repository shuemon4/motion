import { useState, useEffect, useRef, useCallback } from 'react'
import { getSessionToken } from '@/api/session'

interface MjpegStreamState {
  imageUrl: string | null
  streamFps: number
  isConnected: boolean
  error: string | null
}

/**
 * MJPEG stream parser that counts frames client-side for accurate FPS measurement.
 * Parses the multipart/x-mixed-replace stream and extracts JPEG frames.
 */
export function useMjpegStream(cameraId: number, streamKey: number) {
  const [state, setState] = useState<MjpegStreamState>({
    imageUrl: null,
    streamFps: 0,
    isConnected: false,
    error: null,
  })

  // Track frame timestamps for FPS calculation
  const frameTimestamps = useRef<number[]>([])
  const abortControllerRef = useRef<AbortController | null>(null)
  const currentImageUrl = useRef<string | null>(null)

  // Calculate FPS from recent frames (called every second)
  const calculateFps = useCallback(() => {
    const now = Date.now()
    const oneSecondAgo = now - 1000
    // Keep only frames from last second
    frameTimestamps.current = frameTimestamps.current.filter((t) => t > oneSecondAgo)
    return frameTimestamps.current.length
  }, [])

  useEffect(() => {
    // FPS update interval
    const fpsInterval = setInterval(() => {
      const fps = calculateFps()
      setState((prev) => ({ ...prev, streamFps: fps }))
    }, 1000)

    return () => clearInterval(fpsInterval)
  }, [calculateFps])

  useEffect(() => {
    // Abort any existing stream
    if (abortControllerRef.current) {
      abortControllerRef.current.abort()
    }

    // Clean up previous image URL
    if (currentImageUrl.current) {
      URL.revokeObjectURL(currentImageUrl.current)
      currentImageUrl.current = null
    }

    // Reset state
    frameTimestamps.current = []
    setState({
      imageUrl: null,
      streamFps: 0,
      isConnected: false,
      error: null,
    })

    const abortController = new AbortController()
    abortControllerRef.current = abortController

    const startStream = async () => {
      try {
        const token = getSessionToken()
        const params = new URLSearchParams()
        params.set('_k', String(streamKey))
        const url = `/${cameraId}/mjpg/stream?` + params.toString()

        const response = await fetch(url, {
          signal: abortController.signal,
          credentials: 'same-origin',
          headers: token ? { 'X-Session-Token': token } : undefined,
        })

        if (!response.ok) {
          throw new Error(`HTTP ${response.status}: ${response.statusText}`)
        }

        if (!response.body) {
          throw new Error('No response body')
        }

        setState((prev) => ({ ...prev, isConnected: true, error: null }))

        // Parse the multipart stream
        const reader = response.body.getReader()
        let buffer = new Uint8Array(128 * 1024) // 128KB initial capacity
        let bufferLen = 0

        // Parse MJPEG stream looking for JPEG frame boundaries
        // JPEG markers: SOI = 0xFF 0xD8 (start), EOI = 0xFF 0xD9 (end)
        while (true) {
          const { done, value } = await reader.read()

          if (done) {
            break
          }

          // Append new data to buffer with capacity doubling
          if (bufferLen + value.length > buffer.length) {
            const newCapacity = Math.max(buffer.length * 2, bufferLen + value.length)
            const newBuffer = new Uint8Array(newCapacity)
            newBuffer.set(buffer.subarray(0, bufferLen))
            buffer = newBuffer
          }
          buffer.set(value, bufferLen)
          bufferLen += value.length

          // Look for complete JPEG frames in buffer
          let searchStart = 0
          let lastJpegData: Uint8Array | null = null
          let compactFrom = 0

          while (searchStart < bufferLen - 1) {
            // Find SOI marker (0xFF 0xD8)
            let soiIndex = -1
            for (let i = searchStart; i < bufferLen - 1; i++) {
              if (buffer[i] === 0xff && buffer[i + 1] === 0xd8) {
                soiIndex = i
                break
              }
            }

            if (soiIndex === -1) {
              // No SOI found — discard all but last byte (might be partial marker)
              compactFrom = bufferLen - 1
              break
            }

            // Find EOI marker (0xFF 0xD9) after SOI
            let eoiIndex = -1
            for (let i = soiIndex + 2; i < bufferLen - 1; i++) {
              if (buffer[i] === 0xff && buffer[i + 1] === 0xd9) {
                eoiIndex = i
                break
              }
            }

            if (eoiIndex === -1) {
              // Incomplete frame — keep from SOI onwards
              compactFrom = soiIndex
              break
            }

            // Complete frame — extract independent copy (slice, not subarray)
            lastJpegData = buffer.slice(soiIndex, eoiIndex + 2)
            frameTimestamps.current.push(Date.now())

            searchStart = eoiIndex + 2
            compactFrom = searchStart
          }

          // Single compaction point — shift unprocessed data to beginning
          if (compactFrom > 0) {
            if (compactFrom >= bufferLen) {
              bufferLen = 0
            } else {
              buffer.copyWithin(0, compactFrom, bufferLen)
              bufferLen = bufferLen - compactFrom
            }
          }

          // Only create Blob URL for the last frame (frame dropping optimization)
          if (lastJpegData) {
            const blob = new Blob([new Uint8Array(lastJpegData)], { type: 'image/jpeg' })
            const newUrl = URL.createObjectURL(blob)

            // Revoke previous URL to prevent memory leak
            if (currentImageUrl.current) {
              URL.revokeObjectURL(currentImageUrl.current)
            }
            currentImageUrl.current = newUrl

            setState((prev) => ({
              ...prev,
              imageUrl: newUrl,
            }))
          }
        }
      } catch (err) {
        if (err instanceof Error && err.name === 'AbortError') {
          // Stream was intentionally aborted
          return
        }

        console.error('MJPEG stream error:', err)
        setState((prev) => ({
          ...prev,
          isConnected: false,
          error: err instanceof Error ? err.message : 'Stream error',
        }))
      }
    }

    startStream()

    return () => {
      abortController.abort()
      if (currentImageUrl.current) {
        URL.revokeObjectURL(currentImageUrl.current)
        currentImageUrl.current = null
      }
    }
  }, [cameraId, streamKey])

  return state
}
