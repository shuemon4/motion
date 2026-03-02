import { FormSection, FormSelect, FormToggle, FormSlider, FormInput } from '@/components/form';
import { AUTH_METHODS } from '@/utils/parameterMappings';
import { useSystemStatus } from '@/api/queries';

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
  cameraId?: number;
}

export function StreamSettings({ config, onChange, getError, cameraId }: StreamSettingsProps) {
  const { data: systemStatus } = useSystemStatus();

  const getValue = (param: string, defaultValue: string | number | boolean = '') => {
    return config[param]?.value ?? defaultValue;
  };

  const webrtcEnabled = Boolean(getValue('webrtc_enable', false));

  // Get WebRTC encoder status for this camera
  const camStatus = cameraId !== undefined
    ? systemStatus?.status?.[`cam${cameraId}` as `cam${number}`]
    : undefined;

  return (
    <FormSection
      title="Video Streaming"
      description="Live MJPEG and WebRTC stream configuration"
      collapsible
      defaultOpen={false}
    >
      <FormSelect
        label="Streaming Resolution"
        value={String(getValue('stream_preview_scale', 100))}
        onChange={(val) => onChange('stream_preview_scale', Number(val))}
        options={RESOLUTION_PRESETS}
        helpText="Scale stream as percentage of source resolution. Lower = less bandwidth and CPU."
      />

      <FormSlider
        label="Stream Quality"
        value={Number(getValue('stream_quality', 50))}
        onChange={(val) => onChange('stream_quality', val)}
        min={1}
        max={100}
        unit="%"
        helpText="JPEG compression quality (1-100). Higher = better quality, more bandwidth."
        error={getError?.('stream_quality')}
      />

      <FormSlider
        label="Stream Max Framerate"
        value={Number(getValue('stream_maxrate', 15))}
        onChange={(val) => onChange('stream_maxrate', val)}
        min={1}
        max={30}
        unit=" fps"
        helpText="Maximum frames per second (lower = less bandwidth and CPU)"
        error={getError?.('stream_maxrate')}
      />

      <FormToggle
        label="Show Motion Boxes"
        value={Boolean(getValue('stream_motion', false))}
        onChange={(val) => onChange('stream_motion', val)}
        helpText="Display motion detection boxes in stream"
      />

      <div className="border-t border-surface-elevated pt-4 mt-4">
        <h4 className="text-sm font-medium text-gray-300 mb-3">Substream Settings</h4>
        <div className="text-xs text-gray-400 mb-3">
          Substream is a lower-resolution version used for grid thumbnails. Lower scale saves bandwidth.
        </div>

        <FormSelect
          label="Substream Scale"
          value={String(getValue('substream_scale', 50))}
          onChange={(val) => onChange('substream_scale', Number(val))}
          options={[
            { value: '25', label: 'Quarter (25%)' },
            { value: '50', label: 'Half (50%) — Default' },
            { value: '100', label: 'Full (100%)' },
          ]}
          helpText="Resolution scale for substream. Lower = less bandwidth. Requires restart."
        />

        <FormSlider
          label="Substream Quality"
          value={Number(getValue('substream_quality', 40))}
          onChange={(val) => onChange('substream_quality', val)}
          min={1}
          max={100}
          unit="%"
          helpText="JPEG quality for substream (1-100). Default: 40."
        />

        <FormSlider
          label="Substream Max Framerate"
          value={Number(getValue('substream_maxrate', 10))}
          onChange={(val) => onChange('substream_maxrate', val)}
          min={1}
          max={30}
          unit=" fps"
          helpText="Maximum FPS for substream. Default: 10."
        />
      </div>

      <FormSelect
        label="Direct Stream Access Security"
        value={String(getValue('webcontrol_auth_method', 0))}
        onChange={(val) => onChange('webcontrol_auth_method', Number(val))}
        options={AUTH_METHODS.map((method) => ({
          value: String(method.value),
          label: method.label,
        }))}
        helpText="Authentication when streams are accessed directly (embedded in other websites, VLC, home automation). None = open access on trusted networks only. Basic = use with HTTPS. Digest = recommended."
      />

      <div className="text-xs text-gray-400 bg-surface-elevated p-3 rounded mt-4">
        <p><strong>Stream URL:</strong> <code>http://[hostname]:[port]/[cam]/mjpg/stream</code></p>
        <p className="mt-1"><strong>Note:</strong> Streaming resolution scales the output to reduce bandwidth and CPU usage. Server-side resizing is always performed by Motion.</p>
      </div>

      {/* WebRTC / H.264 Streaming */}
      <div className="border-t border-surface-elevated pt-4 mt-4">
        <h4 className="text-sm font-medium text-gray-300 mb-3">WebRTC / H.264 Streaming</h4>
        <div className="text-xs text-gray-400 bg-surface-elevated p-3 rounded mb-3">
          All WebRTC settings require a Motion restart. Viewer preference (MJPEG vs WebRTC) is in UI Preferences.
        </div>

        <FormToggle
          label="Enable WebRTC"
          value={webrtcEnabled}
          onChange={(val) => onChange('webrtc_enable', val)}
          helpText="Enable WebRTC H.264 streaming for low-latency video."
        />

        {webrtcEnabled && (
          <>
            {/* Encoder status badge */}
            {camStatus?.webrtc_encoder && (
              <div className="flex items-center gap-2 text-xs text-gray-400 bg-surface-elevated p-3 rounded mb-3">
                <span className="font-medium text-gray-300">Encoder:</span>
                <code>{camStatus.webrtc_encoder}</code>
                <span className="text-gray-500">|</span>
                <span className="font-medium text-gray-300">State:</span>
                <span className={
                  camStatus.webrtc_encoder_state === 'both' ? 'text-green-400' :
                  camStatus.webrtc_encoder_state === 'idle' ? 'text-gray-500' :
                  'text-blue-400'
                }>
                  {camStatus.webrtc_encoder_state}
                </span>
                <span className="text-gray-500">|</span>
                <span className="font-medium text-gray-300">Peers:</span>
                <span>{camStatus.webrtc_peers ?? 0}</span>
              </div>
            )}

            <FormSlider
              label="WebRTC Quality"
              value={Number(getValue('webrtc_quality', 50))}
              onChange={(val) => onChange('webrtc_quality', val)}
              min={1}
              max={100}
              unit="%"
              helpText="H.264 encoding quality (1-100). Maps to CRF. Higher = better quality, more CPU."
              error={getError?.('webrtc_quality')}
            />

            <FormSlider
              label="GOP Size"
              value={Number(getValue('webrtc_gop', 30))}
              onChange={(val) => onChange('webrtc_gop', val)}
              min={1}
              max={300}
              unit=" frames"
              helpText="Group of Pictures size. Lower = faster seek, more bandwidth. Default: 30."
              error={getError?.('webrtc_gop')}
            />

            <FormSlider
              label="Max Peers"
              value={Number(getValue('webrtc_max_peers', 3))}
              onChange={(val) => onChange('webrtc_max_peers', val)}
              min={1}
              max={10}
              helpText="Maximum simultaneous WebRTC viewers per camera."
              error={getError?.('webrtc_max_peers')}
            />

            {/* Network */}
            <div className="border-t border-surface-elevated pt-4 mt-4">
              <h4 className="text-sm font-medium text-gray-300 mb-3">Network</h4>

              <FormInput
                label="Port Range Min"
                value={String(getValue('webrtc_port_min', ''))}
                onChange={(val) => onChange('webrtc_port_min', Number(val))}
                type="number"
                helpText="UDP port range start for WebRTC media (default: 40000)."
                error={getError?.('webrtc_port_min')}
              />

              <FormInput
                label="Port Range Max"
                value={String(getValue('webrtc_port_max', ''))}
                onChange={(val) => onChange('webrtc_port_max', Number(val))}
                type="number"
                helpText="UDP port range end for WebRTC media (default: 40100)."
                error={getError?.('webrtc_port_max')}
              />

              <FormInput
                label="STUN Server"
                value={String(getValue('webrtc_stun_server', ''))}
                onChange={(val) => onChange('webrtc_stun_server', val)}
                helpText="STUN server for NAT traversal (e.g. stun:stun.l.google.com:19302)."
                error={getError?.('webrtc_stun_server')}
              />
            </div>

            {/* Audio */}
            <div className="border-t border-surface-elevated pt-4 mt-4">
              <h4 className="text-sm font-medium text-gray-300 mb-3">Audio</h4>

              <FormToggle
                label="Enable Audio"
                value={Boolean(getValue('webrtc_audio', false))}
                onChange={(val) => onChange('webrtc_audio', val)}
                helpText="Include audio in WebRTC stream."
              />

              {Boolean(getValue('webrtc_audio', false)) && (
                <>
                  <FormInput
                    label="Audio Device"
                    value={String(getValue('webrtc_audio_device', ''))}
                    onChange={(val) => onChange('webrtc_audio_device', val)}
                    helpText="ALSA audio device (e.g. hw:0,0). Leave empty for default."
                    error={getError?.('webrtc_audio_device')}
                  />

                  <FormSlider
                    label="Opus Bitrate"
                    value={Number(getValue('webrtc_audio_opus_bitrate', 48000))}
                    onChange={(val) => onChange('webrtc_audio_opus_bitrate', val)}
                    min={6000}
                    max={510000}
                    unit=" bps"
                    helpText="Opus audio encoder bitrate (6000-510000). Default: 48000."
                    error={getError?.('webrtc_audio_opus_bitrate')}
                  />
                </>
              )}
            </div>
          </>
        )}
      </div>
    </FormSection>
  );
}
