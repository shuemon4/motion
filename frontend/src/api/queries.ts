import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query';
import { apiGet, apiDelete, apiPatch, apiPost, sendPtzCommand, BULK_DELETE_TIMEOUT } from './client';
import { updateSessionCsrf } from './session';
import type {
  MotionConfig,
  Camera,
  CameraStatus,
  PicturesResponse,
  MoviesResponse,
  DateSummaryResponse,
  FolderContentsResponse,
  DeleteFolderFilesResponse,
  DeleteProgressResponse,
  TemperatureResponse,
  SystemStatus,
  PlatformInfo,
  DetectedCamerasResponse,
  AddCameraRequest,
  TestNetcamRequest,
  PtzCapabilities,
} from './types';

// Query keys for cache management
export const queryKeys = {
  config: (camId?: number) => ['config', camId] as const,
  cameras: ['cameras'] as const,
  pictures: (camId: number) => ['pictures', camId] as const,
  movies: (camId: number) => ['movies', camId] as const,
  mediaDates: (camId: number, type: 'pic' | 'movie') => ['media-dates', camId, type] as const,
  mediaFolders: (camId: number, path: string) => ['media-folders', camId, path] as const,
  temperature: ['temperature'] as const,
  systemStatus: ['systemStatus'] as const,
  cameraStatus: ['cameraStatus'] as const,
  platformInfo: ['platformInfo'] as const,
  detectedCameras: ['detectedCameras'] as const,
};

// Fetch full Motion config (includes cameras list)
export function useMotionConfig() {
  return useQuery({
    queryKey: queryKeys.config(0),
    queryFn: async () => {
      const config = await apiGet<MotionConfig>('/0/api/config');
      // Update session CSRF token when config responses include new tokens
      if (config.csrf_token) {
        updateSessionCsrf(config.csrf_token);
      }
      return config;
    },
    staleTime: 60000, // Cache for 1 minute
  });
}

// Fetch camera list from Motion's API
export function useCameras() {
  return useQuery({
    queryKey: queryKeys.cameras,
    queryFn: async () => {
      const response = await apiGet<{ cameras: Camera[] }>('/0/api/cameras');
      return response.cameras;
    },
    staleTime: 60000, // Cache for 1 minute
  });
}

// Fetch pictures for a camera with pagination
export function usePictures(
  camId: number,
  offset: number = 0,
  limit: number = 100,
  date: string | null = null,
  options?: { enabled?: boolean }
) {
  return useQuery({
    queryKey: [...queryKeys.pictures(camId), offset, limit, date],
    queryFn: () => {
      let url = `/${camId}/api/media/pictures?offset=${offset}&limit=${limit}`;
      if (date) url += `&date=${date}`;
      return apiGet<PicturesResponse>(url);
    },
    staleTime: 30000, // Cache for 30 seconds
    ...options,
  });
}

// Fetch movies for a camera with pagination
export function useMovies(
  camId: number,
  offset: number = 0,
  limit: number = 100,
  date: string | null = null,
  options?: { enabled?: boolean }
) {
  return useQuery({
    queryKey: [...queryKeys.movies(camId), offset, limit, date],
    queryFn: () => {
      let url = `/${camId}/api/media/movies?offset=${offset}&limit=${limit}`;
      if (date) url += `&date=${date}`;
      return apiGet<MoviesResponse>(url);
    },
    staleTime: 30000, // Cache for 30 seconds
    ...options,
  });
}

// Fetch date summary for media type
export function useMediaDates(
  camId: number,
  type: 'pic' | 'movie',
  options?: { enabled?: boolean }
) {
  return useQuery({
    queryKey: queryKeys.mediaDates(camId, type),
    queryFn: () => apiGet<DateSummaryResponse>(`/${camId}/api/media/dates?type=${type}`),
    staleTime: 60000, // Cache date summary longer (1 minute)
    ...options,
  });
}

// Fetch folder contents for media browsing
// mediaType parameter enables server-side filtering for accurate pagination
export function useMediaFolders(
  camId: number,
  path: string = '',
  offset: number = 0,
  limit: number = 100,
  mediaType?: 'pictures' | 'movies',
  options?: { enabled?: boolean }
) {
  return useQuery({
    queryKey: [...queryKeys.mediaFolders(camId, path), offset, limit, mediaType],
    queryFn: () => {
      let url = `/${camId}/api/media/folders?offset=${offset}&limit=${limit}`;
      if (path) url += `&path=${encodeURIComponent(path)}`;
      if (mediaType) url += `&type=${mediaType === 'pictures' ? 'picture' : 'movie'}`;
      return apiGet<FolderContentsResponse>(url);
    },
    staleTime: 30000, // Cache for 30 seconds
    ...options,
  });
}

