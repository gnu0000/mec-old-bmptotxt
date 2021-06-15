#include <os2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "arg.h"


#define WIN   1
#define OS2   2

#define DATE __DATE__
#define TIME __TIME__


typedef unsigned long  DWORD;
typedef unsigned short WORD;

/*                                   */
/* windows bitmap file format        */
/* --------------------------------- */
/* BITMAPFILEHEADER                  */
/* BITMAPINFO     - BITMAPINFOHEADER */
/*                - RGBQUAD[1]       */
/*                                   */
/*                                   */
/* os/2 bitmap file format           */
/* --------------------------------- */
/* BITMAPFILEHEADER                  */
/* BITMAPCOREINFO - BITMAPCOREHEADER */
/*                - GRBTRIPLE[1]     */
/*                                   */
/*                                   */
/*--- STRUCTURES FROM WINDOWS.H ---*/

typedef struct tagRGBTRIPLE
   {
	BYTE	rgbtBlue;                  // b
	BYTE	rgbtGreen;                 // g
	BYTE	rgbtRed;                   // r
   } RGBTRIPLE;

typedef struct tagBITMAPCOREHEADER
   {
	DWORD	bcSize; /*gets color tbl*/ // size of this structure
	WORD	bcWidth;                   // Xsize
	WORD	bcHeight;                  // Ysize
	WORD	bcPlanes;                  // 1
	WORD	bcBitCount;                // 1, 4, 8, 24
   } BITMAPCOREHEADER;

typedef struct tagBITMAPCOREINFO
   { 
   BITMAPCOREHEADER bmciHeader;     //
   RGBTRIPLE		  bmciColors[1];  // 2, 16, 256, NULL
   } BITMAPCOREINFO;                  


typedef struct tagRGBQUAD
   {
	BYTE	rgbBlue;                   // b
	BYTE	rgbGreen;                  // g
	BYTE	rgbRed;                    // r
	BYTE	rgbReserved;               // 0
   } RGBQUAD;

typedef struct tagBITMAPINFOHEADER
   {
  	DWORD	biSize;                    // #bytes in this structure
  	DWORD	biWidth;                   // Xsize
  	DWORD	biHeight;                  // Ysize
  	WORD	biPlanes;                  // 1
  	WORD	biBitCount;                // 1,4,8,or 24 bits per pixel
                                    //
	DWORD	biCompression;             // RGB=0 RLE8=1 RLE4=2
	DWORD	biSizeImage;               // size if image
	DWORD	biXPelsPerMeter;           // 0
	DWORD	biYPelsPerMeter;           // 0
	DWORD	biClrUsed;                 // size of colortable 0=max
	DWORD	biClrImportant;            // 0
   } BITMAPINFOHEADER;

typedef struct tagBITMAPINFO
   { 
   BITMAPINFOHEADER bmiHeader;      //
   RGBQUAD     	  bmiColors[1];   // 2, <=16, <=256, NULL
   } BITMAPINFO;

typedef struct tagBITMAPFILEHEADER
   {
	WORD	bfType;                    // must be 'BM'
	DWORD	bfSize;                    // size of file (round up to dword?)
   WORD  bfReserved1;               // 0
   WORD  bfReserved2;               // 0
	DWORD	bfOffBits;                 // offset from this struct to bitmap
   } BITMAPFILEHEADER;


/*---new structure---*/
typedef struct
   {
   PSZ                pszFileName;
   PSZ                pszOutFile;
   USHORT             uType;
   USHORT             uXSize;
   USHORT             uYSize;
   USHORT             uBPP;
   USHORT             uColors;
   BYTE               pGrayMap [257];  /*- [256] has count         -*/
   FILE               *fpIn;
   FILE               *fpOut;
   int                iXOffset;
   BITMAPFILEHEADER   bfh;          // common file header
   BITMAPINFOHEADER   whdr;         // header for win bmp
   BITMAPCOREHEADER   ohdr;         // header for os2 bmp
   } BMPINFO;
