import { useState, useCallback, useRef, useEffect } from 'react'
import { FormSlider, FormToggle } from '@/components/form'
import { ConfigurationPresets } from '@/components/ConfigurationPresets'
import { useBatchUpdateConfig } from '@/api/queries'
import { percentToPixels, pixelsToPercent } from '@/utils/translations'

interface QuickSettingsProps {
  cameraId: number
  config: Record<string, { value: string | number | boolean }>
}

// Collapsible section for quick settings
function QuickSection({
  title,
  defaultOpen = true,
  children,
}: {
  title: string
  defaultOpen?: boolean
  children: React.ReactNode
}) {
  const [isOpen, setIsOpen] = useState(defaultOpen)

  return (
    <div className="mb-4">
      <button
        type="button"
        className="flex items-center justify-between w-full py-2 text-left"
        onClick={() => setIsOpen(!isOpen)}
      >
        <span className="font-medium text-sm">{title}</span>
        <svg
          className={`w-4 h-4 transition-transform ${isOpen ? 'rotate-180' : ''}`}
          fill="none"
          stroke="currentColor"
          viewBox="0 0 24 24"
        >
          <path
            strokeLinecap="round"
            strokeLinejoin="round"
            strokeWidth={2}
            d="M19 9l-7 7-7-7"
          />
        </svg>
      </button>
      {isOpen && <div className="pt-2">{children}</div>}
    </div>
  )
}

