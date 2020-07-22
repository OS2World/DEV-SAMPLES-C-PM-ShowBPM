#define PROGRAM       "BITMAP"         // Program Name
#define LEVEL         "Level 00"       // Program Level
#define COPYRIGHT     "Copyright (c) 1990 George S. Brickner"

#pragma title (PROGRAM " " LEVEL " - Process Bitmap")
#pragma linesize(120)
#pragma pagesize(55)

/*****************************************************************************
**                                                                          **
**          BITMAP - Load an OS/2 bitmap into a Device Memory Context       **
**                      Level 00, June 19th, 1990                           **
**                                                                          **
*****************************************************************************/
#define  INCL_DOS                      // Include OS/2 Doscalls
#define  INCL_GPI                      // Include OS/2 PM GPI Calls
#define  INCL_WIN                      // Include OS/2 PM Win Calls
#define  INCL_BITMAPFILEFORMAT         // Include OS/2 PM Bitmap File Hdr
#define  INCL_ERRORS                   // Include All OS/2 Errors
#include <os2.h>                       // Standard OS/2 Definitions

#include <mt\process.h>                // C/2 Standard Process Defs
#include <mt\string.h>                 // C/2 Standard String Defs
#include <mt\stdlib.h>                 // C/2 Standard Library Defs
#include <mt\stdio.h>                  // C/2 Standard I/O Defs

#include <SHOWBMP.h>                   // SHOWBMP header file
#include <BITMAP.h>                    // BITMAP header file

static CHAR const eyepopper[] = PROGRAM "-" LEVEL "-" __TIMESTAMP__ "-" COPYRIGHT;

#pragma subtitle ("Read Bitmap File")
#pragma page()
/**********************************************************************
**                         Read Bitmap File                          **
**********************************************************************/
static USHORT ReadBitmapFile (HFILE Han, PVOID Buffer, USHORT Length, ULONG Rba)
  {
  USHORT  rc = NO_ERROR;               // Return code
  USHORT  ActSize = 0;                 // Actual bytes read
  ULONG   NewRba = 0;                  // New RBA

    rc = DosChgFilePtr (Han, Rba, FILE_BEGIN, &NewRba); // Seek to RBA

    if (rc == NO_ERROR)                // Successful - read data
      {
        rc = DosRead (Han, Buffer, Length, &ActSize);

        if (rc == NO_ERROR && ActSize != Length)  // Must be EOF
          {
            rc = ERRBMP_RC_END_OF_FILE;
          }
      }

    return rc;                         // Return to caller
  }