typedef BMPINFO *PBMPINFO;


BYTE  pCharMap [70];   /*- 64 chars light to heavy -*/
BYTE pszLine [1024 * 3];
char Bin0 = ' ';
char Bin1 = '0';



ARGBLK args[] = {{ "info",        "",    0},
                 { "showcolors",  NULL,  0}, 
                 { "newchars",    "",    0}, 
                 { "binary",      NULL,  0}, 
                 { "halfx",       NULL,  0}, 
                 { "halfy",       NULL,  0},
                 { "new0",        "",    0},
                 { "new1",        "",    0},
                 { "help",        NULL,  0},
                 { "h",           NULL,  0},
                 { "?",           NULL,  0},
                 { NULL,          NULL,  0}};


/*--- These  #defines must match the ordering of the above structure ---*/
#define INFO        0   //
#define SHOWCOLORS  1   //
#define NEWCHARS    2   //partial
#define BINARY      3
#define HALFX       4
#define HALFY       5
#define NEW0        6   //
#define NEW1        7   //
#define HELP1       8   //
#define HELP2       9   //
#define HELP3       10  //




/************************************************************************/
void PrintFileHeader (PBMPINFO pbmp)
   {
   printf ("==========BITMAPFILEHEADER==========\n");
   printf ("bfType      = %c%c\n", (BYTE) (pbmp->bfh.bfType & 0xFF), (BYTE)(pbmp->bfh.bfType >> 8));
   printf ("bfSize      = %ld\n", pbmp->bfh.bfSize     );
   printf ("bfReserved1 = %d\n",  pbmp->bfh.bfReserved1);
   printf ("bfReserved2 = %d\n",  pbmp->bfh.bfReserved2);
   printf ("bfOffBits   = %ld\n", pbmp->bfh.bfOffBits  );
   printf ("\n");
   }


void PrintBMPHeader (PBMPINFO pbmp)
   {
   if (pbmp->uType == WIN)
      {
      printf ("======BITMAPINFOHEADER (WIN)=======\n");
      printf ("biSize          = %ld\n", pbmp->whdr.biSize         );
      printf ("biWidth         = %ld\n", pbmp->whdr.biWidth        );
      printf ("biHeight        = %ld\n", pbmp->whdr.biHeight       );
      printf ("biPlanes        = %d\n",  pbmp->whdr.biPlanes       );
      printf ("biBitCount      = %d\n",  pbmp->whdr.biBitCount     );
      printf ("biCompression   = %ld\n", pbmp->whdr.biCompression  );
      printf ("biSizeImage     = %ld\n", pbmp->whdr.biSizeImage    );
      printf ("biXPelsPerMeter = %ld\n", pbmp->whdr.biXPelsPerMeter);
      printf ("biYPelsPerMeter = %ld\n", pbmp->whdr.biYPelsPerMeter);
      printf ("biClrUsed       = %ld\n", pbmp->whdr.biClrUsed      );
      printf ("biClrImportant  = %ld\n", pbmp->whdr.biClrImportant );
      printf ("\n");
      }
   else
      {
      printf ("=====BITMAPCOREHEADER (OS/2)=====\n");
      printf ("bcSize      = %ld\n", pbmp->ohdr.bcSize    );
      printf ("bcWidth     = %d\n",  pbmp->ohdr.bcWidth   );
      printf ("bcHeight    = %d\n",  pbmp->ohdr.bcHeight  );
      printf ("bcPlanes    = %d\n",  pbmp->ohdr.bcPlanes  );
      printf ("bcBitCount  = %d\n",  pbmp->ohdr.bcBitCount);
      printf ("\n");
      }
   }



