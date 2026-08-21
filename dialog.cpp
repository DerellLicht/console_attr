//**************************************************************************
//  Console Palette Changer, Copyright (c) 2006-2007  Daniel D. Miller
//  This application and all associated source code is hereby declared
//  to be in the public domain.
//  
//  dialog.cpp
//  Windows user interface functions for Console Palette Changer.
//  
//  Written by:   Daniel D. Miller
//  
//**************************************************************************
// 1.00  09/26/06    Original version, derived from many other sources
// 1.01  09/26/06    Add starting directory for launched shell
// 1.02  09/29/06    Switch to different binary file format
// 1.03  10/09/06    Add button for help dialog
// 1.04  02/23/07    Avoid multiple instances
//**************************************************************************

static char szClassName[] = "Console Palette Changer V1.04" ;

#include <windows.h>
#include <stdio.h>   //  sprintf, needed for double
#include <stdlib.h>  //  _MAX_PATH
#include <sys/stat.h>
#undef  WINVER
#define WINVER 0x0500
#include <shlobj.h>
#include <htmlhelp.h>

#include "resource.h"
#include "console.attr.h"
#include "ezfont.h"
#include "regif.h"

//lint -e527   code Unreachable

#define BUFFER_SIZE 256
static unsigned dirty_flag = 0 ;

static char szText[BUFFER_SIZE];
static HBRUSH g_hbrBackground = (HBRUSH) (COLOR_WINDOW + 1) ;
static HINSTANCE hInst;
static NOTIFYICONDATA NotifyIconData;

static RECT DialogRect;
static RECT Edit5Rect;

// static HWND PaletteEdithwnd[16] = {
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ;

static DWORD PaletteEditID[16] = {
IDC_PEBTN00, IDC_PEBTN01, IDC_PEBTN02, IDC_PEBTN03,
IDC_PEBTN04, IDC_PEBTN05, IDC_PEBTN06, IDC_PEBTN07,
IDC_PEBTN08, IDC_PEBTN09, IDC_PEBTN10, IDC_PEBTN11,
IDC_PEBTN12, IDC_PEBTN13, IDC_PEBTN14, IDC_PEBTN15 } ;

static TCHAR szPalFilter[]  = TEXT ("Palette Files (*.PLT)\0*.plt\0")  \
                              TEXT ("All Files (*.*)\0*.*\0\0") ;
static TCHAR szExecFilter[] = TEXT ("Executable Files (*.EXE)\0*.exe\0")  \
                              TEXT ("All Files (*.*)\0*.*\0\0") ;
static TCHAR szDirFilter[]  = TEXT ("All Files (*.*)\0*.*\0\0") ;

static registry_iface inireg("dialog") ;
//********************************************************
//  ini variables
//********************************************************
static char palette_filename[MAX_PATH] = "dos.plt" ;
static char cmd_proc_filename[MAX_PATH] = "C:\\WINDOWS\\SYSTEM32\\cmd.exe" ;  //  read this from system
static char starting_path[MAX_PATH] = "C:\\download" ;
static double brighten = 3.0 ;

static BOOL CALLBACK InitProc( HWND hDlgWnd, UINT Message, WPARAM wParam, LPARAM lParam ) ;

#define CrSampleFont EzCreateFont (hdc, "Courier New", 150, 0, EZ_ATTR_BOLD, 0, TRUE)
#define CrDataFont EzCreateFont (hdc, "Bodacious-Normal", 150, 0, 0, 0, TRUE)
// #define CrLabelFont EzCreateFont (hdc, "Times New Roman", 150, 0, 0, 0, TRUE)

//*******************************************************************************
// void debug_dump_rect(char *msg, RECT *drect)
// {
//    wsprintf(tempstr, "%s: L%u, T%u, R%u, B%u", msg,
//       drect->left, drect->top, drect->right, drect->bottom) ;
//    OutputDebugString(tempstr) ;
// }

//*****************************************************************
void show_message(HWND hwnd, char* msg)
{
   MessageBox(hwnd, msg, "warning message", MB_OK | MB_ICONINFORMATION) ;
}

//*****************************************************************
static char chmname[1024] ;
// unsigned chmmode = 0 ;
// unsigned chm_exists = 0 ;

