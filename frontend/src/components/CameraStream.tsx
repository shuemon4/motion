import { useCameraStream } from '@/hooks/useCameraStream'

interface CameraStreamProps {
  cameraId: number
  className?: string
}

export function CameraStream({ cameraId, className = '' }: CameraStreamProps) {
  const { streamUrl, isLoading, error } = useCameraStream(cameraId)

  if (error) {
    return (
      <div className={`bg-surface-elevated rounded-lg p-4 ${className}`}>
        <p className="text-red-500">Error: {error}</p>
      </div>
    )
  }

  if (isLoading) {
    return (
      <div className={`bg-surface-elevated rounded-lg p-4 animate-pulse ${className}`}>
        <div className="h-64 bg-surface-hover rounded"></div>
      </div>
    )
  }

  return (
    <div className={`relative ${className}`}>
      <img
        src={streamUrl}
        alt={`Camera ${cameraId} stream`}
        className="w-full h-auto rounded-lg"
      />
    </div>
  )
}
