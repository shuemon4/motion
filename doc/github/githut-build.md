## Build (g++, musl)
Run g++ --version
  g++ --version
  make -j $(($(nproc)+1))
  shell: /home/runner/rootfs/alpine-edge-x86_64/abin/alpine.sh {0}
g++ (Alpine 15.2.0) 15.2.0
Copyright (C) 2025 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

make  all-recursive
make[1]: Entering directory '/home/runner/work/motion/motion'
Making all in src
make[2]: Entering directory '/home/runner/work/motion/motion/src'
depbase=`echo alg.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg.o -MD -MP -MF $depbase.Tpo -c -o alg.o alg.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo alg_sec.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg_sec.o -MD -MP -MF $depbase.Tpo -c -o alg_sec.o alg_sec.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf.o -MD -MP -MF $depbase.Tpo -c -o conf.o conf.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf_file.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_file.o -MD -MP -MF $depbase.Tpo -c -o conf_file.o conf_file.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf_profile.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_profile.o -MD -MP -MF $depbase.Tpo -c -o conf_profile.o conf_profile.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo dbse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT dbse.o -MD -MP -MF $depbase.Tpo -c -o dbse.o dbse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo draw.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT draw.o -MD -MP -MF $depbase.Tpo -c -o draw.o draw.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo jpegutils.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT jpegutils.o -MD -MP -MF $depbase.Tpo -c -o jpegutils.o jpegutils.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo json_parse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT json_parse.o -MD -MP -MF $depbase.Tpo -c -o json_parse.o json_parse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo libcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT libcam.o -MD -MP -MF $depbase.Tpo -c -o libcam.o libcam.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo logger.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT logger.o -MD -MP -MF $depbase.Tpo -c -o logger.o logger.cpp &&\
mv -f $depbase.Tpo $depbase.Po
logger.cpp: In member function 'void cls_log::add_errmsg(int, int)':
logger.cpp:163:51: warning: format '%s' expects argument of type 'char*', but argument 4 has type 'int' [-Wformat=]
  163 |         (void)snprintf(err_buf, sizeof(err_buf),"%s"
      |                                                  ~^
      |                                                   |
      |                                                   char*
      |                                                  %d
  164 |             , strerror_r(err_save, err_buf, sizeof(err_buf)));
      |               ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      |                         |
      |                         int
depbase=`echo motion.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT motion.o -MD -MP -MF $depbase.Tpo -c -o motion.o motion.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo allcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT allcam.o -MD -MP -MF $depbase.Tpo -c -o allcam.o allcam.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo schedule.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT schedule.o -MD -MP -MF $depbase.Tpo -c -o schedule.o schedule.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo camera.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT camera.o -MD -MP -MF $depbase.Tpo -c -o camera.o camera.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo movie.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT movie.o -MD -MP -MF $depbase.Tpo -c -o movie.o movie.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo netcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT netcam.o -MD -MP -MF $depbase.Tpo -c -o netcam.o netcam.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo parm_registry.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT parm_registry.o -MD -MP -MF $depbase.Tpo -c -o parm_registry.o parm_registry.cpp &&\
mv -f $depbase.Tpo $depbase.Po
netcam.cpp:45:49: warning: '/*' within comment [-Wcomment]
   45 |  * Converts ***host/path to ***host/path
netcam.cpp: In member function 'char* cls_netcam::url_match(regmatch_t, const char*)':
netcam.cpp:275:23: warning: conversion from 'regoff_t' {aka 'long int'} to 'int' may change value [-Wconversion]
  275 |         len = m.rm_eo - m.rm_so;
      |               ~~~~~~~~^~~~~~~~~
netcam.cpp: At global scope:
netcam.cpp:50:20: warning: 'std::string mask_url_credentials(const std::string&)' defined but not used [-Wunused-function]
   50 | static std::string mask_url_credentials(const std::string &url)
      |                    ^~~~~~~~~~~~~~~~~~~~
