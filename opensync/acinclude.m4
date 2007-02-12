dnl AM_PATH_CHECK([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for check, and define CHECK_CFLAGS and CHECK_LIBS
dnl

AC_DEFUN([AM_PATH_CHECK],
[
  AC_ARG_WITH([check],
  [  --with-check=PATH       prefix where check is installed [default=auto]])
 
  min_check_version=ifelse([$1], ,0.8.2,$1)

  AC_MSG_CHECKING(for check - version >= $min_check_version)

  if test x$with_check = xno; then
    AC_MSG_RESULT(disabled)
    ifelse([$3], , AC_MSG_ERROR([disabling check is not supported]), [$3])
  else
    if test "x$with_check" != x; then
      CHECK_CFLAGS="-I$with_check/include"
      CHECK_LIBS="-L$with_check/lib -lcheck"
    else
      CHECK_CFLAGS=""
      CHECK_LIBS="-lcheck"
    fi

    ac_save_CFLAGS="$CFLAGS"
    ac_save_LIBS="$LIBS"

    CFLAGS="$CFLAGS $CHECK_CFLAGS"
    LIBS="$CHECK_LIBS $LIBS"

    rm -f conf.check-test
    AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>

#include <check.h>

#if HAVE_STRING_H
#include <string.h>
#endif

int main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.check-test");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = strdup("$min_check_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_check_version");
     return 1;
   }
    
  if ((CHECK_MAJOR_VERSION != check_major_version) ||
      (CHECK_MINOR_VERSION != check_minor_version) ||
      (CHECK_MICRO_VERSION != check_micro_version))
    {
      printf("\n*** The check header file (version %d.%d.%d) does not match\n",
	     CHECK_MAJOR_VERSION, CHECK_MINOR_VERSION, CHECK_MICRO_VERSION);
      printf("*** the check library (version %d.%d.%d).\n",
	     check_major_version, check_minor_version, check_micro_version);
      return 1;
    }

  if ((check_major_version > major) ||
      ((check_major_version == major) && (check_minor_version > minor)) ||
      ((check_major_version == major) && (check_minor_version == minor) && (check_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** An old version of check (%d.%d.%d) was found.\n",
             check_major_version, check_minor_version, check_micro_version);
      printf("*** You need a version of check being at least %d.%d.%d.\n", major, minor, micro);
      printf("***\n"); 
      printf("*** If you have already installed a sufficiently new version, this error\n");
      printf("*** probably means that the wrong copy of the check library and header\n");
      printf("*** file is being found. Rerun configure with the --with-check=PATH option\n");
      printf("*** to specify the prefix where the correct version was installed.\n");
    }

  return 1;
}
],, no_check=yes, [echo $ac_n "cross compiling; assumed OK... $ac_c"])

    CFLAGS="$ac_save_CFLAGS"
    LIBS="$ac_save_LIBS"

    if test "x$no_check" = x ; then
      AC_MSG_RESULT(yes)
      ifelse([$2], , :, [$2])
    else
      AC_MSG_RESULT(no)
      if test -f conf.check-test ; then
        :
      else
        echo "*** Could not run check test program, checking why..."
        CFLAGS="$CFLAGS $CHECK_CFLAGS"
        LIBS="$CHECK_LIBS $LIBS"
        AC_TRY_LINK([
#include <stdio.h>
#include <stdlib.h>

#include <check.h>
], ,  [ echo "*** The test program compiled, but did not run. This usually means"
        echo "*** that the run-time linker is not finding check. You'll need to set your"
        echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
        echo "*** to the installed location  Also, make sure you have run ldconfig if that"
        echo "*** is required on your system"
	echo "***"
        echo "*** If you have an old version installed, it is best to remove it, although"
        echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
      [ echo "*** The test program failed to compile or link. See the file config.log for"
        echo "*** the exact error that occured." ])
      
        CFLAGS="$ac_save_CFLAGS"
        LIBS="$ac_save_LIBS"
      fi

      CHECK_CFLAGS=""
      CHECK_LIBS=""

      rm -f conf.check-test
      ifelse([$3], , AC_MSG_ERROR([check not found]), [$3])
    fi

    AC_SUBST(CHECK_CFLAGS)
    AC_SUBST(CHECK_LIBS)

    rm -f conf.check-test

  fi
])

dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ax_compare_version.html
dnl
dnl #########################################################################
AC_DEFUN([AX_COMPARE_VERSION], [
  # Used to indicate true or false condition
  ax_compare_version=false

  # Convert the two version strings to be compared into a format that
  # allows a simple string comparison.  The end result is that a version 
  # string of the form 1.12.5-r617 will be converted to the form 
  # 0001001200050617.  In other words, each number is zero padded to four 
  # digits, and non digits are removed.
  AS_VAR_PUSHDEF([A],[ax_compare_version_A])
  A=`echo "$1" | sed -e 's/\([[0-9]]*\)/Z\1Z/g' \
                     -e 's/Z\([[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/Z\([[0-9]][[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/Z\([[0-9]][[0-9]][[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/[[^0-9]]//g'`

  AS_VAR_PUSHDEF([B],[ax_compare_version_B])
  B=`echo "$3" | sed -e 's/\([[0-9]]*\)/Z\1Z/g' \
                     -e 's/Z\([[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/Z\([[0-9]][[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/Z\([[0-9]][[0-9]][[0-9]]\)Z/Z0\1Z/g' \
                     -e 's/[[^0-9]]//g'`

  dnl # In the case of le, ge, lt, and gt, the strings are sorted as necessary 
  dnl # then the first line is used to determine if the condition is true. 
  dnl # The sed right after the echo is to remove any indented white space.
  m4_case(m4_tolower($2),
  [lt],[
    ax_compare_version=`echo "x$A
x$B" | sed 's/^ *//' | sort -r | sed "s/x${A}/false/;s/x${B}/true/;1q"`
  ],
  [gt],[
    ax_compare_version=`echo "x$A
x$B" | sed 's/^ *//' | sort | sed "s/x${A}/false/;s/x${B}/true/;1q"`
  ],
  [le],[
    ax_compare_version=`echo "x$A
x$B" | sed 's/^ *//' | sort | sed "s/x${A}/true/;s/x${B}/false/;1q"`
  ],
  [ge],[
    ax_compare_version=`echo "x$A
x$B" | sed 's/^ *//' | sort -r | sed "s/x${A}/true/;s/x${B}/false/;1q"`
  ],[
    dnl Split the operator from the subversion count if present.
    m4_bmatch(m4_substr($2,2),
    [0],[
      # A count of zero means use the length of the shorter version.
      # Determine the number of characters in A and B.
      ax_compare_version_len_A=`echo "$A" | awk '{print(length)}'`
      ax_compare_version_len_B=`echo "$B" | awk '{print(length)}'`
 
      # Set A to no more than B's length and B to no more than A's length.
      A=`echo "$A" | sed "s/\(.\{$ax_compare_version_len_B\}\).*/\1/"`
      B=`echo "$B" | sed "s/\(.\{$ax_compare_version_len_A\}\).*/\1/"`
    ],
    [[0-9]+],[
      # A count greater than zero means use only that many subversions 
      A=`echo "$A" | sed "s/\(\([[0-9]]\{4\}\)\{m4_substr($2,2)\}\).*/\1/"`
      B=`echo "$B" | sed "s/\(\([[0-9]]\{4\}\)\{m4_substr($2,2)\}\).*/\1/"`
    ],
    [.+],[
      AC_WARNING(
        [illegal OP numeric parameter: $2])
    ],[])

    # Pad zeros at end of numbers to make same length.
    ax_compare_version_tmp_A="$A`echo $B | sed 's/./0/g'`"
    B="$B`echo $A | sed 's/./0/g'`"
    A="$ax_compare_version_tmp_A"

    # Check for equality or inequality as necessary.
    m4_case(m4_tolower(m4_substr($2,0,2)),
    [eq],[
      test "x$A" = "x$B" && ax_compare_version=true
    ],
    [ne],[
      test "x$A" != "x$B" && ax_compare_version=true
    ],[
      AC_WARNING([illegal OP parameter: $2])
    ])
  ])

  AS_VAR_POPDEF([A])dnl
  AS_VAR_POPDEF([B])dnl

  dnl # Execute ACTION-IF-TRUE / ACTION-IF-FALSE.
  if test "$ax_compare_version" = "true" ; then
    m4_ifvaln([$4],[$4],[:])dnl
    m4_ifvaln([$5],[else $5])dnl
  fi
]) dnl AX_COMPARE_VERSION

dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ax_path_bdb.html
dnl
dnl #########################################################################
AC_DEFUN([AX_PATH_BDB], [
  dnl # Used to indicate success or failure of this function.
  ax_path_bdb_ok=no

  # Add --with-bdb-dir option to configure.
  AC_ARG_WITH([bdb-dir],
    [AC_HELP_STRING([--with-bdb-dir=DIR],
                    [Berkeley DB installation directory])])

  # Check if --with-bdb-dir was specified.
  if test "x$with_bdb_dir" = "x" ; then 
    # No option specified, so just search the system.
    AX_PATH_BDB_NO_OPTIONS([$1], [HIGHEST], [
      ax_path_bdb_ok=yes
    ])
   else
     # Set --with-bdb-dir option.
     ax_path_bdb_INC="$with_bdb_dir/include"
     ax_path_bdb_LIB="$with_bdb_dir/lib"
 
     dnl # Save previous environment, and modify with new stuff.
     ax_path_bdb_save_CPPFLAGS="$CPPFLAGS"
     CPPFLAGS="-I$ax_path_bdb_INC $CPPFLAGS"
 
     ax_path_bdb_save_LDFLAGS=$LDFLAGS
     LDFLAGS="-L$ax_path_bdb_LIB $LDFLAGS"
 
     # Check for specific header file db.h
     AC_MSG_CHECKING([db.h presence in $ax_path_bdb_INC])
     if test -f "$ax_path_bdb_INC/db.h" ; then
       AC_MSG_RESULT([yes])
       # Check for library
       AX_PATH_BDB_NO_OPTIONS([$1], [ENVONLY], [
         ax_path_bdb_ok=yes
         BDB_CPPFLAGS="-I$ax_path_bdb_INC"
         BDB_LDFLAGS="-L$ax_path_bdb_LIB"
       ])
     else
       AC_MSG_RESULT([no])
       AC_MSG_NOTICE([no usable Berkeley DB not found]) 
     fi
 
     dnl # Restore the environment.
     CPPFLAGS="$ax_path_bdb_save_CPPFLAGS"
     LDFLAGS="$ax_path_bdb_save_LDFLAGS"

  fi

  dnl # Execute ACTION-IF-FOUND / ACTION-IF-NOT-FOUND.
  if test "$ax_path_bdb_ok" = "yes" ; then
    m4_ifvaln([$2],[$2],[:])dnl
    m4_ifvaln([$3],[else $3])dnl
  fi

]) dnl AX_PATH_BDB

