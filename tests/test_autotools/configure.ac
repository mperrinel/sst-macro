AC_INIT([test_autotools], [1.0])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
LT_INIT

AC_PROG_CC
AC_PROG_CXX

AC_RUN_IFELSE(
  [AC_LANG_PROGRAM(,
    [[ ]])],
  [AC_MSG_RESULT([test program executed])],
  [AC_MSG_FAILURE([test program fails to execute])])

CHECK_THREADING()

CHECK_MPI()

AC_CONFIG_FILES([
 Makefile
])
AC_OUTPUT