void PrintColors (PBMPINFO pbmp, RGBQUAD *wclr, RGBTRIPLE *oclr)
   {
   USHORT i;

   if (pbmp->uType == WIN)
      for (i=0; i < pbmp->uColors; i++)
         printf ("%3.3d> Blue=%3.3d  Green=%3.3d  Red=%3.3d  Gray=%3.3d  Char=%c\n", i,
         (int)wclr[i].rgbBlue, (int)wclr[i].rgbGreen,
         (int)wclr[i].rgbRed,  (int)pbmp->pGrayMap[i],
         CharFromIdx (i));
   else
      for (i=0; i < pbmp->uColors; i++)
         printf ("%3.3d> Blue=%3.3d  Green=%3.3d  Red=%3.3d  Gray=%3.3d  Char=%c\n", i,
         (int)oclr[i].rgbtBlue, (int)oclr[i].rgbtGreen,
         (int)oclr[i].rgbtRed,  (int)pbmp->pGrayMap[i],
         CharFromIdx (i));
   }


void Usage (void)
   {
   printf ("BMPtoTXT   Windows Bitmap to Text File Converter   v1.0 %s %s\n\n", DATE, TIME);
   printf ("\n");
   printf ("USAGE: BMPtoTXT  <params> InFile[.BMP] [OutFile[.TXT]]\n");
   printf ("\n");
   printf ("WHERE: <params> are 0 or more of:\n");
   printf ("          -info .................. Show info about BMP           \n");
   printf ("          -showcolors ............ Show BMP colorTable           \n");
   printf ("          -newchars charstring ... Set New Xlate chars (pal order)\n");
   printf ("          -binary ................ Produce Mono Output (2 chars) \n");
   printf ("          -halfx ................. 1/2 X Resolution              \n");
   printf ("          -halfy ................. 1/2 Y Resolution              \n");
   printf ("          -new0     char.......... Set New Binary 0 char         \n");
   printf ("          -new1     char.......... Set New Binary 1 char         \n");
   exit (0);
   }


void PrintBMPInfo (PBMPINFO pbmp)
   {
   printf ("%s  [%d x %d]  %s Format,  %d Bit%sper Pixel  %d Colors\n",
            pbmp->pszFileName,
            pbmp->uXSize,
            pbmp->uYSize,
            (pbmp->uType == WIN ? "WINDOWS" : "OS/2"),
            pbmp->uBPP,
            (pbmp->uBPP < 2 ? " " : "s "),
            (pbmp->uColors ? pbmp->uColors : 32767));
   printf ("Writing Output to %s\n", pbmp->pszOutFile);
   }






void Error (PSZ psz1, PSZ psz2)
   {
   printf ("Error: %s %s\n", psz1, psz2);
   exit (1);
   }



BYTE Intensity (BYTE b, BYTE g, BYTE r)
   {
   return (BYTE) ((b * 29 + g * 150 + r * 77) >> 8); 
   }



USHORT BuildGrayMap (PVOID pColorMap, PSZ pGrayMap,
                     USHORT uCount, USHORT uType)
   {
   RGBTRIPLE *rgb3;
   RGBQUAD   *rgb4;
   USHORT    i;

   rgb3 = (RGBTRIPLE *)pColorMap;
   rgb4 = (RGBQUAD   *)pColorMap;
   for (i=0; i < uCount; i++, rgb4++, rgb3++)
      {
      if (uType == WIN)
         pGrayMap[i] = Intensity (rgb4->rgbBlue,
                                  rgb4->rgbGreen,
                                  rgb4->rgbRed);
      else /* uType == OS2 */
         pGrayMap[i] = Intensity (rgb3->rgbtBlue,
                                  rgb3->rgbtGreen,
                                  rgb3->rgbtRed);
      }
   return 0;
   }



/* Weights with     */
/* 25 line vga font */
/* -----------------*/
/* 0       29 a     */
/* 4  .    31 e     */
/* 7  -    32 3     */
/* 8  ,    33 F     */
/* 10 :    34 2     */
/* 12 ^    37 G     */
/* 14 +    38 $     */
/* 15 =    40 &     */
/* 16 /    43 H     */
/* 18 |    46 #     */
/* 19 <    48 @     */
/* 20 (    49 0     */
/* 21 %    50 B     */
/* 23 {    52 W     */
/* 25 ?    54 M     */
/* 26 [             */