// Delete all media files in a folder
export function useDeleteFolderFiles() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: async ({ camId, path }: { camId: number; path: string }) => {
      return apiDelete<DeleteFolderFilesResponse>(
        `/${camId}/api/media/folders/files?path=${encodeURIComponent(path)}`,
        BULK_DELETE_TIMEOUT
      );
    },
    onSuccess: (_, { camId, path }) => {
      // Invalidate folder cache for this path and parent
      queryClient.invalidateQueries({ queryKey: queryKeys.mediaFolders(camId, path) });
      // Also invalidate parent folder to update file counts
      const parentPath = path.includes('/') ? path.substring(0, path.lastIndexOf('/')) : '';
      queryClient.invalidateQueries({ queryKey: queryKeys.mediaFolders(camId, parentPath) });
      // Invalidate pictures and movies caches as well
      queryClient.invalidateQueries({ queryKey: queryKeys.pictures(camId) });
      queryClient.invalidateQueries({ queryKey: queryKeys.movies(camId) });
    },
  });
}

// Poll delete progress during bulk operations
export function useDeleteProgress(
  camId: number,
  path: string,
  enabled: boolean = false
) {
  return useQuery({
    queryKey: ['delete-progress', camId, path],
    queryFn: () => {
      const url = `/${camId}/api/media/delete-progress?path=${encodeURIComponent(path)}`;
      return apiGet<DeleteProgressResponse>(url);
    },
    refetchInterval: enabled ? 1000 : false, // Poll every 1s when enabled
    refetchIntervalInBackground: false,      // Pause when tab inactive
    enabled,
    staleTime: 0, // Always fetch fresh data
  });
}

// Fetch system temperature
export function useTemperature() {
  return useQuery({
    queryKey: queryKeys.temperature,
    queryFn: () => apiGet<TemperatureResponse>('/0/api/system/temperature'),
    refetchInterval: 30000, // Refresh every 30s
    refetchIntervalInBackground: false, // Pause when tab inactive to save CPU
  });
}

// Fetch system status (comprehensive)
// Only enabled when user is authenticated (to avoid 401 errors when not logged in)
export function useSystemStatus(options?: { enabled?: boolean }) {
  return useQuery({
    queryKey: queryKeys.systemStatus,
    queryFn: () => apiGet<SystemStatus>('/0/api/system/status'),
    refetchInterval: 10000, // Refresh every 10s
    refetchIntervalInBackground: false, // Pause when tab inactive to save CPU
    staleTime: 5000,
    enabled: options?.enabled ?? true,
    retry: false, // Don't retry on auth errors
  });
}

// Fetch camera status with FPS (faster refresh for real-time display)
// Only enabled when user is authenticated (to avoid 401 errors during auth check)
export function useCameraStatus(options?: { enabled?: boolean }) {
  return useQuery({
    queryKey: queryKeys.cameraStatus,
    queryFn: () => apiGet<SystemStatus>('/0/api/system/status'),
    refetchInterval: 2000, // 2 second refresh for near-real-time FPS
    refetchIntervalInBackground: false, // Pause when tab inactive to save CPU
    staleTime: 1000,
    enabled: options?.enabled ?? true,
    retry: false, // Don't retry on auth errors
    select: (data): CameraStatus[] => {
      // Transform status object into array of camera statuses
      const cameras: CameraStatus[] = [];
      if (data.status) {
        for (const key in data.status) {
          if (key.startsWith('cam')) {
            cameras.push(data.status[key as `cam${number}`]);
          }
        }
      }
      return cameras;
    },
  });
}

// Batch update config parameters (JSON API)
// PATCH /{camId}/api/config with JSON body
export function useBatchUpdateConfig() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: async ({
      camId,
      changes,
    }: {
      camId: number;
      changes: Record<string, string | number | boolean>;
    }) => {
      return apiPatch(`/${camId}/api/config`, changes);
    },
    onSuccess: (_, { camId }) => {
      // Invalidate config cache to refetch fresh data
      queryClient.invalidateQueries({ queryKey: queryKeys.config(camId) });
      queryClient.invalidateQueries({ queryKey: queryKeys.cameras });
    },
  });
}

// Delete a picture
export function useDeletePicture() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: async ({ camId, pictureId }: { camId: number; pictureId: number }) => {
      return apiDelete<{ success: boolean; deleted_id: number }>(
        `/${camId}/api/media/picture/${pictureId}`
      );
    },
    onSuccess: (_, { camId }) => {
      // Invalidate pictures cache to refetch fresh data
      queryClient.invalidateQueries({ queryKey: queryKeys.pictures(camId) });
    },
  });
}

// Delete a movie
export function useDeleteMovie() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: async ({ camId, movieId }: { camId: number; movieId: number }) => {
      return apiDelete<{ success: boolean; deleted_id: number }>(
        `/${camId}/api/media/movie/${movieId}`
      );
    },
    onSuccess: (_, { camId }) => {
      // Invalidate movies cache to refetch fresh data
      queryClient.invalidateQueries({ queryKey: queryKeys.movies(camId) });
    },
  });
}


