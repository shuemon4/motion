import { useState } from 'react';
import { useProfiles, useApplyProfile } from '../hooks/useProfiles';
import { ProfileSaveDialog } from './ProfileSaveDialog';

interface ConfigurationPresetsProps {
  cameraId: number;
}

/**
 * Configuration Presets Component
 *
 * Allows users to:
 * - Select from saved configuration profiles
 * - Apply a profile to quickly change camera settings
 * - Save current settings as a new profile
 * - Manage existing profiles (delete, set as default)
 */
export function ConfigurationPresets({ cameraId }: ConfigurationPresetsProps) {
  const { data: profiles, isLoading, error } = useProfiles(cameraId);
  const { mutate: applyProfile, isPending: isApplying } = useApplyProfile();

  const [selectedProfileId, setSelectedProfileId] = useState<number | null>(null);
  const [showSaveDialog, setShowSaveDialog] = useState(false);

  const handleApply = () => {
    if (selectedProfileId) {
      applyProfile(selectedProfileId, {
        onSuccess: (requiresRestart) => {
          if (requiresRestart.length > 0) {
            // TODO: Show notification about parameters requiring restart
            console.warn('Profile applied, but requires restart:', requiresRestart);
          }
        },
        onError: (error) => {
          console.error('Failed to apply profile:', error);
          // TODO: Show error notification
        },
      });
    }
  };

  if (error) {
    return (
      <div className="mb-4 pb-4 border-b border-surface-elevated">
        <p className="text-sm text-red-400">Failed to load profiles</p>
      </div>
    );
  }

  return (
    <>
      <div className="mb-4 pb-4 border-b border-surface-elevated">
        <label className="block text-sm font-medium text-gray-400 mb-2">
          Configuration Preset
        </label>
        <div className="flex gap-2">
          {/* Preset selector dropdown */}
          <div className="relative flex-1">
            <select
              disabled={isLoading || !profiles || profiles.length === 0}
              value={selectedProfileId ?? ''}
              onChange={(e) => setSelectedProfileId(Number(e.target.value) || null)}
              className="w-full px-3 py-2 bg-surface-elevated border border-gray-600 rounded-lg text-white disabled:text-gray-500 disabled:cursor-not-allowed appearance-none focus:outline-none focus:ring-2 focus:ring-primary"
            >
              <option value="">
                {isLoading
                  ? 'Loading...'
                  : !profiles || profiles.length === 0
                  ? 'No presets available'
                  : 'Select a preset'}
              </option>
              {profiles?.map((profile) => (
                <option key={profile.profile_id} value={profile.profile_id}>
                  {profile.is_default ? '⭐ ' : ''}
                  {profile.name}
                  {profile.description ? ` - ${profile.description}` : ''}
                </option>
              ))}
            </select>
            <div className="absolute inset-y-0 right-0 flex items-center pr-3 pointer-events-none">
              <svg
                className="w-4 h-4 text-gray-400"
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

          {/* Save button */}
          <button
            type="button"
            onClick={() => setShowSaveDialog(true)}
            className="px-4 py-2 bg-surface-elevated border border-gray-600 text-white rounded-lg hover:bg-surface-hover transition-colors"
            title="Save current settings as a new preset"
          >
            Save
          </button>

          {/* Apply button */}
          <button
            type="button"
            onClick={handleApply}
            disabled={!selectedProfileId || isApplying}
            className="px-4 py-2 bg-primary text-white rounded-lg disabled:bg-primary/30 disabled:text-gray-500 disabled:cursor-not-allowed hover:bg-primary-hover transition-colors"
            title="Apply selected preset to camera"
          >
            {isApplying ? 'Applying...' : 'Apply'}
          </button>
        </div>
        <p className="mt-1 text-xs text-gray-500">
          Quick switch camera settings • {profiles?.length || 0} preset{profiles?.length !== 1 ? 's' : ''}
        </p>
      </div>

      {/* Save dialog */}
      {showSaveDialog && (
        <ProfileSaveDialog
          cameraId={cameraId}
          onClose={() => setShowSaveDialog(false)}
        />
      )}
    </>
  );
}
