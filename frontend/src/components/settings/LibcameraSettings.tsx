import { FormSection, FormInput, FormSelect, FormToggle } from '@/components/form';
import {
  AWB_MODES,
  AUTOFOCUS_MODES,
  AUTOFOCUS_RANGES,
  AUTOFOCUS_SPEEDS,
} from '@/utils/parameterMappings';

export interface LibcameraSettingsProps {
  config: Record<string, { value: string | number | boolean }>;
  onChange: (param: string, value: string | number | boolean) => void;
  getError?: (param: string) => string | undefined;
}

export function LibcameraSettings({ config, onChange, getError }: LibcameraSettingsProps) {
  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue;
  };

  const awbEnabled = Boolean(getValue('libcam_awb_enable', false));

  return (
    <FormSection
      title="libcamera Controls"
      description="Raspberry Pi camera controls (libcamera only)"
      collapsible
      defaultOpen={false}
    >
      {/* Image Controls */}
      <FormInput
        label="Brightness"
        value={String(getValue('libcam_brightness', 0))}
        onChange={(val) => onChange('libcam_brightness', Number(val))}
        type="number"
        helpText="Brightness adjustment (-1.0 to 1.0)"
        error={getError?.('libcam_brightness')}
      />

      <FormInput
        label="Contrast"
        value={String(getValue('libcam_contrast', 1))}
        onChange={(val) => onChange('libcam_contrast', Number(val))}
        type="number"
        helpText="Contrast adjustment (0.0 to 32.0)"
        error={getError?.('libcam_contrast')}
      />

      <FormInput
        label="ISO"
        value={String(getValue('libcam_iso', 0))}
        onChange={(val) => onChange('libcam_iso', Number(val))}
        type="number"
        helpText="ISO sensitivity (0=auto, 100-6400)"
        error={getError?.('libcam_iso')}
      />

      {/* Auto White Balance */}
      <FormToggle
        label="Auto White Balance"
        value={awbEnabled}
        onChange={(val) => onChange('libcam_awb_enable', val)}
        helpText="Enable automatic white balance"
      />

      {awbEnabled && (
        <>
          <FormSelect
            label="AWB Mode"
            value={String(getValue('libcam_awb_mode', 0))}
            onChange={(val) => onChange('libcam_awb_mode', Number(val))}
            options={AWB_MODES.map((mode) => ({
              value: String(mode.value),
              label: mode.label,
            }))}
            helpText="White balance mode"
          />

          <FormToggle
            label="Lock AWB"
            value={Boolean(getValue('libcam_awb_locked', false))}
            onChange={(val) => onChange('libcam_awb_locked', val)}
            helpText="Lock white balance settings"
          />
        </>
      )}

      {!awbEnabled && (
        <>
          <FormInput
            label="Color Temperature"
            value={String(getValue('libcam_colour_temp', 0))}
            onChange={(val) => onChange('libcam_colour_temp', Number(val))}
            type="number"
            helpText="Manual color temperature in Kelvin (0-10000)"
            error={getError?.('libcam_colour_temp')}
          />

          <div className="grid grid-cols-2 gap-4">
            <FormInput
              label="Red Gain"
              value={String(getValue('libcam_colour_gain_r', 1))}
              onChange={(val) => onChange('libcam_colour_gain_r', Number(val))}
              type="number"
              helpText="Red color gain (0.0-8.0)"
              error={getError?.('libcam_colour_gain_r')}
            />

            <FormInput
              label="Blue Gain"
              value={String(getValue('libcam_colour_gain_b', 1))}
              onChange={(val) => onChange('libcam_colour_gain_b', Number(val))}
              type="number"
              helpText="Blue color gain (0.0-8.0)"
              error={getError?.('libcam_colour_gain_b')}
            />
          </div>
        </>
      )}

      {/* Autofocus */}
      <FormSelect
        label="Autofocus Mode"
        value={String(getValue('libcam_af_mode', 0))}
        onChange={(val) => onChange('libcam_af_mode', Number(val))}
        options={AUTOFOCUS_MODES.map((mode) => ({
          value: String(mode.value),
          label: mode.label,
        }))}
        helpText="Focus control mode"
      />

      {Number(getValue('libcam_af_mode', 0)) === 0 && (
        <FormInput
          label="Lens Position"
          value={String(getValue('libcam_lens_position', 0))}
          onChange={(val) => onChange('libcam_lens_position', Number(val))}
          type="number"
          helpText="Manual focus position (0.0-15.0 dioptres)"
          error={getError?.('libcam_lens_position')}
        />
      )}

      {Number(getValue('libcam_af_mode', 0)) > 0 && (
        <>
          <FormSelect
            label="Autofocus Range"
            value={String(getValue('libcam_af_range', 0))}
            onChange={(val) => onChange('libcam_af_range', Number(val))}
            options={AUTOFOCUS_RANGES.map((range) => ({
              value: String(range.value),
              label: range.label,
            }))}
            helpText="Focus range preference"
          />

          <FormSelect
            label="Autofocus Speed"
            value={String(getValue('libcam_af_speed', 0))}
            onChange={(val) => onChange('libcam_af_speed', Number(val))}
            options={AUTOFOCUS_SPEEDS.map((speed) => ({
              value: String(speed.value),
              label: speed.label,
            }))}
            helpText="Focus adjustment speed"
          />
        </>
      )}

      {/* Buffer Settings */}
      <FormInput
        label="Buffer Count"
        value={String(getValue('libcam_buffer_count', 4))}
        onChange={(val) => onChange('libcam_buffer_count', Number(val))}
        type="number"
        helpText="Number of camera buffers (2-8, higher = smoother but more memory)"
        error={getError?.('libcam_buffer_count')}
      />
    </FormSection>
  );
}
