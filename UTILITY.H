/********************************************************************
**                      UTILITY Header File                        **
********************************************************************/

USHORT DisplayMsgBox (BOOL Beep, PCHAR Msg, PCHAR Title, USHORT Style);
VOID ShowPMError (HAB hab, USHORT id);
VOID ShowDOSError (HAB hab, USHORT rc, USHORT id);
VOID ShowDebug (HAB hab, SHORT rc, PCHAR msg);
VOID DisplayBusyPtr (BOOL Busy);
VOID EnableMenuItem (SHORT idItem, BOOL fEnable);