static int find_chm_location(void)
{
   static char cmd_line_copy[260] ;
   static char curpath[1024] ;
   char refstr[4] = { ' ', '\n', '\r', 0 } ;
   char *strptr ;
   // struct stat st ;

   //  see if we can find and show parent filename
   // if (GetModuleFileName(g_hInst, filename, sizeof(filename)) == 0) {
   if (GetModuleFileName(NULL, chmname, sizeof(chmname)) != 0) {
      strptr = strrchr(chmname, '.') ;
      //  if no extention on existing path, just concat the name
      if (strptr == 0) {
         strcat(chmname, ".chm") ;
         // chmmode = 1 ;  //  DEBUG 
      } 
      //  if extension *is* present, overwrite with .chm
      else {
         strcpy(strptr, ".chm") ;
         // chmmode = 2 ;  //  DEBUG 
      }
   } 
   //  if GetModuleFileName fails, then we'll use the old method
   else {
      //  find our chm file.
      //  In some cases (Win98SE), GetCommandLine() returns its
      //  result enclosed in double-quotes!!  We need to deal with that.
      strncpy(cmd_line_copy, GetCommandLine(), sizeof(cmd_line_copy)) ;
      // fprintf(fd, "cmdline:[%s]\n", cmd_line_copy) ;
      if (cmd_line_copy[0] == '"') {
         strcpy(cmd_line_copy, &cmd_line_copy[1]) ;
         // fprintf(fd, "cmdline1:[%s]\n", cmd_line_copy) ;
         strptr = cmd_line_copy ;
         while (1) {
            if (*strptr == '"') {
               strcpy(strptr, strptr+1) ;
               break;
            }
            if (*strptr == 0)
               break;
            strptr++ ;
         }
         // fprintf(fd, "cmdline2:[%s]\n", cmd_line_copy) ;
      }

      //  the results of 
      strptr = strpbrk(cmd_line_copy, refstr) ;
      if (strptr != 0) {
         *strptr = 0 ;
      }
      //  interesting lessons from WinNT 4.0:
      //  If the OS is WinNT 4.0, and;
      //  If the executable file is located in the current directory,
      //  THEN:
      //    argv[0] does NOT contain the fully-qualified
      //    path of the EXE, it *only* contains the EXE name.
      //    In all other situations, argv[0] is fully qualified!!
      //  
      //  P.S.  While we're here, derive default chm filename also
      strptr = strrchr(cmd_line_copy, '\\') ;
      //  no path present
      if (strptr == 0) {
         //*******************************************************************
         //  The original version of this program passed NULL to 
         //  SearchPath(), and failed because the actual search path
         //  was not what was expected:
         //*******************************************************************
         //  Note from MSDN:
         //  
         //  Pointer to a null-terminated string that specifies the path to be 
         //  searched for the file.  If this parameter is NULL, the function 
         //  searches for a matching file in the following directories in the 
         //  following sequence: 
         //  
         //  - The directory from which the application loaded. 
         //    (DDM: I wish I could find this directly!!)
         //  - The current directory. 
         //  - Windows 95: The Windows system directory. 
         //    Use the GetSystemDirectory function to get the path of this dir.
         //  - Windows NT: The 32-bit Windows system directory. 
         //    Use the GetSystemDirectory function to get the path of this 
         //    directory. The name of this directory is SYSTEM32. 
         // 
         //  - Windows NT: The 16-bit Windows system directory. There is no 
         //    Win32 function that obtains the path of this directory, but 
         //    it is searched. The name of this directory is SYSTEM. 
         //  - The Windows directory. Use theGetWindowsDirectory function to get 
         //    the path of this directory. 
         //  - The directories that are listed in the PATH environment variable. 
         //*******************************************************************
         //  Later note:
         //  Note that using getenv("PATH") directly does *not* 
         //  search the current directory!!  
         //  Thus, I'll manually prepend it here...
         //*******************************************************************
         wsprintf(curpath, ".;%s", getenv("PATH")) ;
         SearchPath(curpath, cmd_line_copy, ".exe", _MAX_PATH, chmname, NULL) ;
      }
      else {
         //  pick up chm filename
         strcpy(chmname, cmd_line_copy) ;
      }
      strptr = strrchr(chmname, '\\') ;
      if (strptr != 0) {
         char *p ;

         strptr++ ;  //  skip past the backslash
         p = strchr(strptr, '.') ;
         //  if no extention on existing path, just concat the name
         if (p == 0) {
            strcat(chmname, ".chm") ;
            // chmmode = 1 ;  //  DEBUG 
         } 
         //  if extension *is* present, overwrite with .chm
         else {
            strcpy(p, ".chm") ;
            // chmmode = 2 ;  //  DEBUG 
         }
      } else {
         //  this really shouldn't happen
         strcat(chmname, ".chm") ;
         // chmmode = 3 ;  //  DEBUG 
      }
   }

   //  lastly, see if file already exists
   // int result = stat(chmname, &st) ;
   // chm_exists = (result == 0) ? 1 : 0 ;
   return 0;
}

//****************************************************************************
static void dprints(HDC hdc, int x, int y, unsigned attr, char *str)
{
   // if (attr < 0x80) {
   //    SetTextColor(hdc, dos_cref[attr & 0xF]) ;
   //    SetBkColor  (hdc, dos_cref[(attr >> 4) & 0xF]) ;
   //    TextOut (hdc, x, y, str, strlen(str));
   // } else {
      SetBkMode(hdc, TRANSPARENT) ;
      SetTextColor(hdc, attr) ;
      TextOut (hdc, x, y, str, strlen(str));
      SetBkMode(hdc, OPAQUE) ;
   // }
}         

//*****************************************************************
static void dprints_centered_x(HWND hwnd, LONG y, unsigned attr, char *str)
{
   HDC hdc ;
   SIZE ssize ;
   LONG xt ;

   hdc = GetDC (hwnd) ;
   SelectObject (hdc, CrDataFont) ;
   GetTextExtentPoint32(hdc, str, strlen(str), &ssize) ;
   xt = (DialogRect.right - ssize.cx) / 2 ;
   dprints(hdc, xt, y-(ssize.cy/2), attr, str) ;
   DeleteObject (SelectObject (hdc, GetStockObject (SYSTEM_FONT)));
   ReleaseDC (hwnd, hdc) ;
}

//**************************************************************************
BOOL IsAppRunning(void)
{
  HANDLE hMutex = CreateMutex(NULL, TRUE, "console_attr");
  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
      CloseHandle(hMutex);
      return TRUE;
  }
 
  return FALSE;
}

//**************************************************************************
INT WINAPI WinMain( HINSTANCE  hInstance,
                    HINSTANCE  hPrevInstance,
                    LPSTR      lpCmdLine,
                    INT        nCmdShow )
{
   hInst = hInstance;

  if (IsAppRunning()) {
       MessageBox(NULL, "The program is already running", "Error", MB_OK | MB_ICONERROR);
       return 0;
   }
   find_chm_location() ;

   //****************************************************************
   HWND hWnd = CreateDialog( hInstance,
                             (LPCTSTR) "DIALOG_NAME",
                             NULL,
                             (DLGPROC)InitProc );
   MSG    Msg;
   //  remove this line to start out minimized to toolbar
   // ShowWindow(hWnd, nCmdShow) ;

   while(GetMessage(&Msg, NULL,0,0) == TRUE)
   {
       if(!IsDialogMessage(hWnd,&Msg))
       {
           TranslateMessage(&Msg);
           DispatchMessage(&Msg);
       }
   }
   return Msg.wParam;
}  //lint !e715  func params not used

