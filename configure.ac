dnl 
dnl   This file is part of SST/macroscale: 
dnl                The macroscale architecture simulator from the SST suite.
dnl   Copyright (c) 2009-2020, NTESS.
dnl   This software is distributed under the BSD License.
dnl   Under the terms of Contract DE-NA0003525 with NTESS,
dnl   the U.S. Government retains certain rights in this software.
dnl   For more information, see the LICENSE file in the top 
dnl   SST/macroscale directory.
dnl

#-------------------------------------------------
  launch/launch_info.h  \
# Basic setup
#-------------------------------------------------

# Version info, used both in library versioning and inside sstmacro.
m4_define([SSTMAC_VERSION_TAG], 10)
m4_define([SSTMAC_SUBVERSION_TAG], 0)
m4_define([SSTMAC_SUBSUBVERSION_TAG], 0)

dnl Enable this for releases
dnl m4_define([SSTMAC_SNAPSHOT_TAG])
dnl Enable this for beta releases
dnl m4_define([SSTMAC_SNAPSHOT_TAG],-beta1)
# Enable this for development snapshots (should generally be enabled)
#m4_define([SSTMAC_SNAPSHOT_TAG],-snapshot)

# Construct the sstmacro version
m4_define([SSTMAC_ACVERSION_TAG],
          [SSTMAC_VERSION_TAG.SSTMAC_SUBVERSION_TAG.SSTMAC_SUBSUBVERSION_TAG])

dnl AC_PREREQ([2.68]) avoid this if possible
AC_INIT([sstmacro], [SSTMAC_ACVERSION_TAG], [sst-macro-help@sandia.gov])
AC_CONFIG_MACRO_DIR([acinclude])
AC_CONFIG_AUX_DIR(bin)
AC_CONFIG_HEADERS(sstmac/common/config.h)
AX_PREFIX_CONFIG_H(sstmac/common/sstmac_config.h, SSTMAC)
AC_CANONICAL_TARGET

if test "X$prefix" == "XNONE"; then
  MY_PREFIX=$ac_default_prefix
else
  MY_PREFIX=`cd $prefix ; pwd`
fi
MY_ABS_SRCPATH=`cd $srcdir ; pwd`

AC_DEFINE_UNQUOTED([CONFIG_INSTALL_INCLUDE_PATH], "$MY_PREFIX/include/sstmac/configurations",
                    [The include path for .ini configurations])

AC_DEFINE_UNQUOTED([CONFIG_SRC_INCLUDE_PATH], "$MY_ABS_SRCPATH/configurations",
                    [The include path for .ini configurations])

# Construct the libtool version
dnl Libtool library versioning is used to determine compatible libraries. A libtool version
dnl consists of three numbers CURRENT:REVISION:AGE. These have the following meanings:
dnl CURRENT:  The most recent interface number that this library implements.
dnl REVISION: The implementation number of the current interface.
dnl AGE:      The difference between the newest and oldest interfaces
dnl           that this library implements. In other words, the
dnl           library implements all the interface numbers in the
dnl           range from number current - age to current.
dnl For new releases the following procedure is used to determine the new version:
dnl If incompatible changes are made: CURRENT++, REVISION=0, AGE=0
dnl else if interfaces are added: CURRENT++, REVISION=0, AGE++
dnl else (no interface changes): REVISION++
m4_define([SSTMAC_CURRENT_LIBVERS],  10)
m4_define([SSTMAC_REVISION_LIBVERS], 0)
m4_define([SSTMAC_AGE_LIBVERS],      0)
m4_define([SSTMAC_LIBVERSION_TAG],
          [SSTMAC_CURRENT_LIBVERS:SSTMAC_REVISION_LIBVERS:SSTMAC_AGE_LIBVERS])

