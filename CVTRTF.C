/*
 * cvtrtf.c
 * (c) 1991 Info Tech Inc.
 * Craig Fitzgerald
 *
 *   This utility is used to convert Focus rpt style files
 * into RTF files suitable for importint into WinWord
 *
 */

#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "arg.h"

#define LINESIZE 8192
#define PARASIZE 8192
#define BOOL     int
#define TRUE     1 
#define FALSE    0 
#define COMMENT  '#'

#define DATE __DATE__
#define TIME __TIME__

typedef char   *PSZ;

typedef struct
   {
   PSZ   pszFocusName;
   int   uStyleIndex;
   BOOL  bHeader;
   } XLATE;

typedef struct
   {
   PSZ   pszName;
   PSZ   pszCode;
   } STYLE;




PSZ   pszStartup    = NULL;
PSZ   pszFont       = NULL;
PSZ   pszColorTable = NULL;
PSZ   pszStyleSheet = NULL;
PSZ   pszInfo       = NULL;

STYLE *pstyle  = NULL;
XLATE *pxlate  = NULL;

char  szSampleFile [LINESIZE];
char  szXlateFile  [LINESIZE];
char  szInFile     [LINESIZE];
char  szOutFile    [LINESIZE];

BOOL  bShowStyles;
BOOL  bDebug;

char  szBlock [0x4000];



/***********************************************************************/
/*                   general purpose fns                               */
/***********************************************************************/


void Error (PSZ psz1, PSZ psz2)
   {
   printf ("\nError: ");
   printf (psz1, psz2);
   printf ("\n");
   exit (1);
   }

void Warning (PSZ psz1, PSZ psz2)
   {
   printf ("\nWarning: ");
   printf (psz1, psz2);
   printf ("\n");
   }


/***********************************************************************/
/*                   general purpose string fns                        */
/***********************************************************************/


/*
 * Removes trailing chars from str
 */
PSZ Clip (PSZ pszStr, PSZ pszClipList)
   {
   int i;

   i = strlen (pszStr);
   while (i >= 0 && strchr (pszClipList, pszStr[i]) != NULL)
      pszStr[i--] = '\0';
   return pszStr;
   }


/*
 * Removes leading chars from str
 */
PSZ Strip (PSZ pszStr, PSZ pszStripList)
   {
   int uLen, i = 0;

   if (!(uLen = strlen (pszStr)))
      return pszStr;
   while (i < uLen && strchr (pszStripList, pszStr[i]) != NULL)
      i++;
   if (i)
      memmove (pszStr, pszStr + i, uLen - i + 1);
   return pszStr;
   }


/*
 * Extracts removes 1st word from str, returns both pieces. 
 */
int GetWord (PSZ *ppsz, PSZ pszWord, PSZ pszDelim, BOOL bEatDelim)
   {
   *pszWord = '\0';
   Strip (*ppsz, " \t");

   if (*ppsz == NULL || **ppsz == '\0')
      return 0xFFFF;

   while (**ppsz != '\0' && strchr (pszDelim, **ppsz) == NULL)
      *(pszWord++) = *((*ppsz)++);
   *pszWord = '\0';

   if (bEatDelim)
      return (int) (**ppsz ? *((*ppsz)++): 0);
   else
      return (int) (**ppsz ? *(*ppsz): 0);
   }


/*
 * translates {{TAB}} and \t to \tab
 * condenses spaces
 * removes spaces around the {{TAB}}
 */
