/*
 *  This file was originally generated by configure, but has been edited
 *  to provide the correct defines for the Native Win32 build under
 *  Visual Studio.
 */
/* ------------------------------------------------------------------------- */
/* --                     CONFIGURE SPECIFIED FEATURES                    -- */
/* ------------------------------------------------------------------------- */

/* Define if you want to use MySQL as Catalog database */
/* #undef USE_MYSQL_DB */

/* Define if you want SmartAlloc debug code enabled */
#define SMARTALLOC 1

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef daddr_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
#define major_t int

/* Define to `int' if <sys/types.h> doesn't define.  */
#define minor_t int

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef ssize_t */

/* Define if you want to use MySQL */
/* #undef HAVE_MYSQL */

/* Defined if MySQL thread safe library is present */
/* #undef HAVE_THREAD_SAFE_MYSQL */

/* Define if you want to use embedded MySQL */
/* #undef HAVE_EMBEDDED_MYSQL */

/* Define if you want to use SQLite */
/* #undef HAVE_SQLITE */

/* Define if you want to use Berkeley DB */
/* #undef HAVE_BERKELEY_DB */


/* Define if you want to use PostgreSQL */
/* #undef HAVE_PGSQL */

/* Define if you want to use mSQL */
/* #undef HAVE_MSQL */

/* Define if you want to use iODBC */
/* #undef HAVE_IODBC */

/* Define if you want to use unixODBC */
/* #undef HAVE_UNIXODBC */

/* Define if you want to use Solid SQL Server */
/* #undef HAVE_SOLID */

/* Define if you want to use OpenLink ODBC (Virtuoso) */
/* #undef HAVE_VIRT */

/* Define if you want to use EasySoft ODBC */
/* #undef HAVE_EASYSOFT */

/* Define if you want to use Interbase SQL Server */
/* #undef HAVE_IBASE */

/* Define if you want to use Oracle 8 SQL Server */
/* #undef HAVE_ORACLE8 */

/* Define if you want to use Oracle 7 SQL Server */
/* #undef HAVE_ORACLE7 */


/* ------------------------------------------------------------------------- */
/* --                     CONFIGURE DETECTED FEATURES                     -- */
/* ------------------------------------------------------------------------- */

/* Define if you need function prototypes */
#define PROTOTYPES 1

/* Define if you have XPointer typedef */
/* #undef HAVE_XPOINTER */

/* Define if you have _GNU_SOURCE getpt() */
/* #undef HAVE_GETPT */

/* Define if you have GCC */
/* #undef HAVE_GCC 1 */

/* Define if you have the Andrew File System.  */
/* #undef AFS */

/* Define If you want find -nouser and -nogroup to make tables of
   used UIDs and GIDs at startup instead of using getpwuid or
   getgrgid when needed.  Speeds up -nouser and -nogroup unless you
   are running NIS or Hesiod, which make password and group calls
   very expensive.  */
/* #undef CACHE_IDS */

/* Define to use SVR4 statvfs to get filesystem type.  */
/* #undef FSTYPE_STATVFS */

/* Define to use SVR3.2 statfs to get filesystem type.  */
/* #undef FSTYPE_USG_STATFS */

/* Define to use AIX3 statfs to get filesystem type.  */
/* #undef FSTYPE_AIX_STATFS */

/* Define to use 4.3BSD getmntent to get filesystem type.  */
/* #undef FSTYPE_MNTENT 1 */

/* Define to use 4.4BSD and OSF1 statfs to get filesystem type.  */
/* #undef FSTYPE_STATFS */

/* Define to use Ultrix getmnt to get filesystem type.  */
/* #undef FSTYPE_GETMNT */

/* Define to `unsigned long' if <sys/types.h> doesn't define.  */
/* #undef dev_t */

/* Define to `unsigned long' if <sys/types.h> doesn't define.  */
/* #undef ino_t */

/* Define to 1 if utime.h exists and declares struct utimbuf.  */
#ifdef HAVE_MINGW
#define HAVE_UTIME_H 1
#endif

#if (HAVE_MYSQL||HAVE_PGSQL||HAVE_MSQL||HAVE_IODBC||HAVE_UNIXODBC||HAVE_SOLID||HAVE_VIRT||HAVE_IBASE||HAVE_ORACLE8||HAVE_ORACLE7||HAVE_EASYSOFT)
#define HAVE_SQL
#endif

/* Data types */
#define HAVE_U_INT 1
#define HAVE_INTXX_T 1
#define HAVE_U_INTXX_T 1
/* #undef HAVE_UINTXX_T */
#define HAVE_INT64_T 1
#define HAVE_U_INT64_T 1
#define HAVE_INTMAX_T 1
/* #undef HAVE_U_INTMAX_T */

/* Define if you want TCP Wrappers support */
/* #undef HAVE_LIBWRAP */

