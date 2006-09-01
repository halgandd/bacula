@echo off
SETLOCAL

SET PERL_PATH=
FOR /F "tokens=1" %%I in ( "perl.exe" ) DO SET PERL_PATH=%%~$PATH:I

IF NOT "%PERL_PATH%"=="" GOTO :PERL_OK
ECHO Perl must be in your PATH.  Please fix and rerun this script!
EXIT /B 1

:PERL_OK

SET CLOBBER_SOURCE=

FOR /F "eol=# delims=| tokens=1-4" %%I in ( External-msvc ) DO SET URL_%%I=%%J & SET DIR_%%I=%%K & SET MKD_%%I=%%L

SET CWD=%CD%
FOR %%I IN ( %0 ) DO CD %%~dpI
SET SCRIPT_DIR=%CD%

CD ..\..\..
set TOP_DIR=%CD%

cd %TOP_DIR%/depkgs-msvc
set DEPPKG_DIR=%CD%

cd %DEPPKG_DIR%

IF /I "%CLOBBER_SOURCE:~0,1%"=="y" SET CLOBBER_SOURCE=true
IF /I "%CLOBBER_SOURCE:~0,1%"=="t" SET CLOBBER_SOURCE=true
IF /I "%CLOBBER_SOURCE:~0,1%"=="1" SET CLOBBER_SOURCE=true

SET PATH=%DEPPKG_DIR%\tools;%PATH%
SET INCLUDE=%INCLUDE%;%DEPPKG_DIR%\include
SET LIB=%LIB%;%DEPPKG_DIR%\lib

IF NOT EXIST bin\nul MKDIR bin
IF NOT EXIST src\nul MKDIR src
IF NOT EXIST include\nul MKDIR include
IF NOT EXIST lib\nul MKDIR lib

CD src

COPY NUL parse_output.sed
REM ECHO /\\$/N >>parse_output.sed
REM ECHO s/\\\n// >>parse_output.sed
ECHO s/\t\+/ /g >>parse_output.sed
ECHO s/ \+/ /g >>parse_output.sed
ECHO / [Ee]rror: /p >>parse_output.sed
REM ECHO s;.*Entering directory[ ]\+.%DEPPKG_DIR%/\([^^ ]\+\).;Entering \1;p >>parse_output.sed
REM ECHO s;.*Leaving directory[ ]\+.%DEPPKG_DIR%/\([^^ ]\+.\).;Leaving \1;p >>parse_output.sed
ECHO /[Cc][Ll] \^|[Ll][Ii][Nn][Kk] \^|[Ll][Ii][Bb] /!d >>parse_output.sed
ECHO s; \(\.\.\\\)\+; ;g >>parse_output.sed
ECHO s; \.\\; ;g >>parse_output.sed
ECHO s/.* \([^^ ]\+\(\.[Cc]\^|\.[Cc][Pp][Pp]\^|\.[Cc][Cc]\^|\.[Cc][Xx][Xx]\)\)\( .*\^|\)$/Compiling \1/p >>parse_output.sed
ECHO s/.* \([^^ ]\+\.[Aa][Ss][Mm]\)\( .*\^|\)$/Assembling \1/p >>parse_output.sed
ECHO s/.* \/[Oo][Uu][Tt]:\(\.\\\)*\(\.\.\\\)*\([^^ ]\+\)\( .*\^|\)$/Linking \3/p >>parse_output.sed
REM ECHO s/.*[Ll][Ii][Bb] [^^ ]\+ \([^^ ]\+\)\(\( [^^ ]\+\.o\)\+\)/Updating \1 -\2/p >>parse_output.sed

IF NOT "%1"=="" GOTO :ProcessArgs
	CALL :process_zlib
REM	CALL :process_pcre
	CALL :process_pthreads
	CALL :process_openssl
	CALL :process_mysql
REM	CALL :process_sqlite
	CALL :process_postgreSQL
	CALL :process_wx
REM	CALL :process_scons
	CALL :process_nsis
	CALL :process_mtx
	CALL :process_mt
	GOTO :EOF

:ProcessArgs
	IF "%1"=="" GOTO :EOF
	
	CALL :process_%1
	SHIFT
	GOTO :ProcessArgs

:get_source
	SET URL=%1
	SET SRC_DIR=%2
	SET MAKE_SRC_ENABLE=%3
	SET BASENAME=

	IF NOT "%SRC_DIR%"=="" GOTO :get_source_make_src
	FOR %%I IN ( %URL% ) DO (SET BASENAME=%%~nI & IF NOT "%%~xI"==".gz" IF NOT "%%~xI"==".bz2" SET SRC_DIR=%%~nI)
	IF "%SRC_DIR%"=="" FOR %%I IN ( %BASENAME% ) DO SET SRC_DIR=%%~nI