void FilterParagraph (PSZ pszDest, PSZ pszSrc)
   {
   char  szWord [LINESIZE];
   int   cdelim;
   PSZ   pszTmp = pszDest;
   BOOL  bOldSpace = TRUE;
   BOOL  bNewSpace;

   while (TRUE)
      {
      if (*pszSrc == '\0')
         break;

      bNewSpace = (*pszSrc == ' ');

      if (*pszSrc == '{' && !strnicmp (pszSrc, "{{TAB}}", 7))
         {
         /*---remove previous space ---*/
         if (pszDest > pszTmp && *(pszDest-1) == ' ')
            pszDest--;
         strcpy (pszDest, "\\tab ");
         pszDest+=5;
         pszSrc+=7;
         bOldSpace = TRUE;
         continue;
         }

      if (!bOldSpace || !bNewSpace)
         *pszDest++ = *pszSrc++;
      else
         pszSrc++;

      bOldSpace = bNewSpace;
      }
   *pszDest = '\0';
   }



/***********************************************************************/
/*                   general purpose file  fns                         */
/***********************************************************************/

int SkipBlanks (FILE *fpIn)
   {
   int   c;

   while (strchr(" \t\n", c = getc (fpIn)) != NULL)
      ;
   ungetc (c, fpIn);
   return c;
   }


int SkipToEOL (FILE *fpIn)
   {
   int   c;

   while ((c = getc (fpIn)) != (int)'\n' && c != EOF)
      ;
   return c;
   }



/*
 * skips blank and comment lines 
 * returns FALSE at eof
 */
BOOL ReadLine (FILE *fpIn, PSZ pszLine)
   {
   int   c;

   do
      {
      c = SkipBlanks (fpIn);
      if (c == EOF)
         return FALSE;
      if (c == COMMENT)
         SkipToEOL (fpIn);
      } while (c == (int)COMMENT);

   while ((c = getc (fpIn)) != EOF && c != (int)'\n')
      *pszLine++ = (char)c;
   *pszLine = '\0';
   return TRUE;
   }




/***********************************************************************/
/*                    general purpose rtf fns                          */
/***********************************************************************/



/*
 * reads '{' then stuff, up to next '{'
 */
void ReadStart (FILE *fpIn, PSZ pszLine)
   {
   int   c, i;

   SkipBlanks (fpIn);
   for (i = 0; i < 5; i++)
      pszLine[i] = (char) getc (fpIn);
   if (strnicmp (pszLine, "{\\rtf", 5))
      Error ("Sample File not in RTF format", NULL);
   pszLine += 5;

   while ((c = getc (fpIn)) != EOF && c != (int)'}' && c != (int)'{')
      *pszLine++ = (char)c;
   *pszLine = '\0';
   ungetc (c, fpIn);
   }


/*
 * Extracts name of block
 */
void BlockName (PSZ pszBlockName, PSZ pszBlock)
   {
   *pszBlockName = '\0';
   if (pszBlock == NULL || pszBlock[0] == '\0')
      return;
   if (pszBlock[0] != '{' || pszBlock[1] != '\\')
      return;

   pszBlock+=2;
   while (*pszBlock == ' ')
      pszBlock++;

   while (strchr (" \t\\}{;\n", *pszBlock) == NULL)
      *pszBlockName++ = *pszBlock++;
   *pszBlockName = '\0';
   }



/*
 * reads a {--} block and returns first id in block
 * excluding the leading backslash 
 * blocks may contain blocks
 */
BOOL ReadBlock (FILE *fpIn, PSZ pszBlock, PSZ pszName)
   {
   PSZ pszText;
   int   c;

   pszText  = pszBlock;
   *pszText = *pszName = '\0';

   while ((c = getc (fpIn)) != EOF && c != (int)'{' && c != (int)'}')
      ;
   if ((*pszText++ = (char) c) != '{')
      return FALSE;

   while ((c = getc (fpIn)) != EOF && c != (int)'}')
      {
      if (c != (int)'{')
         *pszText++ = (char)c;
      else
         {
         ungetc (c, fpIn);
         ReadBlock (fpIn, pszText, pszName);
         pszText = strchr (pszText, '\0');
         }
      }
   if (c == EOF)
      Error ("Unexpected EOF reading block", NULL);
   *pszText++ = (char)c;
   *pszText   = '\0';
   BlockName (pszName, pszBlock);
   return TRUE;
   }