depbase=`echo picture.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT picture.o -MD -MP -MF $depbase.Tpo -c -o picture.o picture.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo rotate.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT rotate.o -MD -MP -MF $depbase.Tpo -c -o rotate.o rotate.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo sound.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT sound.o -MD -MP -MF $depbase.Tpo -c -o sound.o sound.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo util.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT util.o -MD -MP -MF $depbase.Tpo -c -o util.o util.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo video_v4l2.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT video_v4l2.o -MD -MP -MF $depbase.Tpo -c -o video_v4l2.o video_v4l2.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo video_convert.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT video_convert.o -MD -MP -MF $depbase.Tpo -c -o video_convert.o video_convert.cpp &&\
mv -f $depbase.Tpo $depbase.Po
video_v4l2.cpp: In member function 'int cls_v4l2cam::xioctl(long unsigned int, void*)':
video_v4l2.cpp:94:34: warning: conversion from 'long unsigned int' to 'int' may change value [-Wconversion]
   94 |         retcd = ioctl(fd_device, request, arg);
      |                                  ^~~~~~~
depbase=`echo video_loopback.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT video_loopback.o -MD -MP -MF $depbase.Tpo -c -o video_loopback.o video_loopback.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo webu.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu.o -MD -MP -MF $depbase.Tpo -c -o webu.o webu.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo webu_ans.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_ans.o -MD -MP -MF $depbase.Tpo -c -o webu_ans.o webu_ans.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo webu_file.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_file.o -MD -MP -MF $depbase.Tpo -c -o webu_file.o webu_file.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_ans.cpp:843:48: warning: '/*' within comment [-Wcomment]
  843 |      * 1. Static files (device_id < 0): /assets/* etc.
webu_ans.cpp:1111:46: warning: '/*' within comment [-Wcomment]
 1111 |      * This allows serving files like /assets/*, /settings, / without a camera ID */
