import { FormSection, FormToggle, FormInput } from '@/components/form';

export interface PtzSettingsProps {
  config: Record<string, { value: string | number | boolean }>;
  onChange: (param: string, value: string | number | boolean) => void;
  getError?: (param: string) => string | undefined;
}

export function PtzSettings({ config, onChange, getError }: PtzSettingsProps) {
  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue;
  };

  const ptzEnabled = Boolean(getValue('stream_preview_ptz', false));

  return (
    <FormSection
      title="PTZ Controls"
      description="Pan/Tilt/Zoom controls for camera positioning"
      collapsible
      defaultOpen={false}
    >
      <FormToggle
        label="Enable PTZ Controls"
        value={ptzEnabled}
        onChange={(val) => onChange('stream_preview_ptz', val)}
        helpText="Show PTZ controls as an overlay on the camera stream"
      />

      {ptzEnabled && (
        <>
          <FormToggle
            label="Auto-track"
            value={Boolean(getValue('ptz_auto_track', false))}
            onChange={(val) => onChange('ptz_auto_track', val)}
            helpText="Automatically track moving objects with PTZ commands"
          />

          <FormInput
            label="PTZ Wait (frames)"
            type="number"
            value={Number(getValue('ptz_wait', 10))}
            onChange={(val) => onChange('ptz_wait', Number(val))}
            min={0}
            max={100}
            helpText="Number of frames to skip after executing a PTZ command"
            error={getError?.('ptz_wait')}
          />

          <div className="mt-6 pt-6 border-t border-gray-700">
            <h4 className="text-sm font-medium mb-4">PTZ Command Configuration</h4>
            <p className="text-xs text-gray-400 mb-4">
              Configure shell commands for each PTZ action. Leave empty to disable that action.
              Only non-empty commands will show as buttons in the stream overlay.
            </p>

            <FormInput
              label="Pan Left Command"
              type="text"
              value={String(getValue('ptz_pan_left', ''))}
              onChange={(val) => onChange('ptz_pan_left', val)}
              placeholder="e.g., /usr/local/bin/ptz.sh left"
              helpText="Shell command to pan camera left"
              error={getError?.('ptz_pan_left')}
            />

            <FormInput
              label="Pan Right Command"
              type="text"
              value={String(getValue('ptz_pan_right', ''))}
              onChange={(val) => onChange('ptz_pan_right', val)}
              placeholder="e.g., /usr/local/bin/ptz.sh right"
              helpText="Shell command to pan camera right"
              error={getError?.('ptz_pan_right')}
            />

            <FormInput
              label="Tilt Up Command"
              type="text"
              value={String(getValue('ptz_tilt_up', ''))}
              onChange={(val) => onChange('ptz_tilt_up', val)}
              placeholder="e.g., /usr/local/bin/ptz.sh up"
              helpText="Shell command to tilt camera up"
              error={getError?.('ptz_tilt_up')}
            />

            <FormInput
              label="Tilt Down Command"
              type="text"
              value={String(getValue('ptz_tilt_down', ''))}
              onChange={(val) => onChange('ptz_tilt_down', val)}
              placeholder="e.g., /usr/local/bin/ptz.sh down"
              helpText="Shell command to tilt camera down"
              error={getError?.('ptz_tilt_down')}
            />

            <FormInput
              label="Zoom In Command"
              type="text"
              value={String(getValue('ptz_zoom_in', ''))}
              onChange={(val) => onChange('ptz_zoom_in', val)}
              placeholder="e.g., /usr/local/bin/ptz.sh zoom_in"
              helpText="Shell command to zoom in"
              error={getError?.('ptz_zoom_in')}
            />

            <FormInput
              label="Zoom Out Command"
              type="text"
              value={String(getValue('ptz_zoom_out', ''))}
              onChange={(val) => onChange('ptz_zoom_out', val)}
              placeholder="e.g., /usr/local/bin/ptz.sh zoom_out"
              helpText="Shell command to zoom out"
              error={getError?.('ptz_zoom_out')}
            />
          </div>
        </>
      )}
    </FormSection>
  );
}