PSZ GetFont (PSZ pszSS, STYLE *pstyle)
   {
   BOOL  bSpace = FALSE;
   BOOL  bNewSpace;
   char  szCodes[LINESIZE];
   char  szName [LINESIZE];
   PSZ   pszTxt, pszPtr;

   pszTxt = szCodes;
   /*--- skip '{' ---*/
   pszSS++;

   /*--- Get Codes ---*/
   while (TRUE)
      {
      bNewSpace = (*pszSS == ' ' || *pszSS == '\n');

      if (bSpace && !bNewSpace && *pszSS != '\\')
         break;
      bSpace = bNewSpace;
      *pszTxt++ = *pszSS++;
      }
   *--pszTxt = '\0';

   /*--- strip unwanted parts of style codes ---*/
   while ((pszPtr = strrchr (szCodes, '\\')) != NULL)
      {
      if (strnicmp (pszPtr, "\\sbasedon", 9) &&
            strnicmp (pszPtr, "\\snext", 6))
         break;
      *pszPtr = '\0';
      }
   Clip (szCodes, " \n");
   pstyle->pszCode = strdup (szCodes);

   /*--- get name ---*/
   pszTxt = szName;
   while (*pszSS != ';')
      *pszTxt++ = *pszSS++;
   *pszTxt = '\0';
   pstyle->pszName = strdup (szName);

   /*--- skip ";}" ---*/
   return pszSS += 2;
   }



/*
 * this fn extracts the style information from 
 * the style info block gotten from the sample rtf file
 */
void GetStyleInfo (PSZ pszSS)
   {
   int    i = 0;

   pstyle = (STYLE *) malloc (sizeof (STYLE));
   /*--- skip opening '{' ---*/
   pszSS++;
   while (TRUE)
      {
      while (*pszSS != '{' && *pszSS != '}' && *pszSS != '\0')
         pszSS++;
      /*- at this point we're at { of font or } of StyleSheet ---*/
      pstyle = (STYLE *) realloc (pstyle, sizeof (STYLE) * (++i + 1));
      pstyle[i].pszCode = pstyle[i].pszName = NULL;
      if (*pszSS == '}' || *pszSS == '\0')
         return;
      pszSS = GetFont (pszSS, pstyle + i);
      }
   }







/***************************************************************/
/*                  read/write input  fns                      */
/***************************************************************/



void ReadRTF ()
   {
   FILE  *fpIn;
   char  szBlockType [LINESIZE];

   if ((fpIn = fopen (szSampleFile, "r")) == NULL)
      Error ("Unable to open RTF sample File", NULL);

   printf ("Reading sample rtf file %s ", szSampleFile);

   ReadStart (fpIn, szBlock);
   pszStartup = strdup (szBlock);

   while (ReadBlock (fpIn, szBlock, szBlockType))
      {
      printf (".");
      if (!stricmp (szBlockType, "fonttbl"))
         pszFont = strdup (szBlock);
      else if (!stricmp (szBlockType, "colortbl"))
         pszColorTable = strdup (szBlock);
      else if (!stricmp (szBlockType, "stylesheet"))
         pszStyleSheet = strdup (szBlock);
      else if (!stricmp (szBlockType, "info"))
         pszInfo = strdup (szBlock);
      else
         {
         if (bDebug)
            Warning ("Processed unknown RTF Block in sample file : %s", szBlockType);
         }
      }
   printf ("\n");
   if (pszFont == NULL)
      Warning ("Unable to find font information in RTF sample file", NULL);
   if (pszColorTable == NULL)
      Warning ("Unable to find color table information in RTF sample file", NULL);
   if (pszStyleSheet == NULL)
      Error ("Unable to find style sheet information in RTF sample file", NULL);
   if (pszInfo == NULL)
      Warning ("Unable to find information block in RTF sample file", NULL);

   fclose (fpIn);
   }







