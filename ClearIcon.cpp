//*****************************************************************************************
//  Copyright (c) 2007-2017  Derell Licht
//  ClearIcon.cpp - a utility to change the desktop icon color.
//  
//  This program is licenced under Creative Commons CC0 1.0 Universal
// 
//  The person who associated a work with this deed has dedicated the work to the public domain 
//  by waiving all of his or her rights to the work worldwide under copyright law, including all 
//  related and neighboring rights, to the extent allowed by law.
// 
//  You can copy, modify, distribute and perform the work, even for commercial purposes, 
//  all without asking permission.
// 
//*****************************************************************************************
//  
//  References:
//  The original techinique for setting the desktop color was taken
//  from the TransDesk utility, by Wei Ke (see his original notes below)
//  
//  The ChooseColor hook procedure, which allows ClearIcon to position the 
//  color dialog as desired, was extracted from:
//  "Programming the Windows 95 User Interface", Nancy Cluts, 1995, Chapter 06, cmndlg32.
//  
//  The technique of setting the dialog position by trapping WM_INITDIALOG
//  in the hook procedure, was taken from comments on the web.
//  
//*****************************************************************************************
//  Bugs:
//  This color-setting technique will not work if "drop shadows" are enabled
//  on the desktop.  
//  Unfortunately, the technique for turning off drop shadows, varies from OS to OS...
//*****************************************************************************************
//  Wei Ke's original notes:
//  
//  TransDesk.cpp
//  original author: Wei Ke [kw@iglyph.com]
//  file created: 5/16/98 3:46:29 PM
//  file last modified: 5/20/98 6:12:21 PM
//
//  Toggles desktop icon text background between transparent and Windows' default.
//  
//  THIS CODE, PROGRAM AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY 
//  OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
//  PARTICULAR PURPOSE.
//
//*****************************************************************************************
//  version    changes
//  =======    ======================================
//    1.00     Initial release
//*****************************************************************************************

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#include "common.h"
#include "commonw.h"
#include "console.attr.h"
#include "config.h"

//***********************************************************************
// alt_fg_attr=0x41c345
unsigned ci_attr = 0x41c345 ;

uint color_x0 = 700 ;
uint color_y0 = 300 ;

/****************************************************************************
 * This hook procedure, which allows ClearIcon to position the color dialog
 * as desired, was extracted from Nancy Cluts, Chapter 06, cmndlg32.
 * The technique of setting the dialog position by trapping WM_INITDIALOG
 * in the hook procedure, was taken from comments on the web.
****************************************************************************/
static BOOL APIENTRY ChooseColorHookProc(
        HWND hDlg,              /* window handle of the dialog box */
        UINT message,           /* type of message                 */
        UINT wParam,            /* message-specific information    */
        LONG lParam)
{
   switch (message) {
   case WM_INITDIALOG:
      {
      // DWORD UFLAGS = SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW;
      MoveWindowPos(hDlg, color_x0, color_y0);
      }   
      break;
        
      // case WM_COMMAND:
      //     if (LOWORD(wParam) == IDOK)
      //     {
      //         // if (MessageBox( hDlg, "Are you sure you want to change the color?",
      //         //     "Information", MB_YESNO ) == IDYES )
      //         //     break;
      //         return (TRUE);
      // 
      //     }
      //  else if (LOWORD(wParam) == IDCANCEL)
      //     return TRUE;
      //     break;
   default:
      break;
   }
   return (FALSE);
}  //lint !e715


//****************************************************************
unsigned select_color(COLORREF orig_attr)
{
   static CHOOSECOLOR cc ;
   static COLORREF    crCustColors[16] ;

   ZeroMemory(&cc, sizeof(cc));
   cc.lStructSize    = sizeof (CHOOSECOLOR) ;
   // cc.rgbResult      = RGB (0x80, 0x80, 0x80) ;
   cc.rgbResult      = orig_attr ;
   cc.lpCustColors   = crCustColors ;
   cc.Flags          = CC_RGBINIT | CC_FULLOPEN ;
   cc.Flags          = CC_ENABLEHOOK;
   cc.lpfnHook       = (LPCCHOOKPROC)ChooseColorHookProc;
   cc.lpTemplateName = (LPTSTR)NULL;

   return (ChooseColor(&cc) == TRUE) ? cc.rgbResult : 0 ;
}
