import { useState, useEffect, useCallback, useMemo } from 'react'
import { useQuery } from '@tanstack/react-query'
import { apiGet } from '@/api/client'
import { FormSection, FormInput, FormSelect, FormToggle } from '@/components/form'
import { useToast } from '@/components/Toast'
import { useBatchUpdateConfig } from '@/api/queries'
import { validateConfigParam } from '@/lib/validation'
import { DeviceSettings } from '@/components/settings/DeviceSettings'
import { LibcameraSettings } from '@/components/settings/LibcameraSettings'
import { OverlaySettings } from '@/components/settings/OverlaySettings'
import { StreamSettings } from '@/components/settings/StreamSettings'
import { MotionSettings } from '@/components/settings/MotionSettings'
import { PictureSettings } from '@/components/settings/PictureSettings'
import { MovieSettings } from '@/components/settings/MovieSettings'
import { StorageSettings } from '@/components/settings/StorageSettings'
import { ScheduleSettings } from '@/components/settings/ScheduleSettings'
import { PreferencesSettings } from '@/components/settings/PreferencesSettings'
import { MaskEditor } from '@/components/settings/MaskEditor'
import { NotificationSettings } from '@/components/settings/NotificationSettings'
import { UploadSettings } from '@/components/settings/UploadSettings'
import { systemReboot, systemShutdown } from '@/api/system'

interface ConfigParam {
  value: string | number | boolean
  enabled: boolean
  category: number
  type: string
  list?: string[]
}

interface CameraInfo {
  id: number
  name: string
  url: string
}

interface MotionConfig {
  version: string
  cameras: Record<string, CameraInfo | number>  // "count" is number, rest are CameraInfo
  configuration: {
    default: Record<string, ConfigParam>
    [key: string]: Record<string, ConfigParam>  // cam1, cam2, etc.
  }
  categories: Record<string, { name: string; display: string }>
}

type ConfigChanges = Record<string, string | number | boolean>
type ValidationErrors = Record<string, string>

