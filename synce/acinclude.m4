dnl $Id: libsynce.m4,v 1.5 2004/05/23 08:43:12 twogood Exp $ vim: syntax=config
dnl Check for libsynce

AC_DEFUN([AM_PATH_LIBSYNCE], [

  AC_ARG_WITH(libsynce,
    AC_HELP_STRING(
      [--with-libsynce[=DIR]],
      [Search for libsynce in DIR/include and DIR/lib]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}/include"
				LDFLAGS="$LDFLAGS -L${withval}/lib"
			]
    )

  AC_ARG_WITH(libsynce-include,
    AC_HELP_STRING(
      [--with-libsynce-include[=DIR]],
      [Search for libsynce header files in DIR]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}"
			]
    )

  AC_ARG_WITH(libsynce-lib,
    AC_HELP_STRING(
      [--with-libsynce-lib[=DIR]],
      [Search for libsynce library files in DIR]),
      [
				LDFLAGS="$LDFLAGS -L${withval}"
			]
    )

	AC_CHECK_LIB(synce,main,,[
		AC_MSG_ERROR([Can't find synce library])
		])
	AC_CHECK_HEADERS(synce.h,,[
		AC_MSG_ERROR([Can't find synce.h])
		])

])

dnl $Id: rra.m4,v 1.5 2004/08/04 18:01:54 twogood Exp $ vim: syntax=config
dnl Check for rra

AC_DEFUN([AM_PATH_RRA], [

  AC_ARG_WITH(rra,
    AC_HELP_STRING(
      [--with-rra[=DIR]],
      [Search for RRA in DIR/include and DIR/lib]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}/include/rra -I${withval}/include"
				LDFLAGS="$LDFLAGS -L${withval}/lib"
			]
    )

  AC_ARG_WITH(rra-include,
    AC_HELP_STRING(
      [--with-rra-include[=DIR]],
      [Search for RRA header files in DIR/rra]),
      [
				CPPFLAGS="$CPPFLAGS -I${withval}/rra -I${withval}"
			]
    )

  AC_ARG_WITH(rra-lib,
    AC_HELP_STRING(
      [--with-rra-lib[=DIR]],
      [Search for RRA library files in DIR]),
      [
				LDFLAGS="$LDFLAGS -L${withval}"
			]
    )

	AC_CHECK_LIB(rra,rra_syncmgr_new,,[
		AC_MSG_ERROR([Can't find RRA library])
		])
	AC_CHECK_HEADERS(rra/syncmgr.h,,[
		AC_MSG_ERROR([Can't find rra/syncmgr.h])
		])

])