# More version info.
AH_TEMPLATE([VERSION], [Major version number])
AH_TEMPLATE([SUBVERSION], [Major version number])
AH_TEMPLATE([SUBSUBVERSION], [Major version number])
AC_DEFINE_UNQUOTED(VERSION, [SSTMAC_VERSION_TAG])
AC_DEFINE_UNQUOTED(SUBVERSION, [SSTMAC_SUBVERSION_TAG])
AC_DEFINE_UNQUOTED(SUBSUBVERSION, [SSTMAC_SUBSUBVERSION_TAG])
AC_SUBST(SSTMAC_LIBVERSION, [SSTMAC_LIBVERSION_TAG])

# Init automake
AM_INIT_AUTOMAKE([tar-pax -Wall -Werror foreign -Wno-portability subdir-objects])
AM_PROG_AS
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

#-------------------------------------------------
# Environment 
#-------------------------------------------------

# Find out if this is running Mac OS X.
darwin=false
case $target_os in
  darwin*)  darwin=true ;;
esac
AM_CONDITIONAL([DARWIN],[test "$darwin" = true])

# Before detecting compilers, we must see if a non-native DES core
# will be used. If so, we try to use its compiler and flags.
# Currently only supporting native DES.
nat_des=yes # native DES
sst_des=no  # SST/core DES



# Check compilers and environment
dnl AC_PROG_RANLIB
AC_PROG_CC
AC_PROG_CXX
AC_PROG_FC
AC_LANG_CPLUSPLUS
AC_PROG_LIBTOOL

LT_INIT([shared disable-static dlopen])

#LT_INIT()
LT_PREREQ([2.2.6])
AC_PROG_LN_S

CHECK_SDK()

os=`$srcdir/bin/config_tools/get_os`

if test "X$os" = "Xubuntu"; then
LDFLAGS="$LDFLAGS -Wl,--no-as-needed"
AM_CONDITIONAL([HAVE_UBUNTU], [true])
else
AM_CONDITIONAL([HAVE_UBUNTU], [false])
fi

CHECK_PYTHON()

CHECK_DLOPEN()

CHECK_SST_CORE()

CHECK_SST_ELEMENTS()

CHECK_CLANG()

CHECK_GCC()

CHECK_DEFAULT_INCLUDES()

AC_CHECK_HEADERS([mpi.h],
  have_mpi_header=yes,
  have_mpi_header=no
)

CHECK_CXX_STD()
if test "X$have_integrated_core" != "Xyes"; then
CHECK_MPI_PARALLEL()
fi

CHECK_SPACK()

AM_CONDITIONAL(HAVE_REGEXP, false)

# Check what's available and what's requested for
# thread context switching of applications
CHECK_THREADING()

CHECK_CUSTOM_NEW()

# Check for whether to support event calendar optimizations
CHECK_EVENT_CALENDAR()

# MiniMD uses atomic builtins that may not be there.  We fake it if needed.  
CHECK_ATOMICS()

# Check what debugging features to enable
# and what debugging features to disable for performance
CHECK_DEBUG()

# Define the default environment (serial,thread parallel,mpi parallel,etc)
AH_TEMPLATE([DISTRIBUTED_MEMORY], [Define to indicate distributed memory])
AH_TEMPLATE([DEFAULT_ENV_STRING], [Define to indicate default environment type (mpi/serial)])
AH_TEMPLATE([DEFAULT_RUNTIME_STRING], [Define to indicate default runtime type (mpi/serial)])
AH_TEMPLATE([DEFAULT_EVENT_MANAGER_STRING], [Define to indicate default event manager (event map/clock cycler)])
AH_TEMPLATE([DEFAULT_PARTITION_STRING], [Define to indicate default partitioning strategy])

CHECK_REPO_BUILD([sstmac])

CHECK_OTF2()

CHECK_VTK()

CHECK_THREAD_PARALLEL()

#CHECK_FORTRAN()

CHECK_CALL_GRAPH_VIZ()

CHECK_DOT()

CHECK_INTEGER_TYPES()

CHECK_STL_REPLACEMENT_HEADERS()

CHECK_COMM_SYNC_STATS()

CHECK_COMM_DELAY_STATS()

