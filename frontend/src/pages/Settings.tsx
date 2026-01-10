import { useState, useEffect, useCallback, useMemo } from 'react'
import { useQuery, useQueryClient } from '@tanstack/react-query'
import { apiGet, applyRestartRequiredChanges } from '@/api/client'
import { setCsrfToken } from '@/api/csrf'
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
import { ProfileManager } from '@/components/settings/ProfileManager'
import { ConfigurationPresets } from '@/components/ConfigurationPresets'
import { systemReboot, systemShutdown } from '@/api/system'
import { useAuthContext } from '@/contexts/AuthContext'

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
  const { role } = useAuthContext()
  const { addToast } = useToast()
  const [selectedCamera, setSelectedCamera] = useState('0')
  const [changes, setChanges] = useState<ConfigChanges>({})
  const [validationErrors, setValidationErrors] = useState<ValidationErrors>({})
  const [isSaving, setIsSaving] = useState(false)

  // All hooks must be called before any conditional returns
  const queryClient = useQueryClient()
  const { data: config, isLoading, error } = useQuery({
    queryKey: ['config'],
    queryFn: async () => {
      const cfg = await apiGet<MotionConfig & { csrf_token?: string }>('/0/api/config')
      // Store CSRF token for subsequent PATCH requests
      if (cfg.csrf_token) {
        setCsrfToken(cfg.csrf_token)
      }
      return cfg
    },
  })

  const batchUpdateConfigMutation = useBatchUpdateConfig()

  // Clear changes and errors when camera selection changes
  // This prevents race conditions where settings from one camera could be
  // applied to another if the user switches cameras before saving
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
        // eslint-disable-next-line @typescript-eslint/no-unused-vars
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
  // Also merges in pending changes so UI reflects unsaved edits
  const activeConfig = useMemo(() => {
    if (!config) return {}
    const defaultConfig = config.configuration.default || {}

    let baseConfig: Record<string, ConfigParam>
    if (selectedCamera === '0') {
      baseConfig = defaultConfig
    } else {
      const cameraConfig = config.configuration[`cam${selectedCamera}`] || {}
      // Merge: camera-specific values override defaults
      baseConfig = { ...defaultConfig, ...cameraConfig }
    }

    // Merge pending changes into config so child components see updated values
    const mergedConfig = { ...baseConfig }
    for (const [param, value] of Object.entries(changes)) {
      if (mergedConfig[param]) {
        mergedConfig[param] = { ...mergedConfig[param], value }
      } else {
        // Create a new entry for params not in original config
        mergedConfig[param] = { value, enabled: true, category: 0, type: 'string' }
      }
    }

    return mergedConfig
  }, [config, selectedCamera, changes])

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
      const response = await batchUpdateConfigMutation.mutateAsync({
        camId,
        changes,
      }) as {
        status?: string
        applied?: Array<{ param: string; error?: string; hot_reload?: boolean }>
        summary?: { total: number; success: number; errors: number }
      } | undefined

      // Wait for config to be refetched before clearing changes
      // This prevents the UI from reverting to old values
      await queryClient.invalidateQueries({ queryKey: ['config'] })
      await queryClient.refetchQueries({ queryKey: ['config'] })

      // Check response for partial failures
      const summary = response?.summary
      const applied = response?.applied || []

      if (!summary) {
        // No summary in response - assume success
        addToast(`Saved ${Object.keys(changes).length} setting(s)`, 'success')
        setChanges({})
      } else {
        // Find which parameters require restart (hot_reload: false, no error)
        const restartParams = applied
          .filter((p) => !p.error && p.hot_reload === false)
          .map((p) => p.param)

        if (restartParams.length > 0) {
          // Auto-restart for restart-required parameters
          addToast(
            `Applying ${restartParams.length} setting(s) that require restart: ${restartParams.join(', ')}...`,
            'info'
          )

          try {
            // Write config to disk and restart camera
            await applyRestartRequiredChanges(camId)

            // Clear all changes since restart applies them
            setChanges({})

            // Wait briefly for camera to restart, then refetch config
            await new Promise(resolve => setTimeout(resolve, 2000))
            await queryClient.invalidateQueries({ queryKey: ['config'] })
            await queryClient.refetchQueries({ queryKey: ['config'] })

            addToast(
              `Applied ${restartParams.length} setting(s). Camera restarted.`,
              'success'
            )
          } catch (restartErr) {
            console.error('Failed to restart camera:', restartErr)
            addToast(
              `Saved settings but camera restart failed. Please restart manually.`,
              'warning'
            )
            setChanges({})
          }
        } else if (summary.errors > 0) {
          // Handle errors (not restart-related)
          const failedParams = applied
            .filter((p) => p.error)
            .map((p) => p.param)

          // Only clear changes that succeeded
          const successfulParams = applied
            .filter((p) => !p.error)
            .map((p) => p.param)

          const remainingChanges: ConfigChanges = {}
          for (const [param, value] of Object.entries(changes)) {
            if (!successfulParams.includes(param)) {
              remainingChanges[param] = value
            }
          }
          setChanges(remainingChanges)

          if (summary.success > 0) {
            addToast(
              `Saved ${summary.success} setting(s). ${summary.errors} failed: ${failedParams.join(', ')}`,
              'warning'
            )
          } else {
            addToast(
              `Failed to save settings: ${failedParams.join(', ')}`,
              'error'
            )
          }
        } else {
          // All params applied successfully with no restart needed
          addToast(`Successfully saved ${summary.success} setting(s)`, 'success')
          setChanges({})
        }
      }
    } catch (err) {
      console.error('Failed to save settings:', err)
      addToast('Failed to save settings. Check browser console for details.', 'error')
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

  // Require admin access - check after all hooks
  if (role !== 'admin') {
    return (
      <div className="p-4 sm:p-6">
        <div className="bg-surface-elevated rounded-lg p-8 text-center max-w-2xl mx-auto">
          <svg className="w-16 h-16 mx-auto text-yellow-500 mb-4" fill="none" stroke="currentColor" viewBox="0 0 24 24">
            <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={1.5} d="M12 9v2m0 4h.01m-6.938 4h13.856c1.54 0 2.502-1.667 1.732-3L13.732 4c-.77-1.333-2.694-1.333-3.464 0L3.34 16c-.77 1.333.192 3 1.732 3z" />
          </svg>
          <h1 className="text-2xl font-bold mb-2">Admin Access Required</h1>
          <p className="text-gray-400">You must be logged in as an administrator to access settings.</p>
        </div>
      </div>
    )
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

      {/* Quick Profile Switch - only shown when a camera is selected (not global) */}
      {selectedCamera !== '0' && (
        <div className="bg-surface-elevated rounded-lg p-4 mb-6">
          <ConfigurationPresets cameraId={Number(selectedCamera)} readOnly={false} />
        </div>
      )}

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

      <ProfileManager cameraId={Number(selectedCamera)} />

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
          <h4 className="font-medium mb-3 text-sm">Authentication</h4>
          <p className="text-xs text-gray-400 mb-4">
            Configure web interface authentication. Format: username:password or username:ha1hash
          </p>
          <FormInput
            label="Admin Credentials"
            value={String(getValue('webcontrol_authentication', ''))}
            onChange={(val) => handleChange('webcontrol_authentication', val)}
            type="password"
            helpText="Administrator username:password (full access to all settings)"
            error={getError('webcontrol_authentication')}
          />
          <FormInput
            label="User Credentials"
            value={String(getValue('webcontrol_user_authentication', ''))}
            onChange={(val) => handleChange('webcontrol_user_authentication', val)}
            type="password"
            helpText="View-only username:password (can view streams but not change settings)"
            error={getError('webcontrol_user_authentication')}
          />
        </div>

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