/* Define if you have sys/bitypes.h */
/* #undef HAVE_SYS_BITYPES_H */

/* Directory for PID files */
#define _PATH_BACULA_PIDDIR "/var/run"

/* LOCALEDIR */
#define LOCALEDIR "."

/* Define if you have zlib */
#define HAVE_LIBZ 1

/* General libs */
/* #undef LIBS */

/* File daemon specif libraries */
#define FDLIBS 1

/* Path to Sendmail program */
/* #undef SENDMAIL_PATH */

/* What kind of signals we have */
/*#define HAVE_POSIX_SIGNALS 1 */
/* #undef HAVE_BSD_SIGNALS */
/* #undef HAVE_USG_SIGHOLD */

/* Operating systems */
/* OSes */
/* #undef HAVE_LINUX_OS */
/* #undef HAVE_FREEBSD_OS */
/* #undef HAVE_NETBSD_OS */
/* #undef HAVE_OPENBSD_OS */
/* #undef HAVE_BSDI_OS */
/* #undef HAVE_HPUX_OS */
/* #undef HAVE_SUN_OS */
/* #undef HAVE_IRIX_OS */
/* #undef HAVE_AIX_OS */
/* #undef HAVE_SGI_OS */
/* #define HAVE_CYGWIN 1 */
/* #undef HAVE_OSF1_OS */
/* #undef HAVE_DARWIN_OS */

/* Set to correct scanf value for long long int */
#define lld "lld"
#define llu "llu"

/*#define HAVE_READLINE 1 */

/* #undef HAVE_GMP */

/* #undef HAVE_CWEB */

/* #define HAVE_FCHDIR 1 */

/* #undef HAVE_GETOPT_LONG */

/* #undef HAVE_LIBSM */

/* Check for thread safe routines */
/*#define HAVE_LOCALTIME_R 1 */
/* #undef HAVE_READDIR_R */
/*#define HAVE_STRERROR_R 1*/
/* #undef HAVE_GETHOSTBYNAME_R */

#define HAVE_STRTOLL 1
/* #undef HAVE_INET_PTON */

/*#define HAVE_SOCKLEN_T 1 */

/* #undef HAVE_OLD_SOCKOPT */

/* #undef HAVE_BIGENDIAN */

/* Define to 1 if the `closedir' function returns void instead of `int'. */
/* #undef CLOSEDIR_VOID */

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #undef ENABLE_NLS */

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1
#ifndef HAVE_MINGW
#define alloca _alloca
#endif
/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the <arpa/nameser.h> header file. */
/*#define HAVE_ARPA_NAMESER_H 1 */

/* Define to 1 if you have the <assert.h> header file. */
#define HAVE_ASSERT_H 1

/* Define to 1 if you have the `chflags' function. */
/* #undef HAVE_CHFLAGS */

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the `fchdir' function. */
/*#define HAVE_FCHDIR 1 */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if your system has a working POSIX `fnmatch' function. */
/*#define HAVE_FNMATCH 1 */

/* Define to 1 if you have the `fork' function. */
/*#define HAVE_FORK 1 */

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define to 1 if you have the `getdomainname' function. */
/*#define HAVE_GETDOMAINNAME 1 */

/* Define to 1 if you have the `gethostbyname_r' function. */
/* #undef HAVE_GETHOSTBYNAME_R */

/* Define to 1 if you have the `gethostid' function. */
#define HAVE_GETHOSTID 1

/* Define to 1 if you have the `gethostname' function. */
#define HAVE_GETHOSTNAME 1

/* Define to 1 if you have the `getmntent' function. */
/*#define HAVE_GETMNTENT 1 */

/* Define to 1 if you have the `getpid' function. */
#define HAVE_GETPID 1
#define getpid _getpid

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the <grp.h> header file. */
/*#define HAVE_GRP_H 1*/

/* Define to 1 if you have the `inet_pton' function. */
/* #undef HAVE_INET_PTON */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `lchown' function. */
#define HAVE_LCHOWN 1

/* Define to 1 if you have the <libc.h> header file. */
/* #undef HAVE_LIBC_H */

/* Define to 1 if you have the `inet' library (-linet). */
/* #undef HAVE_LIBINET */

/* Define to 1 if you have the `nsl' library (-lnsl). */
/* #undef HAVE_LIBNSL */

/* Define to 1 if you have the `resolv' library (-lresolv). */
/* #undef HAVE_LIBRESOLV */

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Define to 1 if you have the `sun' library (-lsun). */
/* #undef HAVE_LIBSUN */

/* Define to 1 if you have the `xnet' library (-lxnet). */
/* #undef HAVE_LIBXNET */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the `localtime_r' function. */
#define HAVE_LOCALTIME_R 1

/* Define to 1 if you have the `lstat' function. */
#define HAVE_LSTAT 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <mtio.h> header file. */
/* #undef HAVE_MTIO_H */