webu_file.cpp:116:13: warning: '/*' within comment [-Wcomment]
  116 |  * - /assets/* files are hashed, cache aggressively
depbase=`echo webu_json.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_json.o -MD -MP -MF $depbase.Tpo -c -o webu_json.o webu_json.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo webu_post.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_post.o -MD -MP -MF $depbase.Tpo -c -o webu_post.o webu_post.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_json.cpp: In member function 'void cls_webu_json::apply_hot_reload(int, const std::string&)':
webu_json.cpp:689:64: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  689 |                 app->cam_list[indx]->set_libcam_brightness(atof(parm_val.c_str()));
      |                                                            ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:691:62: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  691 |                 app->cam_list[indx]->set_libcam_contrast(atof(parm_val.c_str()));
      |                                                          ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:693:58: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  693 |                 app->cam_list[indx]->set_libcam_gain(atof(parm_val.c_str()));
      |                                                      ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:703:31: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  703 |                 float r = atof(parm_val.c_str());
      |                           ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:708:31: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  708 |                 float b = atof(parm_val.c_str());
      |                           ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:713:67: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  713 |                 app->cam_list[indx]->set_libcam_lens_position(atof(parm_val.c_str()));
      |                                                               ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:733:51: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  733 |             webua->cam->set_libcam_brightness(atof(parm_val.c_str()));
      |                                               ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:735:49: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  735 |             webua->cam->set_libcam_contrast(atof(parm_val.c_str()));
      |                                             ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:737:45: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  737 |             webua->cam->set_libcam_gain(atof(parm_val.c_str()));
      |                                         ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:747:27: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  747 |             float r = atof(parm_val.c_str());
      |                       ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:752:27: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  752 |             float b = atof(parm_val.c_str());
      |                       ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp:757:54: warning: conversion from 'double' to 'float' may change value [-Wfloat-conversion]
  757 |             webua->cam->set_libcam_lens_position(atof(parm_val.c_str()));
      |                                                  ~~~~^~~~~~~~~~~~~~~~~~
webu_json.cpp: In member function 'void cls_webu_json::api_system_status()':
webu_json.cpp:1303:53: warning: conversion from 'long unsigned int' to 'double' may change value [-Wconversion]
 1303 |             double mem_percent = (double)mem_used / mem_total * 100.0;
      |                                                     ^~~~~~~~~
webu_json.cpp:1320:52: warning: conversion from 'long long unsigned int' to 'double' may change value [-Wconversion]
 1320 |         double disk_percent = (double)used_bytes / total_bytes * 100.0;
      |                                                    ^~~~~~~~~~~
depbase=`echo webu_stream.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_stream.o -MD -MP -MF $depbase.Tpo -c -o webu_stream.o webu_stream.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_ans.cpp: In member function 'mhdrslt cls_webu_ans::mhd_digest()':
webu_ans.cpp:663:5: warning: 'retcd' may be used uninitialized [-Wmaybe-uninitialized]
  663 |     if (retcd == MHD_NO) {
      |     ^~
webu_ans.cpp:607:9: note: 'retcd' was declared here
  607 |     int retcd;
      |         ^~~~~
depbase=`echo webu_getimg.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_getimg.o -MD -MP -MF $depbase.Tpo -c -o webu_getimg.o webu_getimg.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo webu_mpegts.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
g++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_mpegts.o -MD -MP -MF $depbase.Tpo -c -o webu_mpegts.o webu_mpegts.cpp &&\
mv -f $depbase.Tpo $depbase.Po
g++  -std=c++17  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -o motion alg.o alg_sec.o conf.o conf_file.o conf_profile.o dbse.o draw.o jpegutils.o json_parse.o libcam.o logger.o motion.o allcam.o schedule.o camera.o movie.o netcam.o parm_registry.o picture.o rotate.o sound.o util.o video_v4l2.o video_convert.o video_loopback.o webu.o webu_ans.o webu_file.o webu_json.o webu_post.o webu_stream.o webu_getimg.o webu_mpegts.o -lintl -pthread   -ljpeg -lmicrohttpd -lz -lwebpmux -lwebp -lcamera -lcamera-base -lavdevice -lswscale -lavformat -lavcodec -lavutil -lopencv_aruco -lopencv_face -lopencv_ml -lopencv_objdetect -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_optflow -lopencv_tracking -lopencv_highgui -lopencv_plot -lopencv_videostab -lopencv_videoio -lopencv_photo -lopencv_ximgproc -lopencv_video -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_dnn -lopencv_imgproc -lopencv_flann -lopencv_core -L/usr/lib/ -lmariadb -lpq -lsqlite3 -lpulse -pthread -lpulse-simple  -lasound -lfftw3
/usr/lib/gcc/x86_64-alpine-linux-musl/15.2.0/../../../../x86_64-alpine-linux-musl/bin/ld: /usr/lib//libopencv_imgcodecs.so: undefined reference to `Imf_3_4::Chromaticities::Chromaticities(Imath_3_1::Vec2<float> const&, Imath_3_1::Vec2<float> const&, Imath_3_1::Vec2<float> const&, Imath_3_1::Vec2<float> const&)'
/usr/lib/gcc/x86_64-alpine-linux-musl/15.2.0/../../../../x86_64-alpine-linux-musl/bin/ld: /usr/lib//libopencv_imgcodecs.so: undefined reference to `Imf_3_4::Header::Header(int, int, float, Imath_3_1::Vec2<float> const&, float, Imf_3_4::LineOrder, Imf_3_4::Compression)'
collect2: error: ld returned 1 exit status
make[2]: *** [Makefile:471: motion] Error 1
make[2]: Leaving directory '/home/runner/work/motion/motion/src'
make[1]: *** [Makefile:609: all-recursive] Error 1
make[1]: Leaving directory '/home/runner/work/motion/motion'
make: *** [Makefile:430: all] Error 2
Error: Process completed with exit code 2.

## (g++, glibc)
Successful

## Build (clang++, musl)
Run clang++ --version
  clang++ --version
  make -j $(($(nproc)+1))
  shell: /home/runner/rootfs/alpine-edge-x86_64/abin/alpine.sh {0}
Alpine clang version 21.1.8
Target: x86_64-alpine-linux-musl
Thread model: posix
InstalledDir: /usr/lib/llvm21/bin
Configuration file: /etc/clang21/x86_64-alpine-linux-musl.cfg
make  all-recursive
make[1]: Entering directory '/home/runner/work/motion/motion'
Making all in src
make[2]: Entering directory '/home/runner/work/motion/motion/src'
depbase=`echo alg.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg.o -MD -MP -MF $depbase.Tpo -c -o alg.o alg.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo alg_sec.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT alg_sec.o -MD -MP -MF $depbase.Tpo -c -o alg_sec.o alg_sec.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf.o -MD -MP -MF $depbase.Tpo -c -o conf.o conf.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf_file.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_file.o -MD -MP -MF $depbase.Tpo -c -o conf_file.o conf_file.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo conf_profile.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT conf_profile.o -MD -MP -MF $depbase.Tpo -c -o conf_profile.o conf_profile.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo dbse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT dbse.o -MD -MP -MF $depbase.Tpo -c -o dbse.o dbse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo draw.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT draw.o -MD -MP -MF $depbase.Tpo -c -o draw.o draw.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo jpegutils.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT jpegutils.o -MD -MP -MF $depbase.Tpo -c -o jpegutils.o jpegutils.cpp &&\
mv -f $depbase.Tpo $depbase.Po
draw.cpp:1271:28: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1271 |         out[width_miny_x] =~out[width_miny_x];
      |                           ~^~~~~~~~~~~~~~~~~~
draw.cpp:1272:28: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1272 |         out[width_maxy_x] =~out[width_maxy_x];
      |                           ~^~~~~~~~~~~~~~~~~~
draw.cpp:1279:28: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1279 |         out[width_minx_y] =~out[width_minx_y];
      |                           ~^~~~~~~~~~~~~~~~~~
draw.cpp:1280:28: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1280 |         out[width_maxx_y] =~out[width_maxx_y];
      |                           ~^~~~~~~~~~~~~~~~~~
draw.cpp:1290:36: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1290 |             new_var[width_miny_x] =~new_var[width_miny_x];
      |                                   ~^~~~~~~~~~~~~~~~~~~~~~
draw.cpp:1291:36: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1291 |             new_var[width_maxy_x] =~new_var[width_maxy_x];
      |                                   ~^~~~~~~~~~~~~~~~~~~~~~
draw.cpp:1297:36: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1297 |             new_var[width_minx_y] =~new_var[width_minx_y];
      |                                   ~^~~~~~~~~~~~~~~~~~~~~~
draw.cpp:1298:36: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1298 |             new_var[width_maxx_y] =~new_var[width_maxx_y];
      |                                   ~^~~~~~~~~~~~~~~~~~~~~~
draw.cpp:1303:33: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1303 |             new_var[centy + x] =~new_var[centy + x];
      |                                ~^~~~~~~~~~~~~~~~~~~
draw.cpp:1304:29: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1304 |             out[centy + x] =~out[centy + x];
      |                            ~^~~~~~~~~~~~~~~
draw.cpp:1307:43: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1307 |             new_var[cent->x + y * width] =~new_var[cent->x + y * width];
      |                                          ~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
draw.cpp:1308:39: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1308 |             out[cent->x + y * width] =~out[cent->x + y * width];
      |                                      ~^~~~~~~~~~~~~~~~~~~~~~~~~
draw.cpp:1340:28: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1340 |         out[width_miny_x] =~out[width_miny_x];
      |                           ~^~~~~~~~~~~~~~~~~~
draw.cpp:1341:28: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1341 |         out[width_maxy_x] =~out[width_maxy_x];
      |                           ~^~~~~~~~~~~~~~~~~~
draw.cpp:1348:28: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1348 |         out[width_minx_y] =~out[width_minx_y];
      |                           ~^~~~~~~~~~~~~~~~~~
draw.cpp:1349:28: warning: implicit conversion loses integer precision: 'int' to 'u_char' (aka 'unsigned char') [-Wimplicit-int-conversion]
 1349 |         out[width_maxx_y] =~out[width_maxx_y];
      |                           ~^~~~~~~~~~~~~~~~~~
depbase=`echo json_parse.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT json_parse.o -MD -MP -MF $depbase.Tpo -c -o json_parse.o json_parse.cpp &&\
mv -f $depbase.Tpo $depbase.Po
16 warnings generated.
depbase=`echo libcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT libcam.o -MD -MP -MF $depbase.Tpo -c -o libcam.o libcam.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo logger.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT logger.o -MD -MP -MF $depbase.Tpo -c -o logger.o logger.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo motion.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT motion.o -MD -MP -MF $depbase.Tpo -c -o motion.o motion.cpp &&\
mv -f $depbase.Tpo $depbase.Po
logger.cpp:164:15: warning: format specifies type 'char *' but the argument has type 'int' [-Wformat]
  163 |         (void)snprintf(err_buf, sizeof(err_buf),"%s"
      |                                                  ~~
      |                                                  %d
  164 |             , strerror_r(err_save, err_buf, sizeof(err_buf)));
      |               ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
depbase=`echo allcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT allcam.o -MD -MP -MF $depbase.Tpo -c -o allcam.o allcam.cpp &&\
mv -f $depbase.Tpo $depbase.Po
1 warning generated.
depbase=`echo schedule.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT schedule.o -MD -MP -MF $depbase.Tpo -c -o schedule.o schedule.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo camera.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT camera.o -MD -MP -MF $depbase.Tpo -c -o camera.o camera.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo movie.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT movie.o -MD -MP -MF $depbase.Tpo -c -o movie.o movie.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo netcam.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT netcam.o -MD -MP -MF $depbase.Tpo -c -o netcam.o netcam.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo parm_registry.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT parm_registry.o -MD -MP -MF $depbase.Tpo -c -o parm_registry.o parm_registry.cpp &&\
mv -f $depbase.Tpo $depbase.Po
netcam.cpp:45:49: warning: '/*' within block comment [-Wcomment]
   45 |  * Converts ***host/path to ***host/path
      |                                                 ^
netcam.cpp:275:23: warning: implicit conversion loses integer precision: 'regoff_t' (aka 'long') to 'int' [-Wshorten-64-to-32]
  275 |         len = m.rm_eo - m.rm_so;
      |             ~ ~~~~~~~~^~~~~~~~~
netcam.cpp:50:20: warning: unused function 'mask_url_credentials' [-Wunused-function]
   50 | static std::string mask_url_credentials(const std::string &url)
      |                    ^~~~~~~~~~~~~~~~~~~~
depbase=`echo picture.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT picture.o -MD -MP -MF $depbase.Tpo -c -o picture.o picture.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo rotate.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT rotate.o -MD -MP -MF $depbase.Tpo -c -o rotate.o rotate.cpp &&\
mv -f $depbase.Tpo $depbase.Po
3 warnings generated.
depbase=`echo sound.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT sound.o -MD -MP -MF $depbase.Tpo -c -o sound.o sound.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo util.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT util.o -MD -MP -MF $depbase.Tpo -c -o util.o util.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo video_v4l2.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT video_v4l2.o -MD -MP -MF $depbase.Tpo -c -o video_v4l2.o video_v4l2.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo video_convert.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT video_convert.o -MD -MP -MF $depbase.Tpo -c -o video_convert.o video_convert.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo video_loopback.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT video_loopback.o -MD -MP -MF $depbase.Tpo -c -o video_loopback.o video_loopback.cpp &&\
mv -f $depbase.Tpo $depbase.Po
video_v4l2.cpp:94:34: warning: implicit conversion loses integer precision: 'unsigned long' to 'int' [-Wshorten-64-to-32]
   94 |         retcd = ioctl(fd_device, request, arg);
      |                 ~~~~~            ^~~~~~~
depbase=`echo webu.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu.o -MD -MP -MF $depbase.Tpo -c -o webu.o webu.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo webu_ans.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_ans.o -MD -MP -MF $depbase.Tpo -c -o webu_ans.o webu_ans.cpp &&\
mv -f $depbase.Tpo $depbase.Po
1 warning generated.
depbase=`echo webu_file.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_file.o -MD -MP -MF $depbase.Tpo -c -o webu_file.o webu_file.cpp &&\
mv -f $depbase.Tpo $depbase.Po
depbase=`echo webu_json.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_json.o -MD -MP -MF $depbase.Tpo -c -o webu_json.o webu_json.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_ans.cpp:647:16: warning: variable 'retcd' is used uninitialized whenever 'if' condition is false [-Wsometimes-uninitialized]
  647 |     } else if (is_user) {
      |                ^~~~~~~
webu_ans.cpp:663:9: note: uninitialized use occurs here
  663 |     if (retcd == MHD_NO) {
      |         ^~~~~
webu_ans.cpp:647:12: note: remove the 'if' if its condition is always true
  647 |     } else if (is_user) {
      |            ^~~~~~~~~~~~
webu_ans.cpp:607:14: note: initialize the variable 'retcd' to silence this warning
  607 |     int retcd;
      |              ^
      |               = 0
webu_ans.cpp:843:48: warning: '/*' within block comment [-Wcomment]
  843 |      * 1. Static files (device_id < 0): /assets/* etc.
      |                                                ^
webu_ans.cpp:1111:46: warning: '/*' within block comment [-Wcomment]
 1111 |      * This allows serving files like /assets/*, /settings, / without a camera ID */
      |                                              ^