void CreateCharMap ()
   {
   PSZ szTmp;

// STRAIGHT UP
// szTmp = "   ...-,,::^^+=//|<(%%{{?[[[aae3F222G$$&&&HHH##@0BBWMMMMMMMMMMMM";
//          0000000001111111111222222222233333333334444444444555555555566666
//          1234567890123456789012345678901234567890123456789012345678901234
   szTmp = "     ...--,,::^^++==//||<(%%{{?[[[aae3F222G$$&&&HHH##@@00BBWMMMM";
   strcpy (pCharMap, szTmp);

   if (args[NEWCHARS].uiCount)
      strcpy (pCharMap, args[NEWCHARS].pszParam);
   args[NEWCHARS].uiCount = strlen (args[NEWCHARS].pszParam);
   }





void ReadColorInfo (PBMPINFO pbmp)
   {
   RGBQUAD    *wclr = NULL; // colors for win bmp
   RGBTRIPLE  *oclr = NULL; // colors for os2 bmp


   if (pbmp->uType == WIN)
      {
      pbmp->uColors= (USHORT)pbmp->whdr.biClrUsed;
      switch (pbmp->whdr.biBitCount)
         {
         case 1:  pbmp->uColors  = 2;                     break;
         case 4:  pbmp->uColors += !pbmp->uColors * 16;   break;
         case 8:  pbmp->uColors += !pbmp->uColors * 256;  break;
         case 24: pbmp->uColors  = 0;                     break;
         }
      if (pbmp->uColors)
         {
         wclr = (RGBQUAD *) malloc (pbmp->uColors * sizeof (RGBQUAD));
         fread (wclr, sizeof (RGBQUAD), pbmp->uColors, pbmp->fpIn);
         BuildGrayMap (wclr, pbmp->pGrayMap, pbmp->uColors, WIN);
         }
      pbmp->iXOffset -= sizeof (pbmp->whdr) + pbmp->uColors * sizeof (RGBQUAD);
      }
   else
      {
      switch (pbmp->ohdr.bcBitCount)
         {
         case 1:  pbmp->uColors  = 2;               break;
         case 4:  pbmp->uColors  = 16;              break;
         case 8:  pbmp->uColors  = 256;             break;
         case 24: pbmp->uColors  = 0;               break;
         }
      if (pbmp->uColors)
         {
         oclr = (RGBTRIPLE *) malloc (pbmp->uColors * sizeof (RGBTRIPLE));
         fread (oclr, sizeof (RGBTRIPLE), pbmp->uColors, pbmp->fpIn);
         BuildGrayMap (oclr, pbmp->pGrayMap, pbmp->uColors, OS2);
         }
      pbmp->iXOffset -= sizeof (pbmp->ohdr) + pbmp->uColors * sizeof (RGBTRIPLE);
      }
   if (args[SHOWCOLORS].uiCount)
      PrintColors (pbmp, wclr, oclr);


   }



