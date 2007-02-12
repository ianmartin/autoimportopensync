dnl AM_PATH_PILOT_LINK([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for check, and define PILOT_CFLAGS and PILOT_LIBS
dnl

AC_DEFUN([AM_PATH_PILOT_LINK],
[
  AC_ARG_WITH(pilot_link,
  [  --with-pilot_link=PATH       prefix where pilot-link is installed [default=auto]])
 
  min_check_version=ifelse([$1], ,0.11.8,$1)
  
  if test x$with_pilot_link = xno; then
    ifelse([$3], , AC_MSG_ERROR([disabling pilot-link is not supported]), [$3])
  else
    if test "x$with_pilot_link" != x; then
      PILOT_CFLAGS="-I$with_pilot_link/include"
      PILOT_LIBS="-L$with_pilot_link/lib -Wl,-R$with_pilot_link/lib -lpisock"
    else
      PILOT_CFLAGS=""
      PILOT_LIBS="-lpisock"
    fi

	ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"

	CFLAGS="$PILOT_CFLAGS $CFLAGS"
    LIBS="$PILOT_LIBS $LIBS"
    
	AC_CHECK_LIB(pisock,pi_socket,,AC_MSG_ERROR(You must have libpisock libraries installed.))
	
	CFLAGS="$PILOT_CFLAGS $ac_save_CFLAGS"
    LIBS="$PILOT_LIBS $ac_save_LIBS"
	
	AC_MSG_CHECKING(for pilot-link >= $min_check_version)
	
    AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>

#include <pi-version.h>

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = strdup("$min_check_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_check_version");
     return 1;
   }

  if ((PILOT_LINK_VERSION > major) ||
      ((PILOT_LINK_VERSION == major) && (PILOT_LINK_MAJOR > minor)) ||
      ((PILOT_LINK_VERSION == major) && (PILOT_LINK_MAJOR == minor) && (PILOT_LINK_MINOR >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** An old version of pilot-link (%d.%d.%d) was found.\n",
             PILOT_LINK_VERSION, PILOT_LINK_MAJOR, PILOT_LINK_MINOR);
      printf("*** You need a version of check being at least %d.%d.%d.\n", major, minor, micro);
      printf("***\n"); 
    }

  return 1;
}
],, no_pilot_link=yes, [echo $ac_n "cross compiling; assumed OK... $ac_c"])

    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"

    if test "x$no_pilot_link" = x ; then
      AC_MSG_RESULT(yes)
      ifelse([$2], , :, [$2])
    else
      AC_MSG_RESULT(no)

      PILOT_CFLAGS=""
      PILOT_LIBS=""

      ifelse([$3], , AC_MSG_ERROR([pilot-link not found]), [$3])
    fi

    AC_SUBST(PILOT_CFLAGS)
    AC_SUBST(PILOT_LIBS)

  fi
])
