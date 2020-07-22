#define  PROGRAM   "FILEREQ"
#define  LEVEL     "Level 00"
#define  COPYRIGHT "Copyright (c) 1990 George S. Brickner"

#pragma title (PROGRAM " " LEVEL " - File Requester")
#pragma linesize(120)
#pragma pagesize(55)

/********************************************************************
**       FILEREQ - File Requester and Open Dialog Procedure        **
**                 Level 00 - June 19th, 1990                      **
********************************************************************/
#define INCL_WIN                       // Include Windows (PM) Defs
#define INCL_GPI                       // Include OS/2 GPI Calls
#define INCL_ERRORS                    // Include Error Defs
#include <os2.h>                       // OS/2 Standard Defs

#include <mt\stdio.h>                  // IBM C/2 Standard I/O Defs
#include <mt\stdlib.h>                 // IBM C/2 Standard Library Defs
#include <mt\string.h>                 // IBM C/2 String Defs

#include <FILEREQ.h>                   // FILEREQ header defines
#include <SHOWBMP.h>                   // SHOWBMP header defines

/********************************************************************
**                   Define Static And Global Data                 **
********************************************************************/
static CHAR const eyepopper[] = PROGRAM "-" LEVEL "-" __TIMESTAMP__ "-" COPYRIGHT;

static CHAR szFileMask[CCHMAXPATH] = "*.*";    // Search mask buffer

#pragma subtitle ("Get Current Path")
#pragma page()
/********************************************************************
**                        Get Current Path                         **
********************************************************************/
static VOID GetCurrentPath (HWND hwnd, PCHAR pcCurrentPath)
  {
  USHORT      usDriveNum, usCurPathLen;
  ULONG       ulDriveMap;

    DosQCurDisk (&usDriveNum, &ulDriveMap);
    pcCurrentPath [0] = (CHAR) usDriveNum + (UCHAR) '@';
    pcCurrentPath [1] = ':';
    pcCurrentPath [2] = '\\';
    usCurPathLen = 64;
    DosQCurDir (0, pcCurrentPath + 3, &usCurPathLen);

    WinSetDlgItemText (hwnd, IDD_PATH, pcCurrentPath);
  }

#pragma subtitle ("Fill Drive List Box")
#pragma page()
/********************************************************************
**                      Fill Drive List Box                        **
********************************************************************/
static VOID FillDrvListBox (HWND hwnd)
  {
  static CHAR szDrive [] = "[ :]";
  SHORT       sDrive;
  USHORT      usDriveNum;
  ULONG       ulDriveMap;

    DosQCurDisk (&usDriveNum, &ulDriveMap);
    WinSendDlgItemMsg (hwnd, IDD_DEVLIST, LM_DELETEALL, NULL, NULL);

    for (sDrive = 0; sDrive < 26; sDrive++)
      {
        if (ulDriveMap & 1L << sDrive)
          {
            szDrive [1] = (CHAR) sDrive + (UCHAR) 'A';
            WinSendDlgItemMsg (hwnd, IDD_DEVLIST, LM_INSERTITEM,
                               MPFROM2SHORT (LIT_END, 0),
                               MPFROMP (szDrive));
          }
      }
  }

#pragma subtitle ("Fill Directory List Box")
#pragma page()
/********************************************************************
**                    Fill Directory List Box                      **
********************************************************************/
static VOID FillDirListBox (HWND hwnd)
  {
  static CHAR szTemp[CCHMAXPATH];      // Temp directory name buffer
  FILEFINDBUF findbuf;
  HDIR        hDir = 1;
  USHORT      usSearchCount = 1;

    WinSendDlgItemMsg (hwnd, IDD_DIRLIST, LM_DELETEALL, NULL, NULL);
    DosFindFirst ("*.*", &hDir, 0x0017, &findbuf, sizeof findbuf,
                  &usSearchCount, 0L);
    while (usSearchCount)
      {
        if (findbuf.attrFile & 0x0010) // Directory entry
          {
            if (findbuf.achName [0] != '.' ||
                findbuf.achName [1])   // Insert directory into list box
              {
                strcpy (szTemp, "[");  // Enclose directory name in brackets
                strcat (szTemp, findbuf.achName);
                strcat (szTemp, "]");
                WinSendDlgItemMsg (hwnd, IDD_DIRLIST, LM_INSERTITEM,
                                   MPFROM2SHORT (LIT_SORTASCENDING, 0),
                                   MPFROMP (szTemp));
              }
          }
        DosFindNext (hDir, &findbuf, sizeof findbuf, &usSearchCount);
      }
  }