// Camera Detection - Get platform information
export function usePlatformInfo() {
  return useQuery({
    queryKey: queryKeys.platformInfo,
    queryFn: () => apiGet<PlatformInfo>("/0/api/cameras/platform"),
    staleTime: Infinity, // Platform info doesn't change during runtime
  });
}

// Camera Detection - Get detected cameras
// Only enabled when user is authenticated (to avoid 401 errors when not logged in)
export function useDetectedCameras(options?: { enabled?: boolean }) {
  return useQuery({
    queryKey: queryKeys.detectedCameras,
    queryFn: () => apiGet<DetectedCamerasResponse>("/0/api/cameras/detected"),
    staleTime: 30000, // Cache for 30 seconds
    enabled: options?.enabled ?? true,
    retry: false, // Don't retry on auth errors
  });
}

// Camera Detection - Add detected camera
export function useAddCamera() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: async (camera: AddCameraRequest) => {
      return apiPost<{ status: string; message: string }>("/0/api/cameras", camera as unknown as Record<string, unknown>);
    },
    onSuccess: () => {
      // Invalidate config and cameras cache to refetch fresh data
      queryClient.invalidateQueries({ queryKey: queryKeys.config(0) });
      queryClient.invalidateQueries({ queryKey: queryKeys.cameras });
      queryClient.invalidateQueries({ queryKey: queryKeys.detectedCameras });
    },
  });
}

// Camera Detection - Delete camera
export function useDeleteCamera() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: async ({ camId }: { camId: number }) => {
      return apiDelete<{ status: string; message: string }>(`/${camId}/api/cameras`);
    },
    onSuccess: () => {
      // Invalidate config and cameras cache to refetch fresh data
      queryClient.invalidateQueries({ queryKey: queryKeys.config(0) });
      queryClient.invalidateQueries({ queryKey: queryKeys.cameras });
      queryClient.invalidateQueries({ queryKey: queryKeys.detectedCameras });
    },
  });
}

// Camera Detection - Test network camera connection
export function useTestNetcam() {
  return useMutation({
    mutationFn: async (request: TestNetcamRequest) => {
      return apiPost<{ status: string; message: string }>("/0/api/cameras/test", request as unknown as Record<string, unknown>);
    },
  });
}

// PTZ Capabilities - Derive from camera config
export function usePtzCapabilities(cameraId: number) {
  const { data: config } = useMotionConfig();

  return useQuery({
    queryKey: ['ptzCapabilities', cameraId],
    queryFn: (): PtzCapabilities => {
      if (!config) {
        return {
          enabled: false,
          hasPanLeft: false,
          hasPanRight: false,
          hasTiltUp: false,
          hasTiltDown: false,
          hasZoomIn: false,
          hasZoomOut: false,
          hasAnyControl: false,
        };
      }

      // Get camera config (merge default + camera-specific)
      const defaultConfig = (config.configuration.default || {}) as Record<string, { value: string | number | boolean }>;
      const cameraConfig: Record<string, { value: string | number | boolean }> = cameraId === 0
        ? defaultConfig
        : { ...defaultConfig, ...((config.configuration[`cam${cameraId}`] || {}) as Record<string, { value: string | number | boolean }>) };

      // Check if PTZ overlay is enabled
      const enabled = Boolean(cameraConfig.stream_preview_ptz?.value ?? false);

      // Check which commands are configured (non-empty strings)
      const getCommandValue = (param: string): string => {
        return String(cameraConfig[param]?.value ?? '').trim();
      };

      const hasPanLeft = getCommandValue('ptz_pan_left').length > 0;
      const hasPanRight = getCommandValue('ptz_pan_right').length > 0;
      const hasTiltUp = getCommandValue('ptz_tilt_up').length > 0;
      const hasTiltDown = getCommandValue('ptz_tilt_down').length > 0;
      const hasZoomIn = getCommandValue('ptz_zoom_in').length > 0;
      const hasZoomOut = getCommandValue('ptz_zoom_out').length > 0;

      const hasAnyControl = hasPanLeft || hasPanRight || hasTiltUp || hasTiltDown || hasZoomIn || hasZoomOut;

      return {
        enabled,
        hasPanLeft,
        hasPanRight,
        hasTiltUp,
        hasTiltDown,
        hasZoomIn,
        hasZoomOut,
        hasAnyControl,
      };
    },
    enabled: !!config,
    staleTime: 60000, // Cache for 1 minute
  });
}

// PTZ Command - Send PTZ command to camera
export function useSendPtzCommand() {
  return useMutation({
    mutationFn: async ({ camId, action }: { camId: number; action: string }) => {
      return sendPtzCommand(camId, action);
    },
  });
}

