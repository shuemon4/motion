import { useEffect, useRef, useState, useCallback } from 'react'
import { getSessionToken, getCsrfToken } from '@/api/session'

type ConnectionState = 'connecting' | 'connected' | 'disconnected' | 'failed'

interface UseWebRTCOptions {
  cameraId: number
  enabled: boolean
  onFallback?: () => void
}

interface UseWebRTCReturn {
  videoRef: React.RefObject<HTMLVideoElement | null>
  connectionState: ConnectionState
  dataChannel: RTCDataChannel | null
}

/** Timeout before triggering MJPEG fallback (ms) */
const CONNECTION_TIMEOUT_MS = 5000

/**
 * Build common headers for WebRTC signaling requests.
 * Reuses the same session/CSRF token pattern as apiPost in client.ts.
 */
function getAuthHeaders(): Record<string, string> {
  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    'Accept': 'application/json',
  }
  const sessionToken = getSessionToken()
  if (sessionToken) {
    headers['X-Session-Token'] = sessionToken
  }
  const csrfToken = getCsrfToken()
  if (csrfToken) {
    headers['X-CSRF-Token'] = csrfToken
  }
  return headers
}

/**
 * React hook for WebRTC connection lifecycle management.
 *
 * Creates an RTCPeerConnection, performs SDP offer/answer exchange with the
 * Motion backend, handles ICE candidate trickle, and attaches the incoming
 * video track to a <video> element via the returned ref.
 *
 * Falls back (calls onFallback) if no connection is established within 5 s.
 */
export function useWebRTC({
  cameraId,
  enabled,
  onFallback,
}: UseWebRTCOptions): UseWebRTCReturn {
  const videoRef = useRef<HTMLVideoElement | null>(null)
  const pcRef = useRef<RTCPeerConnection | null>(null)
  const timeoutRef = useRef<ReturnType<typeof setTimeout> | null>(null)
  const [connectionState, setConnectionState] = useState<ConnectionState>('disconnected')
  const [dataChannel, setDataChannel] = useState<RTCDataChannel | null>(null)

  // Keep a stable ref to onFallback so we don't re-run the effect when it changes
  const onFallbackRef = useRef(onFallback)
  onFallbackRef.current = onFallback

  /**
   * Tear down the peer connection and release all resources.
   */
  const cleanup = useCallback(() => {
    if (timeoutRef.current) {
      clearTimeout(timeoutRef.current)
      timeoutRef.current = null
    }

    const pc = pcRef.current
    if (pc) {
      // Remove event listeners to prevent callbacks after cleanup
      pc.ontrack = null
      pc.onicecandidate = null
      pc.oniceconnectionstatechange = null
      pc.onconnectionstatechange = null
      pc.ondatachannel = null

      // Close all senders/receivers
      pc.getSenders().forEach((sender) => {
        if (sender.track) {
          sender.track.stop()
        }
      })

      pc.close()
      pcRef.current = null
    }

    // Clear video source
    if (videoRef.current) {
      videoRef.current.srcObject = null
    }

    setDataChannel(null)
  }, [])

  useEffect(() => {
    if (!enabled) {
      cleanup()
      setConnectionState('disconnected')
      return
    }

    let cancelled = false

    async function connect() {
      setConnectionState('connecting')

      // ICE servers — STUN only (no TURN needed for same-network deployment)
      const pc = new RTCPeerConnection({
        iceServers: [{ urls: 'stun:stun.l.google.com:19302' }],
      })
      pcRef.current = pc

      // --- Handle incoming video track ---
      pc.ontrack = (event) => {
        if (cancelled) return
        if (event.track.kind === 'video' && videoRef.current) {
          const stream = new MediaStream([event.track])
          videoRef.current.srcObject = stream
        }
      }

      // --- Handle incoming data channel (for future PTZ control) ---
      pc.ondatachannel = (event) => {
        if (cancelled) return
        setDataChannel(event.channel)
      }

      // --- Trickle ICE candidates to the backend ---
      pc.onicecandidate = (event) => {
        if (cancelled || !event.candidate) return
        // Fire-and-forget — candidate delivery is best-effort
        fetch(`/${cameraId}/api/webrtc/candidate`, {
          method: 'POST',
          headers: getAuthHeaders(),
          credentials: 'same-origin',
          body: JSON.stringify({
            candidate: event.candidate.candidate,
            sdpMid: event.candidate.sdpMid,
            sdpMLineIndex: event.candidate.sdpMLineIndex,
          }),
        }).catch(() => {
          // Ignore candidate delivery failures — connection may still succeed
        })
      }

      // --- Track connection state ---
      pc.onconnectionstatechange = () => {
        if (cancelled) return
        switch (pc.connectionState) {
          case 'connected':
            setConnectionState('connected')
            // Clear the timeout — we are connected
            if (timeoutRef.current) {
              clearTimeout(timeoutRef.current)
              timeoutRef.current = null
            }
            break
          case 'disconnected':
            setConnectionState('disconnected')
            break
          case 'failed':
            setConnectionState('failed')
            onFallbackRef.current?.()
            break
          case 'closed':
            setConnectionState('disconnected')
            break
        }
      }

      // Also watch ICE connection state for browsers that update it before
      // connectionState (e.g. some Safari versions)
      pc.oniceconnectionstatechange = () => {
        if (cancelled) return
        if (pc.iceConnectionState === 'connected' || pc.iceConnectionState === 'completed') {
          setConnectionState('connected')
          if (timeoutRef.current) {
            clearTimeout(timeoutRef.current)
            timeoutRef.current = null
          }
        } else if (pc.iceConnectionState === 'failed') {
          setConnectionState('failed')
          onFallbackRef.current?.()
        }
      }

      // --- Set up transceiver to receive video ---
      pc.addTransceiver('video', { direction: 'recvonly' })

      // --- Create and send SDP offer ---
      try {
        const offer = await pc.createOffer()
        await pc.setLocalDescription(offer)

        const response = await fetch(`/${cameraId}/api/webrtc/offer`, {
          method: 'POST',
          headers: getAuthHeaders(),
          credentials: 'same-origin',
          body: JSON.stringify({ sdp: offer.sdp }),
        })

        if (cancelled) return

        if (!response.ok) {
          setConnectionState('failed')
          onFallbackRef.current?.()
          return
        }

        const answer = await response.json()

        if (cancelled) return

        await pc.setRemoteDescription(
          new RTCSessionDescription({ type: 'answer', sdp: answer.sdp })
        )
      } catch {
        if (cancelled) return
        setConnectionState('failed')
        onFallbackRef.current?.()
        return
      }

      // --- Start connection timeout ---
      timeoutRef.current = setTimeout(() => {
        if (cancelled) return
        // If still not connected after timeout, fallback
        if (
          pc.connectionState !== 'connected' &&
          pc.iceConnectionState !== 'connected' &&
          pc.iceConnectionState !== 'completed'
        ) {
          setConnectionState('failed')
          onFallbackRef.current?.()
        }
      }, CONNECTION_TIMEOUT_MS)
    }

    connect()

    return () => {
      cancelled = true
      cleanup()
    }
  }, [cameraId, enabled, cleanup])

  return { videoRef, connectionState, dataChannel }
}
