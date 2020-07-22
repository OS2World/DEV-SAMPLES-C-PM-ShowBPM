#define PROGRAM       "SHOWBMP"        // Program Name
#define LEVEL         "Level 00"       // Program Level
#define COPYRIGHT     "Copyright (c) 1990 George S. Brickner"

#pragma title (PROGRAM " " LEVEL " - Display Bitmap")
#pragma linesize(120)
#pragma pagesize(55)

/*****************************************************************************
**                                                                          **
**          SHOWBMP - Display an OS/2 bitmap in a PM window.                **
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

#include <SHOWBMP.h>                   // SHOWBMP Header File
#include <UTILITY.h>                   // UTILITY Header File
#include <BITMAP.h>                    // BITMAP Header File
#include <FILEREQ.h>                   // FILEREQ Header File

#define  STACK_SIZE     (16384)        // Thread stack size

#pragma subtitle ("Define Global & Static Data Areas")
#pragma page()
/**********************************************************************
**                        Define Global Data                         **
**********************************************************************/
CHAR   szClientClass[] = PROGRAM;      // Client window class & pgm name
HWND   hwndFrame;                      // Frame Window Handle

/**********************************************************************
**                        Define Static Data                         **
**********************************************************************/
static CHAR const eyepopper[] = PROGRAM "-" LEVEL "-" __TIMESTAMP__ "-" COPYRIGHT;

static HAB    habMain;                 // PM Anchor Block Handle
static HWND   hwndClient;              // Client Window Handle
static HWND   hwndVscroll;             // Vertical Scroll Bar Handle
static HWND   hwndHscroll;             // Horizontal Scroll Bar Handle

static SHORT  cxClient;                // Client Window Width
static SHORT  cyClient;                // Client Window Height
static SHORT  cxScreen;                // Desktop screen Width
static SHORT  cyScreen;                // Desktop screen Height
static SHORT  cxBorder;                // Thin border Width
static SHORT  cyBorder;                // Thin border Height
static SHORT  cyTitleBar;              // Title Bar Height
static SHORT  cyMenu;                  // Menu Bar Height
static SHORT  cxVScroll;               // Vertical scroll bar width
static SHORT  cyHScroll;               // Horizontal scroll bar height

static SHORT  sVscrollPos;             // Vertical Scroll Position
static SHORT  sHscrollPos;             // Horizontal Scroll Position
static SHORT  sVscrollMax;             // Vertical Scroll Limit
static SHORT  sHscrollMax;             // Horizontal Scroll Limit

static BMPCTL BmpCtl;                  // Bitmap control area
static CHAR   szFileName[256];         // File name area