/*
 * determines weather the specified style name
 * has been loaded from the rtf sample file
 *
 * the index of the matching style is returned
 * or -1 if not found
 */
int StyleIndex (PSZ pszStyleName)
   {
   int   i = 1;

   while (pstyle[i].pszName != NULL)
      {
      if (!stricmp (pstyle[i].pszName, pszStyleName))
         return i;
      i++;
      }
   return -1;
   }







/*
 * call GetStyleInfo before this proc
 *
 */
void ReadXlate ()
   {
   FILE   *fpIn;
   int    i = 0;
   char   szLine  [LINESIZE];
   char   szFocus [LINESIZE];
   PSZ    pszStr;

   pxlate = malloc (sizeof (XLATE));
   pxlate[0].pszFocusName = NULL;

   /*-- There is none ---*/
   if (*szXlateFile == '\0')
      return;

   if ((fpIn = fopen (szXlateFile, "r")) == NULL)
      Error ("Unable to open Style Translation File", NULL);

   printf ("Reading translation file %s ", szXlateFile);

   while (TRUE)
      {
      pxlate = realloc (pxlate, sizeof (XLATE) * (i + 1));
      pxlate[i].pszFocusName = NULL;

      if (!ReadLine (fpIn, szLine))
         break;

      printf (".");
      pszStr = szLine;
      GetWord (&pszStr, szFocus, " \t=", TRUE);

      Clip  (szFocus, " \t=");
      Strip (szFocus, " \t=");
      pxlate[i].pszFocusName = strdup (szFocus);

      Clip  (pszStr, " \t=");
      Strip (pszStr, " \t=");

      if (*pszStr == '\0')
         Error ("No word style found for input style %s.", szFocus);

      /*--- see if this focus style is a header type ---*/
      if (pxlate[i].bHeader = (*pszStr == '*'))
         pszStr++;

      /*--- see if this style should be stripped ---*/
      if (!strnicmp (pszStr, "NULL", 4))
         pxlate[i].uStyleIndex = 0;

      else if ((pxlate[i].uStyleIndex = StyleIndex (pszStr)) == -1)
         Error ("Word Style \"%s\" not found in rtf sample file", pszStr);
      i++;
      }
   printf ("\n");
   if (bDebug)
      printf ("%d Xlates read\n", i);
   fclose (fpIn);
   }


BOOL IsHeader  (PSZ pszFocusName)
   {
   int   i = 0;

   while (pxlate[i].pszFocusName != NULL)
      {
      if (!stricmp (pxlate[i].pszFocusName, pszFocusName))
         return pxlate[i].bHeader;
      i++;
      }
   return FALSE;
   }



/*
 * given the focus name for a style
 * this function returns the word style codes
 * associated with that style
 * or null if undefined
 *
 * if no translation between focus style names and word style names
 * was specified, an attempt is made to find the word style codes
 * with the same name as the focus style name
 */
PSZ MatchingCodes (PSZ pszFocusName, BOOL *bHeader)
   {
   int   i = 0;
   int   j;

   *bHeader = FALSE;
   /*--- see if there is a xlation ---*/
   while (pxlate[i].pszFocusName != NULL)
      {
      if (!stricmp (pxlate[i].pszFocusName, pszFocusName))
         {
         if (pxlate[i].uStyleIndex == 0) /*--- this means strip ---*/
            return NULL;
         *bHeader = pxlate[i].bHeader;
         return pstyle[pxlate[i].uStyleIndex].pszCode;
         }
      i++;
      }

   /*--- see if there is a style with the same name ---*/
   if ((j = StyleIndex (pszFocusName)) != -1)
      {
      /*--- add it to xlate table to make next lookup faster ---*/
      pxlate = realloc (pxlate, sizeof (XLATE) * (i + 2));
      pxlate[i+1].pszFocusName = NULL;
      pxlate[i].pszFocusName = strdup (pszFocusName);
      pxlate[i].uStyleIndex = j;
      pxlate[i].bHeader = 0;
      return pstyle[j].pszCode;
      }

   Error ("Focus Style not matched or translated: %s", pszFocusName);
   }




