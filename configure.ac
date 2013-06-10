dnl Process this file with autoconf to produce a configure script.

AC_INIT([y], [0.1])
AC_PREREQ([2.59])
AC_CONFIG_SRCDIR([src/play.cpp])
AM_CONFIG_HEADER(src/config.h)
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_PROG_CXX
AC_PROG_RANLIB
AM_PROG_CC_C_O
AX_BOOST_BASE([1.33.1])
AX_BOOST_THREAD
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_PROGRAM_OPTIONS

dnl TODO: Find a better (more portable) way to enable more optimizations by
dnl default.
AC_ARG_ENABLE([optimize],
   AS_HELP_STRING([--enable-optimize], [set CXXFLAGS to -O3 -g (default is yes)]),
   [optimize=$enableval],
   [optimize=yes])
if test "x$optimize" = "xyes"
then
	CXXFLAGS="-O3 -g"
fi

AC_ARG_ENABLE([assert],
   AS_HELP_STRING([--enable-assert], [enable assertions (default is no)]),
   [assert=$enableval],
   [assert=no])
if test "x$assert" = "xno"
then
	CXXFLAGS="$CXXFLAGS -DNDEBUG"
fi

dnl Location of the fuego source and libraries
AC_ARG_WITH([fuego-root],
	    AS_HELP_STRING([--with-fuego-root=DIR],
                           [location of the root of the Fuego source tree]),
	    [fuegoroot=$withval],
	    [])

AC_ARG_WITH([fuego-build],
	    AS_HELP_STRING([--with-fuego-build=DIR],
                           [location of the root of the Fuego build tree]),
	    [fuegobuild=$withval],
	    [])

if test "x$fuegoroot" = "x"; then
   AC_MSG_ERROR([The path to the Fuego source tree must be set using the "--with-fuego-root" option.])
elif test "x$fuegoroot" = "xyes"; then
   AC_MSG_ERROR([The option "--with-fuego-root" requires a path to the Fuego source tree.])
elif test "x$fuegoroot" = "xno"; then
   AC_MSG_ERROR([A Fuego source tree is required to build pgo.])
else
   FUEGO_ROOT=$fuegoroot
fi

if test "x$fuegobuild" != "x"; then
    if test "x$fuegobuild" = "xyes"; then
       AC_MSG_ERROR([The option "--with-fuego-build" requires a path to the Fuego build tree.])
    elif test "x$fuegobuild" != "xno"; then
       FUEGO_BUILD=$fuegobuild
    fi
fi

if test "x$FUEGO_BUILD" = "x"; then
   FUEGO_BUILD=$FUEGO_ROOT
fi

AC_SUBST(FUEGO_ROOT)
AC_SUBST(FUEGO_BUILD)

AX_CXXFLAGS_WARN_ALL
AX_CXXFLAGS_GCC_OPTION(-Wextra)

AC_OUTPUT([
Makefile \
src/Makefile
])