CHECK_CLANG_LLVM()

CHECK_WERROR()

CHECK_WARNINGS()


#-------------------------------------------------
# Configure subdirs
#-------------------------------------------------

# Configure dumpi.
AC_CONFIG_SUBDIRS([sst-dumpi])


#-------------------------------------------------
# Finalize 
#-------------------------------------------------

dnl Don't reorder options.
dnl AC_PRESERVE_HELP_ORDER

AC_CONFIG_FILES([
 Makefile
 sstmac/Makefile
 sstmac/install/Makefile
 sstmac/dumpi_util/Makefile
 sstmac/libraries/blas/Makefile
 sstmac/libraries/pthread/Makefile
 sstmac/libraries/omp/Makefile
 sstmac/libraries/machines/Makefile
 sstmac/libraries/Makefile
 sstmac/skeletons/Makefile
 sstmac/test_skeletons/Makefile
 sstmac/software/Makefile
 sstmac/hardware/Makefile
 sstmac/sst_core/Makefile
 sstmac/backends/Makefile
 sstmac/backends/common/Makefile
 sstmac/backends/native/Makefile
 sstmac/backends/mpi/Makefile
 sstmac/main/Makefile
 sstmac/common/Makefile
 sstmac/replacements/Makefile
 sstmac/clang_replacements/Makefile
 sprockit/Makefile
 sprockit/sprockit/Makefile
 sprockit/test/Makefile
 bin/Makefile
 include/Makefile
 python/Makefile
 configurations/Makefile
 tests/Makefile
 tests/external/Makefile
 tests/sumi/Makefile
 tests/api/mpi/Makefile
 tests/api/globals/Makefile
 docs/doxygen.cfg
 sumi-mpi/Makefile
 sumi/Makefile
])

AC_CONFIG_FILES([bin/sstmacro-config], [chmod +x bin/sstmacro-config])
AC_CONFIG_FILES([bin/sst++], [chmod +x bin/sst++])
AC_CONFIG_FILES([bin/sstcc], [chmod +x bin/sstcc])
AC_CONFIG_FILES([bin/sstccvars.py])

AC_OUTPUT

if test "X$with_mpiparallel" = "Xtrue"; then
  yesno_mpiparallel="yes"
else
  yesno_mpiparallel="no"
fi

# --------- Print out a configuration summary. ---------
echo
echo "SST Macroscale Configuration Summary:"
echo "---------------------------------------------------------------"
echo "Install prefix     $prefix"
echo "OS                 $os"
echo "C compiler         $CC"
echo "C++ compiler       $CXX"
echo "CFLAGS             $CFLAGS"
echo "CXXFLAGS           $CXXFLAGS $SST_CXXFLAGS $STD_CXXFLAGS"
echo "CPPFLAGS           $CPPFLAGS"
echo "LDFLAGS            $LDFLAGS"
echo "C++ Standard       $cxxstd"
echo

echo "User Space Threading:"
echo "  GNU Pth          $enable_pth"
echo "  Pthread          $enable_pthread"
echo "  Ucontext         $enable_ucontext"
echo "  Fcontext         Auto-included"
echo

echo "Parallel Discrete Event Simulation:"
echo "  MPI PDES         $yesno_mpiparallel"
echo "  Multithreaded    $with_multithread"
echo "  Spinlock         $with_spinlock"
echo "  Thread Affinity  $with_cpu_affinity"
echo

echo "MAC SDK            $enable_sdk"
echo "SST Core           $have_integrated_core"
echo "Python             $pyexe"
echo "Clang Autoskeleton $found_clang"
echo "OTF2 Replay        $build_otf2"
echo "MPI Sync Stats     $with_comm_sync_stats"
echo "Call Graph Viz     $enable_call_graph"
echo "Sanity Checking    $enable_sanity_check"
if test -z "$vtk_path"; then
echo "VTK                no"
else
echo "VTK                $vtk_version @ $vtk_path"
fi

echo "---------------------------------------------------------------"

