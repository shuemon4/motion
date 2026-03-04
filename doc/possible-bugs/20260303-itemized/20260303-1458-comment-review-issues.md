# Code Review Issues — /comment run

Files reviewed: `src/movie.cpp`

## Potential Issues

### 1. Null pointer dereference in passthru_check() — status checked before nullptr guard
- **File**: `src/movie.cpp`
- **Line**: ~1326-1338 (passthru_check)
- **Severity**: high
- **Description**: `netcam_data->status` is accessed on line ~1328 before the `netcam_data == nullptr` check on line ~1335. If `netcam_data` is null, the status check will dereference a null pointer and SEGV. The nullptr guard should come first.
- **Code snippet**:
  ```cpp
  int cls_movie::passthru_check()
  {
      if ((netcam_data->status == NETCAM_NOTCONNECTED  ) ||  // <-- dereferences netcam_data
          (netcam_data->status == NETCAM_RECONNECTING  )) {
          ...
      }

      if (netcam_data == nullptr) {  // <-- too late, already dereferenced above
          ...
      }
  ```

### 2. Uninitialized retcd in passthru_streams() if stream is neither video nor audio
- **File**: `src/movie.cpp`
- **Line**: ~1302-1320 (passthru_streams)
- **Severity**: medium
- **Description**: If a stream in the netcam transfer format is neither `AVMEDIA_TYPE_VIDEO` nor `AVMEDIA_TYPE_AUDIO` (e.g., subtitle or data stream), the if/else-if block is skipped and `retcd` retains its uninitialized value. The subsequent `if (retcd < 0)` check on line ~1316 then reads an uninitialized variable, which is undefined behavior. In practice, `retcd` is declared at the top of the function and may contain stack garbage. This could cause a spurious early return.
- **Code snippet**:
  ```cpp
  int cls_movie::passthru_streams()
  {
      int         retcd, indx;  // retcd uninitialized
      ...
      for (indx= 0; indx < (int)netcam_data->transfer_format->nb_streams; indx++) {
          stream_in = netcam_data->transfer_format->streams[indx];
          if (stream_in->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
              retcd = passthru_streams_video(stream_in);
          } else if (stream_in->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
              retcd = passthru_streams_audio(stream_in);
          }
          // If stream is neither video nor audio, retcd is uninitialized here
          if (retcd < 0) {
              ...
          }
      }
  ```