#pragma subtitle ("Fill File List Box")
#pragma page()
/********************************************************************
**                      Fill File List Box                         **
********************************************************************/
static VOID FillFileListBox (HWND hwnd)
  {
  FILEFINDBUF findbuf;
  HDIR        hDir = 1;
  USHORT      usSearchCount = 1;

    WinSendDlgItemMsg (hwnd, IDD_FILELIST, LM_DELETEALL, NULL, NULL);

    DosFindFirst (szFileMask, &hDir, 0x0007, &findbuf, sizeof findbuf,
                  &usSearchCount, 0L);

    while (usSearchCount)
      {
        WinSendDlgItemMsg (hwnd, IDD_FILELIST, LM_INSERTITEM,
                           MPFROM2SHORT (LIT_SORTASCENDING, 0),
                           MPFROMP (findbuf.achName));
        DosFindNext (hDir, &findbuf, sizeof findbuf, &usSearchCount);
      }
  }

#pragma subtitle ("Set Open Search Mask")
#pragma page()
/********************************************************************
**                    Set Initial Open Search Mask                 **
********************************************************************/
VOID SetOpenMask (PCHAR pSearchMask)
  {
    if (pSearchMask)                   // Search mask supplied
      {
        strcpy (szFileMask, pSearchMask);
      }
    else                               // Use default mask
      {
        strcpy (szFileMask, "*.*");
      }
  }

