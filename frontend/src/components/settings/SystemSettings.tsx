import { FormSection, FormInput, FormSelect, FormToggle } from '@/components/form'
import { useToast } from '@/components/Toast'
import { systemReboot, systemShutdown } from '@/api/system'

export interface SystemSettingsProps {
  config: Record<string, { value: string | number | boolean }>
  onChange: (param: string, value: string | number | boolean) => void
  getError?: (param: string) => string | undefined
}

export function SystemSettings({ config, onChange, getError }: SystemSettingsProps) {
  const { addToast } = useToast()

  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue
  }

  const handleReboot = async () => {
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
  }

  const handleShutdown = async () => {
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
  }

  return (
    <>
      {/* Authentication Section - FIRST and PROMINENT */}
      <FormSection
        title="Authentication"
        description="Web interface login credentials"
        collapsible
        defaultOpen={true}
      >
        <p className="text-xs text-gray-400 mb-4">
          Configure web interface authentication. Format: username:password or username:ha1hash
        </p>
        <FormSelect
          label="Auth Method"
          value={String(getValue('webcontrol_auth_method', '0'))}
          onChange={(val) => onChange('webcontrol_auth_method', val)}
          options={[
            { value: '0', label: 'None - No authentication' },
            { value: '1', label: 'Basic - Simple username/password' },
            { value: '2', label: 'Digest - More secure hash-based' },
          ]}
          helpText="Authentication for direct stream access and external API clients. The web UI uses session login instead."
          error={getError?.('webcontrol_auth_method')}
        />
        <FormInput
          label="Admin Credentials"
          value={String(getValue('webcontrol_authentication', ''))}
          onChange={(val) => onChange('webcontrol_authentication', val)}
          type="password"
          helpText="Administrator username:password (full access to all settings)"
          error={getError?.('webcontrol_authentication')}
        />
        <FormInput
          label="Viewer Credentials"
          value={String(getValue('webcontrol_user_authentication', ''))}
          onChange={(val) => onChange('webcontrol_user_authentication', val)}
          type="password"
          helpText="View-only username:password (can view streams but not change settings)"
          error={getError?.('webcontrol_user_authentication')}
        />
      </FormSection>

      {/* Daemon Settings Section */}
      <FormSection
        title="Daemon"
        description="Motion process settings"
        collapsible
        defaultOpen={false}
      >
        <FormToggle
          label="Run as Daemon"
          value={getValue('daemon', false) as boolean}
          onChange={(val) => onChange('daemon', val)}
          helpText="Run Motion in background mode"
        />
        <FormInput
          label="PID File"
          value={String(getValue('pid_file', ''))}
          onChange={(val) => onChange('pid_file', val)}
          helpText="Path to process ID file (leave empty for default)"
          error={getError?.('pid_file')}
        />
        <FormInput
          label="Log File"
          value={String(getValue('log_file', ''))}
          onChange={(val) => onChange('log_file', val)}
          helpText="Path to Motion log file"
          error={getError?.('log_file')}
        />
        <FormSelect
          label="Log Level"
          value={String(getValue('log_level', '6'))}
          onChange={(val) => onChange('log_level', val)}
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
          error={getError?.('log_level')}
        />
      </FormSection>

      {/* Web Server Section */}
      <FormSection
        title="Web Server"
        description="API server configuration"
        collapsible
        defaultOpen={false}
      >
        <FormInput
          label="Port"
          value={String(getValue('webcontrol_port', '8080'))}
          onChange={(val) => onChange('webcontrol_port', val)}
          type="number"
          helpText="Primary web server port"
          error={getError?.('webcontrol_port')}
        />
        <FormToggle
          label="Localhost Only"
          value={getValue('webcontrol_localhost', false) as boolean}
          onChange={(val) => onChange('webcontrol_localhost', val)}
          helpText="Restrict access to localhost only (127.0.0.1)"
        />
        <FormToggle
          label="TLS/HTTPS"
          value={getValue('webcontrol_tls', false) as boolean}
          onChange={(val) => onChange('webcontrol_tls', val)}
          helpText="Enable HTTPS encryption"
        />
        {getValue('webcontrol_tls', false) && (
          <>
            <FormInput
              label="TLS Certificate"
              value={String(getValue('webcontrol_cert', ''))}
              onChange={(val) => onChange('webcontrol_cert', val)}
              helpText="Path to TLS certificate file (.crt or .pem)"
              error={getError?.('webcontrol_cert')}
            />
            <FormInput
              label="TLS Private Key"
              value={String(getValue('webcontrol_key', ''))}
              onChange={(val) => onChange('webcontrol_key', val)}
              helpText="Path to TLS private key file (.key or .pem)"
              error={getError?.('webcontrol_key')}
            />
          </>
        )}
      </FormSection>

      {/* Device Controls - reboot/shutdown buttons */}
      <FormSection
        title="Device Controls"
        description="System power management"
        collapsible
        defaultOpen={false}
      >
        <div className="flex gap-3">
          <button
            onClick={handleReboot}
            className="px-4 py-2 bg-yellow-600/20 text-yellow-300 hover:bg-yellow-600/30 rounded-lg text-sm transition-colors"
          >
            Restart Pi
          </button>
          <button
            onClick={handleShutdown}
            className="px-4 py-2 bg-red-600/20 text-red-300 hover:bg-red-600/30 rounded-lg text-sm transition-colors"
          >
            Shutdown Pi
          </button>
        </div>
        <p className="text-xs text-gray-500 mt-2">
          Requires <code className="text-xs bg-surface-base px-1 rounded">webcontrol_actions power=on</code> in config
        </p>
      </FormSection>
    </>
  )
}
