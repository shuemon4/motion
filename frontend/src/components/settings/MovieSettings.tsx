import { useState } from 'react';
import { FormSection, FormInput, FormSelect, FormToggle, FormSlider } from '@/components/form';
import {
  MOVIE_CONTAINERS,
  ENCODER_PRESETS,
  isHardwareCodec,
  isHighCpuCodec
} from '@/utils/parameterMappings';
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
  const emulateMotion = getValue('emulate_motion', false) as boolean;
  const currentMode = motionToRecordingMode(movieOutput, movieOutputMotion, emulateMotion);

  const [selectedMode, setSelectedMode] = useState(currentMode);

  const handleRecordingModeChange = (mode: string) => {
    setSelectedMode(mode);
    const motionParams = recordingModeToMotion(mode);

    // Apply the mode's parameter changes
    onChange('movie_output', motionParams.movie_output);
    if (motionParams.movie_output_motion !== undefined) {
      onChange('movie_output_motion', motionParams.movie_output_motion);
    }
    // Set emulate_motion for continuous recording
    if (motionParams.emulate_motion !== undefined) {
      onChange('emulate_motion', motionParams.emulate_motion);
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

  // Current container value for conditional warnings
  const containerValue = String(getValue('movie_container', 'mp4'));

  // Encoder preset only applies to software encoding, not:
  // - Passthrough mode
  // - Hardware encoding (h264_v4l2m2m)
  // - VP8/VP9 (webm)
  const showEncoderPreset = () => {
    const passthrough = getValue('movie_passthrough', false);

    if (passthrough) return false;
    if (isHardwareCodec(containerValue)) return false;
    if (containerValue === 'webm') return false;

    return true;
  };

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
          {movieOutput && `, emulate_motion=${String(emulateMotion)}`}
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
              helpText="Video container format. Hardware options only work on Pi 4."
            />

            {/* Hardware encoding info */}
            {isHardwareCodec(containerValue) && (
              <div className="text-xs text-blue-400 bg-blue-950/30 p-3 rounded mt-2">
                <strong>Hardware Encoding:</strong> Uses Pi 4's built-in H.264 encoder
                for ~10% CPU usage instead of 40-70%. Only available on Raspberry Pi 4.
                <br />
                <span className="text-amber-400">Note: Pi 5 does not have a hardware encoder.
                This will fall back to software encoding on Pi 5.</span>
              </div>
            )}

            {/* High CPU warning for HEVC */}
            {isHighCpuCodec(containerValue) && (
              <div className="text-xs text-amber-400 bg-amber-950/30 p-3 rounded mt-2">
                <strong>High CPU Warning:</strong> H.265/HEVC software encoding uses
                80-100% CPU on Raspberry Pi. Not recommended for continuous recording.
                Consider H.264 for better performance.
              </div>
            )}

            {/* Software encoding tip */}
            {!isHardwareCodec(containerValue) && !getValue('movie_passthrough', false) &&
             !containerValue.includes('webm') && !isHighCpuCodec(containerValue) && (
              <div className="text-xs text-gray-400 bg-surface-elevated p-3 rounded mt-2">
                <strong>Tip:</strong> If using Raspberry Pi 4, consider selecting
                "MKV - H.264 Hardware (Pi 4)" or "MP4 - H.264 Hardware (Pi 4)" for ~10% CPU
                instead of ~40-70% with software encoding.
              </div>
            )}

            {/* WebM format info */}
            {containerValue === 'webm' && (
              <div className="text-xs text-blue-400 bg-blue-950/30 p-3 rounded mt-2">
                <strong>WebM Format:</strong> Uses VP8 codec, optimized for web streaming.
                Encoder preset setting does not apply to VP8.
              </div>
            )}

            {showEncoderPreset() && (
              <FormSelect
                label="Encoder Preset"
                value={String(getValue('movie_encoder_preset', 'medium'))}
                onChange={(val) => onChange('movie_encoder_preset', val)}
                options={ENCODER_PRESETS.map(p => ({
                  value: p.value,
                  label: p.label,
                }))}
                helpText="Tradeoff between CPU usage and video quality. Lower presets use less CPU but produce lower quality video. Requires restart to take effect."
              />
            )}

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
            <strong>Continuous Recording:</strong> Camera will record 24/7 regardless of motion.
            Expected CPU usage on Raspberry Pi:
            <ul className="list-disc ml-4 mt-1">
              <li><strong>Pi 4 with hardware encoder:</strong> ~10% CPU</li>
              <li><strong>Pi 5 or Pi 4 software encoding:</strong> ~35-60% CPU depending on preset</li>
              <li><strong>Passthrough mode:</strong> ~5-10% CPU (if source is H.264)</li>
            </ul>
          </div>
        )}
      </div>
    </FormSection>
  );
}
