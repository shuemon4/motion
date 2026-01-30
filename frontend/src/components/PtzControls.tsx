import { useState, useEffect, useCallback, useRef } from 'react';
import { usePtzCapabilities, useSendPtzCommand } from '@/api/queries';

interface PtzControlsProps {
  cameraId: number;
}

export function PtzControls({ cameraId }: PtzControlsProps) {
  const { data: capabilities } = usePtzCapabilities(cameraId);
  const sendPtzCommand = useSendPtzCommand();

  const [isVisible, setIsVisible] = useState(false);
  const [isMobile, setIsMobile] = useState(false);
  const hideTimeoutRef = useRef<number | null>(null);
  const repeatIntervalRef = useRef<number | null>(null);

  // Detect if device is mobile
  useEffect(() => {
    const checkMobile = () => {
      setIsMobile(window.matchMedia('(max-width: 768px)').matches || 'ontouchstart' in window);
    };
    checkMobile();
    window.addEventListener('resize', checkMobile);
    return () => window.removeEventListener('resize', checkMobile);
  }, []);

  // Handle auto-hide on mobile
  const scheduleHide = useCallback(() => {
    if (hideTimeoutRef.current) {
      clearTimeout(hideTimeoutRef.current);
    }
    hideTimeoutRef.current = setTimeout(() => {
      setIsVisible(false);
    }, 3000);
  }, []);

  const handleToggleVisibility = useCallback(() => {
    if (isMobile) {
      setIsVisible((prev) => {
        const newVisible = !prev;
        if (newVisible) {
          scheduleHide();
        }
        return newVisible;
      });
    }
  }, [isMobile, scheduleHide]);

  const handleMouseEnter = useCallback(() => {
    if (!isMobile) {
      setIsVisible(true);
    }
  }, [isMobile]);

  const handleMouseLeave = useCallback(() => {
    if (!isMobile) {
      setIsVisible(false);
    }
  }, [isMobile]);

  // Cleanup timeouts on unmount
  useEffect(() => {
    return () => {
      if (hideTimeoutRef.current) clearTimeout(hideTimeoutRef.current);
      if (repeatIntervalRef.current) clearInterval(repeatIntervalRef.current);
    };
  }, []);

  const sendCommand = useCallback((action: string) => {
    sendPtzCommand.mutate({ camId: cameraId, action });
  }, [cameraId, sendPtzCommand]);

  const handlePressStart = useCallback((action: string) => {
    // Send immediately
    sendCommand(action);

    // Start repeating after 500ms
    if (repeatIntervalRef.current) {
      clearInterval(repeatIntervalRef.current);
    }
    repeatIntervalRef.current = setInterval(() => {
      sendCommand(action);
    }, 500);
  }, [sendCommand]);

  const handlePressEnd = useCallback(() => {
    // Stop repeating
    if (repeatIntervalRef.current) {
      clearInterval(repeatIntervalRef.current);
      repeatIntervalRef.current = null;
    }
  }, []);

  // Don't render if PTZ is not enabled or no controls are available
  if (!capabilities?.enabled || !capabilities?.hasAnyControl) {
    return null;
  }

  const buttonClass = "w-10 h-10 bg-black/70 hover:bg-black/90 rounded-lg flex items-center justify-center text-white transition-colors touch-none select-none active:bg-primary";
  const disabledButtonClass = "w-10 h-10 bg-transparent rounded-lg"; // Invisible placeholder for layout

  return (
    <>
      {/* Click/tap target for mobile */}
      {isMobile && (
        <div
          className="absolute inset-0 z-10"
          onClick={handleToggleVisibility}
          aria-label="Toggle PTZ controls"
        />
      )}

      {/* PTZ Controls Overlay */}
      <div
        className="absolute inset-0 z-20 pointer-events-none"
        onMouseEnter={handleMouseEnter}
        onMouseLeave={handleMouseLeave}
      >
        <div
          className={`absolute bottom-4 right-4 flex gap-4 pointer-events-auto transition-opacity duration-200 ${
            isVisible ? 'opacity-100' : 'opacity-0'
          }`}
        >
          {/* D-pad (Pan/Tilt) */}
          <div className="flex flex-col items-center gap-1">
            {/* Up */}
            <div>
              {capabilities.hasTiltUp ? (
                <button
                  className={buttonClass}
                  onMouseDown={() => handlePressStart('up')}
                  onMouseUp={handlePressEnd}
                  onMouseLeave={handlePressEnd}
                  onTouchStart={() => handlePressStart('up')}
                  onTouchEnd={handlePressEnd}
                  aria-label="Tilt up"
                >
                  <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M5 15l7-7 7 7" />
                  </svg>
                </button>
              ) : (
                <div className={disabledButtonClass} />
              )}
            </div>

            {/* Left - Center - Right */}
            <div className="flex gap-1">
              {capabilities.hasPanLeft ? (
                <button
                  className={buttonClass}
                  onMouseDown={() => handlePressStart('left')}
                  onMouseUp={handlePressEnd}
                  onMouseLeave={handlePressEnd}
                  onTouchStart={() => handlePressStart('left')}
                  onTouchEnd={handlePressEnd}
                  aria-label="Pan left"
                >
                  <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 19l-7-7 7-7" />
                  </svg>
                </button>
              ) : (
                <div className={disabledButtonClass} />
              )}

              <div className={disabledButtonClass} /> {/* Center placeholder */}

              {capabilities.hasPanRight ? (
                <button
                  className={buttonClass}
                  onMouseDown={() => handlePressStart('right')}
                  onMouseUp={handlePressEnd}
                  onMouseLeave={handlePressEnd}
                  onTouchStart={() => handlePressStart('right')}
                  onTouchEnd={handlePressEnd}
                  aria-label="Pan right"
                >
                  <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 5l7 7-7 7" />
                  </svg>
                </button>
              ) : (
                <div className={disabledButtonClass} />
              )}
            </div>

            {/* Down */}
            <div>
              {capabilities.hasTiltDown ? (
                <button
                  className={buttonClass}
                  onMouseDown={() => handlePressStart('down')}
                  onMouseUp={handlePressEnd}
                  onMouseLeave={handlePressEnd}
                  onTouchStart={() => handlePressStart('down')}
                  onTouchEnd={handlePressEnd}
                  aria-label="Tilt down"
                >
                  <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M19 9l-7 7-7-7" />
                  </svg>
                </button>
              ) : (
                <div className={disabledButtonClass} />
              )}
            </div>
          </div>

          {/* Zoom Controls */}
          {(capabilities.hasZoomIn || capabilities.hasZoomOut) && (
            <div className="flex flex-col gap-1">
              {capabilities.hasZoomIn ? (
                <button
                  className={buttonClass}
                  onMouseDown={() => handlePressStart('zoom_in')}
                  onMouseUp={handlePressEnd}
                  onMouseLeave={handlePressEnd}
                  onTouchStart={() => handlePressStart('zoom_in')}
                  onTouchEnd={handlePressEnd}
                  aria-label="Zoom in"
                >
                  <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M12 4v16m8-8H4" />
                  </svg>
                </button>
              ) : (
                <div className={disabledButtonClass} />
              )}

              <div className={disabledButtonClass} /> {/* Spacer */}

              {capabilities.hasZoomOut ? (
                <button
                  className={buttonClass}
                  onMouseDown={() => handlePressStart('zoom_out')}
                  onMouseUp={handlePressEnd}
                  onMouseLeave={handlePressEnd}
                  onTouchStart={() => handlePressStart('zoom_out')}
                  onTouchEnd={handlePressEnd}
                  aria-label="Zoom out"
                >
                  <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M20 12H4" />
                  </svg>
                </button>
              ) : (
                <div className={disabledButtonClass} />
              )}
            </div>
          )}
        </div>
      </div>
    </>
  );
}
