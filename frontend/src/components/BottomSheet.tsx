import { useEffect, useCallback, useRef, type ReactNode } from 'react'
import { useSheetGestures } from '@/hooks/useSheetGestures'

interface BottomSheetProps {
  isOpen: boolean
  onClose: () => void
  title: string
  children: ReactNode
  headerRight?: ReactNode
}

export function BottomSheet({
  isOpen,
  onClose,
  title,
  children,
  headerRight,
}: BottomSheetProps) {
  const sheetRef = useRef<HTMLDivElement>(null)
  const contentRef = useRef<HTMLDivElement>(null)

  const { handlers, style } = useSheetGestures({
    isOpen,
    onClose,
    sheetRef,
  })

  // Handle escape key
  useEffect(() => {
    if (!isOpen) return

    const handleEscape = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        onClose()
      }
    }

    document.addEventListener('keydown', handleEscape)
    return () => document.removeEventListener('keydown', handleEscape)
  }, [isOpen, onClose])

  // Prevent body scroll when sheet is open
  useEffect(() => {
    if (isOpen) {
      document.body.style.overflow = 'hidden'
    } else {
      document.body.style.overflow = ''
    }
    return () => {
      document.body.style.overflow = ''
    }
  }, [isOpen])

  // Handle backdrop click
  const handleBackdropClick = useCallback(
    (e: React.MouseEvent) => {
      if (e.target === e.currentTarget) {
        onClose()
      }
    },
    [onClose]
  )

  if (!isOpen) return null

  return (
    <>
      {/* Backdrop */}
      <div
        className="fixed inset-0 bg-black/60 z-[100] transition-opacity duration-200"
        onClick={handleBackdropClick}
        aria-hidden="true"
      />

      {/* Sheet */}
      <div
        ref={sheetRef}
        className="fixed bottom-0 left-0 right-0 z-[101] bg-surface rounded-t-2xl shadow-2xl transition-transform duration-300 ease-out"
        style={{
          maxHeight: '85vh',
          transform: style.transform,
        }}
        role="dialog"
        aria-modal="true"
        aria-label={title}
        {...handlers}
      >
        {/* Drag Handle */}
        <div className="flex justify-center pt-3 pb-2 cursor-grab active:cursor-grabbing touch-none">
          <div className="w-12 h-1.5 bg-gray-500 rounded-full" />
        </div>

        {/* Header */}
        <div className="flex items-center justify-between px-4 pb-3 border-b border-surface-elevated">
          <h2 className="text-lg font-semibold">{title}</h2>
          <div className="flex items-center gap-3">
            {headerRight}
            <button
              type="button"
              onClick={onClose}
              className="p-2 hover:bg-surface-elevated rounded-full transition-colors"
              aria-label="Close"
            >
              <svg
                className="w-5 h-5"
                fill="none"
                stroke="currentColor"
                viewBox="0 0 24 24"
              >
                <path
                  strokeLinecap="round"
                  strokeLinejoin="round"
                  strokeWidth={2}
                  d="M6 18L18 6M6 6l12 12"
                />
              </svg>
            </button>
          </div>
        </div>

        {/* Content */}
        <div
          ref={contentRef}
          className="overflow-y-auto overscroll-contain px-4 py-4"
          style={{ maxHeight: 'calc(85vh - 80px)' }}
        >
          {children}
        </div>
      </div>
    </>
  )
}