:get_source_make_src
	SET MAKE_SRC_DIR=
	IF /I "%MAKE_SRC_ENABLE:~0,1%"=="y" SET MAKE_SRC_DIR=true
	IF /I "%MAKE_SRC_ENABLE:~0,1%"=="t" SET MAKE_SRC_DIR=true
	IF /I "%MAKE_SRC_ENABLE:~0,1%"=="1" SET MAKE_SRC_DIR=true

	FOR %%I IN ( %URL% ) DO SET ARCHIVE=%%~nxI
	
	CD %DEPPKG_DIR%\src
	
	IF EXIST %ARCHIVE% GOTO :get_source_check_srcdir
	ECHO Downloading %URL%
	wget -nv --passive-ftp "%URL%"
	IF NOT ERRORLEVEL 1 GOTO :get_source_extract
	ECHO Unable to download %ARCHIVE%
	EXIT /B 2

:get_source_check_srcdir
	IF NOT EXIST %SRC_DIR%\nul GOTO :get_source_extract
	IF NOT "%CLOBBER_SOURCE%"=="true" GOTO :get_source_skipped
	IF EXIST "%SRC_DIR%\nul" RD /s /q "%SRC_DIR%"
:get_source_extract
	SET TAR_ARCHIVE=
	FOR %%I IN ( "%ARCHIVE%" ) do IF "%%~xI"==".gz" SET TAR_ARCHIVE=%%~nI
	FOR %%I IN ( "%ARCHIVE%" ) do IF "%%~xI"==".bz2" SET TAR_ARCHIVE=%%~nI

	SET TARGET_DIR=.
	IF DEFINED MAKE_SRC_DIR IF NOT EXIST "%SRC_DIR%\nul" MKDIR %SRC_DIR%
	IF DEFINED MAKE_SRC_DIR SET TARGET_DIR=%SRC_DIR%

	ECHO Extracting %ARCHIVE%
	7z x -bd -y -o"%TARGET_DIR%" "%ARCHIVE%" 2>&1 > "%ARCHIVE%.log"
	IF ERRORLEVEL 1 EXIT /B 2
	IF NOT DEFINED TAR_ARCHIVE GOTO :get_source_done
	ECHO Extracting %TAR_ARCHIVE%
	7z x -bd -y -o"%TARGET_DIR%" "%TARGET_DIR%\%TAR_ARCHIVE%" & del /q "%TAR_ARCHIVE%" 2>&1 >> "%ARCHIVE%.log"

:get_source_done
	CD "%SRC_DIR%"
	EXIT /B 0

:get_source_skipped
	CD %SRC_DIR%
	EXIT /B 1

:process_zlib
	CALL :get_source %URL_ZLIB% %DIR_ZLIB% %MKD_ZLIB%
	ECHO Building zlib
	COPY /Y nul make.log
REM	XCOPY contrib\masmx86\gvmat32.asm . /Y
REM	XCOPY contrib\masmx86\gvmat32c.c . /Y
REM	XCOPY contrib\masmx86\inffas32.asm . /Y
REM	CALL :do_nmake win32\Makefile.msc AS=nasmw ASFLAGS= LOC="-D_CRT_SECURE_NO_DEPRECATE -DASMV -DASMINF" OBJA="gvmat32c.obj gvmat32.obj inffas32.obj" all
	CALL :do_nmake win32\Makefile.msc LOC="-D_CRT_SECURE_NO_DEPRECATE" all
	ECHO Installing zlib
	XCOPY zlib1.dll.manifest %DEPPKG_DIR%\bin\ /Y
	XCOPY zlib1.dll %DEPPKG_DIR%\bin\ /Y
	XCOPY zlib.lib %DEPPKG_DIR%\lib\ /Y
	XCOPY zdll.lib %DEPPKG_DIR%\lib\ /Y
	XCOPY zlib.h %DEPPKG_DIR%\include\ /Y
	XCOPY zconf.h %DEPPKG_DIR%\include\ /Y
	EXIT /B 0

:process_pcre
	CALL :get_source %URL_PCRE% %DIR_PCRE% %MKD_PCRE%
REM	ECHO Patching PCRE
REM	COPY /Y nul patch.log
REM	do_patch pcre.patch
	ECHO Configuring PCRE
	./configure CC_FOR_BUILD=gcc CXX_FOR_BUILD=g++ --host=mingw32 --prefix=%DEPPKG_DIR% --enable-utf8 --enable-unicode-properties 2>&1 > make.log
	ECHO Building PCRE
	do_make Makefile PREFIX=%DEPPKG_DIR% all
	ECHO Installing PCRE
	do_make Makefile PREFIX=%DEPPKG_DIR% install
	EXIT /B 0

