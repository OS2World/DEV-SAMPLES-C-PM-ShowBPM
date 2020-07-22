#define PROGRAM       "UTILITY"        // Program Name
#define LEVEL         "Level 00"       // Program Level
#define COPYRIGHT     "Copyright (c) 1990 George S. Brickner"

#pragma title (PROGRAM " " LEVEL " - Misc Utilities")
#pragma linesize(120)
#pragma pagesize(55)

/*****************************************************************************
**                                                                          **
**          UTILITY - Misc Presentation Manager Utility Routines.           **
**                      Level 00, June 19th, 1990                           **
**                                                                          **
*****************************************************************************/
#define  INCL_DOS                      // Include OS/2 Doscalls
#define  INCL_WIN                      // Include OS/2 PM Win Calls
#define  INCL_ERRORS                   // Include All OS/2 Errors
#include <os2.h>                       // Standard OS/2 Definitions

#include <mt\string.h>                 // C/2 Standard String Defs
#include <mt\stdlib.h>                 // C/2 Standard Library Defs
#include <mt\stdio.h>                  // C/2 Standard I/O Defs

#include <SHOWBMP.h>                   // SHOWBMP Header File
#include <UTILITY.h>                   // UTILITY Header File

/********************************************************************
**                     Define Static & External Data               **
********************************************************************/
static CHAR const eyepopper[] = PROGRAM "-" LEVEL "-" __TIMESTAMP__ "-" COPYRIGHT;

extern CHAR   szClientClass[];         // Client class string
extern HWND   hwndFrame;               // Frame window handle

#pragma subtitle ("Display Message Box")
#pragma page()
/********************************************************************
**                       Display Message Box                       **
********************************************************************/
USHORT DisplayMsgBox (BOOL Beep, PCHAR Msg, PCHAR Title, USHORT Style)
  {
    if (Beep)                          // Sound Alarm
      {
        WinAlarm (HWND_DESKTOP, WA_ERROR); // Beep User
      }

    return WinMessageBox (             // Display Error Msg
               HWND_DESKTOP,           // Parent Window
               HWND_DESKTOP,           // Parent Window
               Msg,                    // Message Text
               Title,                  // Caption
               0,                      // Help Hook
               Style);                 // Msg Box Style
  }

#pragma subtitle ("Show PM Error")
#pragma page()
/********************************************************************
**                    Show PM Error Via Message Box                **
********************************************************************/
VOID ShowPMError (HAB hab, USHORT id)
  {
  ERRORID  err;                        // Error ID
  PERRINFO pErr;                       // PM Error Info Handle Ptr
  PUSHORT  pRetCode;                   // OS/2 Return Code ptr
  USHORT   rc = NO_ERROR;              // Return Code
  USHORT   size;                       // String Size
  CHAR     szMsg[256];                 // Message Work Area
  CHAR     szFormat[256];              // Format Area
  CHAR     szMessage[256];             // Finished Message Area
  CHAR     szCaption[256];             // Caption Area

    if (pErr = WinGetErrorInfo (hab))  // Then use error info
      {
        err = pErr->idError;           // Save error id code

        if (pErr->offBinaryData)       // Return code provided
          {
            SELECTOROF (pRetCode) = SELECTOROF (pErr);
            OFFSETOF (pRetCode) = pErr->offBinaryData;
            rc = *pRetCode;            // Save return code
          }
      }
    else                               // Just use Last Error
      {
        err = WinGetLastError (hab);
      }

    WinLoadString (hab, (HMODULE) NULL, IDS_FORMAT01, sizeof szFormat, szFormat);
    WinLoadString (hab, (HMODULE) NULL, id, sizeof szMsg, szMsg);

    sprintf (szMessage, szFormat, szMsg, '\r',
        ERRORIDSEV (err), ERRORIDERROR (err), '\r', rc);

    strcpy (szCaption, szClientClass);
    size = strlen (szCaption);
    WinLoadString (hab, (HMODULE) NULL, IDS_ERROR, sizeof szCaption - size, &szCaption[size]);
    DisplayMsgBox (TRUE, szMessage, szCaption, MB_OK | MB_CUACRITICAL);

    if (pErr)                          // Perform cleanup
      {
        WinFreeErrorInfo (pErr);       // Cleanup
      }
  }

#pragma subtitle ("Show DOS Error")
#pragma page()
/********************************************************************
**                    Show DOS Error Via Message Box               **
********************************************************************/
VOID ShowDOSError (HAB hab, USHORT rc, USHORT id)
  {
  USHORT  size;                        // String Size
  CHAR    szMsg[256];                  // Message Work Area
  CHAR    szFormat[256];               // Format Area
  CHAR    szMessage[256];              // Message Area
  CHAR    szCaption[256];              // Caption Area

    WinLoadString (hab, (HMODULE) NULL, IDS_FORMAT02, sizeof szFormat, szFormat);
    WinLoadString (hab, (HMODULE) NULL, id, sizeof szMsg, szMsg);

    sprintf (szMessage, szFormat, szMsg, rc, rc);

    strcpy (szCaption, szClientClass);
    size = strlen (szCaption);
    DisplayMsgBox (TRUE, szMessage, szCaption, MB_OK | MB_CUACRITICAL);
  }

#pragma subtitle ("Show Debug Data")
#pragma page()
/********************************************************************
**                   Show Debug Data Via Message Box               **
********************************************************************/
VOID ShowDebug (HAB hab, SHORT rc, PCHAR msg)
  {
  USHORT  size;                        // String Size
  CHAR    szFormat[256];               // Format Area
  CHAR    szMessage[256];              // Message Area
  CHAR    szCaption[256];              // Caption Area

    WinLoadString (hab, (HMODULE) NULL, IDS_FORMAT03, sizeof szFormat, szFormat);

    sprintf (szMessage, szFormat, msg, rc, rc);

    strcpy (szCaption, szClientClass);
    size = strlen (szCaption);
    WinLoadString (hab, (HMODULE) NULL, IDS_DEBUG, sizeof szCaption - size, &szCaption[size]);
    DisplayMsgBox (FALSE, szMessage, szCaption, MB_OK);
  }

#pragma subtitle ("Display Busy Pointer")
#pragma page()
/********************************************************************
**                      Display Busy Pointer                       **
********************************************************************/
VOID DisplayBusyPtr (BOOL Busy)
  {
  BOOL Mouse;                          // Display Pointer If No Mouse

    if (Busy)                          // Display Busy Pointer
      {
        WinSetPointer (HWND_DESKTOP, WinQuerySysPointer (HWND_DESKTOP, SPTR_WAIT, FALSE));
        Mouse = TRUE;
      }
    else                               // Display Normal Pointer
      {
        WinSetPointer (HWND_DESKTOP, WinQuerySysPointer (HWND_DESKTOP, SPTR_ARROW, FALSE));
        Mouse = FALSE;
      }

    if (WinQuerySysValue (HWND_DESKTOP, SV_MOUSEPRESENT) == 0) // No Mouse
      {
        WinShowPointer (HWND_DESKTOP, Mouse);
      }
  }

#pragma subtitle ("Enable/Disable Menu Items")
#pragma page()
/********************************************************************
**                Enable and Disable Menu Items                    **
********************************************************************/
VOID EnableMenuItem (SHORT idItem, BOOL fEnable)
  {
    WinSendMsg (WinWindowFromID (hwndFrame, FID_MENU),
                MM_SETITEMATTR,
                MPFROM2SHORT (idItem, TRUE),
                MPFROM2SHORT (MIA_DISABLED, fEnable ? ~MIA_DISABLED : MIA_DISABLED)) ;
  }