#pragma subtitle ("Process Open Dialog")
#pragma page()
/********************************************************************
**                        Process Open Dialog                      **
********************************************************************/
MRESULT EXPENTRY OpenDlgProc (HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2)
  {
  static CHAR  szCurrentPath [CCHMAXPATH], szBuffer [CCHMAXPATH];
  static PCHAR pFileName;              // File name ptr
  SHORT        sSelect;                // Current listbox selection
  PCHAR        p;                      // Misc pointer

    switch (msg)                       // Process Dialog Message
      {
        case WM_INITDLG:               // Initialize Dialog Box
          {
            if (mp2)                   // File Name Ptr Supplied
              {
                pFileName = (PCHAR) mp2;   // Save ptr to user's filename area
                WinSendDlgItemMsg (hwnd, IDD_FILEEDIT, EM_SETTEXTLIMIT,
                                   MPFROM2SHORT (CCHMAXPATH, 0), NULL);
                WinSetDlgItemText (hwnd, IDD_FILEEDIT, szFileMask); // Show search mask
                FillDrvListBox (hwnd);
                GetCurrentPath (hwnd, szCurrentPath);
                FillDirListBox (hwnd);
                FillFileListBox (hwnd);
              }
            else                       // File Name Ptr Not Supplied
              {
                pFileName = NULL;
                WinDismissDlg (hwnd, FALSE);
              }
            return FALSE;
          }
        case WM_CONTROL:               // Dialog Control
          {
            if (SHORT1FROMMP (mp1) == IDD_DEVLIST ||  // Drive
                SHORT1FROMMP (mp1) == IDD_DIRLIST ||  // Directory
                SHORT1FROMMP (mp1) == IDD_FILELIST)   // Filename
              {
                sSelect = SHORT1FROMMR (WinSendDlgItemMsg (hwnd,
                              SHORT1FROMMP (mp1), LM_QUERYSELECTION, 0L, 0L));
                WinSendDlgItemMsg (hwnd, SHORT1FROMMP (mp1),
                                   LM_QUERYITEMTEXT,
                                   MPFROM2SHORT (sSelect, sizeof szBuffer),
                                   MPFROMP (szBuffer));
              }

            if (SHORT1FROMMP (mp1) == IDD_DEVLIST) // Drive List Box
              {
                switch (SHORT2FROMMP (mp1))    // Notification code
                  {
                    case LN_ENTER:             // ENTER
                      {
                        if (szBuffer [0] == '[')
                          {
                            DosSelectDisk (szBuffer [1] - '@');
                          }
                        else
                          {
                            DosChDir (szBuffer, 0L);
                          }
                        FillDrvListBox (hwnd);
                        GetCurrentPath (hwnd, szCurrentPath);
                        FillDirListBox (hwnd);
                        FillFileListBox (hwnd);
                        WinSetDlgItemText (hwnd, IDD_FILEEDIT, szFileMask);
                        return FALSE;
                      }
                  }
              }
            else if (SHORT1FROMMP (mp1) == IDD_DIRLIST) // Dir List Box
              {
                switch (SHORT2FROMMP (mp1))    // Notification code
                  {
                    case LN_ENTER:             // ENTER
                      {
                        if (p = strchr (szBuffer, ']')) // Delete "]"
                          {
                            *p = '\0';
                          }
                        DosChDir (&szBuffer[1], 0L);
                        GetCurrentPath (hwnd, szCurrentPath);
                        FillDirListBox (hwnd);
                        FillFileListBox (hwnd);
                        WinSetDlgItemText (hwnd, IDD_FILEEDIT, szFileMask);
                        return FALSE;
                      }
                  }
              }
            else if (SHORT1FROMMP (mp1) == IDD_FILELIST) // File List Box
              {
                switch (SHORT2FROMMP (mp1))    // Notification code
                  {
                    case LN_SELECT:            // Selected
                      {
                        WinSetDlgItemText (hwnd, IDD_FILEEDIT, szBuffer);
                        return FALSE;
                      }
                    case LN_ENTER:             // ENTER
                      {
                        ParseFileName (pFileName, szBuffer);
                        WinDismissDlg (hwnd, TRUE);
                        return FALSE;
                      }
                  }
              }
            else if (SHORT1FROMMP (mp1) == IDD_FILEEDIT) // File edit field
              {
              }
            break;
          }
        case WM_COMMAND:               // Window Command
          {
            switch (COMMANDMSG(&msg)->cmd) // Get Diaglog Item Clicked
              {
                case DID_OK:           // Clicked "OK"
                  {
                    WinQueryDlgItemText (hwnd, IDD_FILEEDIT, sizeof szBuffer, szBuffer);
                    switch (ParseFileName (szCurrentPath, szBuffer))
                      {
                        case 0:        // Invalid Drive Or Directory
                          {
                            WinAlarm (HWND_DESKTOP, WA_ERROR);
                            FillDrvListBox (hwnd);
                            GetCurrentPath (hwnd, szCurrentPath);
                            FillDirListBox (hwnd);
                            FillFileListBox (hwnd);
                            WinSetDlgItemText (hwnd, IDD_FILEEDIT, szFileMask);
                            return FALSE;
                          }
                        case 1:        // Empty Or No Filename
                          {
                            FillDrvListBox (hwnd);
                            GetCurrentPath (hwnd, szCurrentPath);
                            FillDirListBox (hwnd);
                            FillFileListBox (hwnd);
                            WinSetDlgItemText (hwnd, IDD_FILEEDIT, szFileMask);
                            return FALSE;
                          }
                        case 2:        // Valid Path & Filename
                          {
                            strcpy (pFileName, szCurrentPath);
                            WinDismissDlg (hwnd, TRUE);
                            return FALSE;
                          }
                        case 3:        // Wildcards in filename
                          {
                            strcpy (szFileMask, szBuffer); // Save as mask
                            FillDrvListBox (hwnd);
                            GetCurrentPath (hwnd, szCurrentPath);
                            FillDirListBox (hwnd);
                            FillFileListBox (hwnd);
                            WinSetDlgItemText (hwnd, IDD_FILEEDIT, szFileMask);
                            return FALSE;
                          }
                      }
                    break;
                  }
                case DID_CANCEL:       // Clicked "CANCEL"
                  {
                    WinDismissDlg (hwnd, FALSE);
                    return FALSE;
                  }
              }
            break;
          }
      }
    return WinDefDlgProc (hwnd, msg, mp1, mp2);
  }

