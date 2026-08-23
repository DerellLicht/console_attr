//**************************************************************************
//  Console Palette Changer, Copyright (c) 2006-2026  Daniel D. Miller
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
// 1.05  08/21/26    Porting to modern clang++
//**************************************************************************

static char szClassName[] = "Console Palette Changer V1.05" ;

#include <windows.h>
#include <cstdio>   //  sprintf, needed for double
#include <cstdlib>  //  _MAX_PATH
// #include <sys/stat.h>
#include <string>
#include <shlobj.h>
// #include <shobjidl.h>   // IFileDialog, IShellItem
#include <htmlhelp.h>

#include "resource.h"
#include "common.h"
#include "commonw.h"
#include "console.attr.h"
#include "claude_browsers.h"
#include "config.h"

#define BUFFER_SIZE 256
static unsigned dirty_flag = 0 ;

static char szText[BUFFER_SIZE];
static HBRUSH g_hbrBackground = (HBRUSH) (COLOR_WINDOW + 1) ;
static HINSTANCE hInst;
// static NOTIFYICONDATA NotifyIconData;

static RECT DialogRect;
static RECT Edit5Rect;

// static HWND PaletteEdithwnd[16] = {
//    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } ;

static DWORD PaletteEditID[16] = {
IDC_PEBTN00, IDC_PEBTN01, IDC_PEBTN02, IDC_PEBTN03,
IDC_PEBTN04, IDC_PEBTN05, IDC_PEBTN06, IDC_PEBTN07,
IDC_PEBTN08, IDC_PEBTN09, IDC_PEBTN10, IDC_PEBTN11,
IDC_PEBTN12, IDC_PEBTN13, IDC_PEBTN14, IDC_PEBTN15 } ;

// static TCHAR szPalFilter[]  = TEXT ("Palette Files (*.PLT)\0*.plt\0")  \
//                               TEXT ("All Files (*.*)\0*.*\0\0") ;
// static TCHAR szExecFilter[] = TEXT ("Executable Files (*.EXE)\0*.exe\0")  \
//                               TEXT ("All Files (*.*)\0*.*\0\0") ;
// static TCHAR szDirFilter[]  = TEXT ("All Files (*.*)\0*.*\0\0") ;

const COMDLG_FILTERSPEC PalFilter[] = {
 { L"Palette Files (*.plt)", L"*.plt" },
 { L"All files (*.*)",       L"*.*"   }
};

const COMDLG_FILTERSPEC ExecFilter[] = {
 { L"Executable Files (*.exe)", L"*.exe" },
 { L"All files (*.*)",          L"*.*"   }
};

// static registry_iface inireg("dialog") ;
//********************************************************
//  ini variables
//********************************************************
char palette_filename[MAX_PATH_LEN+1] = "" ;
char cmd_proc_filename[MAX_PATH_LEN+1] = "C:\\WINDOWS\\SYSTEM32\\cmd.exe" ;  // NOLINT(modernize-raw-string-literal)
char starting_path[MAX_PATH_LEN+1] = "C:\\download" ;
double brighten = 3.0 ;

static BOOL CALLBACK InitProc( HWND hDlgWnd, UINT Message, WPARAM wParam, LPARAM lParam ) ;

#define CrSampleFont EzCreateFont (hdc, "Courier New", 150, 0, EZ_ATTR_BOLD, 0, TRUE)
#define CrDataFont EzCreateFont (hdc, "Bodacious-Normal", 150, 0, 0, 0, TRUE)
// #define CrLabelFont EzCreateFont (hdc, "Times New Roman", 150, 0, 0, 0, TRUE)

//*******************************************************************************
int setup_palette_filename()
{
   if (palette_filename[0] == 0) {
      get_base_filename(&palette_filename[0]);
      strcat(palette_filename, "\\palettes\\DOS.PLT");
   }
   return 0 ;
}

