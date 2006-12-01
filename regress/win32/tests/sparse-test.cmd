REM
REM Run a simple backup of the Bacula build directory using the Sparse option
REM   then restore it.
REM
SET TestName=sparse-test
SET JobName=SparseTest

CALL scripts\functions set_debug 0
CALL scripts\functions copy_test_confs

ECHO %CD:\=/%/build >\tmp\file-list

CALL scripts\functions start_test

sed -e "s;@JobName@;%JobName%;g" -e "s;@out@;%out%;g" -e "s;@topdir@;%CD:\=/%;g" tests\sparse-test.bscr >tmp\bconcmds

CALL scripts\functions run_bacula
CALL scripts\functions check_for_zombie_jobs storage=File 
CALL scripts\functions stop_bacula

CALL scripts\functions check_two_logs
CALL scripts\functions check_restore_diff
CALL scripts\functions end_test
