import { useState, useCallback } from 'react';
import { useQueryClient } from '@tanstack/react-query';
import { useProfiles } from '../hooks/useProfiles';
import { profilesApi } from '../api/profiles';
import { ProfileSaveDialog } from './ProfileSaveDialog';
import { useToast } from './Toast';
import { applyRestartRequiredChanges } from '@/api/client';
import { useBatchUpdateConfig } from '@/api/queries';
import { resolveProfileConflicts, convertProfileParams } from '@/lib/profileConflicts';

interface ConfigurationPresetsProps {
  cameraId: number;
  readOnly?: boolean;  // Hide save button when true (for Dashboard bottom sheet)
  onPreviewProfile?: (params: Record<string, string | number | boolean>) => void;
  onProfileApplied?: () => void;
}

/**
 * Configuration Presets Component
 *
 * Allows users to:
 * - Select from saved configuration profiles
 * - Apply a profile to quickly change camera settings
 * - Save current settings as a new profile
 * - Manage existing profiles (delete, set as default)
 *
 * Two apply paths:
 *  - Settings page (onPreviewProfile provided): fetches params, resolves conflicts, populates
 *    the parent's "changes" state so the user reviews before saving.
 *  - Dashboard (no onPreviewProfile): fetches params, resolves conflicts, sends via
 *    PATCH /{camId}/api/config so hot-reload, conf_src, and restart detection work correctly.
 */
export function ConfigurationPresets({
  cameraId,
  readOnly = false,
  onPreviewProfile,
  onProfileApplied,
}: ConfigurationPresetsProps) {
  const queryClient = useQueryClient();
  const { data: profiles, isLoading, error } = useProfiles(cameraId);
  const { mutateAsync: batchUpdate, isPending: isApplying } = useBatchUpdateConfig();
  const { addToast } = useToast();

  const [selectedProfileId, setSelectedProfileId] = useState<number | null>(null);
  const [showSaveDialog, setShowSaveDialog] = useState(false);

  const handleApply = useCallback(async () => {
    if (!selectedProfileId) return;

    const profile = profiles?.find(p => p.profile_id === selectedProfileId);
    const profileName = profile?.name || 'profile';

    try {
      // Fetch full profile params from API
      const profileData = await profilesApi.get(selectedProfileId);

      // Convert string values to typed JS values, then resolve conflicts
      const convertedParams = convertProfileParams(profileData.params);
      const resolvedParams = resolveProfileConflicts(convertedParams);

      if (onPreviewProfile) {
        // Settings page path: hand resolved params to parent for user review
        onPreviewProfile(resolvedParams);
        addToast(`Profile "${profileName}" loaded for review. Click Save to apply.`, 'info');
        setSelectedProfileId(null);
      } else {
        // Dashboard path: apply directly via batch config API
        // This path gets hot-reload, conf_src updates, and restart detection for free
        const response = await batchUpdate({ camId: cameraId, changes: resolvedParams }) as {
          applied?: Array<{ param: string; error?: string; hot_reload?: boolean }>;
          summary?: { total: number; success: number; errors: number };
        } | undefined;

        await queryClient.invalidateQueries({ queryKey: ['config'] });

        // Check if any params require a camera restart
        const applied = response?.applied || [];
        const restartParams = applied
          .filter(p => !p.error && p.hot_reload === false)
          .map(p => p.param);

        if (restartParams.length > 0) {
          addToast(`Profile "${profileName}" applied. Restarting camera...`, 'info');
          try {
            await applyRestartRequiredChanges(cameraId);
            await new Promise(resolve => setTimeout(resolve, 2000));
            await queryClient.invalidateQueries({ queryKey: ['config'] });
            addToast(`Profile "${profileName}" applied. Camera restarted.`, 'success');
          } catch (err) {
            console.error('Failed to restart camera:', err);
            addToast(
              `Profile applied but camera restart failed. Please restart manually.`,
              'warning'
            );
          }
        } else {
          addToast(`Profile "${profileName}" applied successfully`, 'success');
        }

        // Clear parent's local override state so profile values are visible
        onProfileApplied?.();
        setSelectedProfileId(null);
      }
    } catch (err) {
      addToast(
        `Failed to apply profile: ${err instanceof Error ? err.message : 'Unknown error'}`,
        'error'
      );
    }
  }, [
    selectedProfileId,
    profiles,
    cameraId,
    onPreviewProfile,
    onProfileApplied,
    batchUpdate,
    queryClient,
    addToast,
  ]);

  if (error) {
    return (
      <div className="mb-4 pb-4 border-b border-surface-elevated">
        <div className="bg-danger/10 border border-danger rounded-lg p-3">
          <p className="text-sm text-danger font-medium">Failed to load profiles</p>
          <p className="text-xs text-gray-400 mt-1">
            {error instanceof Error ? error.message : 'Unable to connect to profiles API'}
          </p>
        </div>
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

          {/* Save button (hidden in read-only mode) */}
          {!readOnly && (
            <button
              type="button"
              onClick={() => setShowSaveDialog(true)}
              className="px-4 py-2 bg-surface-elevated border border-gray-600 text-white rounded-lg hover:bg-surface-hover transition-colors"
              title="Save current settings as a new preset"
            >
              Save
            </button>
          )}

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
