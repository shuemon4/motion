import { useState, useRef, useEffect, type ReactNode } from 'react'

interface HelpTooltipProps {
  content: ReactNode
}

export function HelpTooltip({ content }: HelpTooltipProps) {
  const [open, setOpen] = useState(false)
  const ref = useRef<HTMLDivElement>(null)

  useEffect(() => {
    if (!open) return
    const handleClick = (e: MouseEvent) => {
      if (ref.current && !ref.current.contains(e.target as Node)) {
        setOpen(false)
      }
    }
    document.addEventListener('mousedown', handleClick)
    return () => document.removeEventListener('mousedown', handleClick)
  }, [open])

  return (
    <div ref={ref} className="relative inline-block ml-1.5">
      <button
        type="button"
        onClick={() => setOpen((v) => !v)}
        onMouseEnter={() => setOpen(true)}
        onMouseLeave={() => setOpen(false)}
        className="inline-flex items-center justify-center w-4 h-4 rounded-full bg-gray-600 hover:bg-gray-500 text-gray-300 text-[10px] font-bold leading-none cursor-help transition-colors"
        aria-label="More information"
      >
        ?
      </button>
      {open && (
        <div className="absolute z-50 left-1/2 -translate-x-1/2 mt-2 w-64 p-3 rounded-lg bg-surface-elevated border border-surface-elevated shadow-lg text-xs text-gray-300 leading-relaxed">
          {content}
        </div>
      )}
    </div>
  )
}
