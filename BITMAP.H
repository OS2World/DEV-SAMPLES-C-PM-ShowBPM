/********************************************************************
**                       BITMAP Header File                        **
********************************************************************/
#define  ERRBMP_RC_END_OF_FILE    (-10)        // RC = End Of File
#define  ERRBMP_RC_DEVOPENDC      (-11)        // RC = DevOpenDC failed
#define  ERRBMP_RC_CREATEPS       (-12)        // RC = GpiCreatePS failed
#define  ERRBMP_RC_CREATEBM       (-13)        // RC = GpiCreateBitmap failed
#define  ERRBMP_RC_SETBM          (-14)        // RC = GpiSetBitmap failed
#define  ERRBMP_RC_SETBMBITS      (-15)        // RC = GpiSetBitmapBits failed

#define  ERRBMP_DOSOPEN_FAILED    (0x8001)     // DosOpen Failed
#define  ERRBMP_HDRREAD_FAILED    (0x8002)     // Bitmap header read failed
#define  ERRBMP_INVALID_FILETYPE  (0x8003)     // Invalid Bitmap filetype
#define  ERRBMP_BMPINFO_FAILED    (0x8004)     // BMP Info alloc/read failed
#define  ERRBMP_SCANBUF_FAILED    (0x8005)     // Scanline buffer alloc failed
#define  ERRBMP_SCANLINE_FAILED   (0x8006)     // Scanline read failed

#define  MAX_COLORS     (256)          // Maximum supported colors

/********************************************************************
**                 Define BMPCTL Data Structure                    **
********************************************************************/
typedef struct _BMPCTL                 // Define Bitmap Control area
  {
    TID              tidThread;        // Thread ID
    HAB              habThread;        // Thread HAB
    HWND             hwndClient;       // Client window to notify
    ERRORID          Err;              // PM Error ID
    HSEM             hTriggerSem;      // Thread trigger semaphore handle
    PCHAR            pFileName;        // Bitmap file name ptr
    HDC              hdcBitmap;        // Bitmap device memory context handle
    HPS              hpsBitmap;        // Bitmap Presentation Space handle
    HBITMAP          hbmBitmap;        // Bitmap handle
    USHORT           cColors;          // Number of colors in image
    USHORT           cbLine;           // Scanline size in bytes
    USHORT           cLines;           // Number of scanlines
    PVOID            pScan;            // Bitmap scanline buffer ptr
    PBITMAPINFO      pBmi;             // Bitmap info and color table ptr
    BITMAPFILEHEADER Hdr;              // Bitmap file info header
    LONG    lLogColorTbl[MAX_COLORS];  // Logical Color Table
  } BMPCTL, FAR *PBMPCTL;

/********************************************************************
**                 Define BITMAP Function Calls                    **
********************************************************************/
VOID DestroyBitMap (PBMPCTL pCtl);
USHORT CreateBitmap (PBMPCTL pCtl);