//*******************************************************************************
static int get_cmd_proc_name(char *cmdpath)
{
   int result ;
   struct stat st ;
   GetSystemDirectory(cmdpath, _MAX_PATH) ;
   int slen = strlen(cmdpath) ;
   strcat(cmdpath, "\\cmd.exe") ;
   result = stat(cmdpath, &st) ;
   if (result == 0) 
      return 1;

   *(cmdpath+slen) = 0 ;
   strcat(cmdpath, "\\command.com") ;
   result = stat(cmdpath, &st) ;
   if (result == 0) 
      return 1;
   return 0;
}

//*******************************************************************************
//  well, for some reason, INI files don't work at all in this situation.
//  GetPrivateProfileString() doesn't return the strings in the file
//  at all, even though the file is there and corrent.
//  Okay, I see what the issue is.  
//  First, the section (AppName) entry has to exist in the file.  
//  Second, the file is *not* created if it does not already exist!!
//  I thought it was... 
//*******************************************************************************
static void read_config_data(void)
{

   //  first, see if ini file exists...
   //  Note: GetSystemDirectory()
   if (!inireg.ini_file_exists()) {
      //  find default command processor
      char *strptr ;
      char rcdtemp[_MAX_PATH] ;
      int found = get_cmd_proc_name(rcdtemp) ;
      if (found) {
          strncpy(cmd_proc_filename, rcdtemp, sizeof(cmd_proc_filename)) ;
      }

      // OutputDebugString((result) ? rcdtemp : "no cmd proc found") ;
      //  find default palettes directory.
      //  If anything fails at any point, 
      //  just leave the default name in place
      if (GetModuleFileName(NULL, rcdtemp, sizeof(rcdtemp)) == 0) 
      {
         OutputDebugString("fault A") ;
         goto nevermind;
      }
      struct stat st ;
      //  strip off exe name and leave the path
      strptr = strrchr(rcdtemp, '\\') ;
      if (strptr == 0) 
      {
         OutputDebugString("fault B") ;
         goto nevermind;
      }
      // strcpy(strptr, ".ini") ;
      strptr++ ;
      strcpy(strptr, "palettes") ;
      if (stat(rcdtemp, &st) != 0)
      {
         OutputDebugString("fault C") ;
         goto nevermind;
      }
      //  okay, the path exists... append the default DOS.PAL 
      //  and proceed with generating data.
      strcat(rcdtemp, "\\DOS.PLT") ;
      strncpy(palette_filename, rcdtemp, sizeof(palette_filename)) ;
   } 
nevermind:
   inireg.get_param("cmdprog", cmd_proc_filename) ;
   inireg.get_param("palette", palette_filename) ;
   inireg.get_param("startpath", starting_path) ;
   inireg.get_param("brighten", &brighten) ;

   // wsprintf(tempstr, "cmdprog=%s", cmd_proc_filename) ;
   // OutputDebugString(tempstr) ;
   // wsprintf(tempstr, "palette=%s", palette_filename) ;
   // OutputDebugString(tempstr) ;
   // wsprintf(tempstr, "startpath=%s", starting_path) ;
   // OutputDebugString(tempstr) ;
}

//************************************************************************
// static void show_button_area(HDC hdc, int xl, int yu, int xr, int yl, COLORREF Color)
// {
//    HBRUSH hBrush ;
//    RECT   rect ;
// 
//    SetRect (&rect, xl, yu, xr, yl) ;
//    hBrush = CreateSolidBrush(Color) ;
//    FillRect (hdc, &rect, hBrush) ;
//    DeleteObject (hBrush) ;
// }

/************************************************************************/
// ;* Description: Draw line from (x0,y0) to (x1,y1) using color 'Color'   *
/************************************************************************/
static void Box(HDC hdc, int x0, int y0, int x1, int y1, COLORREF Color)
{
   static HPEN hPen ;

   hPen = CreatePen(PS_SOLID, 1, Color) ;
   SelectObject(hdc, hPen) ;

   MoveToEx(hdc, x0, y0, NULL) ;
   LineTo  (hdc, x1, y0) ;
   LineTo  (hdc, x1, y1) ;
   LineTo  (hdc, x0, y1) ;
   LineTo  (hdc, x0, y0) ;

   SelectObject(hdc, GetStockObject(BLACK_PEN)) ;  //  deselect my pen
   DeleteObject (hPen) ;
}