:process_pthreads
	CALL :get_source %URL_PTHREADS% %DIR_PTHREADS% %MKD_PTHREADS%
	ECHO Installing pthreads
	XCOPY Pre-built\include\*.h %DEPPKG_DIR%\include\ /Y
	XCOPY Pre-built\lib\pthreadVCE.lib %DEPPKG_DIR%\lib\ /Y
	XCOPY Pre-built\lib\pthreadVCE.dll %DEPPKG_DIR%\bin\ /Y
	EXIT /B 0

:process_openssl
	CALL :get_source %URL_OPENSSL% %DIR_OPENSSL% %MKD_OPENSSL%
	IF ERRORLEVEL 2 GOTO :OpenSSLError
	ECHO Configuring openssl
	perl Configure VC-WIN32 --prefix=%DEPPKG_DIR:\=/% > make.log
	ECHO Building openssl
	CALL ms\do_nasm
	@ECHO off
	CALL :do_nmake ms\ntdll.mak 
	@ECHO off
	ECHO Installing openssl
	CALL :do_nmake ms\ntdll.mak install
	EXIT /B 0
:OpenSSLError
	ECHO Unable to download OpenSSL source from %URL_OPENSSL%
	EXIT /B 1

:process_mysql
	CALL :get_source %URL_MYSQL% %DIR_MYSQL% %MKD_MYSQL%
	ECHO Installing mysql
	IF EXIST ..\..\include\mysql\nul RD /s /q ..\..\include\mysql
	XCOPY include\* ..\..\include\mysql\ /e/h/k/y 2>&1
	XCOPY lib\opt\libmysql.lib ..\..\lib\ /y
	XCOPY lib\opt\libmysql.dll ..\..\bin\ /y
	EXIT /B 0

:process_postgreSQL
	CALL :get_source %URL_POSTGRESQL% %DIR_POSTGRESQL% %MKD_POSTGRESQL%
	IF ERRORLEVEL 2 GOTO :pstgreSQL_Error1
	CALL :get_source %URL_POSTGRESQL_SRC% %DIR_POSTGRESQL_SRC% %MKD_POSTGRESQL_SRC%
	IF ERRORLEVEL 2 GOTO :pstgreSQL_Error2
REM	ECHO Patching postgreSQL
REM	COPY /Y nul patch.log
REM	do_patch postgresql.patch
	ECHO Installing postgreSQL
	XCOPY ..\pgsql\include\libpq-fe.h ..\..\include\ /Y 2>&1
	XCOPY ..\pgsql\include\postgres_ext.h ..\..\include\ /Y 2>&1
	LIB /def:src\interfaces\libpq\libpqdll.def /out:..\..\lib\libpqdll.lib /machine:x86 /subsystem:console
	XCOPY ..\pgsql\lib\libpq.dll ..\..\bin\ /Y
	XCOPY ..\pgsql\lib\comerr32.dll ..\..\bin\ /Y
	XCOPY ..\pgsql\lib\krb5_32.dll ..\..\bin\ /Y
	XCOPY ..\pgsql\lib\libintl-2.dll ..\..\bin\ /Y
	XCOPY ..\pgsql\lib\libiconv-2.dll ..\..\bin\ /Y
	EXIT /B 0
:pstgreSQL_Error1
	ECHO Unable to download postgreSQL binaries from %URL_POSTGRESQL%
	EXIT /B 1
:pstgreSQL_Error2
	ECHO Unable to download postgreSQL source from %URL_POSTGRESQL_SRC%
	EXIT /B 1

:process_sqlite
	CALL :get_source %URL_SQLITE% %DIR_SQLITE% %MKD_SQLITE%
	CALL :get_source %URL_SQLITE_WINSRC% %DIR_SQLITE_WINSRC% %MKD_SQLITE_WINSRC%
REM	ECHO Patching SQLite
REM	COPY /Y nul patch.log
REM	do_patch sqlite.patch
	ECHO Configuring SQLite
	IF NOT EXIST bld/nul MKDIR bld
	CD bld
	ECHO Building SQLite
	COPY /Y nul make.log
	do_make ../Makefile.mingw32 CROSSTOOLS=%BIN_DIR% TLIBS="-L%DEPPKG_DIR%/lib" TCL_FLAGS="-I%DEPPKG_DIR%/include" clean all
	ECHO Installing SQLite
	cp -p sqlite3.exe %DEPPKG_DIR%/bin
	cp -p libsqlite3.a %DEPPKG_DIR%/lib
	cp -p sqlite3.h %DEPPKG_DIR%/include
	EXIT /B 0

