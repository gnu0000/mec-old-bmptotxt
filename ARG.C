/*
 * arg.c
 * (c) 1990 Info Tech Inc.
 * Craig Fitzgerald
 * This module helps parse command line parameters
 */

#include <stdio.h>
#include <string.h>
#include "arg.h"

#define BOOL     int
#define TRUE     1 
#define FALSE    0 

/* Returns # of args processed.  Compound args (I.E. -F 5000 ) count   *
 * as two. If an error occurs, the error code is stored in the high    *
 * byte of iError and the relative index of the offending arg is       *
 * stored in the low byte.  In case of multiple errors the last one    *
 * encountered is returned. In case of compound argument redifinitions *
 * the last definition is used.                                        */

int ProcessParams (char *        argv[],
                   ARGBLK        argrec[],
                   unsigned int  *puiError)
   {
   unsigned int   i = 0;
   BOOL   bFound;
   char   *pszArgument;
   ARGBLK *parg;

   *puiError = NO_ERROR;
   while (argv[i] != NULL)
      {
      pszArgument = argv[i];
      if (*pszArgument != '-' && *pszArgument != '/' )
         return i;
      bFound = FALSE;

      for (pszArgument++, parg = argrec; parg->pszArgument != NULL; parg++)
         {
         if (strcmp (pszArgument, parg->pszArgument) != 0)
            continue;
         bFound = TRUE;
         parg->uiCount++;
         if (parg->pszParam != NULL)
            {
            if (argv[i+1] == NULL || *argv[i+1] == '-' || *argv[i+1] == '/' )
               *puiError = (NO_EXTENSION << 8) + i + 1;
            else
               parg->pszParam = argv[++i];
            }
         }
      i += 1;
      *puiError = bFound ? *puiError : (UNKNOWN_CMD << 8) + i;
      }
   return i;
   }
