dnl AM_PATH_CHECK([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for check, and define CHECK_CFLAGS and CHECK_LIBS
dnl

AC_DEFUN(AM_PATH_CHECK,
[
  AC_ARG_WITH(check,
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
