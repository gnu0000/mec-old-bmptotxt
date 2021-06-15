/* 
 * arg.h 
 * Craig Fitzgerald
 * This module helps parse command line parameters
 */

#define  NO_ERROR       0
#define  NO_EXTENSION   1
#define  UNKNOWN_CMD    2

typedef  struct 
   {
   char *            pszArgument;
   char *            pszParam;
   unsigned int   uiCount;
   } ARGBLK;

extern int ProcessParams (char *          argv [], 
                          ARGBLK       argrec [], 
                          unsigned int *uiError);

/* Returns # of args processed.  Compound args (I.E. -F 5000 ) count   *
 * as two. If an error occurs, the error code is stored in the high    *
 * byte of iError and the relative index of the offending arg is       *
 * stored in the low byte.  In case of multiple errors the last one    *
 * encountered is returned. In case of compound argument redifinitions *
 * the last definition is used.                                        */