void ReadBmpHeader (PBMPINFO pbmp)
   {
   fread (&pbmp->bfh, sizeof (pbmp->bfh), 1, pbmp->fpIn);

   if (pbmp->bfh.bfType != (USHORT)'B' + (USHORT)('M' << 8))
      {
      printf ("File Not a Win/OS2 Bitmap (%c%c)\n", (BYTE) (pbmp->bfh.bfType & 0xFF), (BYTE)(pbmp->bfh.bfType >> 8));
      exit (1);
      }
   pbmp->iXOffset = (int)pbmp->bfh.bfOffBits  - sizeof (pbmp->bfh);

   fread (&pbmp->whdr, sizeof (pbmp->whdr), 1, pbmp->fpIn);
   if (pbmp->whdr.biSize == sizeof (pbmp->whdr))
      {
      if (pbmp->whdr.biCompression)
         Error ("This program Does not support compressed BMP images.", NULL);

      pbmp->uType  = WIN;
      pbmp->uXSize = (USHORT) pbmp->whdr.biWidth;   
      pbmp->uYSize = (USHORT) pbmp->whdr.biHeight;  
      pbmp->uBPP   = (USHORT) pbmp->whdr.biBitCount;
      }
   else if (pbmp->whdr.biSize == sizeof (pbmp->ohdr))
      {
      rewind (pbmp->fpIn);
      fread (&pbmp->bfh, sizeof (pbmp->bfh), 1, pbmp->fpIn);
      fread (&pbmp->ohdr, sizeof (pbmp->ohdr), 1, pbmp->fpIn);
      pbmp->uType  = OS2;
      pbmp->uXSize = (USHORT) pbmp->ohdr.bcWidth;   
      pbmp->uYSize = (USHORT) pbmp->ohdr.bcHeight;  
      pbmp->uBPP   = (USHORT) pbmp->ohdr.bcBitCount;
      }
   else
      Error ("Unknown bitmap type", NULL);

   if (args[INFO].uiCount)
      PrintFileHeader (pbmp);
      PrintBMPHeader (pbmp);
   }




void SkipJunk (PBMPINFO pbmp)
   {
   USHORT i;
   if (pbmp->iXOffset < 0)
      Error ("Internal Offset error in BMP file", NULL);

   if (pbmp->iXOffset)
      for (i=0; i< (USHORT) pbmp->iXOffset; i++)
         fread (&i, 1, 1, pbmp->fpIn);
   }





/************************************************************************/
/************************************************************************/
char CharFromIdx (USHORT pixel)
   {
   if (args[NEWCHARS].uiCount)
      {
      if (pixel >= args[NEWCHARS].uiCount)
         Error ("Not enough chars in new char mapping", NULL);

      return pCharMap [pixel];
      }
   return pCharMap [pbmp->pGrayMap [pixel] / 4];
   }



char OutChar (PBMPINFO pbmp, PSZ pszLine, USHORT x, USHORT y,
              USHORT *uPal, BYTE *cGrayVal, BYTE *cChar)
   {
   char   c, ch;
   USHORT i;

   switch (pbmp->uBPP)
      {
      case 1:
         ch = pszLine[x/8];
         *uPal = ch  & (1 << (7 - x % 8));
         c = (BYTE)(*uPal ? Bin1 : Bin0);
         break;
      case 4:
         *uPal = pszLine[x/2] & 0x0F << (x % 2);
         c = pCharMap [pbmp->pGrayMap [*uPal] / 4];
         break;

      case 8:
         *uPal =pszLine[x];
         c = pCharMap [pbmp->pGrayMap [*uPal] / 4];
         break;

      case 24:
         i = Intensity (pszLine[x*3], pszLine[x*3+1], pszLine[x*3+2]);
         c = pCharMap [i /4];
      }
   return c;
   }




