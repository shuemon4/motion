import { useState } from 'react';
import { FormSection, FormInput, FormSelect, FormToggle, FormSlider } from '@/components/form';
import { MOVIE_CONTAINERS } from '@/utils/parameterMappings';
import { recordingModeToMotion, motionToRecordingMode } from '@/utils/translations';

export interface MovieSettingsProps {
  config: Record<string, { value: string | number | boolean }>;
  onChange: (param: string, value: string | number | boolean) => void;
  getError?: (param: string) => string | undefined;
}

export function MovieSettings({ config, onChange, getError }: MovieSettingsProps) {
  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue;
  };

  // Determine current recording mode
  const movieOutput = getValue('movie_output', false) as boolean;
  const movieOutputMotion = getValue('movie_output_motion', true) as boolean;
  const currentMode = motionToRecordingMode(movieOutput, movieOutputMotion);

  const [selectedMode, setSelectedMode] = useState(currentMode);

  const handleRecordingModeChange = (mode: string) => {
    setSelectedMode(mode);
    const motionParams = recordingModeToMotion(mode);

    // Apply the mode's parameter changes
    onChange('movie_output', motionParams.movie_output);
    if (motionParams.movie_output_motion !== undefined) {
      onChange('movie_output_motion', motionParams.movie_output_motion);
    }
  };

  // Recording mode options
  const recordingModeOptions = [
    { value: 'off', label: 'Off' },
    { value: 'motion-triggered', label: 'Motion Triggered' },
    { value: 'continuous', label: 'Continuous Recording' },
  ];

  // Format code reference for help text
  const formatCodes = [
    '%Y - Year',
    '%m - Month',
    '%d - Day',
    '%H - Hour',
    '%M - Minute',
    '%S - Second',
    '%v - Event number',
    '%$ - Camera name',
  ].join(', ');

  return (
    <FormSection
      title="Movie Settings"
      description="Configure video recording settings"
      collapsible
      defaultOpen={false}
    >
      <div className="space-y-4">
        <FormSelect
          label="Recording Mode"
          value={selectedMode}
          onChange={handleRecordingModeChange}
          options={recordingModeOptions}
          helpText="When to record video. Motion Triggered = only during events, Continuous = always record."
        />

        {/* Show conversion explanation */}
        <div className="text-xs text-gray-400 bg-surface-elevated p-3 rounded">
          <strong>Current settings:</strong> movie_output={String(movieOutput)}
          {movieOutput && `, movie_output_motion=${String(movieOutputMotion)}`}
        </div>

        {selectedMode !== 'off' && (
          <>
            <FormSlider
              label="Movie Quality"
              value={Number(getValue('movie_quality', 75))}
              onChange={(val) => onChange('movie_quality', val)}
              min={1}
              max={100}
              unit="%"
              helpText="Video encoding quality (1-100). Higher = better quality, larger files, more CPU."
              error={getError?.('movie_quality')}
            />

            <FormInput
              label="Movie Filename Pattern"
              value={String(getValue('movie_filename', '%Y%m%d%H%M%S'))}
              onChange={(val) => onChange('movie_filename', val)}
              helpText={`Format codes: ${formatCodes}`}
              error={getError?.('movie_filename')}
            />

            <FormSelect
              label="Container Format"
              value={String(getValue('movie_container', 'mkv'))}
              onChange={(val) => onChange('movie_container', val)}
              options={MOVIE_CONTAINERS}
              helpText="Video container format. MKV recommended for reliability."
            />

            <FormInput
              label="Max Duration (seconds)"
              value={String(getValue('movie_max_time', 0))}
              onChange={(val) => onChange('movie_max_time', Number(val))}
              type="number"
              min="0"
              helpText="Maximum movie length (0 = unlimited). Splits long events into multiple files."
              error={getError?.('movie_max_time')}
            />

            <FormToggle
              label="Passthrough Mode"
              value={getValue('movie_passthrough', false) as boolean}
              onChange={(val) => onChange('movie_passthrough', val)}
              helpText="Copy codec without re-encoding. Reduces CPU but may cause compatibility issues."
            />
          </>
        )}

        <div className="border-t border-surface-elevated pt-4">
          <h4 className="font-medium mb-2 text-sm">Format Code Reference</h4>
          <div className="text-xs text-gray-400 space-y-2">
            <p className="font-medium text-gray-300">Dynamic Folder Examples (Recommended):</p>
            <p><code>%Y-%m-%d/%H%M%S</code> → <code>2025-01-29/143022.mkv</code></p>
            <p><code>%Y/%m/%d/%v-%H%M%S</code> → <code>2025/01/29/42-143022.mkv</code></p>
            <p><code>%$/%Y-%m-%d/%v</code> → <code>Camera1/2025-01-29/42.mkv</code></p>
            <p className="mt-2 font-medium text-gray-300">Flat Structure:</p>
            <p><code>%Y%m%d%H%M%S</code> → <code>20250129143022.mkv</code></p>
            <p className="mt-2">Available codes: {formatCodes}</p>
            <p className="mt-2 text-yellow-200"><strong>Tip:</strong> Using date-based folders like <code>%Y-%m-%d/</code> keeps files organized and makes browsing faster.</p>
          </div>
        </div>

        {selectedMode === 'continuous' && (
          <div className="text-xs text-yellow-200 bg-yellow-600/10 border border-yellow-600/30 p-3 rounded">
            <strong>⚠️ CPU Warning:</strong> Continuous recording uses significant CPU for encoding.
            Consider enabling passthrough mode or reducing quality/resolution on resource-constrained devices.
          </div>
        )}
      </div>
    </FormSection>
  );
}