void DumpRTFHeader (FILE *fpOut)
   {
   fprintf (fpOut, pszStartup);
   fprintf (fpOut, pszFont);
   fprintf (fpOut, pszColorTable);
   fprintf (fpOut, pszStyleSheet);
   fprintf (fpOut, pszInfo);
   }


void DumpRTFTail (FILE *fpOut)
   {
   fprintf (fpOut, "}");
   }


void DumpParagraph (FILE *fpOut, PSZ pszFocusStyle, PSZ pszData)
   {
   PSZ   pszCodes;
   char  szParagraph [LINESIZE];
   BOOL  bHeader;
   char  szSpaces [LINESIZE];   // mdh
   unsigned int usSpaces;       // mdh

   if ((pszCodes = MatchingCodes (pszFocusStyle, &bHeader)) == NULL)
      return;   /*--- this code is stripped ---*/

   for (usSpaces = 0; pszData [usSpaces] == ' ';usSpaces++) // mdh
      szSpaces [usSpaces] = ' ';                            // mdh
   szSpaces [usSpaces] = '\0';                              // mdh

   FilterParagraph (szParagraph, pszData);

   if (bHeader)                                                                                      // mdh
      fprintf (fpOut, "{\\header \\pard\\plain %s %s%s\n\\par}", pszCodes, szSpaces, szParagraph);  // mdh
   else                                                                                              // mdh
      fprintf (fpOut, "\\pard\\plain %s %s%s\n\\par", pszCodes, szSpaces, szParagraph);             // mdh
//   if (bHeader)
//      fprintf (fpOut, "{\\header \\pard\\plain %s %s\n\\par}", pszCodes, szParagraph);
//   else
//      fprintf (fpOut, "\\pard\\plain %s %s\n\\par", pszCodes, szParagraph);
   }


void WriteRTFFile ()
   {
   FILE  *fpIn, *fpOut;
   long  i = 0;
   PSZ   pszLine;
   char  szLine [PARASIZE];
   char  szFocusStyle [LINESIZE];

   if ((fpIn = fopen (szInFile, "r")) == NULL)
      Error ("Unable to open Focus Data File %s", szInFile);

   if ((fpOut = fopen (szOutFile, "w")) == NULL)
      Error ("Unable to open Output File %s, szOutFile",NULL);

   printf ("Writing %s, Paragraph %6.6ld", szOutFile, 0L);

   DumpRTFHeader (fpOut);

   while (ReadLine (fpIn, szLine))
      {
      pszLine = szLine;
      GetWord (&pszLine, szFocusStyle, " \t", TRUE);
      DumpParagraph (fpOut, szFocusStyle, szLine+20);   // mdh 
//      DumpParagraph (fpOut, szFocusStyle, pszLine);
      if (!(++i % 25L))
         printf ("\b\b\b\b\b\b%6.6ld", i);
      }
   printf ("\b\b\b\b\b\b%6.6ld\n", i);
   DumpRTFTail (fpOut);
   }


/***************************************************************/
/*                        main                                 */
/***************************************************************/



int ShowStyles ()
   {
   int i = 0;

   printf ("Styles Defined in sample RTF file %s\n", szSampleFile);
   printf ("==============================================\n");
   while (pstyle[++i].pszName != NULL)
      {
      printf ("%2.2d> %s", i, pstyle[i].pszName);
      if (bDebug)
         printf ("\t\t %s", pstyle[i].pszCode);
      printf ("\n");
      }
   return 0;
   }




/*
 * for debug
 */
