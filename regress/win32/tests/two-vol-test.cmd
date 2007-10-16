REM
REM Run a simple backup of the Bacula build directory but 
REM   split the archive into two volumes
REM
SET TestName=two-vol-test
SET JobName=TwoVol

CALL scripts\functions set_debug 0
CALL scripts\functions copy_test_confs

ECHO %CD:\=/%/build >tmp\file-list

CALL scripts\functions change_jobname NightlySave %JobName%
CALL scripts\functions start_test

sed -e "s;@JobName@;%JobName%;g" -e "s;@out@;%out%;g" -e "s;@topdir@;%CD:\=/%;g" tests\two-vol-test.bscr >tmp\bconcmds

CALL scripts\functions run_bacula
CALL scripts\functions check_for_zombie_jobs storage=File1
CALL scripts\functions stop_bacula

CALL scripts\functions check_two_logs
CALL scripts\functions check_restore_diff
CALL scripts\functions end_test