:process_wx
	CALL :get_source %URL_WX% %DIR_WX% %MKD_WX%
	IF ERRORLEVEL 2 GOTO :wx_error
REM	IF ERRORLEVEL 1 GOTO :wx_skip_patch
REM	ECHO Patching wxWidgets
REM	COPY /Y nul patch.log
REM	do_patch wx1.patch -o build/msw/config.mingw32
REM	do_patch wx2.patch
REM	find . -name makefile.gcc -exec sh -c "sed -f %SCRIPT_DIR%/patches/wx.sed {% > \`ECHO {% | sed s/\.gcc$/\.mingw32/\`" \;
	ECHO Building wxWidgets
	cd build\msw
	COPY /Y nul make.log
	CALL :do_nmake makefile.vc BUILD=release SHARED=1 VENDOR=bacula DEBUG_INFO=1 CPPFLAGS=-D_USE_32BIT_TIME_T
	ECHO Installing wxWidgets
	cd ..\..
	IF EXIST ..\..\include\wx\nul RD /s /q ..\..\include\wx
	XCOPY include\wx ..\..\include\wx\ 2>&1
	XCOPY include\wx\generic ..\..\include\wx\generic\ /e 2>&1
	XCOPY include\wx\msw ..\..\include\wx\msw\ /e 2>&1
	XCOPY lib\vc_dll\*.dll ..\..\bin\ /y 
	IF EXIST ..\..\lib\wx_dll\nul RD /s /q ..\..\lib\wx_dll
	XCOPY lib\vc_dll\*.lib ..\..\lib\wx_dll\
	XCOPY lib\vc_dll\msw ..\..\lib\wx_dll\msw\ /e 2>&1
	EXIT /B 0
:wx_error
	ECHO Unable to download wxWidgets source from %URL_MTX%
	EXIT /B 1

:process_nsis
	CALL :get_source %URL_NSIS_BIN% %DIR_NSIS_BIN% %MKD_NSIS_BIN%
	CD ..
	IF EXIST ..\nsis\nul RD /s /q ..\nsis
	MOVE nsis-2.17 ..\nsis
	EXIT /B 0

:process_mtx
	CALL :get_source %URL_MTX% %DIR_MTX% %MKD_MTX%
	IF ERRORLEVEL 2 GOTO :mtx_error
	IF ERRORLEVEL 1 GOTO :mtx_skip_patch
	ECHO Patching mtx
	COPY /Y config.h.in config.h
	COPY /Y Makefile.in Makefile
	COPY /Y nul patch.log
	CALL :do_patch mtx.patch
	CALL :do_patch mtx-msvc1.patch
	CALL :do_patch mtx-msvc2.patch
:mtx_skip_patch
	ECHO Building mtx
	CALL :do_nmake Makefile.msc PREFIX=%DEPPKG_DIR% all
	ECHO Installing mtx
	CALL :do_nmake Makefile.msc PREFIX=%DEPPKG_DIR% install
	EXIT /B 0
:mtx_error
	ECHO Unable to download mtx source from %URL_MTX%
	EXIT /B 1

:process_mt
	CALL :get_source %URL_MT% %DIR_MT% %MKD_MT%
	IF ERRORLEVEL 2 GOTO :mt_error
	IF ERRORLEVEL 1 GOTO :mt_skip_patch
	ECHO Patching mt
	COPY /Y nul patch.log
	CALL :do_patch mt.patch
:mt_skip_patch
	ECHO Building mt
	CALL :do_nmake Makefile.msc PREFIX=%DEPPKG_DIR% all
	ECHO Installing mt
	CALL :do_nmake Makefile.msc PREFIX=%DEPPKG_DIR% install
	EXIT /B 0
:mt_error
	ECHO Unable to download mt source from %URL_MT%
	EXIT /B 1

:do_patch
	SET PATCH_FILE=%SCRIPT_DIR%\patches\%1
	SHIFT
	patch -f -p0 %2 %3 %4 %5 %6 %7 %8 %9 >>patch.log < %PATCH_FILE%
	IF NOT ERRORLEVEL 1 EXIT/B 0
	ECHO Patch failed - Check %CD%\patch.log > con
	EXIT /B 1

:do_nmake
	nmake /nologo -f %* 2>&1 | tee -a make.log | sed -nf "%DEPPKG_DIR%\src\parse_output.sed"
	IF NOT ERRORLEVEL 1 EXIT/B 0
	ECHO Make failed - Check %CD%\make.log > con
	EXIT /B 1

#vss
#Need to download from Microsoft
