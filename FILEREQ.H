/********************************************************************
**                      FILEREQ Header File                        **
********************************************************************/

MRESULT EXPENTRY OpenDlgProc (HWND hwnd, USHORT msg, MPARAM mp1, MPARAM mp2);
USHORT APIENTRY ParseFileName (PCHAR pcOut, PCHAR pcIn);
VOID SetOpenMask (PCHAR pSearchMask);
