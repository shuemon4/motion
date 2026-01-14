import { useRef, useEffect, useState } from 'react';

/**
 * FPS Tracker Hook
 *
 * Calculates client-side FPS for MJPEG streams by tracking frame updates.
 * Uses image onload events to track actual frame arrivals instead of RAF.
 * Returns the current FPS as a rolling average over the last second.
 */
export function useFpsTracker(imgRef: React.RefObject<HTMLImageElement | null>) {
  const [fps, setFps] = useState(0);
  const frameTimestamps = useRef<number[]>([]);
  const lastUpdateTime = useRef(0);

  useEffect(() => {
    const img = imgRef.current;
    if (!img) return;

    const handleLoad = () => {
      const now = performance.now();
      frameTimestamps.current.push(now);

      // Keep only frames from last second
      const oneSecondAgo = now - 1000;
      while (frameTimestamps.current.length > 0 && frameTimestamps.current[0] < oneSecondAgo) {
        frameTimestamps.current.shift();
      }

      // Only update FPS state once per second to avoid excessive re-renders
      // This prevents render loops when parent components respond to FPS changes
      if (now - lastUpdateTime.current >= 1000) {
        setFps(frameTimestamps.current.length);
        lastUpdateTime.current = now;
      }
    };

    img.addEventListener('load', handleLoad);
    return () => {
      img.removeEventListener('load', handleLoad);
      frameTimestamps.current = [];
    };
  }, [imgRef]);

  return fps;
}
