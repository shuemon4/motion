import { useState, useEffect, useCallback } from 'react'
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

interface MotionConfig {
  version: string
  cameras: Record<string, unknown>
  configuration: {
    default: Record<string, {
      value: string | number | boolean
      enabled: boolean
      category: number
      type: string
      list?: string[]
    }>
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
    if (param in changes) {
      return changes[param]
    }
    return config?.configuration.default[param]?.value ?? defaultValue
  }

  const isDirty = Object.keys(changes).length > 0
  const hasValidationErrors = Object.keys(validationErrors).length > 0

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
      <div className="flex items-center justify-between mb-6">
        <h2 className="text-3xl font-bold">Settings</h2>
        <div className="flex gap-2">
          {isDirty && (
            <button
              onClick={handleReset}
              disabled={isSaving}
              className="px-4 py-2 bg-surface-elevated hover:bg-surface rounded-lg transition-colors disabled:opacity-50"
            >
              Discard
            </button>
          )}
          <button
            onClick={handleSave}
            disabled={!isDirty || isSaving || hasValidationErrors}
            className="px-4 py-2 bg-primary hover:bg-primary-hover rounded-lg transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
          >
            {isSaving ? 'Saving...' : hasValidationErrors ? 'Fix Errors' : isDirty ? 'Save Changes *' : 'Save Changes'}
          </button>
        </div>
      </div>

      {isDirty && !hasValidationErrors && (
        <div className="mb-4 p-3 bg-yellow-600/10 border border-yellow-600/30 rounded-lg text-yellow-200 text-sm">
          You have unsaved changes
        </div>
      )}

      {hasValidationErrors && (
        <div className="mb-4 p-3 bg-red-600/10 border border-red-600/30 rounded-lg text-red-200 text-sm">
          Please fix the validation errors below before saving
        </div>
      )}

      <div className="mb-6">
        <label className="block text-sm font-medium mb-2">Camera</label>
        <select
          value={selectedCamera}
          onChange={(e) => setSelectedCamera(e.target.value)}
          className="px-3 py-2 bg-surface border border-surface-elevated rounded-lg"
        >
          <option value="0">Global Settings</option>
          {config.cameras && Object.keys(config.cameras).map((key) => {
            if (key !== 'count') {
              return (
                <option key={key} value={key}>
                  Camera {key}
                </option>
              )
            }
            return null
          })}
        </select>
      </div>

      <DeviceSettings
        config={config.configuration.default}
        onChange={handleChange}
        getError={getError}
      />

      <LibcameraSettings
        config={config.configuration.default}
        onChange={handleChange}
        getError={getError}
      />

      <OverlaySettings
        config={config.configuration.default}
        onChange={handleChange}
        getError={getError}
      />

      <StreamSettings
        config={config.configuration.default}
        onChange={handleChange}
        getError={getError}
      />

      <MotionSettings
        config={config.configuration.default}
        onChange={handleChange}
        getError={getError}
      />

      <PictureSettings
        config={config.configuration.default}
        onChange={handleChange}
        getError={getError}
      />

      <MovieSettings
        config={config.configuration.default}
        onChange={handleChange}
        getError={getError}
      />

      <StorageSettings
        config={config.configuration.default}
        onChange={handleChange}
        getError={getError}
      />

      <ScheduleSettings
        config={config.configuration.default}
        onChange={handleChange}
        getError={getError}
      />

      <PreferencesSettings />

      <FormSection
        title="System"
        description="Core Motion daemon settings"
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
