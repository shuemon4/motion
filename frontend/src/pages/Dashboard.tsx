import { CameraStream } from '@/components/CameraStream'

export function Dashboard() {
  // TODO: Get camera list from API
  const cameras = [0, 1]

  return (
    <div>
      <h2 className="text-3xl font-bold mb-6">Camera Dashboard</h2>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {cameras.map((camId) => (
          <div key={camId} className="bg-surface-elevated rounded-lg p-4">
            <h3 className="text-xl font-semibold mb-4">Camera {camId}</h3>
            <CameraStream cameraId={camId} />
          </div>
        ))}
      </div>
    </div>
  )
}