export function QuickSettings({ cameraId, config }: QuickSettingsProps) {
  const { mutate: updateConfig, isPending } = useBatchUpdateConfig()
  const [lastApplied, setLastApplied] = useState<string | null>(null)
  const [localChanges, setLocalChanges] = useState<Record<string, string | number | boolean>>({})
  const debounceTimers = useRef<Record<string, ReturnType<typeof setTimeout>>>({})

  // Cleanup timers on unmount
  useEffect(() => {
    return () => {
      Object.values(debounceTimers.current).forEach(clearTimeout)
    }
  }, [])

  // Reset local changes when config prop changes (e.g., after API update)
  // This is a sync effect: local overrides should clear when server config updates
  useEffect(() => {
    // eslint-disable-next-line react-hooks/set-state-in-effect
    setLocalChanges({})
  }, [config])

  const getValue = useCallback(
    (param: string, defaultValue: string | number | boolean = '') => {
      // Local changes take precedence over config
      if (param in localChanges) {
        return localChanges[param]
      }
      return config[param]?.value ?? defaultValue
    },
    [config, localChanges]
  )

  // Cancel pending updates when camera changes
  useEffect(() => {
    // Clear all debounce timers when cameraId changes
    Object.values(debounceTimers.current).forEach(clearTimeout)
    debounceTimers.current = {}
  }, [cameraId])

  // Debounced change handler for sliders
  const handleChange = useCallback(
    (param: string, value: string | number | boolean) => {
      // Update local state immediately so slider moves
      setLocalChanges((prev) => ({ ...prev, [param]: value }))

      // Clear existing timer for this param
      if (debounceTimers.current[param]) {
        clearTimeout(debounceTimers.current[param])
      }

      // Debounce API call - capture cameraId in closure
      const currentCameraId = cameraId
      debounceTimers.current[param] = setTimeout(() => {
        updateConfig(
          { camId: currentCameraId, changes: { [param]: value } },
          {
            onSuccess: () => {
              setLastApplied(param)
              setTimeout(() => setLastApplied(null), 1000)
            },
          }
        )
      }, 300)
    },
    [cameraId, updateConfig]
  )

  // Immediate change handler for toggles/selects
  const handleImmediateChange = useCallback(
    (param: string, value: string | number | boolean) => {
      // Update local state immediately
      setLocalChanges((prev) => ({ ...prev, [param]: value }))

      updateConfig(
        { camId: cameraId, changes: { [param]: value } },
        {
          onSuccess: () => {
            setLastApplied(param)
            setTimeout(() => setLastApplied(null), 1000)
          },
        }
      )
    },
    [cameraId, updateConfig]
  )

  // Get current resolution for threshold percentage conversion
  const width = Number(getValue('width', 640))
  const height = Number(getValue('height', 480))
  const thresholdPixels = Number(getValue('threshold', 1500))
  const thresholdPercent = pixelsToPercent(thresholdPixels, width, height)

  const handleThresholdChange = useCallback(
    (percentValue: number) => {
      const pixels = percentToPixels(percentValue, width, height)
      handleChange('threshold', pixels)
    },
    [width, height, handleChange]
  )

  return (
    <div className="space-y-2">
      {/* Configuration Presets (load-only in bottom sheet) */}
      <ConfigurationPresets cameraId={cameraId} readOnly={true} />

      {/* Stream Settings */}
      <QuickSection title="Stream" defaultOpen={true}>
        <FormSlider
          label="Quality"
          value={Number(getValue('stream_quality', 50))}
          onChange={(val) => handleChange('stream_quality', val)}
          min={1}
          max={100}
          unit="%"
          helpText="JPEG compression quality"
        />

        <FormSlider
          label="Max Framerate"
          value={Number(getValue('stream_maxrate', 15))}
          onChange={(val) => handleChange('stream_maxrate', val)}
          min={1}
          max={30}
          unit=" fps"
          helpText="Maximum stream framerate"
        />

        <FormToggle
          label="Show Motion Boxes"
          value={Boolean(getValue('stream_motion', false))}
          onChange={(val) => handleImmediateChange('stream_motion', val)}
          helpText="Display motion detection boxes"
        />
      </QuickSection>

      {/* Image/Camera Settings (libcamera) */}
      <QuickSection title="Image" defaultOpen={true}>
        <FormSlider
          label="Brightness"
          value={Number(getValue('libcam_brightness', 0))}
          onChange={(val) => handleChange('libcam_brightness', val)}
          min={-1}
          max={1}
          step={0.1}
          helpText="Brightness adjustment"
        />

        <FormSlider
          label="Contrast"
          value={Number(getValue('libcam_contrast', 1))}
          onChange={(val) => handleChange('libcam_contrast', val)}
          min={0}
          max={32}
          step={0.5}
          helpText="Contrast adjustment"
        />

        <FormSlider
          label="Gain"
          value={Number(getValue('libcam_gain', 1))}
          onChange={(val) => handleChange('libcam_gain', val)}
          min={0}
          max={10}
          step={0.1}
          helpText="Analog gain (0=auto, 1.0-10.0)"
        />
      </QuickSection>

      {/* Motion Detection */}
      <QuickSection title="Detection" defaultOpen={false}>
        <FormSlider
          label="Threshold"
          value={thresholdPercent}
          onChange={handleThresholdChange}
          min={0}
          max={20}
          step={0.1}
          unit="%"
          helpText="Motion sensitivity (higher = less sensitive)"
        />

        <FormSlider
          label="Noise Level"
          value={Number(getValue('noise_level', 32))}
          onChange={(val) => handleChange('noise_level', val)}
          min={1}
          max={255}
          helpText="Noise tolerance"
        />

        <FormToggle
          label="Auto-tune Noise"
          value={Boolean(getValue('noise_tune', false))}
          onChange={(val) => handleImmediateChange('noise_tune', val)}
          helpText="Automatically adjust noise level"
        />
      </QuickSection>

      {/* Overlay Settings */}
      <QuickSection title="Overlay" defaultOpen={false}>
        <FormSlider
          label="Text Scale"
          value={Number(getValue('text_scale', 1))}
          onChange={(val) => handleChange('text_scale', val)}
          min={1}
          max={10}
          unit="x"
          helpText="Overlay text size"
        />
      </QuickSection>

      {/* Status indicator */}
      {isPending && (
        <div className="text-center text-sm text-gray-400 py-2">
          Applying...
        </div>
      )}
      {lastApplied && !isPending && (
        <div className="text-center text-sm text-green-400 py-2">
          Applied!
        </div>
      )}
    </div>
  )
}
