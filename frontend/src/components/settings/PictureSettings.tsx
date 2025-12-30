import { useState } from 'react';
import { FormSection, FormInput, FormSelect } from '@/components/form';
import { captureModeToMotion, motionToCaptureMode } from '@/utils/translations';

export interface PictureSettingsProps {
  config: Record<string, { value: string | number | boolean }>;
  onChange: (param: string, value: string | number | boolean) => void;
  getError?: (param: string) => string | undefined;
}

export function PictureSettings({ config, onChange, getError }: PictureSettingsProps) {
  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue;
  };

  // Determine current capture mode
  const pictureOutput = String(getValue('picture_output', 'off'));
  const snapshotInterval = Number(getValue('snapshot_interval', 0));
  const currentMode = motionToCaptureMode(pictureOutput, snapshotInterval);

  const [selectedMode, setSelectedMode] = useState(currentMode);

  const handleCaptureModeChange = (mode: string) => {
    setSelectedMode(mode);
    const motionParams = captureModeToMotion(mode);

    // Apply the mode's parameter changes
    onChange('picture_output', motionParams.picture_output);
    if (motionParams.snapshot_interval !== undefined) {
      onChange('snapshot_interval', motionParams.snapshot_interval);
    }
  };

  // Capture mode options
  const captureModeOptions = [
    { value: 'off', label: 'Off' },
    { value: 'motion-triggered', label: 'Motion Triggered (all frames)' },
    { value: 'motion-triggered-one', label: 'Motion Triggered (first frame only)' },
    { value: 'best', label: 'Best Quality Frame' },
    { value: 'center', label: 'Center Frame' },
    { value: 'interval-snapshots', label: 'Interval Snapshots' },
    { value: 'manual', label: 'Manual Only' },
  ];

  // Format code reference for help text
  const formatCodes = [
    '%Y - Year (4 digits)',
    '%m - Month (01-12)',
    '%d - Day (01-31)',
    '%H - Hour (00-23)',
    '%M - Minute (00-59)',
    '%S - Second (00-59)',
    '%q - Frame number',
    '%v - Event number',
    '%$ - Camera name',
  ].join(', ');

  return (
    <FormSection
      title="Picture Settings"
      description="Configure picture capture and snapshots"
      collapsible
      defaultOpen={false}
    >
      <div className="space-y-4">
        <FormSelect
          label="Capture Mode"
          value={selectedMode}
          onChange={handleCaptureModeChange}
          options={captureModeOptions}
          helpText="When to capture still images during motion events"
        />

        {/* Show conversion explanation */}
        <div className="text-xs text-gray-400 bg-surface-elevated p-3 rounded">
          <strong>Current settings:</strong> picture_output={pictureOutput}
          {snapshotInterval > 0 && `, snapshot_interval=${snapshotInterval}s`}
        </div>

        {selectedMode === 'interval-snapshots' && (
          <FormInput
            label="Snapshot Interval (seconds)"
            value={String(getValue('snapshot_interval', 60))}
            onChange={(val) => onChange('snapshot_interval', Number(val))}
            type="number"
            min="1"
            helpText="Seconds between snapshots (independent of motion)"
            error={getError?.('snapshot_interval')}
          />
        )}

        <FormInput
          label="Picture Quality"
          value={String(getValue('picture_quality', 75))}
          onChange={(val) => onChange('picture_quality', Number(val))}
          type="number"
          min="1"
          max="100"
          helpText="JPEG quality (1-100). Higher = better quality, larger files."
          error={getError?.('picture_quality')}
        />

        <FormInput
          label="Picture Filename Pattern"
          value={String(getValue('picture_filename', '%Y%m%d%H%M%S-%q'))}
          onChange={(val) => onChange('picture_filename', val)}
          helpText={`Format codes: ${formatCodes}`}
          error={getError?.('picture_filename')}
        />

        <div className="border-t border-surface-elevated pt-4">
          <h4 className="font-medium mb-2 text-sm">Format Code Reference</h4>
          <div className="text-xs text-gray-400 space-y-1">
            <p><code>%Y%m%d%H%M%S-%q</code> → <code>20250129143022-05.jpg</code></p>
            <p><code>%$/%v/%Y-%m-%d_%H-%M-%S</code> → <code>Camera1/42/2025-01-29_14-30-22.jpg</code></p>
            <p>Available codes: {formatCodes}</p>
          </div>
        </div>
      </div>
    </FormSection>
  );
}
