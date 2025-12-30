import { FormSection, FormInput, FormSelect, FormToggle } from '@/components/form';
import { AUTH_METHODS } from '@/utils/parameterMappings';

// Resolution presets matching MotionEye
const RESOLUTION_PRESETS = [
  { value: '100', label: 'Full (100%)' },
  { value: '75', label: 'High (75%)' },
  { value: '50', label: 'Medium (50%)' },
  { value: '25', label: 'Low (25%)' },
  { value: '10', label: 'Minimal (10%)' },
];

export interface StreamSettingsProps {
  config: Record<string, { value: string | number | boolean }>;
  onChange: (param: string, value: string | number | boolean) => void;
  getError?: (param: string) => string | undefined;
}

export function StreamSettings({ config, onChange, getError }: StreamSettingsProps) {
  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue;
  };

  // stream_localhost controls whether streaming is enabled (false = enabled, true = disabled)
  const streamingEnabled = !getValue('stream_localhost', false);

  return (
    <FormSection
      title="Video Streaming"
      description="Live MJPEG stream configuration"
      collapsible
      defaultOpen={false}
    >
      <FormToggle
        label="Enable Video Streaming"
        value={streamingEnabled}
        onChange={(val) => onChange('stream_localhost', !val)}
        helpText="Enable/disable live MJPEG streaming. When disabled, stream is only accessible from localhost."
      />

      {streamingEnabled && (
        <>
          <FormSelect
            label="Streaming Resolution"
            value={String(getValue('stream_preview_scale', 100))}
            onChange={(val) => onChange('stream_preview_scale', Number(val))}
            options={RESOLUTION_PRESETS}
            helpText="Scale stream as percentage of source resolution. Lower = less bandwidth and CPU."
          />

          <FormInput
            label="Stream Quality"
            value={String(getValue('stream_quality', 50))}
            onChange={(val) => onChange('stream_quality', Number(val))}
            type="number"
            min="1"
            max="100"
            helpText="JPEG compression quality (1-100). Higher = better quality, more bandwidth."
            error={getError?.('stream_quality')}
          />

          <FormInput
            label="Stream Max Framerate"
            value={String(getValue('stream_maxrate', 15))}
            onChange={(val) => onChange('stream_maxrate', Number(val))}
            type="number"
            min="1"
            max="100"
            helpText="Maximum frames per second (lower = less bandwidth and CPU)"
            error={getError?.('stream_maxrate')}
          />

          <FormToggle
            label="Show Motion Boxes"
            value={Boolean(getValue('stream_motion', false))}
            onChange={(val) => onChange('stream_motion', val)}
            helpText="Display motion detection boxes in stream"
          />

          <FormSelect
            label="Authentication Method"
            value={String(getValue('webcontrol_auth_method', 0))}
            onChange={(val) => onChange('webcontrol_auth_method', Number(val))}
            options={AUTH_METHODS.map((method) => ({
              value: String(method.value),
              label: method.label,
            }))}
            helpText="Require authentication for stream access"
          />

          <div className="text-xs text-gray-400 bg-surface-elevated p-3 rounded mt-4">
            <p><strong>Stream URL:</strong> <code>http://[hostname]:[port]/[cam]/mjpg/stream</code></p>
            <p className="mt-1"><strong>Note:</strong> Streaming resolution scales the output to reduce bandwidth and CPU usage. Server-side resizing is always performed by Motion.</p>
          </div>
        </>
      )}
    </FormSection>
  );
}
