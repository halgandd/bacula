REM
REM Run a simple backup of the Bacula build directory but 
REM   split the archive into two volumes, then build a BSR with
REM   the restore command and use bextract to restore the files.
REM
SET TestName=bextract-test
SET JobName=bextract

CALL scripts\functions set_debug 0
CALL scripts\functions copy_test_confs

ECHO %CD:\=/%/build >\tmp\file-list

CALL scripts\functions change_jobname NightlySave %JobName%
CALL scripts\functions start_test

sed -e "s;@JobName@;%JobName%;g" -e "s;@out@;%out%;g" -e "s;@topdir@;%CD:\=/%;g" tests\bextract-test.bscr >tmp\bconcmds

CALL scripts\functions run_bacula
CALL scripts\functions check_for_zombie_jobs storage=File1
CALL scripts\functions stop_bacula

MKDIR %CD%\tmp\bacula-restores
bin\bextract -b tmp/restore.bsr -c bin/bacula-sd.conf %CD:\=/%/tmp %CD:\=/%/tmp/bacula-restores 2>&1 >nul
SET rstat=%ERRORLEVEL%
grep "^  Termination: *Backup OK" tmp\log1.out 2>&1 >nul
SET bstat=%ERRORLEVEL%
CALL scripts\functions check_restore_diff
CALL scripts\functions end_test
