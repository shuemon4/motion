import { useState, useEffect, useRef } from 'react'
import { getSessionToken } from '@/api/session'

interface MpegTsStreamState {
  isConnected: boolean
  error: string | null
  streamFps: number
}

// Module-level cache so mpegts.js is only loaded once
let mpegtsPromise: Promise<typeof import('mpegts.js')> | null = null

function loadMpegTs() {
  if (!mpegtsPromise) {
    mpegtsPromise = import('mpegts.js')
  }
  return mpegtsPromise
}

/**
 * Detect if MSE or ManagedMediaSource (iOS 17.1+) is available for H.264 playback.
 * Returns true on all modern desktop browsers, Android Chrome, and iOS 17.1+.
 * Returns false on iOS < 17.1 and legacy browsers.
 */
export function isMseSupported(): boolean {
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const MSApi = (window as any).ManagedMediaSource || window.MediaSource
  if (!MSApi || typeof MSApi.isTypeSupported !== 'function') return false
  return MSApi.isTypeSupported('video/mp4; codecs="avc1.42E01E"')
}

function buildStreamUrl(cameraId: number, streamKey: number): string {
  const token = getSessionToken()
  const params = new URLSearchParams()
  params.set('_k', String(streamKey))
  if (token) {
    params.set('token', token)
  }
  return `/${cameraId}/mpegts/stream?${params.toString()}`
}

/**
 * MPEG-TS streaming hook using mpegts.js and the browser's Media Source Extensions API.
 * Provides H.264 hardware-accelerated playback with ~300ms-1.5s latency.
 * Falls back gracefully: callers should check isMseSupported() before using this hook.
 */
export function useMpegTsStream(
  cameraId: number,
  streamKey: number,
  videoRef: React.RefObject<HTMLVideoElement | null>
) {
  const [state, setState] = useState<MpegTsStreamState>({
    isConnected: false,
    error: null,
    streamFps: 0,
  })

  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  const playerRef = useRef<any>(null)
  const lastFrameCountRef = useRef<number>(0)
  const lastFpsTimeRef = useRef<number>(Date.now())

  useEffect(() => {
    let destroyed = false
    let playRetryTimer: ReturnType<typeof setTimeout> | null = null

    const tryPlay = (videoEl: HTMLVideoElement) => {
      if (destroyed || !videoEl) return
      videoEl.play().catch((err) => {
        console.warn('[mpegts] play() rejected:', err.message)
      })
    }

    const startStream = async () => {
      try {
        const mpegtsModule = await loadMpegTs()
        const mp = mpegtsModule.default

        if (destroyed || !videoRef.current) return

        if (!mp.getFeatureList().mseLivePlayback) {
          setState(prev => ({ ...prev, error: 'MSE not supported on this browser' }))
          return
        }

        const videoEl = videoRef.current

        // iOS ManagedMediaSource requires disableRemotePlayback.
        // Setting it immediately after DOM insertion can fail in Safari,
        // so we use a microtask delay.
        // eslint-disable-next-line @typescript-eslint/no-explicit-any
        if ((window as any).ManagedMediaSource) {
          queueMicrotask(() => {
            if (videoEl) {
              videoEl.disableRemotePlayback = true
            }
          })
        }

        const player = mp.createPlayer(
          {
            type: 'mpegts',
            isLive: true,
            url: buildStreamUrl(cameraId, streamKey),
          },
          {
            // Minimize latency for live surveillance
            enableStashBuffer: false,
            stashInitialSize: 384,
            // Disabled: MSE Worker can fail silently in some browser configs,
            // causing a black screen with no error events.
            enableWorkerForMSE: false,
            // Latency chasing: adjusts playback rate to stay near live edge
            liveBufferLatencyChasing: true,
            liveBufferLatencyMinRemain: 0.3,
            liveBufferLatencyMaxLatency: 1.5,
            // Auto-clean old segments to prevent memory growth
            autoCleanupSourceBuffer: true,
            autoCleanupMaxBackwardDuration: 3,
            autoCleanupMinBackwardDuration: 1,
          }
        )

        playerRef.current = player

        player.on(mp.Events.ERROR, (errorType: string, errorDetail: string, info: unknown) => {
          console.error('[mpegts] ERROR:', errorType, errorDetail, info)
          if (!destroyed) {
            setState(prev => ({
              ...prev,
              isConnected: false,
              error: `${errorType}: ${errorDetail}`,
            }))
          }
        })

        player.on(mp.Events.MEDIA_INFO, () => {
          console.info('[mpegts] Media info received')
        })

        player.on(mp.Events.STATISTICS_INFO, (stats: { decodedFrames: number; playerDuration: number }) => {
          if (!destroyed) {
            const now = Date.now()
            const elapsed = (now - lastFpsTimeRef.current) / 1000
            const frameDelta = stats.decodedFrames - lastFrameCountRef.current

            if (elapsed >= 1.0) {
              const fps = Math.round(frameDelta / elapsed)
              lastFpsTimeRef.current = now
              lastFrameCountRef.current = stats.decodedFrames
              setState(prev => ({ ...prev, streamFps: fps }))
            }
          }
        })

        player.attachMediaElement(videoEl)
        player.load()

        // Mark connected when browser actually starts rendering frames.
        // This is the definitive signal — browser-native, independent of
        // mpegts.js event quirks (MEDIA_INFO doesn't fire for all stream types).
        const onPlaying = () => {
          if (!destroyed) {
            console.info('[mpegts] Video playing — frames rendering')
            setState(prev => ({ ...prev, isConnected: true, error: null }))
          }
        }
        videoEl.addEventListener('playing', onPlaying, { once: true })

        // Primary play trigger: when browser has enough data buffered
        videoEl.addEventListener('canplay', () => {
          tryPlay(videoEl)
        }, { once: true })

        // Retry play after 2s if video hasn't started (handles race conditions
        // where canplay fires before listener is registered)
        playRetryTimer = setTimeout(() => {
          if (!destroyed && videoEl && videoEl.paused) {
            console.info('[mpegts] Play retry — video still paused after 2s')
            tryPlay(videoEl)
          }
        }, 2000)

      } catch (err) {
        console.error('[mpegts] Init error:', err)
        if (!destroyed) {
          setState(prev => ({
            ...prev,
            isConnected: false,
            error: err instanceof Error ? err.message : 'Failed to start stream',
          }))
        }
      }
    }

    startStream()

    return () => {
      destroyed = true
      if (playRetryTimer) clearTimeout(playRetryTimer)
      if (playerRef.current) {
        playerRef.current.pause()
        playerRef.current.unload()
        playerRef.current.detachMediaElement()
        playerRef.current.destroy()
        playerRef.current = null
      }
    }
  }, [cameraId, streamKey, videoRef])

  return state
}
