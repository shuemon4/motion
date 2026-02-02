## Build on Debian: 12 (FFMPEG 5.x)

### Build

Run make -j$(nproc)
  make -j$(nproc)
  shell: sh -e {0}
make  all-recursive
make[1]: Entering directory '/__w/motion/motion'
Making all in src
make[2]: Entering directory '/__w/motion/motion/src'
depbase=`echo alg.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg.o -MD -MP -MF $depbase.Tpo -c -o alg.o alg.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo alg_sec.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg_sec.o -MD -MP -MF $depbase.Tpo -c -o alg_sec.o alg_sec.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo cam_detect.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT cam_detect.o -MD -MP -MF $depbase.Tpo -c -o cam_detect.o cam_detect.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf.o -MD -MP -MF $depbase.Tpo -c -o conf.o conf.cpp &&\
mv -f $depbase.Tpo $depbase.Po
cam_detect.cpp: In member function 'bool cls_cam_detect::test_netcam(const std::string&, const std::string&, const std::string&, int)':
cam_detect.cpp:495:77: warning: unused parameter 'user' [-Wunused-parameter]
  495 | bool cls_cam_detect::test_netcam(const std::string &url, const std::string &user,
      |                                                          ~~~~~~~~~~~~~~~~~~~^~~~
cam_detect.cpp:496:52: warning: unused parameter 'pass' [-Wunused-parameter]
  496 |                                 const std::string &pass, int timeout_sec)
      |                                 ~~~~~~~~~~~~~~~~~~~^~~~
depbase=`echo conf_file.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_file.o -MD -MP -MF $depbase.Tpo -c -o conf_file.o conf_file.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf_profile.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_profile.o -MD -MP -MF $depbase.Tpo -c -o conf_profile.o conf_profile.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo dbse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT dbse.o -MD -MP -MF $depbase.Tpo -c -o dbse.o dbse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo draw.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT draw.o -MD -MP -MF $depbase.Tpo -c -o draw.o draw.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo jpegutils.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT jpegutils.o -MD -MP -MF $depbase.Tpo -c -o jpegutils.o jpegutils.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo json_parse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT json_parse.o -MD -MP -MF $depbase.Tpo -c -o json_parse.o json_parse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
jpegutils.cpp: In function 'int jpgutl_put_yuv420p(u_char*, int, u_char*, int, int, int, cls_camera*, timespec*, ctx_coord*)':
jpegutils.cpp:867:13: warning: variable 'pad_row' might be clobbered by 'longjmp' or 'vfork' [-Wclobbered]
  867 |     u_char *pad_row = nullptr;
      |             ^~~~~~~
depbase=`echo libcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
      |                                           ~~~~^~~~~~~~~~~~~
webu_json.cpp: In member function 'void cls_webu_json::api_system_status()':
webu_json.cpp:2195:53: warning: conversion from 'long unsigned int' to 'double' may change value [-Wconversion]
 2195 |             double mem_percent = (double)mem_used / mem_total * 100.0;
      |                                                     ^~~~~~~~~
webu_json.cpp:2212:52: warning: conversion from 'long long unsigned int' to 'double' may change value [-Wconversion]
 2212 |         double disk_percent = (double)used_bytes / total_bytes * 100.0;
      |                                                    ^~~~~~~~~~~
depbase=`echo webu_stream.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_stream.o -MD -MP -MF $depbase.Tpo -c -o webu_stream.o webu_stream.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_json.cpp: In lambda function:
webu_json.cpp:2345:27: warning: ignoring return value of 'int system(const char*)' declared with attribute 'warn_unused_result' [-Wunused-result]
 2345 |                     system("sudo /sbin/init 6");
      |                     ~~~~~~^~~~~~~~~~~~~~~~~~~~~
webu_json.cpp: In lambda function:
webu_json.cpp:2402:27: warning: ignoring return value of 'int system(const char*)' declared with attribute 'warn_unused_result' [-Wunused-result]
 2402 |                     system("sudo /sbin/init 0");
      |                     ~~~~~~^~~~~~~~~~~~~~~~~~~~~