export function Settings() {
  const { addToast } = useToast()
  const [selectedCamera, setSelectedCamera] = useState('0')
  const [changes, setChanges] = useState<ConfigChanges>({})
  const [validationErrors, setValidationErrors] = useState<ValidationErrors>({})
  const [isSaving, setIsSaving] = useState(false)

  const { data: config, isLoading, error } = useQuery({
    queryKey: ['config'],
    queryFn: () => apiGet<MotionConfig>('/0/api/config'),
  })

  const batchUpdateConfigMutation = useBatchUpdateConfig()

  // Clear changes and errors when camera selection changes
  useEffect(() => {
    setChanges({})
    setValidationErrors({})
  }, [selectedCamera])

  const handleChange = useCallback((param: string, value: string | number | boolean) => {
    setChanges((prev) => ({ ...prev, [param]: value }))

    // Validate the new value
    const result = validateConfigParam(param, String(value))
    setValidationErrors((prev) => {
      if (result.success) {
        // Remove error if validation passes
        const { [param]: _, ...rest } = prev
        return rest
      } else {
        // Add error
        return { ...prev, [param]: result.error ?? 'Invalid value' }
      }
    })
  }, [])

  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    // If there's a pending change, use that
    if (param in changes) {
      return changes[param]
    }

    // For camera-specific settings, read from cam{id} config with fallback to default
    if (selectedCamera !== '0') {
      const camConfig = config?.configuration[`cam${selectedCamera}`]
      if (camConfig && param in camConfig) {
        return camConfig[param]?.value ?? defaultValue
      }
    }

    // Fall back to global default
    return config?.configuration.default[param]?.value ?? defaultValue
  }

  const isDirty = Object.keys(changes).length > 0
  const hasValidationErrors = Object.keys(validationErrors).length > 0

  // Get the active config for the selected camera
  // Merges camera-specific config with defaults (camera values override defaults)
  const activeConfig = useMemo(() => {
    if (!config) return {}
    const defaultConfig = config.configuration.default || {}

    if (selectedCamera === '0') {
      return defaultConfig
    }

    const cameraConfig = config.configuration[`cam${selectedCamera}`] || {}
    // Merge: camera-specific values override defaults
    return { ...defaultConfig, ...cameraConfig }
  }, [config, selectedCamera])

  const handleSave = async () => {
    if (!isDirty) {
      addToast('No changes to save', 'info')
      return
    }

    // Block save if there are validation errors
    if (hasValidationErrors) {
      addToast('Please fix validation errors before saving', 'error')
      return
    }

    setIsSaving(true)
    const camId = parseInt(selectedCamera, 10)

    try {
      // Use batch API - send all changes in one request
      await batchUpdateConfigMutation.mutateAsync({
        camId,
        changes,
      })

      addToast(`Successfully saved ${Object.keys(changes).length} setting(s)`, 'success')
      setChanges({})
    } catch (err) {
      console.error('Failed to save settings:', err)
      addToast('Failed to save settings', 'error')
    } finally {
      setIsSaving(false)
    }
  }

  const handleReset = () => {
    setChanges({})
    setValidationErrors({})
    addToast('Changes discarded', 'info')
  }

  // Helper to get error for a parameter
  const getError = (param: string): string | undefined => {
    return validationErrors[param]
  }

  if (isLoading) {
    return (
      <div className="p-6">
        <h2 className="text-3xl font-bold mb-6">Settings</h2>
        <div className="animate-pulse">
          <div className="h-32 bg-surface-elevated rounded-lg mb-4"></div>
          <div className="h-32 bg-surface-elevated rounded-lg mb-4"></div>
        </div>
      </div>
    )
  }

  if (error) {
    return (
      <div className="p-6">
        <h2 className="text-3xl font-bold mb-6">Settings</h2>
        <div className="bg-danger/10 border border-danger rounded-lg p-4">
          <p className="text-danger">Failed to load configuration</p>
        </div>
      </div>
    )
  }

  if (!config) {
    return null
  }

  return (
    <div className="p-6">
      {/* Sticky sub-header for save controls */}
      <div className="sticky top-[73px] z-40 -mx-6 px-6 py-3 bg-surface/95 backdrop-blur border-b border-gray-800 mb-6">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-4">
            <h2 className="text-2xl font-bold">Settings</h2>
            <div>
              <select
                value={selectedCamera}
                onChange={(e) => setSelectedCamera(e.target.value)}
                className="px-3 py-1.5 bg-surface-elevated border border-gray-700 rounded-lg text-sm"
              >
                <option value="0">Global Settings</option>
                {config.cameras && Object.entries(config.cameras).map(([key, cam]) => {
                  // Skip the 'count' property
                  if (key === 'count' || typeof cam === 'number') return null
                  const camera = cam as CameraInfo
                  return (
                    <option key={camera.id} value={String(camera.id)}>
                      {camera.name || `Camera ${camera.id}`}
                    </option>
                  )
                })}
              </select>
            </div>
          </div>
          <div className="flex items-center gap-3">
            {isDirty && !hasValidationErrors && (
              <span className="text-yellow-200 text-sm">Unsaved changes</span>
            )}
            {hasValidationErrors && (
              <span className="text-red-200 text-sm">Fix errors below</span>
            )}
            {isDirty && (
              <button
                onClick={handleReset}
                disabled={isSaving}
                className="px-3 py-1.5 text-sm bg-surface-elevated hover:bg-surface rounded-lg transition-colors disabled:opacity-50"
              >
                Discard
              </button>
            )}
            <button
              onClick={handleSave}
              disabled={!isDirty || isSaving || hasValidationErrors}
              className="px-4 py-1.5 text-sm bg-primary hover:bg-primary-hover rounded-lg transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
            >
              {isSaving ? 'Saving...' : isDirty ? 'Save Changes' : 'Saved'}
            </button>
          </div>
        </div>
      </div>

      <DeviceSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <LibcameraSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <OverlaySettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <StreamSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <MotionSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      {/* Mask Editor - only shown when a camera is selected */}
      {selectedCamera !== '0' && (
        <MaskEditor cameraId={parseInt(selectedCamera, 10)} />
      )}

      <PictureSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <MovieSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <StorageSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <ScheduleSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <NotificationSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <UploadSettings
        config={activeConfig}
        onChange={handleChange}
        getError={getError}
      />

      <PreferencesSettings />

      <FormSection
        title="System"
        description="Core Motion daemon settings and device controls"
        collapsible
        defaultOpen={false}
      >
        <FormToggle
          label="Run as Daemon"
          value={getValue('daemon', false) as boolean}
          onChange={(val) => handleChange('daemon', val)}
          helpText="Run Motion in background mode"
        />
        <FormInput
          label="Log File"
          value={String(getValue('log_file', ''))}
          onChange={(val) => handleChange('log_file', val)}
          helpText="Path to Motion log file"
          error={getError('log_file')}
        />
        <FormSelect
          label="Log Level"
          value={String(getValue('log_level', '6'))}
          onChange={(val) => handleChange('log_level', val)}
          options={[
            { value: '1', label: 'Emergency' },
            { value: '2', label: 'Alert' },
            { value: '3', label: 'Critical' },
            { value: '4', label: 'Error' },
            { value: '5', label: 'Warning' },
            { value: '6', label: 'Notice' },
            { value: '7', label: 'Info' },
            { value: '8', label: 'Debug' },
            { value: '9', label: 'All' },
          ]}
          helpText="Verbosity level for logging"
          error={getError('log_level')}
        />

        <div className="border-t border-surface-elevated pt-4 mt-4">
          <h4 className="font-medium mb-3 text-sm">Device Controls</h4>
          <div className="flex gap-3">
            <button
              onClick={async () => {
                if (window.confirm('Are you sure you want to reboot the Pi? The system will restart and be unavailable for about a minute.')) {
                  try {
                    await systemReboot()
                    addToast('Rebooting... The system will be back online shortly.', 'info')
                  } catch (error: unknown) {
                    const err = error as { message?: string }
                    addToast(
                      err.message || 'Failed to reboot. Power control may be disabled in config.',
                      'error'
                    )
                  }
                }
              }}
              className="px-4 py-2 bg-yellow-600/20 text-yellow-300 hover:bg-yellow-600/30 rounded-lg text-sm transition-colors"
            >
              Restart Pi
            </button>
            <button
              onClick={async () => {
                if (window.confirm('Are you sure you want to shutdown the Pi? You will need to physically power it back on.')) {
                  try {
                    await systemShutdown()
                    addToast('Shutting down... The system will power off.', 'warning')
                  } catch (error: unknown) {
                    const err = error as { message?: string }
                    addToast(
                      err.message || 'Failed to shutdown. Power control may be disabled in config.',
                      'error'
                    )
                  }
                }
              }}
              className="px-4 py-2 bg-red-600/20 text-red-300 hover:bg-red-600/30 rounded-lg text-sm transition-colors"
            >
              Shutdown Pi
            </button>
          </div>
          <p className="text-xs text-gray-500 mt-2">
            Requires <code className="text-xs bg-surface-base px-1 rounded">webcontrol_actions power=on</code> in config
          </p>
        </div>
      </FormSection>

      <FormSection
        title="About"
        description="Motion version information"
        collapsible
        defaultOpen={false}
      >
        <p className="text-sm text-gray-400">
          Motion Version: {config.version}
        </p>
      </FormSection>
    </div>
  )
}