void Dump ()
   {
   int i = 0;

   printf ("PSZSTARTUP =%s\n\n", pszStartup   );
   printf ("PSZFONT =%s\n\n", pszFont      );
   printf ("PSZCOLORTABLE =%s\n\n", pszColorTable);
   printf ("PSZSTYLESHEET =%s\n\n", pszStyleSheet);
   printf ("PSZINFO =%s\n\n", pszInfo      );

   while (pxlate[i].pszFocusName != NULL)
      {
      printf ("%d> pxlate.pszFocusName=%s\t\tppxlate.uStyleIndex=%d\n",
              i, pxlate[i].pszFocusName, pxlate[i].uStyleIndex);
      i++;
      }
   }









void Usage ()
   {
   printf ("\n CVTRTF    DOS-OS/2  RTF file conversion utility   v1.0 %s %s\n\n", DATE, TIME);
   printf (" USAGE: CVTRTF [options] <inputfile>\n\n");
   printf (" WHERE: [options] are 0 or more of:\n");
   printf ("          -sample <samplefile> . The sample Word RTF file containing any \n");
   printf ("                                  styles needed in the file conversion.\n");
   printf ("                                  Defaults to SAMPLE.RTF\n");
   printf ("          -xlate <xlatefile> ... The file describing the translation between\n");
   printf ("                                  focus style names and word style names.\n");
   printf ("                                  Defaults to no translation file.\n");
   printf ("          -outfile <outfile> ... The output filename. Defaults to the input\n");
   printf ("                                  file name with a .RTF extention\n");
   printf ("          -showstyles .......... Show styles in sample file only.\n");
   printf ("          -help ................ This help.\n");
   printf ("          -helpxlate ........... Help on <xlatefile> format.\n");
   printf ("          -debug ............... Debug info.\n\n");
   printf (" EXAMPLE: CVTRTF -xlate xlate.txt SAEPX.RPT\n");
   printf ("            In this example CVTREF will use Word styles defined in\n");
   printf ("            SAMPLE.RTF, load style translations from XLATE.TXT, and\n");
   printf ("            will convert the Focus file SAEPX.RPT to the word file\n");
   printf ("            SAEPX.RTF.");
   exit (0);
   }



void XlateHelp ()
   {
   printf ("\n CVTRTF Translation File Format\n\n", DATE, TIME);
   printf ("DESCRIPTION:\n");
   printf ("   This file is optional.\n");
   printf ("   This file describes the mapping of style names between the input file\n");
   printf ("   and the output file. The file is a straight text file with 2 columns per\n");
   printf ("   line. The column on the left is the style that the input file uses. The\n");
   printf ("   column on the right is the Word style that will replace the style on the\n");
   printf ("   left. If the style names are the same they need not be specified. If the\n");
   printf ("   Word Style is \"NULL\" the paragraph is stripped from the output file.\n");
   printf ("   If the paragraph style is meant to be a header style, preceed the style\n");
   printf ("   name with an asterisk '*'. This will cause the associated text to be used\n");
   printf ("   as a header. Any Word style used (other than NULL) must be defined in\n");
   printf ("   the samplefile or an error will occur. Lines beginning with a '#' are\n");
   printf ("   considered comment lines.\n\n");
   printf ("EXAMPLE:\n\n");
   printf ("   # Sample.rtf   --- last modified 08/08/91 ---\n");
   printf ("   #\n");
   printf ("   # This is an example Style Translation File for use with\n");
   printf ("   # the CVTRTF utility.\n");
   printf ("   #\n");
   printf ("   # The Word styles used here go with the sample rtf file\n");
   printf ("   # SAMPLE.RTF which i keep on my Z: drive\n");
   printf ("   #\n");
   printf ("   #FOCUS STYLES               Word Styles \n");
   printf ("   #===========================================================\n");
   printf ("   SAEPXHDR                    Header 3\n");
   printf ("   SAEPXDET                    Normal\n");
   printf ("   #  \n");
   printf ("   # strip SLEEZBAG and NOTE paragraphs from the file\n");
   printf ("   SLEEZBAG                    NULL\n");
   printf ("   NOTE                        NULL\n");
   printf ("   #  \n");
   printf ("   # these are header styles\n");
   printf ("   DDBYNMHDR     *heading 3\n");
   printf ("   DDBYIDHDR     *heading 3\n");
   printf ("   IONIXHDR      *heading 3\n");
   printf ("   IOINXHDR      *IOINXHDR\n");
   printf ("   #  \n");
   printf ("   #the rest of the Focus styles map directly to the Word Styles\n");
   printf ("   <EOF>\n");
   exit (0);
}









