import { createContext, useContext, useState, useCallback, useEffect, type ReactNode } from 'react'

type ToastType = 'success' | 'error' | 'info' | 'warning'

interface Toast {
  id: string
  message: string
  type: ToastType
  timeoutId?: ReturnType<typeof setTimeout>
}

interface ToastContextType {
  toasts: Toast[]
  addToast: (message: string, type?: ToastType) => void
  removeToast: (id: string) => void
}

const ToastContext = createContext<ToastContextType | null>(null)

export function ToastProvider({ children }: { children: ReactNode }) {
  const [toasts, setToasts] = useState<Toast[]>([])

  const addToast = useCallback((message: string, type: ToastType = 'info') => {
    const id = Math.random().toString(36).slice(2)

    // Auto-remove after 5 seconds and store timeout ID
    const timeoutId = setTimeout(() => {
      setToasts((prev) => prev.filter((t) => t.id !== id))
    }, 5000)

    setToasts((prev) => [...prev, { id, message, type, timeoutId }])
  }, [])

  const removeToast = useCallback((id: string) => {
    setToasts((prev) => {
      const toast = prev.find((t) => t.id === id)
      if (toast?.timeoutId) {
        clearTimeout(toast.timeoutId)
      }
      return prev.filter((t) => t.id !== id)
    })
  }, [])

  // Cleanup all timeouts on unmount
  useEffect(() => {
    return () => {
      toasts.forEach((toast) => {
        if (toast.timeoutId) {
          clearTimeout(toast.timeoutId)
        }
      })
    }
  }, [toasts])

  return (
    <ToastContext.Provider value={{ toasts, addToast, removeToast }}>
      {children}
      <ToastContainer toasts={toasts} onRemove={removeToast} />
    </ToastContext.Provider>
  )
}

export function useToast() {
  const context = useContext(ToastContext)
  if (!context) throw new Error('useToast must be used within ToastProvider')
  return context
}

function ToastContainer({
  toasts,
  onRemove,
}: {
  toasts: Toast[]
  onRemove: (id: string) => void
}) {
  if (toasts.length === 0) return null

  const typeStyles: Record<ToastType, string> = {
    success: 'bg-green-600',
    error: 'bg-danger',
    warning: 'bg-yellow-600',
    info: 'bg-primary',
  }

  return (
    <div className="fixed bottom-4 right-4 z-50 flex flex-col gap-2">
      {toasts.map((toast) => (
        <div
          key={toast.id}
          className={`${typeStyles[toast.type]} text-white px-4 py-3 rounded-lg shadow-lg flex items-center gap-3 min-w-[250px] animate-slide-in`}
        >
          <span className="flex-1">{toast.message}</span>
          <button
            onClick={() => onRemove(toast.id)}
            className="text-white/70 hover:text-white"
          >
            &times;
          </button>
        </div>
      ))}
    </div>
  )
}
