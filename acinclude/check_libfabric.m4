
AC_DEFUN([CHECK_LIBFABRIC], [
AC_ARG_ENABLE(libfabric,
  [AS_HELP_STRING(
    [--(dis|en)able-libfabric],
    [Control whether or not SST/macro libfabric wrappers get installed (default=disable)],
    )],
  [
    enable_libfabric=$enableval
  ], [
    enable_libfabric=no
  ]
)
if test "X$enable_libfabric" = "Xyes"; then
  AC_DEFINE_UNQUOTED([HAVE_LIBFABRIC], 1, "Call graph utility is available for use")
  AM_CONDITIONAL([HAVE_LIBFABRIC], true)
else
  AM_CONDITIONAL([HAVE_LIBFABRIC], false)
  AC_DEFINE_UNQUOTED([HAVE_LIBFABRIC], 0, "Call graph utility is not available for use")
fi

])