//*******************************************************************************
static void update_edit5(HWND hDlgWnd, HDC hdc) 
// void update_edit5(HWND hDlgWnd) 
{
   // HWND hEditwnd = GetDlgItem(hDlgWnd, IDC_EDIT5);
   // HDC hdc = GetDC(hDlgWnd) ;
   if (hdc == NULL) {
      wsprintf(tempstr, "update_edit5: GetDC: %s", get_system_message()) ;
      OutputDebugString(tempstr) ;
   }

   //*********************************
   //  Clear edit window
   //*********************************
   RECT rect ;
   rect.left   = Edit5Rect.left + 4 ;
   rect.right  = Edit5Rect.right - 4 ;
   rect.top    = Edit5Rect.top + 4 ;
   rect.bottom = Edit5Rect.bottom - 4 ;

   // GetWindowRect(hEdit5wnd, &Edit5Rect) ;
   // debug_dump_rect("in update_edit5 (Window)", &Edit5Rect) ;
   // debug_dump_rect("in update_edit5 (Client)", &rect) ;

   Box(hdc, Edit5Rect.left, Edit5Rect.top, Edit5Rect.right, Edit5Rect.bottom, 0);
   // show_message(hDlgWnd, "does this draw??");
   // ReleaseDC (hDlgWnd, hdc) ;
   // Box(hdc, rect.left, rect.top, rect.right, rect.bottom, 0);
   // HDC hdc = GetDC(hEdit5wnd) ;
   HBRUSH hBrush = CreateSolidBrush (curr_attr[0]) ;
   FillRect (hdc, &rect, hBrush) ;
   DeleteObject (hBrush) ;

   // COLORREF oldcr = SetBkColor(hdc, curr_attr[0]);
   // ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, "", 0, 0);
   // SetBkColor(hdc, oldcr);

   // wsprintf(tempstr, "hwnd5 (paint): l%u, t%u, r%u, b%u",
   //    rect.left,
   //    rect.top,
   //    rect.right,
   //    rect.bottom) ;
   // OutputDebugString(tempstr) ;   //  this is how to use it

   //*************************************************************
   //   draw sample texts in Edit5 area
   //*************************************************************
   SelectObject (hdc, CrSampleFont) ;
   // TEXTMETRIC tm ;
   // GetTextMetrics(hdc, &tm) ;
   // diffY = tm.tmHeight + tm.tmExternalLeading ;

   SIZE ssize ;
   GetTextExtentPoint32(hdc, " AttribXX ", 10, &ssize) ;

   unsigned xboundary = (rect.right - rect.left) - ssize.cx ;
   unsigned j ;   
   unsigned row = rect.top + 18 ;
   unsigned xoffset = 16 ;
   unsigned col = rect.left + xoffset ;
   char str[20] ;
   for (j=1; j<16; j++) {
      sprintf(str, " Attrib%02u ", j) ;
      // SetBkMode(hdc, TRANSPARENT) ;
      SetTextColor(hdc, curr_attr[j]);
      SetBkColor(hdc, curr_attr[0]);
      TextOut (hdc, col, row, str, strlen(str));
      // SetBkMode(hdc, OPAQUE) ;
      //  update cursor
      col += ssize.cx ;
      if (col > xboundary) {
         col = rect.left + xoffset ;
         row += ssize.cy ;
      }
   }
   DeleteObject (SelectObject (hdc, GetStockObject (SYSTEM_FONT)));

   // wsprintf(tempstr, "yl=%u", yl) ; //  yl = 485
   // OutputDebugString(tempstr) ;
   // ReleaseDC (hDlgWnd, hdc) ;
}  //lint !e715  func params not used

//****************************************************************
static int select_color(unsigned idx)
{
   static CHOOSECOLOR cc ;
   static COLORREF    crCustColors[16] ;

   ZeroMemory(&cc, sizeof(cc));
   cc.lStructSize    = sizeof (CHOOSECOLOR) ;
   // cc.rgbResult      = RGB (0x80, 0x80, 0x80) ;
   cc.rgbResult      = curr_attr[idx] ;
   cc.lpCustColors   = crCustColors ;
   cc.Flags          = CC_RGBINIT | CC_FULLOPEN ;

   if (ChooseColor(&cc) == TRUE) {
      // return cc.rgbResult ;   //  contains the selected color
      curr_attr[idx] = cc.rgbResult ;   //  contains the selected color
      return 1 ;
   } else {
      return 0 ;
   }
}

//**************************************************************************
// A function to create a button
//**************************************************************************
static HWND CreateButtonEx(char *tempText, DWORD flags, int x, int y, int width, int height,
   int identifier, HWND hwnd, HINSTANCE g_hInst)
{
   HWND hButtonTemp;

   hButtonTemp = CreateWindowEx(flags, "BUTTON",
      tempText,
      WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
      x, y, width, height, hwnd, (HMENU) identifier, g_hInst, NULL);

   return hButtonTemp;
   // BS_OWNERDRAW
}

//************************************************************************
static void Solid_Rect(HDC hdc, int xl, int yu, int xr, int yl, COLORREF Color)
{
   HBRUSH hBrush ;
   RECT   rect ;

   SetRect (&rect, xl, yu, xr, yl) ;
   hBrush = CreateSolidBrush (Color) ;
   FillRect (hdc, &rect, hBrush) ;
   DeleteObject (hBrush) ;
}