#pragma subtitle ("About Dialog Box Procedure")
#pragma page()
/********************************************************************
**                    About Dialog Box Procedure                   **
********************************************************************/
MRESULT EXPENTRY AboutDlgProc (HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
  {
    switch (msg)                       // Process Dialog Message
      {
        case WM_COMMAND:               // Command Message
          {
            switch (COMMANDMSG (&msg)->cmd)    // Process Command ID
              {
                case DID_OK:                   // User Clicked "OK"
                  {
                    WinDismissDlg (hwnd, TRUE);    // Done With Dialog
                    break;                         // Normal Exit
                  }
                default:                       // Default Command Processing
                  {
                    return WinDefDlgProc (hwnd, msg, mp1, mp2);
                  }
              }
            break;                         // Normal Exit
          }
        default:                       // Default Dialog Processing
          {
            return WinDefDlgProc (hwnd, msg, mp1, mp2);
          }
      }
    return MPFROMSHORT (FALSE);        // Return To PM
  }

#pragma subtitle ("Bitmap Fetch Thread")
#pragma page()
/********************************************************************
**                      Bitmap Fetch Thread                        **
********************************************************************/
static VOID _cdecl FAR BitmapThread (PBMPCTL pCtl)
  {
  USHORT    rc;                        // Return code

    pCtl->habThread = WinInitialize (0);   // Connect To PM

    while (DosSemWait (&pCtl->hTriggerSem, SEM_INDEFINITE_WAIT) == NO_ERROR)
      {
        DosSemSet (&pCtl->hTriggerSem);    // Set semaphore

        rc = CreateBitmap (pCtl);      // Load bitmap into memory

        if (rc == NO_ERROR)            // Bitmap load successful
          {
            WinPostMsg (pCtl->hwndClient, UWM_BITMAP_READY, 0, 0);
          }
        else                           // Bitmap load failed
          {
            WinPostMsg (pCtl->hwndClient, UWM_BITMAP_ERROR, MPFROM2SHORT (rc, 0), (MPARAM) pCtl->Err);
          }
      }
    WinTerminate (pCtl->habThread);    // Disconnect From PM
    _endthread ();                     // Terminate thread
  }

#pragma subtitle ("Set Window Title Text")
#pragma page()
/********************************************************************
**                       Set Window Title Text                     **
********************************************************************/
static VOID SetTitleText (PCHAR pTitle)
  {
  USHORT Size;                         // Data size
  CHAR   szTitleText[256];             // Window Title Text

    WinLoadString (habMain, (HMODULE) NULL, IDS_TITLE,
        sizeof szTitleText - 1, szTitleText);
    strcat (szTitleText, " - ");
    if (pTitle)                        // Title string supplied
      {
        strcat (szTitleText, pTitle);
      }
    else                               // Title string omitted - substitute
      {
        Size = strlen (szTitleText);   // Get title length so far
        WinLoadString (habMain, (HMODULE) NULL, IDS_UNTITLED,
            sizeof (szTitleText - 1) - Size, &szTitleText[Size]);
      }
    WinSetWindowText (hwndFrame, szTitleText);
  }

#pragma subtitle ("View Bitmap Information")
#pragma page()
/********************************************************************
**                      View Bitmap Information                    **
********************************************************************/
static VOID ViewInformation (PBMPCTL pCtl)
  {
  USHORT  size;                        // String Size
  CHAR    szFormat[256];               // Format Area
  CHAR    szMessage[256];              // Message Area
  CHAR    szCaption[256];              // Caption Area

    WinLoadString (habMain, (HMODULE) NULL, IDS_FORMAT04, sizeof szFormat, szFormat);

    sprintf (szMessage, szFormat, pCtl->Hdr.bmp.cx, pCtl->Hdr.bmp.cy, pCtl->cColors);

    strcpy (szCaption, szClientClass);
    size = strlen (szCaption);
    WinLoadString (habMain, (HMODULE) NULL, IDS_VIEWINFO, sizeof szCaption - size, &szCaption[size]);
    DisplayMsgBox (FALSE, szMessage, szCaption, MB_OK);
  }


#pragma subtitle ("Main Client Window Procedure")
#pragma page()
/********************************************************************
**                    Main Client Window Procedure                 **
********************************************************************/
MRESULT EXPENTRY ClientWndProc (HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
  {
  static  BOOL   BitmapReady;          // Bitmap ready flag
  HPS     hps;                         // Presentation Space Handle
  RECTL   rcl;                         // Rectangle
  POINTL  aptl[3];                     // GpiBitBlt point array
  USHORT  rc;                          // Return code
  SHORT   cxNew, cyNew, xNew, yNew;    // New window size, position
  SHORT   sHscrollInc;                 // Horiz scroll Increment
  SHORT   sVscrollInc;                 // Vert scroll Increment
  PBMPCTL pCtl;                        // Bitmap Control Ptr
  CHAR    szText[16];                  // Text

    pCtl = &BmpCtl;                    // Set Ptr
    switch (msg)                       // Process Window Message
      {
        case WM_CREATE:                // Window being created
          {
            hwndVscroll = WinWindowFromID (
                WinQueryWindow (hwnd, QW_PARENT, FALSE),
                FID_VERTSCROLL);       // Get Vert Scroll Handle

            hwndHscroll = WinWindowFromID (
                WinQueryWindow (hwnd, QW_PARENT, FALSE),
                FID_HORZSCROLL);       // Get Horiz Scroll Handle

            SetOpenMask ("*.BMP");     // Set initial open search mask
            BitmapReady = FALSE;       // Initialize switch

            pCtl->hTriggerSem = NULL;
            DosSemSet (&pCtl->hTriggerSem);  // Set semaphore

            pCtl->tidThread = _beginthread (BitmapThread, malloc (STACK_SIZE), STACK_SIZE, pCtl);

            if (pCtl->tidThread != -1)     // Thread started
              {
                if (pCtl->pFileName)       // Bitmap on cmd line - fetch
                  {
                    DosSemClear (&pCtl->hTriggerSem);
                  }
              }
            else                   // Thread start failed
              {
                rc = _doserrno;
                ShowDOSError (habMain, rc, IDS_ERROR05);
                return MPFROMSHORT (TRUE);     // Error return To PM
              }
            break;                     // Normal Exit
          }
        case WM_SIZE:                  // Window being sized
          {
            cxClient = SHORT1FROMMP (mp2); // New window width
            cyClient = SHORT2FROMMP (mp2); // New window height

            sHscrollMax = max (0, (SHORT) pCtl->Hdr.bmp.cx - cxClient);
            sHscrollPos = min (sHscrollPos, sHscrollMax);

            WinSendMsg (               // Set Scroll Bar Pos/Limits
                hwndHscroll, SBM_SETSCROLLBAR,
                MPFROM2SHORT (sHscrollPos, 0),     // Set Current Position
                MPFROM2SHORT (0, sHscrollMax));    // Set Min & Max Limits

            WinEnableWindow (          // Enable Scrollbar If Not Zero
                hwndHscroll,
                sHscrollMax ? TRUE : FALSE);

            sVscrollMax = max (0, (SHORT) pCtl->Hdr.bmp.cy - cyClient);
            sVscrollPos = min (sVscrollPos, sVscrollMax);

            WinSendMsg (               // Set Scroll Bar Pos/Limits
                hwndVscroll, SBM_SETSCROLLBAR,
                MPFROM2SHORT (sVscrollPos, 0),     // Set Current Position
                MPFROM2SHORT (0, sVscrollMax));    // Set Min & Max Limits

            WinEnableWindow (          // Enable Scrollbar If Not Zero
                hwndVscroll,
                sVscrollMax ? TRUE : FALSE);
            break;                     // Normal Exit
          }
        case WM_HSCROLL:               // Horizontal Scroll Bar Moved
          {
            switch (SHORT2FROMMP (mp2))        // Process Scroll Amount
              {
                case SB_LINELEFT:              // Move Left 1/25 window
                  {
                    sHscrollInc = -(cxClient / 25);
                    break;                     // Send Msg To Scroll Bar
                  }
                case SB_LINERIGHT:             // Move Right 1/25 window
                  {
                    sHscrollInc = cxClient / 25;
                    break;                     // Send Msg To Scroll Bar
                  }
                case SB_PAGELEFT:              // Move Left 1/4 window
                  {
                    sHscrollInc = -(cxClient / 4);
                    break;                     // Send Msg To Scroll Bar
                  }
                case SB_PAGERIGHT:             // Move Right 1/4 window
                  {
                    sHscrollInc = cxClient / 4;
                    break;                     // Send Msg To Scroll Bar
                  }
                case SB_SLIDERPOSITION:        // Slider Movement
                  {
                    sHscrollInc = SHORT1FROMMP (mp2) - sHscrollPos;
                    break;                     // Send Msg To Scroll Bar
                  }
                default:                       // Ignore Other Scroll Msgs
                  {
                    sHscrollInc = 0;           // No Scroll
                    break;
                  }
              }

            sHscrollInc = max (-sHscrollPos, min (sHscrollInc, sHscrollMax - sHscrollPos));

            if (sHscrollInc)                   // Perform Scroll
              {
                sHscrollPos += sHscrollInc;    // Update Scroll Pos
                WinScrollWindow (hwndClient, -sHscrollInc, 0,    // Horz Scroll
                    NULL, NULL, NULL, NULL, SW_INVALIDATERGN);
                WinSendMsg (hwndHscroll, SBM_SETPOS,       // Set Scroll Bar Pos
                    MPFROM2SHORT (sHscrollPos, 0), NULL);
                WinUpdateWindow (hwnd);
              }
            break;                     // Normal exit
          }
        case WM_VSCROLL:               // Vertical Scroll Bar Moved
          {
            switch (SHORT2FROMMP (mp2))        // Process Scroll Amount
              {
                case SB_LINEUP:                // Move up 1/25 window
                  {
                    sVscrollInc = -(cyClient / 25);
                    break;                     // Send Msg To Scroll Bar
                  }
                case SB_LINEDOWN:              // Move down 1/25 window
                  {
                    sVscrollInc = cyClient / 25;
                    break;                     // Send Msg To Scroll Bar
                  }
                case SB_PAGEUP:                // Move up 1/4 window
                  {
                    sVscrollInc = -(cyClient / 4);
                    break;                     // Send Msg To Scroll Bar
                  }
                case SB_PAGEDOWN:              // Move down 1/4 window
                  {
                    sVscrollInc = cyClient / 4;
                    break;                     // Send Msg To Scroll Bar
                  }
                case SB_SLIDERPOSITION:        // Slider Movement
                  {
                    sVscrollInc = SHORT1FROMMP (mp2) - sVscrollPos;
                    break;                     // Send Msg To Scroll Bar
                  }
                default:                       // Ignore Other Scroll Msgs
                  {
                    sVscrollInc = 0;           // No Scroll
                    break;
                  }
              }

            sVscrollInc = max (-sVscrollPos, min (sVscrollInc, sVscrollMax - sVscrollPos));

            if (sVscrollInc)                   // Perform Scroll
              {
                sVscrollPos += sVscrollInc;    // Update Scroll Pos
                WinScrollWindow (hwndClient, 0, sVscrollInc, // Vert Scroll
                    NULL, NULL, NULL, NULL, SW_INVALIDATERGN);
                WinSendMsg (hwndVscroll, SBM_SETPOS,       // Set Scroll Bar Pos
                    MPFROM2SHORT (sVscrollPos, 0), NULL);
                WinUpdateWindow (hwnd);
              }
            break;                     // Normal exit
          }
        case WM_PAINT:                 // Paint The Window
          {
            DisplayBusyPtr (TRUE);     // Display hourglass pointer
            hps = WinBeginPaint (hwnd, NULL, &rcl);

            if (BitmapReady)           // Bitmap available - display
              {
                GpiCreateLogColorTable (hps, LCOL_RESET, LCOLF_CONSECRGB,
                    0L, (LONG) pCtl->cColors, pCtl->lLogColorTbl);
                // Target bottom left corner of client window
                aptl[0].x = rcl.xLeft;
                aptl[0].y = rcl.yBottom;
                // Target top right corner of client window
                aptl[1].x = rcl.xRight;
                aptl[1].y = rcl.yTop;
                // Source bottom left corner of bitmap
                aptl[2].x = rcl.xLeft + sHscrollPos;
                aptl[2].y = rcl.yBottom + sVscrollMax - sVscrollPos;
                GpiBitBlt (hps, pCtl->hpsBitmap, 3L, aptl, ROP_SRCCOPY, BBO_IGNORE);
              }
            else if (pCtl->pFileName)  // Display "Working..."
              {
                WinLoadString (habMain, (HMODULE) NULL, IDS_WORKING,
                    sizeof szText - 1, szText);
                WinQueryWindowRect (hwnd, &rcl);
                WinDrawText (hps, -1, szText, &rcl, 0L, 0L,
                    DT_CENTER | DT_VCENTER | DT_TEXTATTRS | DT_ERASERECT);
              }
            else                       // Just erase client area
              {
                GpiErase (hps);
              }
            WinEndPaint (hps);
            DisplayBusyPtr (FALSE);    // Display normal pointer
            break;                     // Normal Exit
          }
        case UWM_BITMAP_READY:         // Bitmap ready to display
          {
            cxNew = min ((SHORT) (pCtl->Hdr.bmp.cx + (2 * cxBorder) + cxVScroll - 1), cxScreen);
            cyNew = min ((SHORT) (pCtl->Hdr.bmp.cy + (2 * cyBorder) + cyTitleBar + cyMenu + cyHScroll - 1), cyScreen);

            xNew = (cxScreen - cxNew) / 2; // New X co-ordinate
            yNew = (cyScreen - cyNew) / 2; // New Y co-ordinate

            BitmapReady = TRUE;        // Bitmap is ready for display
            sHscrollPos = sVscrollPos = 0; // Reset scroll bars
            WinSetWindowPos (hwndFrame, 0, xNew, yNew, cxNew, cyNew, SWP_SIZE | SWP_MOVE);
            WinInvalidateRect (hwnd, NULL, FALSE); // Force re-paint
            break;                     // Normal edit
          }
        case UWM_BITMAP_ERROR:         // Bitmap processing failed
          {
            BitmapReady = FALSE;       // Bitmap is not ready for display
            pCtl->pFileName = NULL;    // Nothing to display
            WinSetErrorInfo ((ERRORID) mp2, SEI_DOSERROR, SHORT1FROMMP (mp1)); // Save Error info
            ShowPMError (habMain, IDS_ERROR04);
            WinInvalidateRect (hwnd, NULL, FALSE); // Force re-paint
            break;                     // Normal edit
          }
        case WM_HELP:                  // Process help command
          {
            DisplayMsgBox (            // Display dummy help
                FALSE,                 // No beep
                "Sorry, there is no help for you.", // Message
                "Help",                // Title
                MB_OK                  // Use OK button
              );
            break;                     // Normal Exit
          }
        case WM_CHAR:                  // Process keystroke
          {
            switch (CHARMSG (&msg)->vkey)      // Process Keystroke
              {
                case VK_LEFT:                  // Scroll Left
                case VK_RIGHT:                 // Scroll Right
                  {
                    return WinSendMsg (hwndHscroll, msg, mp1, mp2);
                  }
                case VK_UP:                    // Scroll Up One Line
                case VK_DOWN:                  // Scroll Down One Line
                case VK_PAGEUP:                // Scroll Up One Page
                case VK_PAGEDOWN:              // Scroll Down One Page
                  {
                    return WinSendMsg (hwndVscroll, msg, mp1, mp2);
                  }
                default:                       // Default Processing
                  {
                    return WinDefWindowProc (hwnd, msg, mp1, mp2);
                  }
              }
            break;                     // Normal exit
          }
        case WM_COMMAND:               // Process Menu Command
          {
            switch (COMMANDMSG (&msg)->cmd) // Process menu selection
              {
                case IDM_FILE_NEW:         // Process New Request
                  {
                    SetTitleText (NULL);   // Default title
                    pCtl->pFileName = NULL;    // No bitmap file name
                    BitmapReady = FALSE;   // Bitmap is not ready for display
                    DestroyBitMap (pCtl);  // Clean up existing bitmap
                    WinInvalidateRect (hwnd, NULL, FALSE); // Force repaint
                    break;                 // Normal exit
                  }
                case IDM_FILE_OPEN:        // Process Open Request
                  {
                    pCtl->pFileName = NULL;    // No bitmap file name
                    BitmapReady = FALSE;   // Bitmap is not ready for display
                    DestroyBitMap (pCtl);  // Clean up existing bitmap

                    if (WinDlgBox (        // Invoke Open Dialog Box
                        HWND_DESKTOP,      // Desktop Handle
                        hwnd,              // Window Handle
                        OpenDlgProc,       // Dialog Processing
                        (HMODULE) NULL,    // Resource Is In .EXE
                        IDD_OPEN,          // Dialog ID
                        szFileName))       // Dialog Data Ptr
                      {
                        pCtl->pFileName = szFileName;    // Set file name ptr
                        SetTitleText (pCtl->pFileName);  // Set new title
                        WinInvalidateRect (hwnd, NULL, FALSE); // Force repaint
                        DosSemClear (&pCtl->hTriggerSem);     // Post thread
                      }
                    else                    // Open Failed
                      {
                        SetTitleText (NULL);   // Default title
                        pCtl->pFileName = NULL;
                      }
                    break;                  // Normal Exit
                  }
                case IDM_FILE_ABOUT:        // Display About Box
                  {
                    WinDlgBox (             // Invoke Dialog Box
                        HWND_DESKTOP,       // Desktop Handle
                        hwnd,               // Window Handle
                        AboutDlgProc,       // Dialog Processing
                        (HMODULE) NULL,     // Resource Is In .EXE
                        IDD_ABOUT,          // Dialog ID
                        NULL                // Dialog Data Ptr
                      );
                    break;                  // Normal Exit
                  }
                case IDM_FILE_EXIT:         // Exit program
                  {
                    WinPostMsg (hwnd, WM_QUIT, NULL, NULL);
                    break;                  // Normal exit
                  }
                case IDM_FILE_INFO:         // View information
                  {
                    ViewInformation (pCtl);
                    break;                  // Normal exit
                  }
                default:                    // Default processing
                  {
                    return WinDefWindowProc (hwnd, msg, mp1, mp2);
                  }
              }
            break;                     // Normal exit
          }
        case WM_INITMENU:              // Initialize Menu items
          {
            switch (SHORT1FROMMP (mp1))
              {
                case IDM_FILE:         // File menu - Disable
                  {
                    EnableMenuItem (IDM_FILE_SAVE,   FALSE);
                    EnableMenuItem (IDM_FILE_SAVEAS, FALSE);
                    break;             // Normal Exit
                  }
              }
            break;                     // Normal exit
          }
        case WM_DESTROY:               // Window being destroyed
          {
            DosSuspendThread (pCtl->tidThread);
            DestroyBitMap (pCtl);      // Clean up
            break;                     // Normal exit
          }
        default:                       // Default Window Processing
          {
            return WinDefWindowProc (hwnd, msg, mp1, mp2);
          }
      }
    return MPFROMSHORT (FALSE);        // Normal return To PM
  }

#pragma subtitle ("Initialize Client Window")
#pragma page()
/********************************************************************
**                    Initialize Client Window                     **
********************************************************************/
BOOL InitClientWindow (PBMPCTL pCtl)
  {
  BOOL Response = FALSE;               // Response to caller
  ULONG flFrameFlags;                  // Frame flags

    flFrameFlags = FCF_TITLEBAR   |    // Title bar
                   FCF_SYSMENU    |    // System menu
                   FCF_MINBUTTON  |    // Minimumize button
                   FCF_BORDER     |    // Thin border
                   FCF_ICON       |    // Icon present
                   FCF_MENU       |    // Menu bar
                   FCF_ACCELTABLE |    // Keyboard accelerator table
                   FCF_SHELLPOSITION | // Let shell position window
                   FCF_TASKLIST   |    // Task list entry
                   FCF_VERTSCROLL |    // Vertical scroll bar
                   FCF_HORZSCROLL;     // Horizontal scroll bar

    if (WinRegisterClass (             // Register client window class
        habMain,                       // Anchor block handle
        szClientClass,                 // Name of class being registered
        ClientWndProc,                 // Window procedure for class
        0L,                            // Class style
        0))                            // Extra bytes to reserve
      {
        hwndFrame = WinCreateStdWindow (   // Create Frame & Client Windows
            HWND_DESKTOP,              // Parent window handle
            WS_VISIBLE,                // Style of frame window
            &flFrameFlags,             // Pointer to control data
            szClientClass,             // Client window class name
            NULL,                      // Title bar text
            0L,                        // Style of client window
            (HMODULE) NULL,            // Resources in EXE file
            ID_MAINWIN,                // ID of resources
            &hwndClient);              // Pointer to client window handle

        if (hwndFrame && hwndClient)   // Window Created - continue
          {
            pCtl->hwndClient = hwndClient;   // Client window to notify
            SetTitleText (pCtl->pFileName);  // Initial title
            cxScreen   = (SHORT) WinQuerySysValue (HWND_DESKTOP, SV_CXSCREEN);
            cyScreen   = (SHORT) WinQuerySysValue (HWND_DESKTOP, SV_CYSCREEN);
            cxBorder   = (SHORT) WinQuerySysValue (HWND_DESKTOP, SV_CXBORDER);
            cyBorder   = (SHORT) WinQuerySysValue (HWND_DESKTOP, SV_CYBORDER);
            cyTitleBar = (SHORT) WinQuerySysValue (HWND_DESKTOP, SV_CYTITLEBAR);
            cyMenu     = (SHORT) WinQuerySysValue (HWND_DESKTOP, SV_CYMENU);
            cxVScroll  = (SHORT) WinQuerySysValue (HWND_DESKTOP, SV_CXVSCROLL);
            cyHScroll  = (SHORT) WinQuerySysValue (HWND_DESKTOP, SV_CYHSCROLL);
            Response = TRUE;           // Indicate success
          }
        else                           // Client WinCreateStdWindow Failed
          {
            ShowPMError (habMain, IDS_ERROR03);
          }
      }
    else                               // Client WinRegisterClass Failed
      {
        ShowPMError (habMain, IDS_ERROR02);
      }
    return Response;                   // Return to caller
  }

#pragma subtitle ("Main Program")
#pragma page()
/********************************************************************
**                    Main Program Entry Point                     **
********************************************************************/
USHORT main (USHORT argc, PCHAR argv[])
  {
  HMQ     hmq = NULL;                  // Message Queue Handle
  QMSG    qmsg;                        // Message Queue Element
  PBMPCTL pCtl;                        // Bitmap control area ptr

    pCtl = &BmpCtl;                    // Set bitmap control ptr
    memset (pCtl, 0x00, sizeof (BMPCTL));

    if (argc > 1)                      // Filename supplied
      {
        pCtl->pFileName = argv[1];
      }

    habMain = WinInitialize (0);       // Connect To PM

    if (habMain)                       // Successful - Continue
      {
        hmq = WinCreateMsgQueue (habMain, 0);  // Create Message Queue

        if (hmq)                       // Successful - Continue
          {
            if (InitClientWindow (pCtl))   // Client Window OK
              {
                while (WinGetMsg (habMain, &qmsg, NULL, 0, 0)) // Msg loop
                  {
                    WinDispatchMsg (habMain, &qmsg);   // Dispatch message
                  }
                WinDestroyWindow (hwndFrame);  // Destroy Window
              }
            WinDestroyMsgQueue (hmq);  // Destroy Message Queue
          }
        else  // WinCreateMsgQueue Failed
          {
            ShowPMError (habMain, IDS_ERROR01);
          }
        WinTerminate (habMain);        // Disconnect From PM
      }
    else  // WinInitialize Failed
      {
        WinAlarm (HWND_DESKTOP, WA_ERROR);
      }
    return 0;
  }
