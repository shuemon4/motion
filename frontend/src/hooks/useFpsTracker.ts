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

      setFps(frameTimestamps.current.length);
    };

    img.addEventListener('load', handleLoad);
    return () => {
      img.removeEventListener('load', handleLoad);
      frameTimestamps.current = [];
    };
  }, [imgRef]);

  return fps;
}