/* Define to 1 if you have the `nanosleep' function. */
#define HAVE_NANOSLEEP 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the `putenv' function. */
#define HAVE_PUTENV 1

/* Define to 1 if you have the <pwd.h> header file. */
/*#define HAVE_PWD_H 1*/

/* Define to 1 if you have the `readdir_r' function. */
/* #undef HAVE_READDIR_R */

/* Define to 1 if you have the <resolv.h> header file. */
/*#define HAVE_RESOLV_H 1*/

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the `setlocale' function. */
#undef HAVE_SETLOCALE

#undef HAVE_NL_LANGINFO

/* Define to 1 if you have the `setpgid' function. */
#define HAVE_SETPGID 1

/* Define to 1 if you have the `setpgrp' function. */
#define HAVE_SETPGRP 1

/* Define to 1 if you have the `setsid' function. */
#define HAVE_SETSID 1

/* Define to 1 if you have the `signal' function. */
/*#define HAVE_SIGNAL 1 */

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the <stdarg.h> header file. */
/*#define HAVE_STDARG_H 1*/

/* Define to 1 if you have the <stdint.h> header file. */
/*#define HAVE_STDINT_H 1 */

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `strerror_r' function. */
#define HAVE_STRERROR_R 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncmp' function. */
#define HAVE_STRNCMP 1

/* Define to 1 if you have the `strncpy' function. */
#define HAVE_STRNCPY 1

/* Define to 1 if you have the `strtoll' function. */
#define HAVE_STRTOLL 1

/* Define to 1 if `st_blksize' is member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BLKSIZE 1

/* Define to 1 if `st_blocks' is member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_BLOCKS 1

/* Define to 1 if `st_rdev' is member of `struct stat'. */
#define HAVE_STRUCT_STAT_ST_RDEV 1

/* Define to 1 if `tm_zone' is member of `struct tm'. */
/* #undef HAVE_STRUCT_TM_TM_ZONE */

/* Define to 1 if your `struct stat' has `st_blksize'. Deprecated, use
   `HAVE_STRUCT_STAT_ST_BLKSIZE' instead. */
#define HAVE_ST_BLKSIZE 1

/* Define to 1 if your `struct stat' has `st_blocks'. Deprecated, use
   `HAVE_STRUCT_STAT_ST_BLOCKS' instead. */
#define HAVE_ST_BLOCKS 1

/* Define to 1 if your `struct stat' has `st_rdev'. Deprecated, use
   `HAVE_STRUCT_STAT_ST_RDEV' instead. */
#define HAVE_ST_RDEV 1

/* Define to 1 if you have the <sys/byteorder.h> header file. */
/* #undef HAVE_SYS_BYTEORDER_H */

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mtio.h> header file. */
#define HAVE_SYS_MTIO_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the `tcgetattr' function. */
#define HAVE_TCGETATTR 1

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if your `struct tm' has `tm_zone'. Deprecated, use
   `HAVE_STRUCT_TM_TM_ZONE' instead. */
/* #undef HAVE_TM_ZONE */

/* Define to 1 if you don't have `tm_zone' but do have the external array
   `tzname'. */
#define HAVE_TZNAME 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <varargs.h> header file. */
/* #undef HAVE_VARARGS_H */

/* Define to 1 if you have the `vfprintf' function. */
#define HAVE_VFPRINTF 1

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the <zlib.h> header file. */
#define HAVE_ZLIB_H 1

/* Define to 1 if `major', `minor', and `makedev' are declared in <mkdev.h>.
   */
/* #undef MAJOR_IN_MKDEV */

/* Define to 1 if `major', `minor', and `makedev' are declared in
   <sysmacros.h>. */
/* #undef MAJOR_IN_SYSMACROS */

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if the `setpgrp' function takes no argument. */
#define SETPGRP_VOID 1

/* The size of a `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `int *', as computed by sizeof. */
#define SIZEOF_INT_P 4

/* The size of a `long int', as computed by sizeof. */
#define SIZEOF_LONG_INT 4

/* The size of a `long long int', as computed by sizeof. */
#define SIZEOF_LONG_LONG_INT 8

/* The size of a `short int', as computed by sizeof. */
#define SIZEOF_SHORT_INT 2

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Define to 1 if the X Window System is missing or not being used. */
/* #undef X_DISPLAY_MISSING */

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define to make fseeko etc. visible, on some hosts. */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `long' if <sys/types.h> does not define. */
/* #undef daddr_t */

/* Define to `unsigned long' if <sys/types.h> does not define. */
/* #undef dev_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `unsigned long' if <sys/types.h> does not define. */
/* #undef ino_t */

/* Define to `int' if <sys/types.h> does not define. */
#define major_t int

/* Define to `int' if <sys/types.h> does not define. */
#define minor_t int

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `long' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef ssize_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */
