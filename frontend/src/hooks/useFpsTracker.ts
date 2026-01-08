import { useRef, useEffect, useState } from 'react';

/**
 * FPS Tracker Hook
 *
 * Calculates client-side FPS for MJPEG streams by tracking frame updates.
 * Returns the current FPS as a rolling average over the last second.
 */
export function useFpsTracker(imgRef: React.RefObject<HTMLImageElement | null>) {
  const [fps, setFps] = useState(0);
  const frameTimestamps = useRef<number[]>([]);
  const lastUpdateTime = useRef<number>(0);

  useEffect(() => {
    if (!imgRef.current) return;

    let animationFrameId: number;

    const trackFrame = () => {
      const now = performance.now();

      // Only track if image has actually updated (src changed)
      if (now - lastUpdateTime.current > 16) { // ~60fps max sampling
        frameTimestamps.current.push(now);
        lastUpdateTime.current = now;

        // Keep only timestamps from the last second
        const oneSecondAgo = now - 1000;
        frameTimestamps.current = frameTimestamps.current.filter(
          (timestamp) => timestamp > oneSecondAgo
        );

        // Calculate FPS as number of frames in the last second
        const currentFps = frameTimestamps.current.length;
        setFps(currentFps);
      }

      animationFrameId = requestAnimationFrame(trackFrame);
    };

    // Start tracking
    animationFrameId = requestAnimationFrame(trackFrame);

    return () => {
      cancelAnimationFrame(animationFrameId);
      frameTimestamps.current = [];
    };
  }, [imgRef]);

  return fps;
}
