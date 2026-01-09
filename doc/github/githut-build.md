## build (g++, glibc)

Run g++ --version
g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
Copyright (C) 2023 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

make  all-recursive
make[1]: Entering directory '/home/runner/work/motion/motion'
Making all in src
make[2]: Entering directory '/home/runner/work/motion/motion/src'
depbase=`echo alg.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg.o -MD -MP -MF $depbase.Tpo -c -o alg.o alg.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo alg_sec.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg_sec.o -MD -MP -MF $depbase.Tpo -c -o alg_sec.o alg_sec.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf.o -MD -MP -MF $depbase.Tpo -c -o conf.o conf.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf_file.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_file.o -MD -MP -MF $depbase.Tpo -c -o conf_file.o conf_file.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf_profile.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_profile.o -MD -MP -MF $depbase.Tpo -c -o conf_profile.o conf_profile.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo dbse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT dbse.o -MD -MP -MF $depbase.Tpo -c -o dbse.o dbse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo draw.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT draw.o -MD -MP -MF $depbase.Tpo -c -o draw.o draw.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo jpegutils.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT jpegutils.o -MD -MP -MF $depbase.Tpo -c -o jpegutils.o jpegutils.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo json_parse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT json_parse.o -MD -MP -MF $depbase.Tpo -c -o json_parse.o json_parse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo libcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT libcam.o -MD -MP -MF $depbase.Tpo -c -o libcam.o libcam.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo logger.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT logger.o -MD -MP -MF $depbase.Tpo -c -o logger.o logger.cpp &&\
mv -f $depbase.Tpo $depbase.Po
libcam.cpp: In member function ‘void cls_libcam::config_controls()’:
libcam.cpp:778:73: warning: conversion from ‘int’ to ‘float’ may change value [-Wconversion]
  778 |     controls.set(controls::AnalogueGain, iso_to_gain(cam->cfg->parm_cam.libcam_iso));
  384 |         , NULL, &webu_mpegts_avio_buf, NULL);
      |                 ^~~~~~~~~~~~~~~~~~~~~
      |                 |
      |                 int (*)(void*, myuint*, int) {aka int (*)(void*, const unsigned char*, int)}
In file included from /usr/include/x86_64-linux-gnu/libavformat/avformat.h:319,
                 from motion.hpp:68,
                 from webu_mpegts.cpp:19:
/usr/include/x86_64-linux-gnu/libavformat/avio.h:420:25: note:   initializing argument 6 of ‘AVIOContext* avio_alloc_context(unsigned char*, int, int, void*, int (*)(void*, uint8_t*, int), int (*)(void*, uint8_t*, int), int64_t (*)(void*, int64_t, int))’
  420 |                   int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
      |                   ~~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
make[2]: *** [Makefile:523: webu_mpegts.o] Error 1
make[2]: *** Waiting for unfinished jobs....
make[2]: Leaving directory '/home/runner/work/motion/motion/src'
make[1]: *** [Makefile:601: all-recursive] Error 1
make[1]: Leaving directory '/home/runner/work/motion/motion'
make: *** [Makefile:422: all] Error 2
Error: Process completed with exit code 2.

## build (g++, musl)

Run autoreconf -fiv
autoreconf: export WARNINGS=
autoreconf: Entering directory '.'
autoreconf: running: autopoint --force
Copying file ABOUT-NLS
Copying file config.rpath
Creating directory m4
Copying file m4/codeset.m4
Copying file m4/extern-inline.m4
Copying file m4/fcntl-o.m4
Copying file m4/gettext.m4
Copying file m4/glibc2.m4
Copying file m4/glibc21.m4
Copying file m4/iconv.m4
Copying file m4/intdiv0.m4
Copying file m4/intl.m4
Copying file m4/intldir.m4
Copying file m4/intlmacosx.m4
Copying file m4/intmax.m4
Copying file m4/inttypes-pri.m4
Copying file m4/inttypes_h.m4
Copying file m4/lcmessage.m4
Copying file m4/lib-ld.m4
Copying file m4/lib-link.m4
Copying file m4/lib-prefix.m4
Copying file m4/lock.m4
Copying file m4/longlong.m4
Copying file m4/nls.m4
Copying file m4/po.m4
Copying file m4/printf-posix.m4
Copying file m4/progtest.m4
Copying file m4/size_max.m4
Copying file m4/stdint_h.m4
Copying file m4/threadlib.m4
Copying file m4/uintmax_t.m4
Copying file m4/visibility.m4
Copying file m4/wchar_t.m4
Copying file m4/wint_t.m4
Copying file m4/xsize.m4
Copying file po/Makefile.in.in
Copying file po/Makevars.template
Copying file po/Rules-quot
Copying file po/boldquot.sed
Copying file po/en@boldquot.header
Copying file po/en@quot.header
Copying file po/insert-header.sin
Copying file po/quot.sed
Copying file po/remove-potcdate.sin
autoreconf: running: aclocal --force -I m4
./scripts/version.sh: line 2: git: not found
./scripts/version.sh: line 2: git: not found
autoreconf: configure.ac: tracing
./scripts/version.sh: line 2: git: not found
./scripts/version.sh: line 2: git: not found
autoreconf: configure.ac: not using Libtool
autoreconf: configure.ac: not using Intltool
autoreconf: configure.ac: not using Gtkdoc
autoreconf: running: /usr/bin/autoconf --force
./scripts/version.sh: line 2: git: not found
./scripts/version.sh: line 2: git: not found
autoreconf: running: /usr/bin/autoheader --force
./scripts/version.sh: line 2: git: not found
./scripts/version.sh: line 2: git: not found
autoreconf: running: automake --add-missing --copy --force-missing
configure.ac:4: installing './compile'
configure.ac:8: installing './config.guess'
configure.ac:8: installing './config.sub'
configure.ac:2: installing './install-sh'
configure.ac:2: installing './missing'
src/Makefile.am: installing './depcomp'
autoreconf: Leaving directory '.'
./configure: exec: line 132: /bin/bash: not found
Error: Process completed with exit code 127.

## build (clang, glibc)

Run clang++ --version
Ubuntu clang version 18.1.3 (1ubuntu1)
Target: x86_64-pc-linux-gnu
Thread model: posix
InstalledDir: /usr/bin
make  all-recursive
make[1]: Entering directory '/home/runner/work/motion/motion'
Making all in src
make[2]: Entering directory '/home/runner/work/motion/motion/src'
depbase=`echo alg.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg.o -MD -MP -MF $depbase.Tpo -c -o alg.o alg.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo alg_sec.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg_sec.o -MD -MP -MF $depbase.Tpo -c -o alg_sec.o alg_sec.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf.o -MD -MP -MF $depbase.Tpo -c -o conf.o conf.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf_file.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_file.o -MD -MP -MF $depbase.Tpo -c -o conf_file.o conf_file.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf_profile.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_profile.o -MD -MP -MF $depbase.Tpo -c -o conf_profile.o conf_profile.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo dbse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT dbse.o -MD -MP -MF $depbase.Tpo -c -o dbse.o dbse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo draw.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT draw.o -MD -MP -MF $depbase.Tpo -c -o draw.o draw.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo jpegutils.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT jpegutils.o -MD -MP -MF $depbase.Tpo -c -o jpegutils.o jpegutils.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo json_parse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT json_parse.o -MD -MP -MF $depbase.Tpo -c -o json_parse.o json_parse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo libcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT libcam.o -MD -MP -MF $depbase.Tpo -c -o libcam.o libcam.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo logger.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT logger.o -MD -MP -MF $depbase.Tpo -c -o logger.o logger.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo motion.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT motion.o -MD -MP -MF $depbase.Tpo -c -o motion.o motion.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo allcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
depbase=`echo webu_mpegts.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1   -I/usr/include/webp  -I/usr/include/libcamera  -I/usr/include/x86_64-linux-gnu  -I/usr/include/opencv4  -I/usr/include/mariadb/  -I/usr/include/postgresql   -D_REENTRANT    -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_mpegts.o -MD -MP -MF $depbase.Tpo -c -o webu_mpegts.o webu_mpegts.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_json.cpp:680:60: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  680 |                 app->cam_list[indx]->set_libcam_brightness(atof(parm_val.c_str()));
      |                                      ~~~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:682:58: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  682 |                 app->cam_list[indx]->set_libcam_contrast(atof(parm_val.c_str()));
      |                                      ~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:684:53: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  684 |                 app->cam_list[indx]->set_libcam_iso(atof(parm_val.c_str()));
      |                                      ~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:694:27: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  694 |                 float r = atof(parm_val.c_str());
      |                       ~   ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:699:27: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  699 |                 float b = atof(parm_val.c_str());
      |                       ~   ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:704:63: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  704 |                 app->cam_list[indx]->set_libcam_lens_position(atof(parm_val.c_str()));
      |                                      ~~~~~~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:724:47: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  724 |             webua->cam->set_libcam_brightness(atof(parm_val.c_str()));
      |                         ~~~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:726:45: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  726 |             webua->cam->set_libcam_contrast(atof(parm_val.c_str()));
      |                         ~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:728:40: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  728 |             webua->cam->set_libcam_iso(atof(parm_val.c_str()));
      |                         ~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:738:23: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  738 |             float r = atof(parm_val.c_str());
      |                   ~   ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:743:23: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  743 |             float b = atof(parm_val.c_str());
      |                   ~   ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:748:50: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  748 |             webua->cam->set_libcam_lens_position(atof(parm_val.c_str()));
      |                         ~~~~~~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:1133:53: warning: implicit conversion from 'unsigned long' to 'double' may lose precision [-Wimplicit-int-float-conversion]
 1133 |             double mem_percent = (double)mem_used / mem_total * 100.0;
      |                                                   ~ ^~~~~~~~~
webu_json.cpp:1150:52: warning: implicit conversion from 'unsigned long long' to 'double' may lose precision [-Wimplicit-int-float-conversion]
 1150 |         double disk_percent = (double)used_bytes / total_bytes * 100.0;
      |                                                  ~ ^~~~~~~~~~~
webu_json.cpp:1216:21: warning: ignoring return value of function declared with 'warn_unused_result' attribute [-Wunused-result]
 1216 |                     system("sudo /sbin/init 6");
      |                     ^~~~~~ ~~~~~~~~~~~~~~~~~~~
webu_json.cpp:1273:21: warning: ignoring return value of function declared with 'warn_unused_result' attribute [-Wunused-result]
 1273 |                     system("sudo /sbin/init 0");
      |                     ^~~~~~ ~~~~~~~~~~~~~~~~~~~
webu_mpegts.cpp:382:18: error: no matching function for call to 'avio_alloc_context'
  382 |     fmtctx->pb = avio_alloc_context(
      |                  ^~~~~~~~~~~~~~~~~~
/usr/include/x86_64-linux-gnu/libavformat/avio.h:413:14: note: candidate function not viable: no known conversion from 'int (*)(void *, myuint *, int)' (aka 'int (*)(void *, const unsigned char *, int)') to 'int (*)(void *, uint8_t *, int)' (aka 'int (*)(void *, unsigned char *, int)') for 6th argument
  413 | AVIOContext *avio_alloc_context(
      |              ^
  414 |                   unsigned char *buffer,
  415 |                   int buffer_size,
  416 |                   int write_flag,
  417 |                   void *opaque,
  418 |                   int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
  419 | #if FF_API_AVIO_WRITE_NONCONST
  420 |                   int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
      |                   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
1 error generated.
make[2]: *** [Makefile:523: webu_mpegts.o] Error 1
make[2]: *** Waiting for unfinished jobs....
16 warnings generated.
make[2]: Leaving directory '/home/runner/work/motion/motion/src'
make[1]: *** [Makefile:601: all-recursive] Error 1
make[1]: Leaving directory '/home/runner/work/motion/motion'
make: *** [Makefile:422: all] Error 2
Error: Process completed with exit code 2.

## build (clang, musl)

Run autoreconf -fiv
autoreconf: export WARNINGS=
autoreconf: Entering directory '.'
autoreconf: running: autopoint --force
Copying file ABOUT-NLS
Copying file config.rpath
Creating directory m4
Copying file m4/codeset.m4
Copying file m4/extern-inline.m4
Copying file m4/fcntl-o.m4
Copying file m4/gettext.m4
Copying file m4/glibc2.m4
Copying file m4/glibc21.m4
Copying file m4/iconv.m4
Copying file m4/intdiv0.m4
Copying file m4/intl.m4
Copying file m4/intldir.m4
Copying file m4/intlmacosx.m4
Copying file m4/intmax.m4
Copying file m4/inttypes-pri.m4
Copying file m4/inttypes_h.m4
Copying file m4/lcmessage.m4
Copying file m4/lib-ld.m4
Copying file m4/lib-link.m4
Copying file m4/lib-prefix.m4
Copying file m4/lock.m4
Copying file m4/longlong.m4
Copying file m4/nls.m4
Copying file m4/po.m4
Copying file m4/printf-posix.m4
Copying file m4/progtest.m4
Copying file m4/size_max.m4
Copying file m4/stdint_h.m4
Copying file m4/threadlib.m4
Copying file m4/uintmax_t.m4
Copying file m4/visibility.m4
Copying file m4/wchar_t.m4
Copying file m4/wint_t.m4
Copying file m4/xsize.m4
Copying file po/Makefile.in.in
Copying file po/Makevars.template
Copying file po/Rules-quot
Copying file po/boldquot.sed
Copying file po/en@boldquot.header
Copying file po/en@quot.header
Copying file po/insert-header.sin
Copying file po/quot.sed
Copying file po/remove-potcdate.sin
autoreconf: running: aclocal --force -I m4
./scripts/version.sh: line 2: git: not found
./scripts/version.sh: line 2: git: not found
autoreconf: configure.ac: tracing
./scripts/version.sh: line 2: git: not found
./scripts/version.sh: line 2: git: not found
autoreconf: configure.ac: not using Libtool
autoreconf: configure.ac: not using Intltool
autoreconf: configure.ac: not using Gtkdoc
autoreconf: running: /usr/bin/autoconf --force
./scripts/version.sh: line 2: git: not found
./scripts/version.sh: line 2: git: not found
autoreconf: running: /usr/bin/autoheader --force
./scripts/version.sh: line 2: git: not found
./scripts/version.sh: line 2: git: not found
autoreconf: running: automake --add-missing --copy --force-missing
configure.ac:4: installing './compile'
configure.ac:8: installing './config.guess'
configure.ac:8: installing './config.sub'
configure.ac:2: installing './install-sh'
configure.ac:2: installing './missing'
src/Makefile.am: installing './depcomp'
autoreconf: Leaving directory '.'
./configure: exec: line 132: /bin/bash: not found
Error: Process completed with exit code 127.