webu_json.cpp: In lambda function:
webu_json.cpp:2455:15: warning: ignoring return value of 'int system(const char*)' declared with attribute 'warn_unused_result' [-Wunused-result]
 2455 |         system("sudo /usr/bin/systemctl restart motion");
      |         ~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
depbase=`echo webu_getimg.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_getimg.o -MD -MP -MF $depbase.Tpo -c -o webu_getimg.o webu_getimg.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo webu_mpegts.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_mpegts.o -MD -MP -MF $depbase.Tpo -c -o webu_mpegts.o webu_mpegts.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo motion_setup.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1    -I/usr/include/x86_64-linux-gnu   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT motion_setup.o -MD -MP -MF $depbase.Tpo -c -o motion_setup.o motion_setup.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_mpegts.cpp: In member function 'int cls_webu_mpegts::open_mpegts()':
webu_mpegts.cpp:405:17: error: invalid conversion from 'int (*)(void*, const uint8_t*, int)' {aka 'int (*)(void*, const unsigned char*, int)'} to 'int (*)(void*, uint8_t*, int)' {aka 'int (*)(void*, unsigned char*, int)'} [-fpermissive]
  405 |         , NULL, &webu_mpegts_avio_buf, NULL);
      |                 ^~~~~~~~~~~~~~~~~~~~~
      |                 |
      |                 int (*)(void*, const uint8_t*, int) {aka int (*)(void*, const unsigned char*, int)}
In file included from /usr/include/x86_64-linux-gnu/libavformat/avformat.h:321,
                 from motion.hpp:81,
                 from webu_mpegts.cpp:28:
/usr/include/x86_64-linux-gnu/libavformat/avio.h:416:25: note:   initializing argument 6 of 'AVIOContext* avio_alloc_context(unsigned char*, int, int, void*, int (*)(void*, uint8_t*, int), int (*)(void*, uint8_t*, int), int64_t (*)(void*, int64_t, int))'
  416 |                   int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
      |                   ~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
make[2]: *** [Makefile:540: webu_mpegts.o] Error 1
make[2]: *** Waiting for unfinished jobs....
make[2]: Leaving directory '/__w/motion/motion/src'
make[1]: *** [Makefile:581: all-recursive] Error 1
make[1]: Leaving directory '/__w/motion/motion'
make: *** [Makefile:425: all] Error 2
Error: Process completed with exit code 2.

## Build on Ubuntu:24.04 ((FFMPEG 6.x)

### Test Motion Binary

Run /tmp/motion-install/usr/local/bin/motion --help
/tmp/motion-install/usr/local/bin/motion: invalid option -- '-'
Motion version 5.0.0-gitUNKNOWN, Copyright 2020-2025

usage:	motion [options]


Possible options:

-b			Run in background (daemon) mode.
-n			Run in non-daemon mode.
-c config		Full path and filename of config file.
-d level		Log level (1-9) (EMG, ALR, CRT, ERR, WRN, NTC, INF, DBG, ALL). default: 6 / NTC.
-k type			Type of log (COR, STR, ENC, NET, DBL, EVT, TRK, VID, ALL). default: ALL.
-p process_id_file	Full path and filename of process id file (pid file).
-l log file 		Full path and filename of log file.
-m			Disable detection at startup.
-h			Show this screen.

Error: Process completed with exit code 1.

## Build on Fedora:40 (FFMPEG 6.x)

### Test Motion Binary

Run /tmp/motion-install/usr/local/bin/motion --help
/tmp/motion-install/usr/local/bin/motion: invalid option -- '-'
Motion version 5.0.0-gitUNKNOWN, Copyright 2020-2025

usage:	motion [options]


Possible options:

-b			Run in background (daemon) mode.
-n			Run in non-daemon mode.
-c config		Full path and filename of config file.
-d level		Log level (1-9) (EMG, ALR, CRT, ERR, WRN, NTC, INF, DBG, ALL). default: 6 / NTC.
-k type			Type of log (COR, STR, ENC, NET, DBL, EVT, TRK, VID, ALL). default: ALL.
-p process_id_file	Full path and filename of process id file (pid file).
-l log file 		Full path and filename of log file.
-m			Disable detection at startup.
-h			Show this screen.

Error: Process completed with exit code 1.