depbase=`echo webu_post.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_post.o -MD -MP -MF $depbase.Tpo -c -o webu_post.o webu_post.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_file.cpp:116:13: warning: '/*' within block comment [-Wcomment]
  116 |  * - /assets/* files are hashed, cache aggressively
      |             ^
depbase=`echo webu_stream.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_stream.o -MD -MP -MF $depbase.Tpo -c -o webu_stream.o webu_stream.cpp &&\
mv -f $depbase.Tpo $depbase.Po
1 warning generated.
depbase=`echo webu_getimg.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_getimg.o -MD -MP -MF $depbase.Tpo -c -o webu_getimg.o webu_getimg.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_json.cpp:689:60: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  689 |                 app->cam_list[indx]->set_libcam_brightness(atof(parm_val.c_str()));
      |                                      ~~~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:691:58: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  691 |                 app->cam_list[indx]->set_libcam_contrast(atof(parm_val.c_str()));
      |                                      ~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:693:54: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  693 |                 app->cam_list[indx]->set_libcam_gain(atof(parm_val.c_str()));
      |                                      ~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:703:27: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  703 |                 float r = atof(parm_val.c_str());
      |                       ~   ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:708:27: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  708 |                 float b = atof(parm_val.c_str());
      |                       ~   ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:713:63: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  713 |                 app->cam_list[indx]->set_libcam_lens_position(atof(parm_val.c_str()));
      |                                      ~~~~~~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:733:47: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  733 |             webua->cam->set_libcam_brightness(atof(parm_val.c_str()));
      |                         ~~~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:735:45: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  735 |             webua->cam->set_libcam_contrast(atof(parm_val.c_str()));
      |                         ~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:737:41: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  737 |             webua->cam->set_libcam_gain(atof(parm_val.c_str()));
      |                         ~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:747:23: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  747 |             float r = atof(parm_val.c_str());
      |                   ~   ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:752:23: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  752 |             float b = atof(parm_val.c_str());
      |                   ~   ^~~~~~~~~~~~~~~~~~~~~~
webu_json.cpp:757:50: warning: implicit conversion loses floating-point precision: 'double' to 'float' [-Wimplicit-float-conversion]
  757 |             webua->cam->set_libcam_lens_position(atof(parm_val.c_str()));
      |                         ~~~~~~~~~~~~~~~~~~~~~~~~ ^~~~~~~~~~~~~~~~~~~~~~
3 warnings generated.
depbase=`echo webu_mpegts.o | sed 's|[^/]*$|.deps/&|;s|\.o$||'`;\
clang++ -DHAVE_CONFIG_H -I. -I..  -Dconfigdir=\"/usr/local/var/lib/motion\" -Dsysconfdir=\"/usr/local/etc/motion\" -DLOCALEDIR=\"/usr/local/share/locale\" -D_THREAD_SAFE    -I/usr/include/p11-kit-1  -I/usr/include/webp -I/usr/include/libcamera  -I/usr/include/opencv4 -I/usr/include/mysql/ -I/usr/include/postgresql  -D_REENTRANT   -W -O3 -Wall -Wextra -Wconversion -Wformat -Wshadow -Wpointer-arith -Wwrite-strings -Wredundant-decls -Wno-sign-conversion -Wno-sign-compare -ggdb -g3  -fstack-protector-strong -fstack-clash-protection -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fPIE -Wformat -Werror=format-security  -std=c++17 -MT webu_mpegts.o -MD -MP -MF $depbase.Tpo -c -o webu_mpegts.o webu_mpegts.cpp &&\
mv -f $depbase.Tpo $depbase.Po
webu_json.cpp:1303:53: warning: implicit conversion from 'unsigned long' to 'double' may lose precision [-Wimplicit-int-float-conversion]
 1303 |             double mem_percent = (double)mem_used / mem_total * 100.0;
      |                                                   ~ ^~~~~~~~~
webu_json.cpp:1320:52: warning: implicit conversion from 'unsigned long long' to 'double' may lose precision [-Wimplicit-int-float-conversion]
 1320 |         double disk_percent = (double)used_bytes / total_bytes * 100.0;
      |                                                  ~ ^~~~~~~~~~~
14 warnings generated.
clang++  -std=c++17  -pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -o motion alg.o alg_sec.o conf.o conf_file.o conf_profile.o dbse.o draw.o jpegutils.o json_parse.o libcam.o logger.o motion.o allcam.o schedule.o camera.o movie.o netcam.o parm_registry.o picture.o rotate.o sound.o util.o video_v4l2.o video_convert.o video_loopback.o webu.o webu_ans.o webu_file.o webu_json.o webu_post.o webu_stream.o webu_getimg.o webu_mpegts.o -lintl -pthread   -ljpeg -lmicrohttpd -lz -lwebpmux -lwebp -lcamera -lcamera-base -lavdevice -lswscale -lavformat -lavcodec -lavutil -lopencv_aruco -lopencv_face -lopencv_ml -lopencv_objdetect -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_optflow -lopencv_tracking -lopencv_highgui -lopencv_plot -lopencv_videostab -lopencv_videoio -lopencv_photo -lopencv_ximgproc -lopencv_video -lopencv_calib3d -lopencv_imgcodecs -lopencv_features2d -lopencv_dnn -lopencv_imgproc -lopencv_flann -lopencv_core -L/usr/lib/ -lmariadb -lpq -lsqlite3 -lpulse -pthread -lpulse-simple  -lasound -lfftw3
/usr/lib/gcc/x86_64-alpine-linux-musl/15.2.0/../../../../x86_64-alpine-linux-musl/bin/ld: /usr/lib//libopencv_imgcodecs.so: undefined reference to `Imf_3_4::Chromaticities::Chromaticities(Imath_3_1::Vec2<float> const&, Imath_3_1::Vec2<float> const&, Imath_3_1::Vec2<float> const&, Imath_3_1::Vec2<float> const&)'
/usr/lib/gcc/x86_64-alpine-linux-musl/15.2.0/../../../../x86_64-alpine-linux-musl/bin/ld: /usr/lib//libopencv_imgcodecs.so: undefined reference to `Imf_3_4::Header::Header(int, int, float, Imath_3_1::Vec2<float> const&, float, Imf_3_4::LineOrder, Imf_3_4::Compression)'
clang++: error: linker command failed with exit code 1 (use -v to see invocation)
make[2]: *** [Makefile:471: motion] Error 1
make[2]: Leaving directory '/home/runner/work/motion/motion/src'
make[1]: *** [Makefile:609: all-recursive] Error 1
make[1]: Leaving directory '/home/runner/work/motion/motion'
make: *** [Makefile:430: all] Error 2
Error: Process completed with exit code 2.

## Build (clang++, glibc)
Successful