//**************************************************************************
static BOOL CALLBACK InitProc( HWND hDlgWnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
   static HMENU hMenu;
   POINT MouseCoordinates;
   HDC hdc ;
   PAINTSTRUCT ps;

   switch(Message) {
   case WM_CTLCOLORDLG:
       return (LONG) g_hbrBackground;

   case WM_CTLCOLORSTATIC:
   {
       HDC hdcStatic = (HDC)wParam;
       SetTextColor(hdcStatic, RGB(0, 0, 0));
       //  Error: this was passing a color index, NOT a COLORREF !!
       // SetBkColor(hdcStatic, (COLOR_WINDOW + 1));
       SetBkColor(hdcStatic, (GetSysColor(COLOR_WINDOW+1)));
       SetBkMode(hdcStatic, TRANSPARENT);
       return (LONG) g_hbrBackground;
   }
   break;

   case WM_INITDIALOG:
      {
      RECT DesktopRect;
      RECT Button5Rect;
      RECT Button6Rect;

      //  at this point, we should resolve %system_path% into the
      //  real system path, to properly set up cmd_proc_filename

      //  read configuration *before* creating edit fields
      read_config_data() ;

      SetWindowText(hDlgWnd, szClassName) ;
      SetDlgItemText(hDlgWnd, IDC_HEADER, szClassName) ;

      //  do some setup stuff before creating dialog
      int result = read_palette_file(palette_filename, brighten) ;
      if (result != 0) {
         // show_message(NULL, "cannot read palette file") ;
         wsprintf(tempstr, "read_palette_file: %s: %d\n", palette_filename, result) ;
         OutputDebugString(tempstr) ;
         restore_default_colors();
      }
      build_console_list() ;
      enum_all_consoles();

      //  debug dumps
      // dump_palette_data() ;
      // dump_console_list() ;
      //  get relevant coordinates
      HWND hBtn5wnd = GetDlgItem(hDlgWnd, IDC_BUTTON5);
      HWND hBtn6wnd = GetDlgItem(hDlgWnd, IDC_BUTTON6);
      GetWindowRect(GetDesktopWindow(), &DesktopRect);
      GetWindowRect(hDlgWnd, &DialogRect);
      //  manually build a graphic area where Edit5 was previously
      //  get button info to derive Edit5 graphics region.
      //  NOTE:  These must be *immediately* after the previous
      //  GetWindowRect() calls, or they will return global
      //  coordinates instead of "relative to hDlgWnd" coordinates!!
      GetWindowRect(hBtn5wnd, &Button5Rect) ;
      GetWindowRect(hBtn6wnd, &Button6Rect) ;

      // debug_dump_rect("DialogRect (Window)", &DialogRect) ;

      SetWindowPos( hDlgWnd, 
                    HWND_TOP, 
                    (DesktopRect.right - DialogRect.right) / 2,
                    (DesktopRect.bottom - DialogRect.bottom) / 2,
                    0,0, SWP_NOSIZE );

      SendMessage(hDlgWnd, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON)));
      SendMessage(hDlgWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON)));

      // debug_dump_rect("Button5Rect (Window)", &Button5Rect) ;
      // debug_dump_rect("Button6Rect (Window)", &Button6Rect) ;
      Edit5Rect.left   = Button5Rect.left - 1 ;
      Edit5Rect.right  = Button6Rect.right - 1 ;
      Edit5Rect.top    = Button5Rect.bottom - 5;
      Edit5Rect.bottom = Edit5Rect.top + 120 ;
                    
      //*************************************************************
      //  draw graphics templates for palette-editing buttons
      //*************************************************************

      //*************************************************************
      //  find client area of Edit5 region
      //*************************************************************
      RECT rect ;
      rect.left   = Edit5Rect.left + 4 ;
      rect.right  = Edit5Rect.right - 4 ;
      rect.top    = Edit5Rect.top + 4 ;
      rect.bottom = Edit5Rect.bottom - 4 ;

      unsigned gap = 12 ;
      // unsigned e5width = right - Edit5Rect.left ;
      unsigned e5width = rect.right - rect.left ;
      unsigned bwidth = (e5width - (15 * gap)) / 16 ;
      unsigned wprime = e5width - bwidth ;
      unsigned yu = Edit5Rect.bottom + 40 ;
      unsigned yl = yu + 25 ;
      for (unsigned xx=0; xx<16; xx++) {
         unsigned xl = wprime * xx / 15 ;
         xl += rect.left ;
         unsigned xr = xl + bwidth ;
         DWORD flags = 0 ;
         // show_button_area(hdc, xl, yu, xr, yl, curr_attr[xx-1]) ;
         wsprintf(tempstr, "[%02u]", xx) ;
         // PaletteEdithwnd[xx] = CreateButtonEx(tempstr, flags, 
         CreateButtonEx(tempstr, flags, xl, yu, 
            (xr - xl + 1), (yl - yu + 1),
            PaletteEditID[xx], hDlgWnd, hInst);
         // wsprintf(tempstr, "create button %02u, hwnd=%08X", xx, (unsigned) PaletteEdithwnd[xx]) ;
         // OutputDebugString(tempstr) ;
      }

      //**********************************************************
      //  do other config tasks *after* creating fields,
      //  so we can display status messages.
      //**********************************************************

      // Create status bar (source resides in statbar.cpp).
      // hwndStatusBar = InitStatusBar (hDlgWnd, NO_RESIZE) ;
      // Statusbar_ShowMessage (msgtext) ;   //  this is how to use it

      // create tray menu
      hMenu = LoadMenu (hInst, MAKEINTRESOURCE (ID_TRAYMENU));

      // put the icon into a system tray
      NotifyIconData.cbSize = sizeof (NOTIFYICONDATA);
      NotifyIconData.hWnd = hDlgWnd;
      NotifyIconData.uID = 0;
      NotifyIconData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
      NotifyIconData.uCallbackMessage = WM_USER; // tray events will generate WM_USER event
      // NotifyIconData.hIcon = (HICON) LoadImage (hInstance, MAKEINTRESOURCE (IDAPPLICON), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR); // load 16 x 16 pixels icon
      NotifyIconData.hIcon = (HICON) LoadIcon (hInst, MAKEINTRESOURCE (IDI_ICON));
      lstrcpy (NotifyIconData.szTip, szClassName); // max 64 characters

      Shell_NotifyIcon (NIM_ADD, &NotifyIconData);

      SetFocus(hDlgWnd) ;
      // InvalidateRect(hDlgWnd, NULL, TRUE);
      // UpdateWindow(hDlgWnd) ;
      // InvalidateRect(hDlgWnd, NULL, TRUE);
      return FALSE ;
   }
   break;

   case WM_SETFOCUS:
      // wsprintf(tempstr, "%u: WM_SETFOCUS", (unsigned) hDlgWnd) ;
      // OutputDebugString("WM_SETFOCUS") ;
      
      // InvalidateRgn (hDlgWnd, NULL, FALSE);
      InvalidateRect(hDlgWnd, NULL, TRUE);
      UpdateWindow(hDlgWnd) ;
      return FALSE ;
      // return TRUE;

   case WM_PAINT:
      // wsprintf(tempstr, "%u: WM_PAINT", (unsigned) hDlgWnd) ;
      // OutputDebugString(tempstr) ;
      // OutputDebugString("WM_PAINT") ;
      
      hdc = BeginPaint (hDlgWnd, &ps);
      SetDlgItemText(hDlgWnd, IDC_EDIT2, cmd_proc_filename) ;
      SetDlgItemText(hDlgWnd, IDC_EDIT3, palette_filename) ;
      SetDlgItemText(hDlgWnd, IDC_EDIT4, starting_path) ;
      sprintf(tempstr, "%.1f", brighten) ;
      SetDlgItemText(hDlgWnd, IDC_EDIT1, tempstr) ;
      update_edit5(hDlgWnd, hdc) ;
      dprints_centered_x(hDlgWnd, Edit5Rect.bottom+20, 0, "Palette Edit Buttons");
      EndPaint (hDlgWnd, &ps);
      // return TRUE;   //  this causes WM_PINT to resend repeatedly
      return 0;

   case WM_ACTIVATE:
      switch (LOWORD(wParam)) {
      case WA_ACTIVE:
      case WA_CLICKACTIVE:  
         SetFocus(hDlgWnd) ;
         break;

      default:
         break;
      }
      return TRUE;
   break;

   // WM_DRAWITEM 
   // idCtl = (UINT) wParam;             // control identifier 
   // lpdis = (LPDRAWITEMSTRUCT) lParam; // item-drawing information 
   // typedef struct tagDRAWITEMSTRUCT {  // dis 
   //     UINT  CtlType; 
   //     UINT  CtlID; 
   //     UINT  itemID; 
   //     UINT  itemAction; 
   //     UINT  itemState; 
   //     HWND  hwndItem; 
   //     HDC   hDC; 
   //     RECT  rcItem; 
   //     DWORD itemData; 
   // } DRAWITEMSTRUCT; 

   case WM_DRAWITEM:
      {
      // wsprintf(tempstr, "WM_SETFOCUS: %u", (UINT) wParam) ;
      // OutputDebugString(tempstr) ;
      UINT hID = (UINT) wParam ;
      if (hID < IDC_PEBTN00  ||  hID > IDC_PEBTN15) 
         return FALSE;
      hID %= 16 ;
      LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT) lParam;
      hdc = lpdis->hDC ;

      //  figure out how to draw a solid rectangle inside the button.
      // FillSolidRect(rt, RGB(0, 0, 255));     //Fill button with blue color
      // Box(hdc, Edit5Rect.left, Edit5Rect.top, Edit5Rect.right, Edit5Rect.bottom, 0);
      Solid_Rect(hdc, lpdis->rcItem.left, lpdis->rcItem.top, lpdis->rcItem.right, lpdis->rcItem.bottom, curr_attr[hID]) ;

      UINT state = lpdis->itemState; //Get state of the button
      if ( (state & ODS_SELECTED) )    // If it is pressed
      {
         DrawEdge(hdc, &lpdis->rcItem, EDGE_SUNKEN, BF_RECT);    // Draw a sunken face
      }
      else
      {
         DrawEdge(hdc, &lpdis->rcItem, EDGE_RAISED,BF_RECT);   // Draw a raised face
      }
      //  draw the text
      // SetTextColor(hdc, curr_attr[hID]);     // Set the color of the caption to be yellow
      SetTextColor(hdc, 0);     // Set the color of the caption to be yellow
      // SetBkColor(hdc, (COLOR_WINDOW + 1));
      SetBkMode(hdc, TRANSPARENT);
      // CString strTemp;
      char strTemp[5] ;
      if (hID == 0) {
         wsprintf(strTemp, "BG") ;
      } else {
         wsprintf(strTemp, "%u", hID) ;
      }
      // GetWindowText(strTemp);    // Get the caption which have been set
      DrawText(hdc, strTemp, strlen(strTemp), &lpdis->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE);    // Draw out the caption