dnl #########################################################################
dnl Check for berkeley DB of at least MINIMUM-VERSION on system.
dnl 
dnl The OPTION argument determines how the checks occur, and can be one of:
dnl
dnl   HIGHEST -  Check both the environment and the default installation 
dnl              directories for Berkeley DB and choose the version that
dnl              is highest. (default)
dnl   ENVFIRST - Check the environment first, and if no satisfactory 
dnl              library is found there check the default installation 
dnl              directories for Berkeley DB which is /usr/local/BerkeleyDB*
dnl   ENVONLY -  Check the current environment only.
dnl   
dnl Requires AX_PATH_BDB_PATH_GET_VERSION, AX_PATH_BDB_PATH_FIND_HIGHEST,
dnl          AX_PATH_BDB_ENV_CONFIRM_LIB, AX_PATH_BDB_ENV_GET_VERSION, and
dnl          AX_COMPARE_VERSION macros.
dnl
dnl Result: sets ax_path_bdb_no_options_ok to yes or no
dnl         sets BDB_LIBS, BDB_CPPFLAGS, BDB_LDFLAGS, BDB_VERSION
dnl
dnl AX_PATH_BDB_NO_OPTIONS([MINIMUM-VERSION], [OPTION], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
AC_DEFUN([AX_PATH_BDB_NO_OPTIONS], [
  dnl # Used to indicate success or failure of this function.
  ax_path_bdb_no_options_ok=no

  # Values to add to environment to use Berkeley DB.
  BDB_VERSION=''
  BDB_LIBS=''
  BDB_CPPFLAGS=''
  BDB_LDFLAGS=''

  # Check cross compilation here.
  if test "x$cross_compiling" = "xyes" ; then
    # If cross compiling, can't use AC_RUN_IFELSE so do these tests.
    # The AC_PREPROC_IFELSE confirms that db.h is preprocessable,
    # and extracts the version number from it.
    AC_MSG_CHECKING([for db.h])

    AS_VAR_PUSHDEF([HEADER_VERSION],[ax_path_bdb_no_options_HEADER_VERSION])dnl
    HEADER_VERSION=''
    AC_PREPROC_IFELSE([
      AC_LANG_SOURCE([[
#include <db.h>
AX_PATH_BDB_STUFF DB_VERSION_MAJOR,DB_VERSION_MINOR,DB_VERSION_PATCH
      ]])
    ],[
      # Extract version from preprocessor output.
      HEADER_VERSION=`eval "$ac_cpp conftest.$ac_ext" 2> /dev/null \
        | grep AX_PATH_BDB_STUFF | sed 's/[[^0-9,]]//g;s/,/./g;1q'`
    ],[])

    if test "x$HEADER_VERSION" = "x" ; then
      AC_MSG_RESULT([no])
    else
      AC_MSG_RESULT([$HEADER_VERSION])

      # Check that version is high enough.
      AX_COMPARE_VERSION([$HEADER_VERSION],[ge],[$1],[
        # get major and minor version numbers
        AS_VAR_PUSHDEF([MAJ],[ax_path_bdb_no_options_MAJOR])dnl
        MAJ=`echo $HEADER_VERSION | sed 's,\..*,,'`
        AS_VAR_PUSHDEF([MIN],[ax_path_bdb_no_options_MINOR])dnl
        MIN=`echo $HEADER_VERSION | sed 's,^[[0-9]]*\.,,;s,\.[[0-9]]*$,,'`

	dnl # Save LIBS.
	ax_path_bdb_no_options_save_LIBS="$LIBS"

        # Check that we can link with the library.
        AC_SEARCH_LIBS([db_version], 
          [db db-$MAJ.$MIN db$MAJ.$MIN db$MAJ$MIN db-$MAJ db$MAJ],[
            # Sucessfully found library.
            ax_path_bdb_no_options_ok=yes
            BDB_VERSION=$HEADER_VERSION
	    
	    # Extract library from LIBS
	    ax_path_bdb_no_options_LEN=` \
              echo "x$ax_path_bdb_no_options_save_LIBS" \
              | awk '{print(length)}'`
            BDB_LIBS=`echo "x$LIBS " \
              | sed "s/.\{$ax_path_bdb_no_options_LEN\}\$//;s/^x//;s/ //g"`
        ],[])

        dnl # Restore LIBS
	LIBS="$ax_path_bdb_no_options_save_LIBS"

        AS_VAR_POPDEF([MAJ])dnl
        AS_VAR_POPDEF([MIN])dnl
      ])
    fi

    AS_VAR_POPDEF([HEADER_VERSION])dnl
  else
    # Not cross compiling.
    # Check version of Berkeley DB in the current environment.
    AX_PATH_BDB_ENV_GET_VERSION([
      AX_COMPARE_VERSION([$ax_path_bdb_env_get_version_VERSION],[ge],[$1],[
        # Found acceptable version in current environment.
        ax_path_bdb_no_options_ok=yes
        BDB_VERSION="$ax_path_bdb_env_get_version_VERSION"
        BDB_LIBS="$ax_path_bdb_env_get_version_LIBS"
      ])
    ])

    # Determine if we need to search /usr/local/BerkeleyDB*
    ax_path_bdb_no_options_DONE=no
    if test "x$2" = "xENVONLY" ; then
      ax_path_bdb_no_options_DONE=yes
    elif test "x$2" = "xENVFIRST" ; then
      ax_path_bdb_no_options_DONE=$ax_path_bdb_no_options_ok
    fi  

    if test "$ax_path_bdb_no_options_DONE" = "no" ; then
      # Check for highest in /usr/local/BerkeleyDB*
      AX_PATH_BDB_PATH_FIND_HIGHEST([
        if test "$ax_path_bdb_no_options_ok" = "yes" ; then
        # If we already have an acceptable version use this if higher.
          AX_COMPARE_VERSION(
             [$ax_path_bdb_path_find_highest_VERSION],[gt],[$BDB_VERSION])
        else
          # Since we didn't have an acceptable version check if this one is.
          AX_COMPARE_VERSION(
             [$ax_path_bdb_path_find_highest_VERSION],[ge],[$1])
        fi
      ])

      dnl # If result from _AX_COMPARE_VERSION is true we want this version.
      if test "$ax_compare_version" = "true" ; then
        ax_path_bdb_no_options_ok=yes
        BDB_LIBS="-ldb"
        BDB_CPPFLAGS="-I$ax_path_bdb_path_find_highest_DIR/include"
        BDB_LDFLAGS="-L$ax_path_bdb_path_find_highest_DIR/lib"
        BDB_VERSION="$ax_path_bdb_path_find_highest_VERSION"
      fi
    fi
  fi

  dnl # Execute ACTION-IF-FOUND / ACTION-IF-NOT-FOUND.
  if test "$ax_path_bdb_no_options_ok" = "yes" ; then
    AC_MSG_NOTICE([using Berkeley DB version $BDB_VERSION]) 
    AC_DEFINE([HAVE_DB_H],[1],
              [Define to 1 if you have the <db.h> header file.])
    m4_ifvaln([$3],[$3])dnl
  else
    AC_MSG_NOTICE([no Berkeley DB version $1 or higher found]) 
    m4_ifvaln([$4],[$4])dnl
  fi 
]) dnl AX_PATH_BDB_NO_OPTIONS

dnl #########################################################################
dnl Check the default installation directory for Berkeley DB which is
dnl of the form /usr/local/BerkeleyDB* for the highest version.
dnl
dnl Result: sets ax_path_bdb_path_find_highest_ok to yes or no,
dnl         sets ax_path_bdb_path_find_highest_VERSION to version,
dnl         sets ax_path_bdb_path_find_highest_DIR to directory.
dnl
dnl AX_PATH_BDB_PATH_FIND_HIGHEST([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
AC_DEFUN([AX_PATH_BDB_PATH_FIND_HIGHEST], [
  dnl # Used to indicate success or failure of this function.
  ax_path_bdb_path_find_highest_ok=no

  AS_VAR_PUSHDEF([VERSION],[ax_path_bdb_path_find_highest_VERSION])dnl
  VERSION=''

  ax_path_bdb_path_find_highest_DIR=''

  # find highest verison in default install directory for Berkeley DB 
  AS_VAR_PUSHDEF([CURDIR],[ax_path_bdb_path_find_highest_CURDIR])dnl
  AS_VAR_PUSHDEF([CUR_VERSION],[ax_path_bdb_path_get_version_VERSION])dnl

  for CURDIR in `ls -d /usr/local/BerkeleyDB* 2> /dev/null`
  do
    AX_PATH_BDB_PATH_GET_VERSION([$CURDIR],[
      AX_COMPARE_VERSION([$CUR_VERSION],[gt],[$VERSION],[
        ax_path_bdb_path_find_highest_ok=yes
        ax_path_bdb_path_find_highest_DIR="$CURDIR"
        VERSION="$CUR_VERSION"
      ])
    ])
  done

  AS_VAR_POPDEF([VERSION])dnl
  AS_VAR_POPDEF([CUR_VERSION])dnl
  AS_VAR_POPDEF([CURDIR])dnl

  dnl # Execute ACTION-IF-FOUND / ACTION-IF-NOT-FOUND.
  if test "$ax_path_bdb_path_find_highest_ok" = "yes" ; then
    m4_ifvaln([$1],[$1],[:])dnl
    m4_ifvaln([$2],[else $2])dnl
  fi

]) dnl AX_PATH_BDB_PATH_FIND_HIGHEST

dnl #########################################################################
dnl Checks for Berkeley DB in specified directory's lib and include 
dnl subdirectories.  
dnl
dnl Result: sets ax_path_bdb_path_get_version_ok to yes or no,
dnl         sets ax_path_bdb_path_get_version_VERSION to version.
dnl
dnl AX_PATH_BDB_PATH_GET_VERSION(BDB-DIR, [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
AC_DEFUN([AX_PATH_BDB_PATH_GET_VERSION], [
  dnl # Used to indicate success or failure of this function.
  ax_path_bdb_path_get_version_ok=no

  # Indicate status of checking for Berkeley DB header.
  AC_MSG_CHECKING([in $1/include for db.h])
  ax_path_bdb_path_get_version_got_header=no
  test -f "$1/include/db.h" && ax_path_bdb_path_get_version_got_header=yes
  AC_MSG_RESULT([$ax_path_bdb_path_get_version_got_header])

  # Indicate status of checking for Berkeley DB library.
  AC_MSG_CHECKING([in $1/lib for library -ldb])

  ax_path_bdb_path_get_version_VERSION=''

  if test -d "$1/include" && test -d "$1/lib" &&
     test "$ax_path_bdb_path_get_version_got_header" = "yes" ; then
    dnl # save and modify environment
    ax_path_bdb_path_get_version_save_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="-I$1/include $CPPFLAGS"

    ax_path_bdb_path_get_version_save_LIBS="$LIBS"
    LIBS="$LIBS -ldb"

    ax_path_bdb_path_get_version_save_LDFLAGS="$LDFLAGS"
    LDFLAGS="-L$1/lib $LDFLAGS"

    # Compile and run a program that compares the version defined in
    # the header file with a version defined in the library function
    # db_version.
    AC_RUN_IFELSE([
      AC_LANG_SOURCE([[
#include <stdio.h>
#include <db.h>
int main(int argc,char **argv)
{
  int major,minor,patch;
  db_version(&major,&minor,&patch);
  if (argc > 1)
    printf("%d.%d.%d\n",DB_VERSION_MAJOR,DB_VERSION_MINOR,DB_VERSION_PATCH);
  if (DB_VERSION_MAJOR == major && DB_VERSION_MINOR == minor &&
      DB_VERSION_PATCH == patch)
    return 0;          
  else
    return 1;
}
      ]])
    ],[
      # Program compiled and ran, so get version by adding argument.
      ax_path_bdb_path_get_version_VERSION=`./conftest$ac_exeext x`
      ax_path_bdb_path_get_version_ok=yes
    ],[],[])

    dnl # restore environment
    CPPFLAGS="$ax_path_bdb_path_get_version_save_CPPFLAGS"
    LIBS="$ax_path_bdb_path_get_version_save_LIBS"
    LDFLAGS="$ax_path_bdb_path_get_version_save_LDFLAGS"
  fi

  dnl # Finally, execute ACTION-IF-FOUND / ACTION-IF-NOT-FOUND.
  if test "$ax_path_bdb_path_get_version_ok" = "yes" ; then
    AC_MSG_RESULT([$ax_path_bdb_path_get_version_VERSION])
    m4_ifvaln([$2],[$2])dnl
  else
    AC_MSG_RESULT([no])
    m4_ifvaln([$3],[$3])dnl
  fi 
]) dnl AX_PATH_BDB_PATH_GET_VERSION

#############################################################################
dnl Checks if version of library and header match specified version.  
dnl Only meant to be used by AX_PATH_BDB_ENV_GET_VERSION macro.
dnl 
dnl Requires AX_COMPARE_VERSION macro.
dnl 
dnl Result: sets ax_path_bdb_env_confirm_lib_ok to yes or no.
dnl
dnl AX_PATH_BDB_ENV_CONFIRM_LIB(VERSION, [LIBNAME])
AC_DEFUN([AX_PATH_BDB_ENV_CONFIRM_LIB], [
  dnl # Used to indicate success or failure of this function.
  ax_path_bdb_env_confirm_lib_ok=no

  dnl # save and modify environment to link with library LIBNAME
  ax_path_bdb_env_confirm_lib_save_LIBS="$LIBS"
  LIBS="$LIBS $2"

  # Compile and run a program that compares the version defined in
  # the header file with a version defined in the library function
  # db_version.
  AC_RUN_IFELSE([
    AC_LANG_SOURCE([[
#include <stdio.h>
#include <db.h>
int main(int argc,char **argv)
{
  int major,minor,patch;
  db_version(&major,&minor,&patch);
  if (argc > 1)
    printf("%d.%d.%d\n",DB_VERSION_MAJOR,DB_VERSION_MINOR,DB_VERSION_PATCH);
  if (DB_VERSION_MAJOR == major && DB_VERSION_MINOR == minor &&
      DB_VERSION_PATCH == patch)
    return 0;          
  else
    return 1;
}
    ]])
  ],[
    # Program compiled and ran, so get version by giving an argument,
    # which will tell the program to print the output.
    ax_path_bdb_env_confirm_lib_VERSION=`./conftest$ac_exeext x`

    # If the versions all match up, indicate success.
    AX_COMPARE_VERSION([$ax_path_bdb_env_confirm_lib_VERSION],[eq],[$1],[
      ax_path_bdb_env_confirm_lib_ok=yes
    ])
  ],[],[])

  dnl # restore environment
  LIBS="$ax_path_bdb_env_confirm_lib_save_LIBS"

]) dnl AX_PATH_BDB_ENV_CONFIRM_LIB

#############################################################################
dnl Finds the version and library name for Berkeley DB in the
dnl current environment.  Tries many different names for library.
dnl
dnl Requires AX_PATH_BDB_ENV_CONFIRM_LIB macro.
dnl
dnl Result: set ax_path_bdb_env_get_version_ok to yes or no,
dnl         set ax_path_bdb_env_get_version_VERSION to the version found,
dnl         and ax_path_bdb_env_get_version_LIBNAME to the library name.
dnl
dnl AX_PATH_BDB_ENV_GET_VERSION([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
AC_DEFUN([AX_PATH_BDB_ENV_GET_VERSION], [
  dnl # Used to indicate success or failure of this function.
  ax_path_bdb_env_get_version_ok=no

  ax_path_bdb_env_get_version_VERSION=''
  ax_path_bdb_env_get_version_LIBS=''

  AS_VAR_PUSHDEF([HEADER_VERSION],[ax_path_bdb_env_get_version_HEADER_VERSION])dnl
  AS_VAR_PUSHDEF([TEST_LIBNAME],[ax_path_bdb_env_get_version_TEST_LIBNAME])dnl

  # Indicate status of checking for Berkeley DB library.
  AC_MSG_CHECKING([for db.h])

  # Compile and run a program that determines the Berkeley DB version
  # in the header file db.h.
  HEADER_VERSION=''
  AC_RUN_IFELSE([
    AC_LANG_SOURCE([[
#include <stdio.h>
#include <db.h>
int main(int argc,char **argv)
{
  if (argc > 1)
    printf("%d.%d.%d\n",DB_VERSION_MAJOR,DB_VERSION_MINOR,DB_VERSION_PATCH);
  return 0;            
}
    ]])
  ],[
    # Program compiled and ran, so get version by adding an argument.
    HEADER_VERSION=`./conftest$ac_exeext x`
    AC_MSG_RESULT([$HEADER_VERSION])
  ],[AC_MSG_RESULT([no])],[AC_MSG_RESULT([no])])

  # Have header version, so try to find corresponding library.
  # Looks for library names in the order:
  #   nothing, db, db-X.Y, dbX.Y, dbXY, db-X, dbX
  # and stops when it finds the first one that matches the version
  # of the header file.  
  if test "x$HEADER_VERSION" != "x" ; then 
    AC_MSG_CHECKING([for library containing Berkeley DB $HEADER_VERSION])

    AS_VAR_PUSHDEF([MAJOR],[ax_path_bdb_env_get_version_MAJOR])dnl
    AS_VAR_PUSHDEF([MINOR],[ax_path_bdb_env_get_version_MINOR])dnl

    # get major and minor version numbers
    MAJOR=`echo $HEADER_VERSION | sed 's,\..*,,'`
    MINOR=`echo $HEADER_VERSION | sed 's,^[[0-9]]*\.,,;s,\.[[0-9]]*$,,'`

    # see if it is already specified in LIBS
    TEST_LIBNAME=''
    AX_PATH_BDB_ENV_CONFIRM_LIB([$HEADER_VERSION], [$TEST_LIBNAME])

    if test "$ax_path_bdb_env_confirm_lib_ok" = "no" ; then
      # try format "db"
      TEST_LIBNAME='-ldb'
      AX_PATH_BDB_ENV_CONFIRM_LIB([$HEADER_VERSION], [$TEST_LIBNAME])
    fi

    if test "$ax_path_bdb_env_confirm_lib_ok" = "no" ; then
      # try format "db-X.Y"
      TEST_LIBNAME="-ldb-${MAJOR}.$MINOR"
      AX_PATH_BDB_ENV_CONFIRM_LIB([$HEADER_VERSION], [$TEST_LIBNAME])
    fi

    if test "$ax_path_bdb_env_confirm_lib_ok" = "no" ; then
      # try format "dbX.Y"
      TEST_LIBNAME="-ldb${MAJOR}.$MINOR"
      AX_PATH_BDB_ENV_CONFIRM_LIB([$HEADER_VERSION], [$TEST_LIBNAME])
    fi

    if test "$ax_path_bdb_env_confirm_lib_ok" = "no" ; then
      # try format "dbXY"
      TEST_LIBNAME="-ldb$MAJOR$MINOR"
      AX_PATH_BDB_ENV_CONFIRM_LIB([$HEADER_VERSION], [$TEST_LIBNAME])
    fi

    if test "$ax_path_bdb_env_confirm_lib_ok" = "no" ; then
      # try format "db-X"
      TEST_LIBNAME="-ldb-$MAJOR"
      AX_PATH_BDB_ENV_CONFIRM_LIB([$HEADER_VERSION], [$TEST_LIBNAME])
    fi

    if test "$ax_path_bdb_env_confirm_lib_ok" = "no" ; then
      # try format "dbX"
      TEST_LIBNAME="-ldb$MAJOR"
      AX_PATH_BDB_ENV_CONFIRM_LIB([$HEADER_VERSION], [$TEST_LIBNAME])
    fi

    dnl # Found a valid library.
    if test "$ax_path_bdb_env_confirm_lib_ok" = "yes" ; then
      if test "x$TEST_LIBNAME" = "x" ; then
        AC_MSG_RESULT([none required])
      else
        AC_MSG_RESULT([$TEST_LIBNAME])
      fi
      ax_path_bdb_env_get_version_VERSION="$HEADER_VERSION"
      ax_path_bdb_env_get_version_LIBS="$TEST_LIBNAME"
      ax_path_bdb_env_get_version_ok=yes
    else
      AC_MSG_RESULT([no])
    fi

    AS_VAR_POPDEF([MAJOR])dnl
    AS_VAR_POPDEF([MINOR])dnl
  fi

  AS_VAR_POPDEF([HEADER_VERSION])dnl
  AS_VAR_POPDEF([TEST_LIBNAME])dnl

  dnl # Execute ACTION-IF-FOUND / ACTION-IF-NOT-FOUND.
  if test "$ax_path_bdb_env_confirm_lib_ok" = "yes" ; then
    m4_ifvaln([$1],[$1],[:])dnl
    m4_ifvaln([$2],[else $2])dnl
  fi

]) dnl BDB_ENV_GET_VERSION

#############################################################################

dnl a macro to check for ability to create python extensions
dnl  AM_CHECK_PYTHON_HEADERS([ACTION-IF-POSSIBLE], [ACTION-IF-NOT-POSSIBLE])
dnl function also defines PYTHON_INCLUDES
AC_DEFUN([AM_CHECK_PYTHON_HEADERS],
[AC_REQUIRE([AM_PATH_PYTHON])
AC_MSG_CHECKING(for headers required to compile python extensions)
dnl deduce PYTHON_INCLUDES
py_prefix=`$PYTHON -c "import sys; print sys.prefix"`
py_exec_prefix=`$PYTHON -c "import sys; print sys.exec_prefix"`
PYTHON_INCLUDES="-I${py_prefix}/include/python${PYTHON_VERSION}"
if test "$py_prefix" != "$py_exec_prefix"; then
  PYTHON_INCLUDES="$PYTHON_INCLUDES -I${py_exec_prefix}/include/python${PYTHON_VERSION}"
fi
AC_SUBST(PYTHON_INCLUDES)
dnl check if the headers exist:
save_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $PYTHON_INCLUDES"
AC_TRY_CPP([#include <Python.h>],dnl
[AC_MSG_RESULT(found)
$1],dnl
[AC_MSG_RESULT(not found)
$2])
CPPFLAGS="$save_CPPFLAGS"
])

# Copyright 1999, 2000, 2001, 2002, 2003  Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# AM_PATH_PYTHON([MINIMUM-VERSION])

# Adds support for distributing Python modules and packages.  To
# install modules, copy them to $(pythondir), using the python_PYTHON
# automake variable.  To install a package with the same name as the
# automake package, install to $(pkgpythondir), or use the
# pkgpython_PYTHON automake variable.

# The variables $(pyexecdir) and $(pkgpyexecdir) are provided as
# locations to install python extension modules (shared libraries).
# Another macro is required to find the appropriate flags to compile
# extension modules.

# If your package is configured with a different prefix to python,
# users will have to add the install directory to the PYTHONPATH
# environment variable, or create a .pth file (see the python
# documentation for details).

# If the MINIMUM-VERSION argument is passed, AM_PATH_PYTHON will
# cause an error if the version of python installed on the system
# doesn't meet the requirement.  MINIMUM-VERSION should consist of
# numbers and dots only.

AC_DEFUN([AM_PATH_PYTHON],
 [
  dnl Find a Python interpreter.  Python versions prior to 1.5 are not
  dnl supported because the default installation locations changed from
  dnl $prefix/lib/site-python in 1.4 to $prefix/lib/python1.5/site-packages
  dnl in 1.5.
  m4_define([_AM_PYTHON_INTERPRETER_LIST],
            [python python2 python2.4 python2.3 python2.2 python2.1 python2.0 python1.6 python1.5])

  m4_if([$1],[],[
    dnl No version check is needed.
    # Find any Python interpreter.
    AC_PATH_PROGS([PYTHON], _AM_PYTHON_INTERPRETER_LIST)
    am_display_PYTHON=python
  ], [
    dnl A version check is needed.
    if test -n "$PYTHON"; then
      # If the user set $PYTHON, use it and don't search something else.
      AC_MSG_CHECKING([whether $PYTHON version >= $1])
      AM_PYTHON_CHECK_VERSION([$PYTHON], [$1],
                              [AC_MSG_RESULT(yes)],
                              [AC_MSG_ERROR(too old)])
    else
      # Otherwise, try each interpreter until we find one that satisfies
      # VERSION.
      AC_CACHE_CHECK([for a Python interpreter with version >= $1],
        [am_cv_pathless_PYTHON],[
        for am_cv_pathless_PYTHON in _AM_PYTHON_INTERPRETER_LIST : ; do
          if test "$am_cv_pathless_PYTHON" = : ; then
            AC_MSG_ERROR([no suitable Python interpreter found])
          fi
          AM_PYTHON_CHECK_VERSION([$am_cv_pathless_PYTHON], [$1], [break])
        done])
      # Set $PYTHON to the absolute path of $am_cv_pathless_PYTHON.
      AC_PATH_PROG([PYTHON], [$am_cv_pathless_PYTHON])
      am_display_PYTHON=$am_cv_pathless_PYTHON
    fi
  ])

  dnl Query Python for its version number.  Getting [:3] seems to be
  dnl the best way to do this; it's what "site.py" does in the standard
  dnl library.

  AC_CACHE_CHECK([for $am_display_PYTHON version], [am_cv_python_version],
    [am_cv_python_version=`$PYTHON -c "import sys; print sys.version[[:3]]"`])
  AC_SUBST([PYTHON_VERSION], [$am_cv_python_version])

  dnl Use the values of $prefix and $exec_prefix for the corresponding
  dnl values of PYTHON_PREFIX and PYTHON_EXEC_PREFIX.  These are made
  dnl distinct variables so they can be overridden if need be.  However,
  dnl general consensus is that you shouldn't need this ability.

  AC_SUBST([PYTHON_PREFIX], ['${prefix}'])
  AC_SUBST([PYTHON_EXEC_PREFIX], ['${exec_prefix}'])

  dnl At times (like when building shared libraries) you may want
  dnl to know which OS platform Python thinks this is.

  AC_CACHE_CHECK([for $am_display_PYTHON platform], [am_cv_python_platform],
    [am_cv_python_platform=`$PYTHON -c "import sys; print sys.platform"`])
    AC_SUBST([PYTHON_PLATFORM], [$am_cv_python_platform])


  dnl Set up 4 directories:

  dnl pythondir -- where to install python scripts.  This is the
  dnl   site-packages directory, not the python standard library
  dnl   directory like in previous automake betas.  This behavior
  dnl   is more consistent with lispdir.m4 for example.
  dnl Query distutils for this directory.  distutils does not exist in
  dnl Python 1.5, so we fall back to the hardcoded directory if it
  dnl doesn't work.
  AC_CACHE_CHECK([for $am_display_PYTHON script directory],
    [am_cv_python_pythondir],
    [am_cv_python_pythondir=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_python_lib(1,0,prefix='$PYTHON_PREFIX')" 2>/dev/null ||
     echo "$PYTHON_PREFIX/lib/python$PYTHON_VERSION/site-packages"`])
  AC_SUBST([pythondir], [$am_cv_python_pythondir])

  dnl pkgpythondir -- $PACKAGE directory under pythondir.  Was
  dnl   PYTHON_SITE_PACKAGE in previous betas, but this naming is
  dnl   more consistent with the rest of automake.

  AC_SUBST([pkgpythondir], [\${pythondir}/$PACKAGE])

  dnl pyexecdir -- directory for installing python extension modules
  dnl   (shared libraries)
  dnl Query distutils for this directory.  distutils does not exist in
  dnl Python 1.5, so we fall back to the hardcoded directory if it
  dnl doesn't work.
  AC_CACHE_CHECK([for $am_display_PYTHON extension module directory],
    [am_cv_python_pyexecdir],
    [am_cv_python_pyexecdir=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_python_lib(1,0,prefix='$PYTHON_EXEC_PREFIX')" 2>/dev/null ||
     echo "${PYTHON_EXEC_PREFIX}/lib/python${PYTHON_VERSION}/site-packages"`])
  AC_SUBST([pyexecdir], [$am_cv_python_pyexecdir])

  dnl pkgpyexecdir -- $(pyexecdir)/$(PACKAGE)

  AC_SUBST([pkgpyexecdir], [\${pyexecdir}/$PACKAGE])
])

# AM_PYTHON_CHECK_VERSION(PROG, VERSION, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
# ---------------------------------------------------------------------------
# Run ACTION-IF-TRUE if the Python interpreter PROG has version >= VERSION.
# Run ACTION-IF-FALSE otherwise.
# This test uses sys.hexversion instead of the string equivalent (first
# word of sys.version), in order to cope with versions such as 2.2c1.
# hexversion has been introduced in Python 1.5.2; it's probably not
# worth to support older versions (1.5.1 was released on October 31, 1998).
AC_DEFUN([AM_PYTHON_CHECK_VERSION],
 [prog="import sys, string
# split strings by '.' and convert to numeric.  Append some zeros
# because we need at least 4 digits for the hex conversion.
minver = map(int, string.split('$2', '.')) + [[0, 0, 0]]
minverhex = 0
for i in xrange(0, 4): minverhex = (minverhex << 8) + minver[[i]]
sys.exit(sys.hexversion < minverhex)"
  AS_IF([AM_RUN_LOG([$1 -c "$prog"])], [$3], [$4])])

# AM_RUN_LOG(COMMAND)
# -------------------
# Run COMMAND, save the exit status in ac_status, and log it.
# (This has been adapted from Autoconf's _AC_RUN_LOG macro.)
AC_DEFUN([AM_RUN_LOG],
[{ echo "$as_me:$LINENO: $1" >&AS_MESSAGE_LOG_FD
   ($1) >&AS_MESSAGE_LOG_FD 2>&AS_MESSAGE_LOG_FD
   ac_status=$?
   echo "$as_me:$LINENO: \$? = $ac_status" >&AS_MESSAGE_LOG_FD
   (exit $ac_status); }])

dnl @synopsis AC_PROG_SWIG([major.minor.micro])
dnl
dnl This macro searches for a SWIG installation on your system. If
dnl found you should call SWIG via $(SWIG). You can use the optional
dnl first argument to check if the version of the available SWIG is
dnl greater than or equal to the value of the argument. It should have
dnl the format: N[.N[.N]] (N is a number between 0 and 999. Only the
dnl first N is mandatory.)
dnl
dnl If the version argument is given (e.g. 1.3.17), AC_PROG_SWIG checks
dnl that the swig package is this version number or higher.
dnl
dnl In configure.in, use as:
dnl
dnl             AC_PROG_SWIG(1.3.17)
dnl             SWIG_ENABLE_CXX
dnl             SWIG_MULTI_MODULE_SUPPORT
dnl             SWIG_PYTHON
dnl
dnl @category InstalledPackages
dnl @author Sebastian Huber <sebastian-huber@web.de>
dnl @author Alan W. Irwin <irwin@beluga.phys.uvic.ca>
dnl @author Rafael Laboissiere <rafael@laboissiere.net>
dnl @author Andrew Collier <abcollier@yahoo.com>
dnl @version 2004-09-20
dnl @license GPLWithACException

AC_DEFUN([AC_PROG_SWIG],[
        AC_PATH_PROG([SWIG],[swig])
        if test -z "$SWIG" ; then
                AC_MSG_WARN([cannot find 'swig' program. You should look at http://www.swig.org])
                SWIG='echo "Error: SWIG is not installed. You should look at http://www.swig.org" ; false'
        elif test -n "$1" ; then
                AC_MSG_CHECKING([for SWIG version])
                [swig_version=`$SWIG -version 2>&1 | grep 'SWIG Version' | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`]
                AC_MSG_RESULT([$swig_version])
                if test -n "$swig_version" ; then
                        # Calculate the required version number components
                        [required=$1]
                        [required_major=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_major" ; then
                                [required_major=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_minor=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_minor" ; then
                                [required_minor=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_patch=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_patch" ; then
                                [required_patch=0]
                        fi
                        # Calculate the available version number components
                        [available=$swig_version]
                        [available_major=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_major" ; then
                                [available_major=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_minor=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_minor" ; then
                                [available_minor=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_patch=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_patch" ; then
                                [available_patch=0]
                        fi
                        if test $available_major -ne $required_major \
                                -o $available_minor -ne $required_minor \
                                -o $available_patch -lt $required_patch ; then
                                AC_MSG_WARN([SWIG version >= $1 is required.  You have $swig_version.  You should look at http://www.swig.org])
                                SWIG='echo "Error: SWIG version >= $1 is required.  You have '"$swig_version"'.  You should look at http://www.swig.org" ; false'
                        else
                                AC_MSG_NOTICE([SWIG executable is '$SWIG'])
                                SWIG_LIB=`$SWIG -swiglib`
                                AC_MSG_NOTICE([SWIG library directory is '$SWIG_LIB'])
                        fi
                else
                        AC_MSG_WARN([cannot determine SWIG version])
                        SWIG='echo "Error: Cannot determine SWIG version.  You should look at http://www.swig.org" ; false'
                fi
        fi
        AC_SUBST([SWIG_LIB])
])

# SWIG_ENABLE_CXX()
#
# Enable SWIG C++ support.  This affects all invocations of $(SWIG).
AC_DEFUN([SWIG_ENABLE_CXX],[
        AC_REQUIRE([AC_PROG_SWIG])
        AC_REQUIRE([AC_PROG_CXX])
        SWIG="$SWIG -c++"
])

# SWIG_MULTI_MODULE_SUPPORT()
#
# Enable support for multiple modules.  This effects all invocations
# of $(SWIG).  You have to link all generated modules against the
# appropriate SWIG runtime library.  If you want to build Python
# modules for example, use the SWIG_PYTHON() macro and link the
# modules against $(SWIG_PYTHON_LIBS).
#
AC_DEFUN([SWIG_MULTI_MODULE_SUPPORT],[
        AC_REQUIRE([AC_PROG_SWIG])
        SWIG="$SWIG -noruntime"
])

# SWIG_PYTHON([use-shadow-classes = {no, yes}])
#
# Checks for Python and provides the $(SWIG_PYTHON_CPPFLAGS),
# and $(SWIG_PYTHON_OPT) output variables.
#
# $(SWIG_PYTHON_OPT) contains all necessary SWIG options to generate
# code for Python.  Shadow classes are enabled unless the value of the
# optional first argument is exactly 'no'.  If you need multi module
# support (provided by the SWIG_MULTI_MODULE_SUPPORT() macro) use
# $(SWIG_PYTHON_LIBS) to link against the appropriate library.  It
# contains the SWIG Python runtime library that is needed by the type
# check system for example.
AC_DEFUN([SWIG_PYTHON],[
        AC_REQUIRE([AC_PROG_SWIG])
        AC_REQUIRE([AC_PYTHON_DEVEL])
        test "x$1" != "xno" || swig_shadow=" -noproxy"
        AC_SUBST([SWIG_PYTHON_OPT],[-python$swig_shadow])
        AC_SUBST([SWIG_PYTHON_CPPFLAGS],[$PYTHON_CPPFLAGS])
])


dnl @synopsis AC_LIB_WAD
dnl
dnl This macro searches for installed WAD library.
dnl
AC_DEFUN([AC_LIB_WAD],
[
        AC_REQUIRE([AC_PYTHON_DEVEL])
        AC_ARG_ENABLE(wad,
        AC_HELP_STRING([--enable-wad], [enable wad module]),
        [
                case "${enableval}" in
                        no)     ;;
                        *)      if test "x${enableval}" = xyes;
                                then
                                        check_wad="yes"
                                fi ;;
                esac
        ], [])

        if test -n "$check_wad";
        then
                AC_CHECK_LIB(wadpy, _init, [WADPY=-lwadpy], [], $PYTHON_LDFLAGS $PYTHON_EXTRA_LIBS)
                AC_SUBST(WADPY)
        fi
])

dnl @synopsis AC_PYTHON_DEVEL
dnl
dnl Checks for Python and tries to get the include path to 'Python.h'.
dnl It provides the $(PYTHON_CPPFLAGS) and $(PYTHON_LDFLAGS) output variable.
dnl
dnl @author Sebastian Huber <sebastian-huber@web.de>, Alan W. Irwin <irwin@beluga.phys.uvic.ca>, Rafael Laboissiere <laboissiere@psy.mpg.de> and Andrew Collier <colliera@nu.ac.za>.
dnl
dnl @version $Id: ac_python_devel.m4,v 1.1 2004/12/24 01:03:11 guidod Exp $
dnl
AC_DEFUN([AC_PYTHON_DEVEL],[
	#
	# should allow for checking of python version here...
	#
	AC_REQUIRE([AM_PATH_PYTHON])

	# Check for Python include path
	AC_MSG_CHECKING([for Python include path])
	python_path=`echo $PYTHON | sed "s,/bin.*$,,"`
	for i in "$python_path/include/python$PYTHON_VERSION/" "$python_path/include/python/" "$python_path/" ; do
		python_path=`find $i -type f -name Python.h -print | sed "1q"`
		if test -n "$python_path" ; then
			break
		fi
	done
	python_path=`echo $python_path | sed "s,/Python.h$,,"`
	AC_MSG_RESULT([$python_path])
	if test -z "$python_path" ; then
		AC_MSG_ERROR([cannot find Python include path])
	fi
	AC_SUBST([PYTHON_CPPFLAGS],[-I$python_path])

	# Check for Python library path
	AC_MSG_CHECKING([for Python library path])
	python_path=`echo $PYTHON | sed "s,/bin.*$,,"`
	for i in "$python_path/lib/python$PYTHON_VERSION/config/" "$python_path/lib/python$PYTHON_VERSION/" "$python_path/lib/python/config/" "$python_path/lib/python/" "$python_path/" ; do
		python_path=`find $i -type f -name libpython$PYTHON_VERSION.* -print | sed "1q"`
		if test -n "$python_path" ; then
			break
		fi
	done
	python_path=`echo $python_path | sed "s,/libpython.*$,,"`
	AC_MSG_RESULT([$python_path])
	if test -z "$python_path" ; then
		AC_MSG_ERROR([cannot find Python library path])
	fi
	AC_SUBST([PYTHON_LDFLAGS],["-L$python_path -lpython$PYTHON_VERSION"])
	#
	python_site=`echo $python_path | sed "s/config/site-packages/"`
	AC_SUBST([PYTHON_SITE_PKG],[$python_site])
	#
	# libraries which must be linked in when embedding
	#
	AC_MSG_CHECKING(python extra libraries)
	PYTHON_EXTRA_LIBS=`$PYTHON -c "import distutils.sysconfig; \
                conf = distutils.sysconfig.get_config_var; \
                print conf('LOCALMODLIBS')+' '+conf('LIBS')"
	AC_MSG_RESULT($PYTHON_EXTRA_LIBS)`
	AC_SUBST(PYTHON_EXTRA_LIBS)
])
dnl as-ac-expand.m4 0.2.0
dnl autostars m4 macro for expanding directories using configure's prefix
dnl thomas@apestaart.org

dnl AS_AC_EXPAND(VAR, CONFIGURE_VAR)
dnl example
dnl AS_AC_EXPAND(SYSCONFDIR, $sysconfdir)
dnl will set SYSCONFDIR to /usr/local/etc if prefix=/usr/local

AC_DEFUN([AS_AC_EXPAND],
[
  EXP_VAR=[$1]
  FROM_VAR=[$2]

  dnl first expand prefix and exec_prefix if necessary
  prefix_save=$prefix
  exec_prefix_save=$exec_prefix

  dnl if no prefix given, then use /usr/local, the default prefix
  if test "x$prefix" = "xNONE"; then
    prefix="$ac_default_prefix"
  fi
  dnl if no exec_prefix given, then use prefix
  if test "x$exec_prefix" = "xNONE"; then
    exec_prefix=$prefix
  fi

  full_var="$FROM_VAR"
  dnl loop until it doesn't change anymore
  while true; do
    new_full_var="`eval echo $full_var`"
    if test "x$new_full_var" = "x$full_var"; then break; fi
    full_var=$new_full_var
  done

  dnl clean up
  full_var=$new_full_var
  AC_SUBST([$1], "$full_var")

  dnl restore prefix and exec_prefix
  prefix=$prefix_save
  exec_prefix=$exec_prefix_save
])