ARGBLK args[] = {{ "sample",     "sample.rtf", 0},
                 { "xlate",      "",           0},
                 { "showstyles", NULL,         0},
                 { "outfile",    "",           0},
                 { "help",       NULL,         0},
                 { "h",          NULL,         0},
                 { "?",          NULL,         0},
                 { "debug",      NULL,         0},
                 { "helpxlate",  NULL,         0}, 
                 { NULL,         NULL,         0}};

/*--- These  #defines must match the ordering of the above structure ---*/
#define SAMPLE   0
#define XLATE    1
#define SHOW     2
#define OUTFILE  3
#define HELP     4
#define HELPH    5
#define HELPQ    6
#define DEBUG    7
#define HELPX    8




void ParseArgs (int argc, char *argv[])
   {
   int uFileIndex = 0;
   int uReturn    = 0;
   int uError;
   PSZ pszTmp;

   if (argc == 1)
      Usage ();
   do
      {
      uReturn += 1 + (int) ProcessParams (argv+uReturn+1, args, &uError);
      if (uError || uFileIndex && argv[uReturn] != NULL)
         {
         printf ("Error processing param \"%s\"\nType 'cvtrtf -help' for command line options\n",
                  argv[(uError?uError%0x0100:uReturn)]);
         exit (uError >> 8);
         }
      if (!uFileIndex)
         uFileIndex = uReturn;
      }
   while (uReturn + 1 < argc);

   if (args[HELPX].uiCount)
      XlateHelp ();
   if (args[HELP].uiCount || args[HELPH].uiCount || args[HELPQ].uiCount)
      Usage ();

   strcpy(szSampleFile, args[SAMPLE].pszParam);
   strcpy(szXlateFile, args[XLATE].pszParam);
   bShowStyles  = args[SHOW].uiCount;
   bDebug       = args[DEBUG].uiCount;

   if (bShowStyles)
      return;

   if (argv[uFileIndex] == NULL)
      Error ("Input file must be specified. type CVTRTF -help for help", NULL);
   if (strchr (strcpy (szInFile, argv[uFileIndex]), '.') == NULL)
      strcat (szInFile, ".rpt");

   if (args[OUTFILE].uiCount)
      {
      if (strchr (strcpy (szOutFile, args[OUTFILE].pszParam), '.') == NULL)
         strcat (szOutFile, ".rtf");
      }
   else /*--- outfile not explicitly specified ---*/
      {
      pszTmp = strchr (strcpy (szOutFile, szInFile), '.');
      *pszTmp = '\0';
      strcat (szOutFile, ".rtf");
      }

   if (*argv[uFileIndex] == '?')
      Usage ();

   if (!stricmp (szInFile, szOutFile))
      Error ("Input and output filenames must be different", NULL);
   }








main (int argc, char *argv[])
   {
   ParseArgs (argc, argv);

   ReadRTF ();
   GetStyleInfo (pszStyleSheet);

   if (bShowStyles)
      exit (ShowStyles ());
   if (bDebug)
      ShowStyles ();

   ReadXlate ();
   if (bDebug)
      Dump ();

   WriteRTFFile ();
   printf ("Done.\n");
   return 0;
   }