//       if ( (state & ODS_FOCUS ) )       // If the button is focused
//       {
//          // Draw a focus rect which indicates the user 
//          // that the button is focused
//          int iChange = 3;
//          rt.top += iChange;
//          rt.left += iChange;
//          rt.right -= iChange;
//          rt.bottom -= iChange;
//          dc.DrawFocusRect(rt);
//       }
      return TRUE;
      }         
   break;

   case WM_USER:
      // event genereted by a system tray - the type of tray event that
      // generated the message can be found in lParam
      switch (lParam)   {
      case WM_RBUTTONUP:
         // display a tray menu
         GetCursorPos (&MouseCoordinates);
         TrackPopupMenu (GetSubMenu (hMenu, 0), TPM_RIGHTALIGN | TPM_LEFTBUTTON, MouseCoordinates.x, MouseCoordinates.y, 0, hDlgWnd, NULL);
         return TRUE;

      case WM_LBUTTONUP:
         // show window as response to right-clicking the tray icon
         ShowWindow (hDlgWnd, SW_SHOWNORMAL);
         SetForegroundWindow (hDlgWnd);
         return TRUE;
      }  //lint !e744  switch lParam
   break;

   case WM_SYSCOMMAND:
      switch (LOWORD(wParam)) {
      case SC_MINIMIZE:
         ShowWindow (hDlgWnd, SW_HIDE);
         return TRUE;

      case SC_CLOSE:
         DestroyWindow (hDlgWnd);
         return TRUE;

      //  any WM_SYSCOMMAND that we don't handle ourselves,
      //  needs to be passed on to the system.
      //  Otherwise, the window isn't movable!!
      default:
         return DefWindowProc (hDlgWnd, Message, wParam, lParam);
         break;
      }  //  switch (LOWORD(wParam))
   break;
      
   case WM_COMMAND:
      switch(LOWORD(wParam)) {
      case IDCANCEL:
         DestroyWindow (hDlgWnd);
         // ExitProcess(0);
         return TRUE;
      break;

      case IDOK:
         MessageBox(hDlgWnd, "Ok", "Success", MB_OK);
         GetDlgItemText(hDlgWnd, IDC_DLG_TEXT, szText, BUFFER_SIZE);                    
         DestroyWindow (hDlgWnd);
         // ExitProcess(0);
         return TRUE;
      break;

      case IDC_BUTTON1:   //  update brightness
         GetDlgItemText(hDlgWnd, IDC_EDIT1, szText, BUFFER_SIZE);                    
         // strncpy(cmd_proc_filename, ofn.lpstrFile, sizeof(cmd_proc_filename)) ;
         brighten = strtod(szText, NULL) ;
         if (brighten < 1.0)
             brighten = 1.0 ;
         sprintf(szText, "%.1f", brighten) ;
         // IDC_EDIT3
         SetDlgItemText(hDlgWnd, IDC_EDIT1, szText) ;
         inireg.set_param("brighten", brighten) ;
         return TRUE;
      break;   //  end IDC_BUTTON1

      case IDC_BUTTON2:   //  select command processor
         {
         OPENFILENAME ofn;       // common dialog box structure
         char szFile[260];       // buffer for file name
         char dirFile[260];       // buffer for file name
         // HWND hwnd;              // owner window
         // HANDLE hf;              // file handle

         // Initialize OPENFILENAME
         ZeroMemory(&ofn, sizeof(ofn));
         ofn.lStructSize = sizeof(ofn);
         ofn.hwndOwner = hDlgWnd;
         ofn.lpstrFile = szFile;
         //
         // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
         // use the contents of szFile to initialize itself.
         //
         ofn.lpstrFile[0] = '\0';
         strcpy(dirFile, cmd_proc_filename) ;
         char *strptr = strrchr(dirFile, '\\') ;
         if (strptr != 0) {
            strptr++ ;  //  leave the backslash in place
            *strptr = 0 ;  //  strip off filename
            // OutputDebugString(dirFile) ;
         }
         ofn.lpstrInitialDir = dirFile ;
         ofn.nMaxFile = sizeof(szFile);
         // ofn.lpstrFilter = "All\0*.*\0EXE files\0*.EXE\0";
         ofn.lpstrFilter = szExecFilter ;
         ofn.lpstrDefExt = TEXT ("exe") ;
         ofn.nFilterIndex = 1;
         ofn.lpstrFileTitle = NULL;
         ofn.lpstrTitle = "select command processor" ;
         ofn.nMaxFileTitle = 0;
         ofn.lpstrInitialDir = NULL;
         ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

         // Display the Open dialog box. 
         if (GetOpenFileName(&ofn) == TRUE) {
            strncpy(cmd_proc_filename, ofn.lpstrFile, sizeof(cmd_proc_filename)) ;
            inireg.set_param("cmdprog", cmd_proc_filename) ;
            return TRUE;
         }
         }
      break;   //  end IDC_BUTTON2

      case IDC_BUTTON3:   //  select palette file
         {
         OPENFILENAME ofn;       // common dialog box structure
         char szFile[260];       // buffer for file name
         char oldFile[260];       // buffer for file name
         char dirFile[260];       // buffer for file name
         // HWND hwnd;              // owner window
         // HANDLE hf;              // file handle

         // Initialize OPENFILENAME
         ZeroMemory(&ofn, sizeof(ofn));
         ofn.lStructSize = sizeof(ofn);
         ofn.hwndOwner = hDlgWnd;
         ofn.lpstrFile = szFile;
         //
         // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
         // use the contents of szFile to initialize itself.
         //
         ofn.lpstrFile[0] = '\0';
         strcpy(dirFile, palette_filename) ;
         char *strptr = strrchr(dirFile, '\\') ;
         if (strptr != 0) {
            strptr++ ;  //  leave the backslash in place
            *strptr = 0 ;  //  strip off filename
            // OutputDebugString(dirFile) ;
         }
         ofn.lpstrInitialDir = dirFile ;
         ofn.nMaxFile = sizeof(szFile);
         ofn.lpstrFilter = szPalFilter ;
         ofn.nFilterIndex = 1;
         ofn.lpstrTitle = "select palette file" ;
         ofn.lpstrFileTitle = NULL ;
         ofn.lpstrDefExt = TEXT ("plt") ;
         // ofn.nMaxFileTitle = 0;
         // ofn.lpstrInitialDir = NULL;
         ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

         // Display the Open dialog box. 
         if (GetOpenFileName(&ofn) == TRUE) {
            // hf = CreateFile(ofn.lpstrFile, GENERIC_READ,
            //     0, (LPSECURITY_ATTRIBUTES) NULL,
            //     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
            //     (HANDLE) NULL);         
            strncpy(oldFile, palette_filename, sizeof(oldFile)) ;
            strncpy(palette_filename, ofn.lpstrFile, sizeof(palette_filename)) ;
            // SetDlgItemText(hDlgWnd, IDC_EDIT3, palette_filename) ;
            inireg.set_param("palette", palette_filename) ;

            int result = read_palette_file(palette_filename, brighten);
            if (result != 0) {
               wsprintf(tempstr, "%s: %s", palette_filename, strerror(result)) ;
               OutputDebugString(tempstr) ;
               strncpy(palette_filename, oldFile, sizeof(palette_filename)) ;
            }
            dirty_flag = 1 ;
            SetFocus(hDlgWnd) ;
            return TRUE;
         }
         }
      break;   //  end IDC_BUTTON3

      case IDC_BUTTON4:   //  select starting directory
         {
         OPENFILENAME ofn;       // common dialog box structure
         char szFile[260];       // buffer for file name
         char dirFile[260];       // buffer for file name

         // Initialize OPENFILENAME
         ZeroMemory(&ofn, sizeof(ofn));
         ofn.lStructSize = sizeof(ofn);
         ofn.hwndOwner = hDlgWnd;
         ofn.lpstrFile = szFile;
         //
         // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
         // use the contents of szFile to initialize itself.
         //
         ofn.lpstrFile[0] = '\0';
         strcpy(dirFile, starting_path) ;
         // char *strptr = strrchr(dirFile, '\\') ;
         // if (strptr != 0) {
         //    strptr++ ;  //  leave the backslash in place
         //    *strptr = 0 ;  //  strip off filename
         //    // OutputDebugString(dirFile) ;
         // }
         ofn.lpstrInitialDir = dirFile ;
         ofn.nMaxFile = sizeof(szFile);
         ofn.lpstrFilter = szDirFilter ;
         ofn.nFilterIndex = 1;
         ofn.lpstrTitle = "select console launch directory" ;
         ofn.lpstrFileTitle = NULL ;
         ofn.lpstrDefExt = TEXT ("") ;
         // ofn.nMaxFileTitle = 0;
         // ofn.lpstrInitialDir = NULL;
         ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

         // Display the Open dialog box. 
         if (GetOpenFileName(&ofn) == TRUE) {
            strncpy(starting_path, ofn.lpstrFile, sizeof(starting_path)) ;
            char *strptr = strrchr(starting_path, '\\') ;
            if (strptr != 0) {
               // strptr++ ;  //  leave the backslash in place
               *strptr = 0 ;  //  strip off filename
               // OutputDebugString(dirFile) ;
            }
            inireg.set_param("startpath", starting_path) ;
            SetFocus(hDlgWnd) ;
            return TRUE;
         }
         }
      break;   //  end IDC_BUTTON3

      case IDC_BUTTON5:   //  write settings to registry
         write_all_consoles() ;
         dirty_flag = 0 ;
         return TRUE;
      break;

      case IDC_BUTTON6:   //  launch new command processor
         if (dirty_flag) {
            // OutputDebugString("writing dirty buffers") ;
            write_all_consoles() ;
            dirty_flag = 0 ;
         }
         sprintf(tempstr, "start /D\"%s\" %s", starting_path, cmd_proc_filename) ;
         // OutputDebugString(tempstr) ;
         system(tempstr) ;
         return TRUE;
      break;

      case IDC_BUTTON7:   //  save current palette file
         // sprintf(tempstr, "start %s", cmd_proc_filename) ;
         // system(tempstr) ;
         {
         OPENFILENAME ofn;       // common dialog box structure
         char szFile[260];       // buffer for file name
         char oldFile[260];       // buffer for file name

         // Initialize OPENFILENAME
         ZeroMemory(&ofn, sizeof(ofn));
         ofn.lStructSize = sizeof(ofn);
         ofn.hwndOwner = hDlgWnd;
         ofn.lpstrFile = szFile;
         //
         // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
         // use the contents of szFile to initialize itself.
         //
         ofn.lpstrFile[0] = '\0';
         ofn.nMaxFile = sizeof(szFile);
         // ofn.lpstrFilter = "All\0*.*\0Palette files\0*.PAL\0";
         ofn.lpstrFilter = szPalFilter ;
         ofn.nFilterIndex = 1;
         ofn.lpstrTitle = "select output palette file" ;
         ofn.lpstrDefExt = TEXT ("pal") ;
         // ofn.lpstrFileTitle = NULL;
         // ofn.nMaxFileTitle = 0;
         // ofn.lpstrInitialDir = NULL;
         ofn.Flags = OFN_OVERWRITEPROMPT ;

         // Display the Open dialog box. 
         if (GetSaveFileName(&ofn) == TRUE) {
            // hf = CreateFile(ofn.lpstrFile, GENERIC_READ,
            //     0, (LPSECURITY_ATTRIBUTES) NULL,
            //     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
            //     (HANDLE) NULL);         
            strncpy(oldFile, palette_filename, sizeof(oldFile)) ;
            strncpy(palette_filename, ofn.lpstrFile, sizeof(palette_filename)) ;
            // IDC_EDIT3
            SetDlgItemText(hDlgWnd, IDC_EDIT3, palette_filename) ;
            inireg.set_param("palette", palette_filename) ;

            int result = write_palette_file(palette_filename, brighten);
            if (result != 0) {
               wsprintf(tempstr, "%s: %s", palette_filename, strerror(result)) ;
               OutputDebugString(tempstr) ;
               strncpy(palette_filename, oldFile, sizeof(palette_filename)) ;
            }
            SetFocus(hDlgWnd) ;
            return TRUE;
         }
         }
         return TRUE;
      break;

      case IDC_BUTTON8:   //  display help dialog
         //  MinGw gives a couple of indecipherable warnings about this:
         // Warning: .drectve `-defaultlib:uuid.lib ' unrecognized
         // Warning: .drectve `-defaultlib:uuid.lib ' unrecognized   
         //  But ignoring them doesn't seem to hurt anything...
         HtmlHelp(hDlgWnd, chmname, HH_DISPLAY_TOPIC, 0L);
         return TRUE;
      break;

      case IDC_PEBTN00:
      case IDC_PEBTN01:
      case IDC_PEBTN02:
      case IDC_PEBTN03:
      case IDC_PEBTN04:
      case IDC_PEBTN05:
      case IDC_PEBTN06:
      case IDC_PEBTN07:
      case IDC_PEBTN08:
      case IDC_PEBTN09:
      case IDC_PEBTN10:
      case IDC_PEBTN11:
      case IDC_PEBTN12:
      case IDC_PEBTN13:
      case IDC_PEBTN14:
      case IDC_PEBTN15:
         {
         unsigned idx = LOWORD(wParam)  % 16 ;
         // wsprintf(tempstr, "edit palette entry %u", idx) ;
         // OutputDebugString(tempstr) ;
         select_color(idx) ;
         dirty_flag = 1 ;
         SetFocus(hDlgWnd) ;
         return TRUE ;
         }         
         break;

      case ID_TRAYOPEN: // menu option
         // open dialog
         ShowWindow (hDlgWnd, SW_SHOW);
         return TRUE;
         break;

      case ID_TRAYEXIT:    // menu option - exit from program
         DestroyWindow (hDlgWnd);
         return 1;
      }  //lint !e744 switch(LOWORD(wParam)) (WM_COMMAND)
   break;   //  case WM_COMMAND

   case WM_DESTROY:
      // remove the icon from a system tray and free .dll handle
      Shell_NotifyIcon (NIM_DELETE, &NotifyIconData);

      PostQuitMessage (0);
   break;

   }  //lint !e744    end switch(Message) 
   return FALSE;
}

//lint +e527   code Unreachable