#pragma subtitle ("Parse File Name")
#pragma page()
/********************************************************************
**                       Parse File Name                           **
**                                                                 **
** Input:    pcOut -- Pointer to parsed file specification.        **
**           pcIn  -- Pointer to raw file specification.           **
**                                                                 **
** Returns:  0 -- pcIn had invalid drive or directory.             **
**           1 -- pcIn was empty or had no filename.               **
**           2 -- pcOut points to drive, full dir, and file name.  **
**           3 -- pcIn contains wildcard characters.               **
**                                                                 **
** Changes current drive and directory per pcIn string.            **
********************************************************************/
USHORT APIENTRY ParseFileName (PCHAR pcOut, PCHAR pcIn)
  {
  PCHAR  pcLastSlash, pcFileOnly;
  ULONG  ulDriveMap;
  USHORT usDriveNum, usDirLen = 64;

    strupr (pcIn);

    if (pcIn [0] == '\0')  // If input string is empty, return 1
      {
        return 1;
      }

    if (pcIn [1] == ':')  // Get drive from input string or current drive
      {
        if (DosSelectDisk (pcIn [0] - '@'))
          {
            return 0;
          }
        pcIn += 2;
      }

    DosQCurDisk (&usDriveNum, &ulDriveMap);
    *pcOut++ = (CHAR) usDriveNum + (UCHAR) '@';
    *pcOut++ = ':';
    *pcOut++ = '\\';
    *pcOut   = '\0';

    if (strpbrk (pcIn, "*?"))          // Wildcard chars in file name
      {
        return 3;
      }

    if (pcIn [0] == '\0') // If rest of string is empty, return 1
      {
        return 1;
      }

    // Search for last backslash.  If none, could be directory.

    if (NULL == (pcLastSlash = strrchr (pcIn, '\\')))
      {
        if (!DosChDir (pcIn, 0L))
          {
            return 1;
          }

        // Otherwise, get current dir & attach input filename

        DosQCurDir (0, pcOut, &usDirLen);

        if (strlen (pcIn) > 12)
          {
            return 0;
          }

        if (*(pcOut + strlen (pcOut) - 1) != '\\')
          {
            strcat (pcOut++, "\\");
          }

        strcat (pcOut, pcIn);
        return 2;
      }

    if (pcIn == pcLastSlash) // If the only backslash is at beginning, change to root
      {
        DosChDir ("\\", 0L);

        if (pcIn [1] == '\0')
          {
            return 1;
          }

        strcpy (pcOut, pcIn + 1);
        return 2;
      }

    *pcLastSlash = '\0';

    if (DosChDir (pcIn, 0L)) // Attempt to change directory -- Get current dir if OK
      {
        return 0;
      }

    DosQCurDir (0, pcOut, &usDirLen);

    pcFileOnly = pcLastSlash + 1; // Append input filename, if any

    if (*pcFileOnly == '\0')
      {
        return 1;
      }

    if (strlen (pcFileOnly) > 12)
      {
        return 0;
      }

    if (*(pcOut + strlen (pcOut) - 1) != '\\')
      {
        strcat (pcOut++, "\\");
      }

    strcat (pcOut, pcFileOnly);
    return 2;
  }
