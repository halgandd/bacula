/* */
#define VERSION "1.31"
#define VSTRING "1"
#define BDATE   "10 Jul 2003"
#define LSMDATE "10Jul03"

/* Debug flags */
#define DEBUG 1
#define TRACEBACK 1
#define SMCHECK     
#define TRACE_FILE 1  

#define FULL_DEBUG 1    /* normally on for testing only */

/* Turn this on ONLY if you want all Dmsg() to append to the
 *   trace file. Implemented mainly for Win32 ...
 */
/*  #define SEND_DMSG_TO_FILE 1 */

/* Turn this on if you want to try the new Job semaphore code */
#define USE_SEMAPHORE

/* #define NO_ATTRIBUTES_TEST 1 */
/* #define NO_TAPE_WRITE_TEST 1 */
/* #define FD_NO_SEND TEST 1 */
/* #define DEBUG_MUTEX 1 */
