import { FormSection, FormInput, FormSelect, FormToggle } from '@/components/form';
import { AUTH_METHODS } from '@/utils/parameterMappings';

export interface StreamSettingsProps {
  config: Record<string, { value: string | number | boolean }>;
  onChange: (param: string, value: string | number | boolean) => void;
  getError?: (param: string) => string | undefined;
}

export function StreamSettings({ config, onChange, getError }: StreamSettingsProps) {
  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue;
  };

  return (
    <FormSection
      title="Video Streaming"
      description="Live MJPEG stream configuration"
      collapsible
      defaultOpen={false}
    >
      <FormInput
        label="Stream Quality"
        value={String(getValue('stream_quality', 50))}
        onChange={(val) => onChange('stream_quality', Number(val))}
        type="number"
        helpText="JPEG compression quality (1-100, higher = better quality, more bandwidth)"
        error={getError?.('stream_quality')}
      />

      <FormInput
        label="Stream Max Framerate"
        value={String(getValue('stream_maxrate', 15))}
        onChange={(val) => onChange('stream_maxrate', Number(val))}
        type="number"
        helpText="Maximum frames per second for stream (lower = less bandwidth)"
        error={getError?.('stream_maxrate')}
      />

      <FormInput
        label="Stream Preview Scale"
        value={String(getValue('stream_preview_scale', 100))}
        onChange={(val) => onChange('stream_preview_scale', Number(val))}
        type="number"
        helpText="Scale preview as percentage of source resolution (1-100%)"
        error={getError?.('stream_preview_scale')}
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
    </FormSection>
  );
}
