import { useQuery } from '@tanstack/react-query';
import { apiGet } from '@/api/client';

export interface DeviceInfo {
  device_model?: string;
  pi_generation?: number;
  hardware_encoders?: {
    h264_v4l2m2m: boolean;
  };
  temperature?: {
    celsius: number;
    fahrenheit: number;
  };
  uptime?: {
    seconds: number;
    days: number;
    hours: number;
  };
  memory?: {
    total: number;
    used: number;
    free: number;
    available: number;
    percent: number;
  };
  disk?: {
    total: number;
    used: number;
    free: number;
    available: number;
    percent: number;
  };
  version?: string;
}

export function useDeviceInfo(options?: { enabled?: boolean }) {
  return useQuery({
    queryKey: ['deviceInfo'],
    queryFn: () => apiGet<DeviceInfo>('/0/api/system/status'),
    staleTime: 60000, // Cache for 1 minute
    retry: false, // Don't retry on auth errors
    enabled: options?.enabled ?? true,
  });
}

// Helper functions for device detection
export function isPi5(deviceInfo?: DeviceInfo): boolean {
  return deviceInfo?.pi_generation === 5;
}

export function isPi4(deviceInfo?: DeviceInfo): boolean {
  return deviceInfo?.pi_generation === 4;
}

export function isPi3(deviceInfo?: DeviceInfo): boolean {
  return deviceInfo?.pi_generation === 3;
}

export function isRaspberryPi(deviceInfo?: DeviceInfo): boolean {
  return (deviceInfo?.pi_generation ?? 0) > 0;
}

export function hasHardwareEncoder(deviceInfo?: DeviceInfo): boolean {
  return deviceInfo?.hardware_encoders?.h264_v4l2m2m === true;
}

export function isHighTemperature(deviceInfo?: DeviceInfo, threshold = 70): boolean {
  return (deviceInfo?.temperature?.celsius ?? 0) > threshold;
}