#pragma subtitle ("Load Bit Map")
#pragma page()
/**********************************************************************
**              Create Bit Map In Memory Device Context              **
**********************************************************************/
USHORT CreateBitmap (PBMPCTL pCtl)
  {
  USHORT  rc = NO_ERROR;               // Return code
  USHORT  OpenAction;                  // Open actions
  USHORT  cbHeader;                    // Header & color table size in bytes
  USHORT  cbAdj;                       // Adjustment
  USHORT  i;                           // Scanline & color table index
  ULONG   Rba = 0L;                    // Bitmap file RBA
  HFILE   hFile;                       // Bitmap file handle
  SIZEL   sizl;                        // Size

    rc = DosOpen (                     // Open bitmap file for input
        pCtl->pFileName,               // Bitmap file name
        &hFile,                        // Bitmap file handle
        &OpenAction,                   // Open action taken
        0L,                            // File size - not used
        FILE_NORMAL,
        OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
        OPEN_ACCESS_READONLY       | OPEN_SHARE_DENYNONE,
        NULL                           // Reserved
      );

    if (rc == NO_ERROR)                // Process bitmap file
      {
        cbHeader = sizeof (BITMAPFILEHEADER);  // Bitmap file header size
        rc = ReadBitmapFile (hFile, &pCtl->Hdr, cbHeader, Rba); // Read bit map file hdr

        if (rc == NO_ERROR)            // Success - process header
          {
            if (pCtl->Hdr.usType == BFT_BMAP ||  // Bitmap is valid file type
                pCtl->Hdr.usType == BFT_ICON ||  // Icon is valid file type
                pCtl->Hdr.usType == BFT_POINTER) // Pointer is valid file type
              {
                pCtl->cbLine = (pCtl->Hdr.bmp.cx * pCtl->Hdr.bmp.cBitCount) / 8; // Scan line size
                if ((pCtl->Hdr.bmp.cx * pCtl->Hdr.bmp.cBitCount) % 8) // Add another byte
                  {
                    pCtl->cbLine++;
                  }
                cbAdj = pCtl->cbLine % sizeof (ULONG); // Compute required adjustment
                if (cbAdj)                       // Round to multiple of ULONG
                  {
                    pCtl->cbLine += (sizeof (ULONG) - cbAdj);
                  }
                pCtl->cLines = pCtl->Hdr.bmp.cy;   // Number of scan lines

                pCtl->cColors = 1 << pCtl->Hdr.bmp.cBitCount;  // Get color count
                cbHeader = sizeof (BITMAPINFOHEADER) + (sizeof (RGB) * pCtl->cColors);

                pCtl->pBmi = calloc (1, cbHeader); // Allocate bitmap info & color table

                if (pCtl->pBmi)            // Bitmap info block allocated
                  {
                    Rba = (ULONG)(sizeof (BITMAPFILEHEADER) - sizeof (BITMAPINFOHEADER));
                    rc = ReadBitmapFile (hFile, pCtl->pBmi, cbHeader, Rba);  // Read bit map info & color table

                    if (rc == NO_ERROR)        // Success - continue
                      {
                        for (i=0; i < pCtl->cColors; i++) // Copy color table
                          {
                            pCtl->lLogColorTbl[i] = ((ULONG) pCtl->pBmi->argbColor[i].bRed * 65536L) +
                                                    ((ULONG) pCtl->pBmi->argbColor[i].bGreen * 256L) +
                                                    (ULONG)  pCtl->pBmi->argbColor[i].bBlue;
                          }
                        pCtl->pScan = calloc (1, pCtl->cbLine); // Allocate scan line buffer

                        if (pCtl->pScan)           // Success - continue
                          {
                            pCtl->hdcBitmap = DevOpenDC (pCtl->habThread, OD_MEMORY, "*", 0L, NULL, NULL);

                            if (pCtl->hdcBitmap != DEV_ERROR) // Success - continue
                              {
                                sizl.cx = sizl.cy = 0;
                                pCtl->hpsBitmap = GpiCreatePS (pCtl->habThread, pCtl->hdcBitmap, &sizl,
                                    PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC);

                                if (pCtl->hpsBitmap != GPI_ERROR) // Success - Continue
                                  {
                                    pCtl->hbmBitmap = GpiCreateBitmap (pCtl->hpsBitmap, &pCtl->Hdr.bmp,
                                        0L, NULL, NULL);

                                    if (pCtl->hbmBitmap != GPI_ERROR) // Success - Continue
                                      {
                                        if (GpiSetBitmap (pCtl->hpsBitmap, pCtl->hbmBitmap)
                                            != HBM_ERROR) // Success - continue
                                          {
                                            Rba = pCtl->Hdr.offBits;   // Get bitmap RBA
                                            for (i=0; i < pCtl->cLines; i++) // Read & process scan lines
                                              {
                                                rc = ReadBitmapFile (  // Read bit map scan line
                                                    hFile, pCtl->pScan, pCtl->cbLine, Rba);

                                                if (rc == NO_ERROR)    // Successful
                                                  {
                                                    if (GpiSetBitmapBits (pCtl->hpsBitmap, (LONG) i, (LONG) 1,
                                                        pCtl->pScan, pCtl->pBmi)
                                                        == GPI_ALTERROR)   // Error - GpiSetBitmapBits failed
                                                      {
                                                        rc = ERRBMP_RC_SETBMBITS;
                                                        pCtl->Err = WinGetLastError (pCtl->habThread);
                                                        break;             // Exit loop
                                                      }
                                                    Rba += pCtl->cbLine;   // Bump to next line
                                                  }
                                                else               // Scan line read failed
                                                  {
                                                    pCtl->Err = MAKEERRORID (SEVERITY_ERROR, ERRBMP_SCANLINE_FAILED);
                                                    break;         // Exit loop
                                                  }
                                              }
                                          }
                                        else  // Error - GpiSetBitmap failed
                                          {
                                            rc = ERRBMP_RC_SETBM;
                                            pCtl->Err = WinGetLastError (pCtl->habThread);
                                          }
                                      }
                                    else  // Error - GpiCreateBitmap failed
                                      {
                                        rc = ERRBMP_RC_CREATEBM;
                                        pCtl->Err = WinGetLastError (pCtl->habThread);
                                      }
                                  }
                                else  // Error - GpiCreatePS failed
                                  {
                                    rc = ERRBMP_RC_CREATEPS;
                                    pCtl->Err = WinGetLastError (pCtl->habThread);
                                  }
                              }
                            else  // Error - DevOpenDC failed
                              {
                                rc = ERRBMP_RC_DEVOPENDC;
                                pCtl->Err = WinGetLastError (pCtl->habThread);
                              }
                          }
                        else                   // Bitmap scanline buffer allocation failed
                          {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                            pCtl->Err = MAKEERRORID (SEVERITY_ERROR, ERRBMP_SCANBUF_FAILED);
                          }
                      }
                    else                       // DosRead of bitmap info/color table failed
                      {
                        pCtl->Err = MAKEERRORID (SEVERITY_ERROR, ERRBMP_BMPINFO_FAILED);
                      }
                  }
                else                           // Calloc failed
                  {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    pCtl->Err = MAKEERRORID (SEVERITY_ERROR, ERRBMP_BMPINFO_FAILED);
                  }
              }
            else                       // Invalid file type
              {
                rc = ERROR_INVALID_DATA;
                pCtl->Err = MAKEERRORID (SEVERITY_ERROR, ERRBMP_INVALID_FILETYPE);
              }
          }
        else                           // Header read failed
          {
            pCtl->Err = MAKEERRORID (SEVERITY_ERROR, ERRBMP_HDRREAD_FAILED);
          }

        DosClose (hFile);              // Close bitmap file
      }
    else                               // DosOpen failed
      {
        pCtl->Err = MAKEERRORID (SEVERITY_ERROR, ERRBMP_DOSOPEN_FAILED);
      }

    if (rc)                            // Error occured - cleanup
      {
        DestroyBitMap (pCtl);          // Free bitmap storage
      }

    return rc;                         // Return to caller
  }

#pragma subtitle ("Destroy Bit Map")
#pragma page()
/**********************************************************************
**                     Destroy Bit Map In Memory                     **
**********************************************************************/
VOID DestroyBitMap (PBMPCTL pCtl)
  {
    if (pCtl)                          // Pointer passed - ok
      {
        if (pCtl->pScan)               // Free scan line buffer
          {
            free (pCtl->pScan);
          }

        if (pCtl->pBmi)                // Free Bitmap Info
          {
            free (pCtl->pBmi);
          }
        GpiDestroyPS (pCtl->hpsBitmap);    // Destroy Presentation Space
        DevCloseDC (pCtl->hdcBitmap);      // Close Dev Context
        GpiDeleteBitmap (pCtl->hbmBitmap); // Destroy bitmap
      }
  }