void WriteOutput (PBMPINFO pbmp)
   {
   USHORT uLineSize, x, y;

   uLineSize  =  (pbmp->uXSize * pbmp->uBPP)/ 8 + !!((pbmp->uXSize * pbmp->uBPP) % 8);

   if (uLineSize %4)
      uLineSize += 4 - (uLineSize %4);

   fprintf (pbmp->fpOut, "%s [%d - %d]\n", pbmp->pszFileName, pbmp->uXSize, pbmp->uYSize);

   for (y = 0; y < pbmp->uYSize; y++)
      {
      fread (pszLine, 1, uLineSize, pbmp->fpIn);
      for (x = 0; x < pbmp->uXSize; x++)
         fputc (OutChar (pbmp, pszLine, x, y), pbmp->fpOut);
      fprintf (pbmp->fpOut, "\n");
      }
   }






   if (pixel >= args[NEWCHARS].uiCount)
      {
      for (y = 0; y < pbmp->uYSize; y++)
         {
         fread (pszLine, 1, uLineSize, pbmp->fpIn);
         for (x = 0; x < pbmp->uXSize; x++)
            fputc (OutNewChar (pbmp, pszLine, x, y), pbmp->fpOut);
         fprintf (pbmp->fpOut, "\n");
         }
      }
   else
      {
      }
      for (y = 0; y < pbmp->uYSize; y++)
         {
         fread (pszLine, 1, uLineSize, pbmp->fpIn);

         for (x = 0; x < pbmp->uXSize; x++)
            {
            c = NewChar (pbmp, pszLine, x, y)
            }
         fprintf (pbmp->fpOut, "\n");
         }




/************************************************************************/
/************************************************************************/


void ProcessBmp (PSZ pszInFile, PSZ pszOutFile)
   {
   BMPINFO bmp;


   if (!(bmp.fpIn = fopen (pszInFile, "rb")))
      Error ("Unable to open input file ", pszInFile);

   if (!(bmp.fpOut = fopen (pszOutFile, "w")))
      Error ("Unable to open output file ", pszOutFile);

   bmp.pszFileName = strdup (pszInFile);
   bmp.pszOutFile  = strdup (pszOutFile);

   ReadBmpHeader (&bmp);
   ReadColorInfo (&bmp);
   SkipJunk (&bmp);
   PrintBMPInfo (&bmp);
   WriteOutput (&bmp);

   fclose (bmp.fpOut);
   fclose (bmp.fpIn);
   }






void ParseArgs (int argc, char *argv[], PSZ pszInFile, PSZ pszOutFile)
   {
   int uInFile  = 0;
   int uOutFile = 0;
   int uReturn  = 0;
   int uError;
   PSZ pszTmp;

   if (argc == 1)
      Usage ();
   do
      {
      uReturn += 1 + (int) ProcessParams (argv+uReturn+1, args, &uError);
      if (uError || uInFile && argv[uReturn] != NULL)
         {
         printf ("Error processing param \"%s\"\nType 'BMPtoTXT -help' for command line options\n",
                  argv[(uError?uError%0x0100:uReturn)]);
         exit (uError >> 8);
         }
      if (!uInFile)
         uInFile = uReturn;
      else
         uOutFile = uReturn;
      }

   while (uReturn + 1 < argc);

   if (args[HELP1].uiCount || args[HELP2].uiCount || args[HELP3].uiCount)
      Usage ();

   if (argv[uInFile] == NULL)
      Error ("Input file must be specified. type BMPtoTXT -help for help", NULL);
   if (strchr (strcpy (pszInFile, argv[uInFile]), '.') == NULL)
      strcat (pszInFile, ".bmp");

   if (uOutFile == 0 || argv[uOutFile] == NULL)
      {
      if (strchr (strcpy (pszOutFile, argv[uOutFile]), '.') == NULL)
         strcat (pszOutFile, ".txt");
      }
   else /*--- outfile not explicitly specified ---*/
      {
      pszTmp = strchr (strcpy (pszOutFile, pszInFile), '.');
      *pszTmp = '\0';
      strcat (pszOutFile, ".txt");
      }

   if (*argv[uInFile] == '?')
      Usage ();

   if (!stricmp (pszInFile, pszOutFile))
      Error ("Input and output filenames must be different", NULL);

   if (args[NEW0].uiCount)
      Bin0 = *(args[NEW0].pszParam);
   if (args[NEW1].uiCount)
      Bin1 = *(args[NEW1].pszParam);
   }




main (int argc, char *argv[])
   {
   char  szIn[80], szOut[80];

   ParseArgs (argc, argv, szIn, szOut);
   CreateCharMap ();
   ProcessBmp (szIn, szOut);
   return 0;
   }











