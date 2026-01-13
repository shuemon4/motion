// Camera from Motion's config
export interface Camera {
  id: number;
  name: string;
  url: string;
  width?: number;
  height?: number;
  all_xpct_st?: number;
  all_xpct_en?: number;
  all_ypct_st?: number;
  all_ypct_en?: number;
}

// Cameras list response from /0/config
export interface CamerasResponse {
  count: number;
  [key: string]: Camera | number;
}

// Full config response from /0/config
export interface MotionConfig {
  csrf_token?: string;
  version: string;
  cameras: CamerasResponse;
  configuration: {
    [key: string]: unknown;
  };
  categories: {
    [key: string]: {
      name: string;
      display: string;
    };
  };
}

// Single config parameter value
export interface ConfigParam {
  name: string;
  value: string | number | boolean;
  category?: string;
  type?: 'string' | 'number' | 'boolean' | 'list';
}

// Media item (snapshot or movie)
export interface MediaItem {
  id: number;
  filename: string;
  path: string;
  date: string;
  time?: string;
  size: number;
}

// Pictures API response from /{cam}/api/media/pictures
export interface PicturesResponse {
  pictures: MediaItem[];
}

// Movies API response from /{cam}/api/media/movies
export interface MoviesResponse {
  movies: MediaItem[];
}

// System temperature response from /0/api/system/temperature
export interface TemperatureResponse {
  celsius: number;
  fahrenheit: number;
}

// System status response from /0/api/system/status
export interface SystemStatus {
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
  version: string;
}

// API error
export interface ApiError {
  message: string;
  status?: number;
}

// Camera control capabilities from supportedControls in status.json
export interface CameraCapabilities {
  // Autofocus
  AfMode?: boolean;
  LensPosition?: boolean;
  AfTrigger?: boolean;
  AfRange?: boolean;
  AfSpeed?: boolean;
  AfMetering?: boolean;
  // Exposure
  ExposureTime?: boolean;
  ExposureValue?: boolean;
  AnalogueGain?: boolean;
  AeEnable?: boolean;
  AeMeteringMode?: boolean;
  AeConstraintMode?: boolean;
  AeExposureMode?: boolean;
  // White Balance
  AwbEnable?: boolean;
  AwbMode?: boolean;
  AwbLocked?: boolean;
  ColourGains?: boolean;
  ColourTemperature?: boolean;
  // Image Controls
  Brightness?: boolean;
  Contrast?: boolean;
  Saturation?: boolean;
  Sharpness?: boolean;
  // Other
  DigitalGain?: boolean;
  ScalerCrop?: boolean;
}
