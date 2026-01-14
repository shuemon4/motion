## Build (g++, musl)
Run jirutka/setup-alpine@master
Run sudo -E ./setup-alpine.sh
Prepare rootfs directory
Download static apk-tools
Initialize Alpine Linux edge (x86_64)
Bind filesystems into chroot
Copy action scripts
Install packages
  ▷ Installing build-base clang file autoconf autoconf-archive automake libtool bash gettext-dev libzip-dev jpeg-dev v4l-utils-libs libcamera-dev opencv-dev openexr-dev imath-dev ffmpeg-dev libmicrohttpd-dev sqlite-dev mariadb-dev alsa-lib-dev pulseaudio-dev fftw-dev 
  ERROR: unable to select packages:
    so:libicui18n.so.76 (no such package):
      required by: qt6-qtbase-6.10.1-r0[so:libicui18n.so.76]
                   qt5-qtbase-5.15.10_git20230714-r4[so:libicui18n.so.76]
    so:libicuuc.so.76 (no such package):
      required by: qt6-qtbase-6.10.1-r0[so:libicuuc.so.76]
                   qt5-qtbase-5.15.10_git20230714-r4[so:libicuuc.so.76]
                   xerces-c-3.2.5-r3[so:libicuuc.so.76]
                   re2-2025.11.05-r0[so:libicuuc.so.76]
  
  Error occurred at line 318:
    315 | 		echo '▷ Installing $pkgs'
    316 | 		apk add --update-cache $pkgs
    317 | 	SHELL
  > 318 | 	abin/"$INPUT_SHELL_NAME" --root /.setup.sh
    319 | fi
    320 | 
    321 | 
  Error: Error occurred at line 318: 	abin/"$INPUT_SHELL_NAME" --root /.setup.sh (see the job log for more information)
  Error: Process completed with exit code 1.

## (g++, glibc)
Successful

## Build (clang++, musl)
Run jirutka/setup-alpine@master
  with:
    branch: edge
    packages: build-base clang file autoconf autoconf-archive automake libtool bash gettext-dev libzip-dev jpeg-dev v4l-utils-libs libcamera-dev opencv-dev openexr-dev imath-dev ffmpeg-dev libmicrohttpd-dev sqlite-dev mariadb-dev alsa-lib-dev pulseaudio-dev fftw-dev
  
    apk-tools-url: https://gitlab.alpinelinux.org/api/v4/projects/5/packages/generic/v2.14.10/x86_64/apk.static#!sha256!34bb1a96f0258982377a289392d4ea9f3f4b767a4bb5806b1b87179b79ad8a1c
    arch: x86_64
    mirror-url: http://dl-cdn.alpinelinux.org/alpine
    shell-name: alpine.sh
Run sudo -E ./setup-alpine.sh
Prepare rootfs directory
Download static apk-tools
Initialize Alpine Linux edge (x86_64)
Bind filesystems into chroot
Copy action scripts
Install packages
  ▷ Installing build-base clang file autoconf autoconf-archive automake libtool bash gettext-dev libzip-dev jpeg-dev v4l-utils-libs libcamera-dev opencv-dev openexr-dev imath-dev ffmpeg-dev libmicrohttpd-dev sqlite-dev mariadb-dev alsa-lib-dev pulseaudio-dev fftw-dev 
  ERROR: unable to select packages:
    so:libicui18n.so.76 (no such package):
      required by: qt6-qtbase-6.10.1-r0[so:libicui18n.so.76]
                   qt5-qtbase-5.15.10_git20230714-r4[so:libicui18n.so.76]
    so:libicuuc.so.76 (no such package):
      required by: qt6-qtbase-6.10.1-r0[so:libicuuc.so.76]
                   qt5-qtbase-5.15.10_git20230714-r4[so:libicuuc.so.76]
                   xerces-c-3.2.5-r3[so:libicuuc.so.76]
                   re2-2025.11.05-r0[so:libicuuc.so.76]
  
  Error occurred at line 318:
    315 | 		echo '▷ Installing $pkgs'
    316 | 		apk add --update-cache $pkgs
    317 | 	SHELL
  > 318 | 	abin/"$INPUT_SHELL_NAME" --root /.setup.sh
    319 | fi
    320 | 
    321 | 
  Error: Error occurred at line 318: 	abin/"$INPUT_SHELL_NAME" --root /.setup.sh (see the job log for more information)
  Error: Process completed with exit code 1.

## Build (clang++, glibc)
Successful