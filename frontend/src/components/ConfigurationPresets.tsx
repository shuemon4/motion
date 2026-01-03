interface ConfigurationPresetsProps {
  cameraId: number
}

/**
 * Configuration Presets placeholder component
 *
 * This displays a "Coming Soon" UI for the saved configuration presets feature.
 * When the backend API is implemented, this component will:
 * - Load available presets from GET /0/api/presets
 * - Allow selecting and applying a preset
 * - Allow saving current config as a new preset
 */
export function ConfigurationPresets(_props: ConfigurationPresetsProps) {
  return (
    <div className="mb-4 pb-4 border-b border-surface-elevated">
      <label className="block text-sm font-medium text-gray-400 mb-2">
        Configuration Preset
      </label>
      <div className="flex gap-2">
        {/* Preset selector dropdown */}
        <div className="relative flex-1">
          <select
            disabled
            className="w-full px-3 py-2 bg-surface-elevated border border-gray-600 rounded-lg text-gray-500 cursor-not-allowed appearance-none"
            title="Configuration presets coming soon"
          >
            <option>Default (Coming Soon)</option>
          </select>
          <div className="absolute inset-y-0 right-0 flex items-center pr-3 pointer-events-none">
            <svg
              className="w-4 h-4 text-gray-500"
              fill="none"
              stroke="currentColor"
              viewBox="0 0 24 24"
            >
              <path
                strokeLinecap="round"
                strokeLinejoin="round"
                strokeWidth={2}
                d="M19 9l-7 7-7-7"
              />
            </svg>
          </div>
        </div>

        {/* Apply button */}
        <button
          type="button"
          disabled
          className="px-4 py-2 bg-primary/30 text-gray-500 rounded-lg cursor-not-allowed"
          title="Configuration presets coming soon"
        >
          Apply
        </button>
      </div>
      <p className="mt-1 text-xs text-gray-500">
        Save and load configuration presets (coming soon)
      </p>
    </div>
  )
}