//*******************************************************************************
// void debug_dump_rect(char *msg, RECT *drect)
// {
//    syslog("%s: L%u, T%u, R%u, B%u", msg,
//       drect->left, drect->top, drect->right, drect->bottom) ;
// }

//*****************************************************************
static char chmname[1024] ;

static int find_chm_location(void)
{
   int result = derive_filename_from_exec(chmname, ".chm") ;
   if (result != 0) {
      syslog("could not resolve .chm name\n");
      return result;
   }
   // syslog("chm name: %s\n", chmname);
   // chm name: D:\SourceCode\Git\console_attr\console_attr.chm
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

   MoveToEx(hdc, x0, y0, nullptr) ;
   LineTo  (hdc, x1, y0) ;
   LineTo  (hdc, x1, y1) ;
   LineTo  (hdc, x0, y1) ;
   LineTo  (hdc, x0, y0) ;

   SelectObject(hdc, GetStockObject(BLACK_PEN)) ;  //  deselect my pen
   DeleteObject (hPen) ;
}

//*******************************************************************************
//  EDIT5 is a graphics region where example text is written
//*******************************************************************************
static void update_edit5(HWND hDlgWnd, HDC hdc) 
{
   // HWND hEditwnd = GetDlgItem(hDlgWnd, IDC_EDIT5);

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

   // syslog("hwnd5 (paint): l%u, t%u, r%u, b%u",
   //    rect.left, rect.top, rect.right, rect.bottom) ;

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
}  //lint !e715  func params not used

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
   // static HMENU hMenu;
   // POINT MouseCoordinates;
   HDC hdc ;
   PAINTSTRUCT ps;

   switch(Message) {
   case WM_CTLCOLORDLG:
       return (LONG) g_hbrBackground;

   case WM_INITDIALOG:
      {
      RECT DesktopRect;
      RECT Button5Rect;
      RECT Button6Rect;

      //  Add program icons to title bar
      SetClassLongA(hDlgWnd, GCL_HICON,   (LONG) LoadIcon(GetModuleHandle(nullptr), (LPCSTR) IDI_ICON));
      SetClassLongA(hDlgWnd, GCL_HICONSM, (LONG) LoadIcon(GetModuleHandle(nullptr), (LPCSTR) IDI_ICON));

      get_monitor_dimens(hDlgWnd);
      //  at this point, we should resolve %system_path% into the
      //  real system path, to properly set up cmd_proc_filename

      //  read configuration *before* creating edit fields
      init_config() ;
      // read_config_data() ;

      SetWindowText(hDlgWnd, szClassName) ;
      SetDlgItemText(hDlgWnd, IDC_HEADER, szClassName) ;

      //  do some setup stuff before creating dialog
      int result = read_palette_file(palette_filename, brighten) ;
      if (result != 0) {
         // show_message(NULL, "cannot read palette file") ;
         syslog("Error: read_palette_file: %s: %d\n", palette_filename, result) ;
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
      
      // syslog("WM_ID: window left: %u, top: %u\n", window_left, window_top);
      if (window_left == 0  ||  window_top == 0) {
         window_left = (DesktopRect.right - DialogRect.right) / 2 ;
         window_top  = (DesktopRect.bottom - DialogRect.bottom) / 2 ;
         // SetWindowPos( hDlgWnd, HWND_TOP, window_left, window_top, 0,0, SWP_NOSIZE );
         save_cfg_file() ;
      }
      // else {
         //  restore previously-saved window size/position from the .ini file. 
         // restore_dialog_settings(hwnd);
      //    SetWindowPos(hDlgWnd, HWND_TOP, window_left, window_top, 0, 0, SWP_NOSIZE);
      // }
      MoveWindowPos(hDlgWnd, window_left, window_top);

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
         char tempstr[20];
         wsprintf(tempstr, "[%02u]", xx) ;
         // PaletteEdithwnd[xx] = CreateButtonEx(tempstr, flags, 
         CreateButtonEx(tempstr, flags, xl, yu, 
            (xr - xl + 1), (yl - yu + 1), PaletteEditID[xx], hDlgWnd, hInst);
         // syslog("create button %02u, hwnd=%08X", xx, (unsigned) PaletteEdithwnd[xx]) ;
      }

      //**********************************************************
      //  do other config tasks *after* creating fields,
      //  so we can display status messages.
      //**********************************************************

      SetFocus(hDlgWnd) ;
      // InvalidateRect(hDlgWnd, NULL, TRUE);
      // UpdateWindow(hDlgWnd) ;
      // InvalidateRect(hDlgWnd, NULL, TRUE);
      return FALSE ;
   }

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

   case WM_EXITSIZEMOVE:
      {
      RECT rect ;
      GetWindowRect(hDlgWnd, &rect);
      window_top = rect.top ;
      window_left = rect.left ;
      save_cfg_file() ;
      }
      break ;
   
   case WM_SETFOCUS:
      // syslog("%u: WM_SETFOCUS", (unsigned) hDlgWnd) ;
      
      // InvalidateRgn (hDlgWnd, NULL, FALSE);
      InvalidateRect(hDlgWnd, NULL, TRUE);
      UpdateWindow(hDlgWnd) ;
      return FALSE ;
      // return TRUE;

   case WM_PAINT:
      // syslog("%u: WM_PAINT", (unsigned) hDlgWnd) ;
      
      hdc = BeginPaint (hDlgWnd, &ps);
      SetDlgItemText(hDlgWnd, IDC_EDIT2, cmd_proc_filename) ;
      SetDlgItemText(hDlgWnd, IDC_EDIT3, palette_filename) ;
      SetDlgItemText(hDlgWnd, IDC_EDIT4, starting_path) ;
      {
      //  write brightness value
      char tempstr[20];
      sprintf(tempstr, "%.1f", brighten) ;
      SetDlgItemText(hDlgWnd, IDC_EDIT1, tempstr) ;
      }
      //  update sample-text region
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
      // syslog("WM_DRAWITEM: %u", (UINT) wParam) ;
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
      }         
      return TRUE;

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
         save_cfg_file() ;
         return TRUE;
      break;   //  end IDC_BUTTON1

      case IDC_BUTTON2:   //  select command processor
         {
         std::wstring fileExec;
         if (BrowseForFile(hDlgWnd, fileExec, ExecFilter, ARRAYSIZE(ExecFilter))) {
             // use file
            std::string fileExecA = WideToNarrow(fileExec);
         
            strncpy(cmd_proc_filename, fileExecA.c_str(), sizeof(cmd_proc_filename)) ;
            save_cfg_file() ;
            SetFocus(hDlgWnd) ;  //  this generates WM_PAINT, to redraw the text controls
            return TRUE;
         }
         }
      break;   //  end IDC_BUTTON2

      case IDC_BUTTON3:   //  select palette file
         {
         char oldFile[MAX_PATH_LEN];       // buffer for file name
         // const COMDLG_FILTERSPEC PalFilter[] = {
         std::wstring filePal;
         if (BrowseForFile(hDlgWnd, filePal, PalFilter, ARRAYSIZE(PalFilter))) {
             // use file
            std::string filePalA = WideToNarrow(filePal);
         
            strncpy(oldFile, palette_filename, sizeof(oldFile)) ;
            strncpy(palette_filename, filePalA.c_str(), sizeof(palette_filename)) ;
            save_cfg_file() ;

            int result = read_palette_file(palette_filename, brighten);
            if (result != 0) {
               syslog("%s: %s", palette_filename, strerror(result)) ;
               strncpy(palette_filename, oldFile, sizeof(palette_filename)) ;
            }
            dirty_flag = 1 ;
            SetFocus(hDlgWnd) ;  //  this generates WM_PAINT, to redraw the text controls
         }
         }
      break;   //  end IDC_BUTTON3

      case IDC_BUTTON4:   //  select starting directory
         {
         // return select_starting_directory(hDlgWnd);
         std::wstring folder;
         if (BrowseForFolder(hDlgWnd, folder)) {
            std::string folderA = WideToNarrow(folder);
            strncpy(starting_path, folderA.c_str(), sizeof(starting_path)) ;
            // syslog("browse returned: %s\n", folderA.c_str());
            // strncpy(starting_path, reinterpret_cast<const char *>(folderA.c_str()), sizeof(starting_path)) ;
            save_cfg_file() ;
            SetFocus(hDlgWnd) ;  //  this generates WM_PAINT, to redraw the text controls
         }
         }
         return TRUE;

      case IDC_BUTTON5:   //  write settings to registry
         write_all_consoles() ;
         dirty_flag = 0 ;
         return TRUE;
      break;

      case IDC_BUTTON6:   //  launch new command processor
         if (dirty_flag) {
            // syslog("writing dirty buffers\n") ;
            write_all_consoles() ;
            dirty_flag = 0 ;
         }
         {
         char tempstr[1024] ;
         sprintf(tempstr, "start /D\"%s\" %s", starting_path, cmd_proc_filename) ;
         // syslog("%s\n", tempstr) ;
         system(tempstr) ;
         }
         return TRUE;
      break;

      case IDC_BUTTON7:   //  "Save Palette File" button
         // sprintf(tempstr, "start %s", cmd_proc_filename) ;
         // system(tempstr) ;
         {
         // const COMDLG_FILTERSPEC PalFilter[] = {
         char oldFile[MAX_PATH_LEN];       // buffer for file name
         std::wstring filePal;
      // if (BrowseForFileSave(hwndMain, file, filters, ARRAYSIZE(filters), L"untitled.cfg")) {
         if (BrowseForFileSave(hDlgWnd, filePal, PalFilter, ARRAYSIZE(PalFilter), L"untitled.pal")) {
             // use file
            std::string filePalA = WideToNarrow(filePal);
            
            strncpy(oldFile, palette_filename, sizeof(oldFile)) ;
            strncpy(palette_filename, filePalA.c_str(), sizeof(palette_filename)) ;
            // IDC_EDIT3
            SetDlgItemText(hDlgWnd, IDC_EDIT3, palette_filename) ;
            save_cfg_file() ;

            int result = write_palette_file(palette_filename, brighten);
            if (result != 0) {
               syslog("%s: %s", palette_filename, strerror(result)) ;
               strncpy(palette_filename, oldFile, sizeof(palette_filename)) ;
            }
            SetFocus(hDlgWnd) ;  //  this generates WM_PAINT, to redraw the text controls
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

      //  this handles the Palette Edit Buttons
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
         // syslog("edit palette entry %u", idx) ;
         select_color(idx) ;
         dirty_flag = 1 ;
         SetFocus(hDlgWnd) ;
         return TRUE ;
         }         

      case IDB_CLOSE:   
         PostMessageA(hDlgWnd, WM_CLOSE, 0, 0);
         break;
         
      }  //lint !e744 switch(LOWORD(wParam)) (WM_COMMAND)
   break;   //  case WM_COMMAND

   //********************************************************************
   //  application shutdown handlers
   //********************************************************************
   case WM_CLOSE:
      DestroyWindow(hDlgWnd);
      break;

   case WM_DESTROY:
      PostQuitMessage (0);
   break;

   }  //lint !e744    end switch(Message) 
   return FALSE;
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
   load_exec_filename() ;     //  get our executable name

  if (IsAppRunning()) {
       syslog("The program is already running\n");
       return 0;
   }
   find_chm_location() ;

   //****************************************************************
   HWND hWnd = CreateDialog( hInstance, MAKEINTRESOURCE(IDD_DIALOG), nullptr, (DLGPROC)InitProc );
   MSG    Msg;
   while(GetMessage(&Msg, nullptr,0,0) == TRUE) {
       if(!IsDialogMessage(hWnd,&Msg)) {
           TranslateMessage(&Msg);
           DispatchMessage(&Msg);
       }
   }
   return Msg.wParam;
}  //lint !e715  func params not used


