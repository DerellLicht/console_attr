//**************************************************************************
//  Console Palette Changer, Copyright (c) 2006  Daniel D. Miller
//  This application and all associated source code is hereby declared
//  to be in the public domain.
//  
//  console.attr.cpp
//  Registry and file functions for Console Palette Changer.
//  
//  Written by:   Daniel D. Miller
//  
//**************************************************************************
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>  //  _MAX_PATH
#include <string.h>  //  memcpy
#include <errno.h>
#include <malloc.h>
#include <fcntl.h>

#include "common.h"
#include "console.attr.h"

typedef unsigned char  u8 ;

#define  FULL_KEY_LEN   1024

static HKEY target_key = HKEY_LOCAL_MACHINE ;

//**************************************************************
//  build list of eligible consoles in registry
//**************************************************************
typedef struct console_info_s {
   struct console_info_s *next ;
   char path[FULL_KEY_LEN] ;
   unsigned attr_count ;
   unsigned attr[16] ;
} console_info_t, *console_info_p ;

static console_info_p console_list = 0 ;
static console_info_p console_tail = 0 ;

//**************************************************************
//  dos.pal
// 000 00 00 00 00 00 2A 00 2A  00 00 2A 2A 2A 00 00 2A   
// 010 00 2A 2A 15 00 2A 2A 2A  15 15 15 15 15 3F 15 3F   
// 020 15 15 3F 3F 3F 15 15 3F  15 3F 3F 3F 15 3F 3F 3F   
// 030 81 C7 D9
//**************************************************************
unsigned curr_attr[16] ;
// unsigned console_attr[16] ;

//  original console settings
static unsigned default_attr[16] = {
0x00000000, 0x00800000, 0x00008000, 0x00808000,
0x00000080, 0x00800080, 0x00008080, 0x00C0C0C0,
0x00808080, 0x00FF0000, 0x0000FF00, 0x00FFFF00,
0x000000FF, 0x00FF00FF, 0x0000FFFF, 0x00FFFFFF } ;

//**************************************************************
static console_info_p add_console_entry(char *path)
{
   // console_info_p cptr = (console_info_p) malloc(sizeof(console_info_t)) ;
   console_info_p cptr = new console_info_t ;
   //  memset() takes void*, and console_info_p converts to void*
   //  implicitly -- the old (char *) cast was doing nothing useful
   //  and was what cppcheck was flagging, so it's simply removed.
   // memset(cptr, 0, sizeof(console_info_t)) ;
   ZeroMemory(cptr, sizeof(console_info_t));
   if (path == 0)
      wsprintf(cptr->path, "Console") ;
   else
      wsprintf(cptr->path, "Console\\%s", path) ;

   //  add new entry to list
   if (console_list == 0)
       console_list = cptr ;
   else
       console_tail->next = cptr ;
   console_tail = cptr ;
   return cptr;
}

//**************************************************************
//  if maxlevel < 0, don't use that limit
//**************************************************************
static void enum_subkeys(char *pszPath, int maxlevel)
{
   HKEY hKey ;
   DWORD disposition ;

   int plen = strlen(pszPath) ;

   // build a debug string
   static unsigned depth = 0 ;
   char depthstr[30] ;
   if (depth > 0) {
      memset(depthstr, '_', depth) ;
   }
   depthstr[depth] = 0 ;

   //  build current branch

   //  open current branch
   // DWORD result = RegCreateKeyEx(HKEY_LOCAL_MACHINE, pszPath, 
   DWORD result = RegCreateKeyEx(target_key, pszPath, 
      0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, 
      NULL, &hKey, &disposition) ;
   if (result != ERROR_SUCCESS) {
      printf("Create: %s: D%u: %s\n", pszPath, depth, get_system_message()) ;
      return ;
   }

   //  see how many subkeys there are
   DWORD dwKeyCount;
   DWORD dwValueCount;
   if (RegQueryInfoKey(hKey, NULL, NULL, 0, &dwKeyCount, NULL, &dwValueCount, 
      NULL, NULL, NULL, NULL, NULL ) != ERROR_SUCCESS) {
      printf("RegQueryInfoKey: %s: D%u: %s\n", pszPath, depth, get_system_message()) ;
      RegCloseKey(hKey) ;
      return ;
   }
   // printf("RegQueryInfoKey: subkeys=%u, values=%u\n", 
   //    (unsigned) dwKeyCount, (unsigned) dwValueCount) ;

   //  enumerate sub-branches
   FILETIME ft;
   char pszTemp[1024];
   long lResult ;
   unsigned j ;
   for (j=0; j<dwKeyCount; j++) {
      DWORD dwKeyLen = _MAX_PATH;
      lResult = RegEnumKeyEx(hKey, j, pszTemp, &dwKeyLen, NULL, NULL, NULL, &ft);
      if (lResult != ERROR_SUCCESS) {
         // printf("Enum: %s [%u]: %s\n", pszPath, j, get_system_message()) ;
         printf("Enum: %s [%u]: %s\n", pszPath, j, get_system_message()) ;
         break;
      }
      // if (pszTemp[0] != 'V')
      //    continue;
      //  debug output
      // printf("%s%s\n", depthstr, pszTemp) ;
      console_info_p cptr = add_console_entry(pszTemp) ;
      if (cptr == 0) {
         printf("out of memory") ;
         break;
      }

      //  append this subkey to base key
      if (maxlevel < 0  ||  depth < (unsigned) maxlevel) {
         //  if the string gets too long, stop recursing
         int slen = strlen(pszTemp) ;
         if ((plen+slen+1) >= FULL_KEY_LEN) {
            continue;
         }
         strcat(pszPath, "\\") ;
         strcat(pszPath, pszTemp) ;
         // printf("++%s\n", pszPath) ;
         depth++ ;
         enum_subkeys(pszPath, maxlevel) ;
         depth-- ;
         *(pszPath+plen) = 0 ;   //  strip off this subkey
      }
   } 

   //  release resources and exit
   RegCloseKey(hKey) ;
}

//**************************************************************
static void enum_values(char *pszPath, console_info_p cptr)
{
   HKEY hKey ;
   // DWORD disposition ;

   //  open current branch
   DWORD result = RegOpenKeyEx(target_key, pszPath, 0,
         KEY_READ, &hKey) ;
   if (result != ERROR_SUCCESS) {
      printf("Create: %s: %s\n", pszPath, get_system_message()) ;
      return ;
   }

   //  see how many values there are for this key.
   //  Note: this function was actually returning 0,
   //  even though the key I was searching had 5 values.
   //  So, instead, we'll just loop until we get NO_MORE_ITEMS.
   // DWORD dwKeyCount;
   // DWORD dwValueCount;
   // if (RegQueryInfoKey(hKey, NULL, NULL, 0, &dwKeyCount, NULL, &dwValueCount, 
   //    NULL, NULL, NULL, NULL, NULL ) != ERROR_SUCCESS) {
   //    printf("RegQueryInfoKey: %s: %s\n", pszPath, get_system_message()) ;
   //    RegCloseKey(hKey) ;
   //    return ;
   // }
   // printf("RegQueryInfoKey: subkeys=%u, values=%u\n", 
   //    (unsigned) dwKeyCount, (unsigned) dwValueCount) ;

   //  enumerate sub-branches
   char pszTemp[1024];
   DWORD vType ;
   BYTE vData[40] ;
   DWORD DataLen ;
   long lResult ;
   unsigned j = 0 ;
   // dwValueCount = 10 ;  //@@@  DEBUG
   for (j=0; ; j++) {
      DWORD dwKeyLen = _MAX_PATH;
      DataLen = 40 ;
      lResult = RegEnumValue(
         hKey,       //  HKEY hKey,
         j,          //  DWORD dwIndex,
         pszTemp,    //  LPTSTR lpValueName,
         &dwKeyLen,  //  LPDWORD lpcValueName,
         0,          //  LPDWORD lpReserved,
         &vType,     //  LPDWORD lpType,
         vData,      //  LPBYTE lpData,
         &DataLen    //  LPDWORD lpcbData
         );

      if (lResult != ERROR_SUCCESS) {
         if (lResult != ERROR_NO_MORE_ITEMS) 
            printf("Enum: %s [%u]: %ld, %s\n", pszPath, j, lResult, get_system_message()) ;
         break;
      }
      //  debug output
      if (strncmp(pszTemp, "ColorTable", 10) != 0)
         continue;
      unsigned idx = atoi(pszTemp+10) ;
      // printf("[%s], len=%u, 0x%08X\n", pszTemp, (unsigned) DataLen, *((unsigned *) &vData[0])) ;
      // console_attr[idx] = (unsigned) *((unsigned *) &vData[0]);
      //  vData is a BYTE (unsigned char) array; reading it through a
      //  (unsigned *) is the direction of type-punning that's actually
      //  undefined behavior (strict-aliasing violation, and vData has
      //  no guaranteed 4-byte alignment). memcpy() is the well-defined
      //  way to reinterpret the bytes as an unsigned -- and on any
      //  target compiler it optimizes down to the same load instruction.
      //  cptr->attr[idx] = (unsigned) *((unsigned *) &vData[0]);
      unsigned regvalue ;
      memcpy(&regvalue, &vData[0], sizeof(regvalue)) ;
      cptr->attr[idx] = regvalue ;
      cptr->attr_count++ ;
   } 

   //  release resources and exit
   RegCloseKey(hKey) ;
}

//**************************************************************
int enum_all_consoles(void)
{
   char path[FULL_KEY_LEN] ;
   console_info_p cptr = console_list ;
   while (cptr != 0) {
      // syslog("enumerate %s: ", cptr->path) ;
      strcpy(path, cptr->path) ;
      enum_values(path, cptr) ;
      // syslog("%u entries found\n", cptr->attr_count) ;
      cptr = cptr->next ;
   }
   return 0;
}

//**************************************************************
void restore_default_colors(void)
{
   for (unsigned j=0; j<16; j++) 
      curr_attr[j] = default_attr[j] ;
}

//**************************************************************
static int write_new_attr(console_info_p cptr)
{
   char szPath[1024] ;
   char *pszPath ;

   strcpy(szPath, cptr->path) ;
   pszPath = szPath ;

   HKEY hKey ;
   DWORD result = RegOpenKeyEx(target_key, pszPath, 0, KEY_ALL_ACCESS, &hKey) ;
   if (result != ERROR_SUCCESS) {
      // printf("Open: %s: [%u] %s\n", pszPath, (unsigned) result, get_system_message()) ;
      syslog("Open: %s: [%u] %s\n", pszPath, (unsigned) result, strerror(result)) ;
      return 1 ;
   }

   // for (unsigned j=0; j<16; j++) {
   // syslog( "changing %s\n", cptr->path) ;
   for (unsigned j=0; j<cptr->attr_count; j++) {
      // wsprintf(pszPath, "Console\\ColorTable%02u", j) ;
      wsprintf(pszPath, "ColorTable%02u", j) ;

      //  Unlike the vData read above, this direction is well-defined:
      //  the standard explicitly permits examining any object's bytes
      //  through an unsigned char* (BYTE is unsigned char). We're not
      //  reinterpreting a byte buffer AS an unsigned -- we're viewing
      //  an actual unsigned's bytes as bytes, which is legal. It's
      //  still a pointer-type reinterpretation though, which is exactly
      //  what reinterpret_cast exists to make explicit.
      LONG wresult = RegSetValueEx(hKey, pszPath, 0, REG_DWORD, reinterpret_cast<BYTE *>(&curr_attr[j]), sizeof(unsigned)) ;
      if (wresult != ERROR_SUCCESS) {
         syslog("SetValue: %s: %s\n", pszPath, get_system_message()) ;
      } else {
         // printf("SetValue: %s: 0x%08X => 0x%08X\n", pszPath, console_attr[j], curr_attr[j]) ;
         // printf("SetValue: %s: 0x%08X => 0x%08X\n", pszPath, cptr->attr[j], curr_attr[j]) ;
      }

   }
   puts("") ;
   RegCloseKey(hKey) ;

   return 0;
}

//**************************************************************
int write_all_consoles(void)
{
   // puts("new attribute data") ;
   // dump_palette_data() ;
   // puts("") ;
   // syslog( "console list: 0x%08X\n", (unsigned) console_list) ;

   //  now iterate over list
   console_info_p cptr = console_list ;
   while (cptr != 0) {
      if (cptr->attr_count != 0) {
         // syslog( "changing %s\n", cptr->path) ;
         write_new_attr(cptr) ;
      }
      cptr = cptr->next ;
   }
   return 0;
}

//**************************************************************
static unsigned make_brighter(unsigned utemp, double dbrighten)
{
   unsigned uvalue = (unsigned) ((double) utemp * dbrighten) ;
   if (uvalue > 255)
       uvalue = 255 ;
   return uvalue ;
}

//**************************************************************
static unsigned make_dimmer(unsigned utemp, double dbrighten)
{
   unsigned uvalue = (unsigned) ((double) utemp / dbrighten) ;
   return uvalue ;
}

//**************************************************************
int write_palette_file(char *palette_name, double dbrighten)
{
   // u8 pdata[51] ; //  old style
   u8 pdata[64] ;
   unsigned utemp ;
   ul2uc_t uconv {};

   //  build temp buffer
   unsigned idx = 0 ;
   // for (unsigned pidx=0; pidx<16; pidx++) {
   //  reference *not* required here, 
   //  because curr_attr_element will be read from.
   for (unsigned curr_attr_element : curr_attr) {
      // uconv.ul = curr_attr[pidx] ;
      uconv.ul = curr_attr_element ;
      utemp = uconv.uc[0] ;
      pdata[idx++] = (u8) make_dimmer(utemp, dbrighten) ;
      utemp = uconv.uc[1] ;
      pdata[idx++] = (u8) make_dimmer(utemp, dbrighten) ;
      utemp = uconv.uc[2] ;
      pdata[idx++] = (u8) make_dimmer(utemp, dbrighten) ;
      pdata[idx++] = 0 ;
   }

   int hdl = open(palette_name, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, 0666) ;
   if (hdl < 0) {
      syslog( "open (write): %s: %s\n", palette_name, get_system_message()) ;
      return errno ;
   }
   int wrbytes = write(hdl, pdata, sizeof(pdata)) ;
   close(hdl) ;
   if (wrbytes != sizeof(pdata)) {
      syslog( "write returned %d vs %u", wrbytes, sizeof(pdata)) ;
      return EINVAL ;
   }
   return 0;
}

//**************************************************************
int read_palette_file(char *palette_name, double dbrighten)
{
   // u8 pdata[52] ;
   u8 pdata[65] ;
   unsigned utemp ;
   int hdl = open(palette_name, O_BINARY | O_RDONLY) ;
   if (hdl < 0) {
      syslog( "Error: open (read): %s: %s\n", palette_name, get_system_message()) ;
      // Error: open (read): C:\SourceCode\win32\console_attr\palettes\IRELAND.PLT: 
      // The system cannot find the path specified.
      
      return errno ;
   }
   int rdbytes = read(hdl, pdata, sizeof(pdata)+1) ;
   close(hdl) ;
   if (rdbytes != 64) {
      // return -1;
      syslog( "read: %s: %d vs %d\n", palette_name, rdbytes, sizeof(pdata)) ;
      return EINVAL ;
   }

   ul2uc_t uconv {};
   u8 *uptr = &pdata[0] ;
   // for (unsigned j=0; j<16; j++) {
   //  reference required here, because curr_attr_element will be written to
   for (unsigned int & curr_attr_element : curr_attr) {
      //  bytes are reached in r,g,b sequence in binary file.
      //  Also, don't brighten index 0 (background color)
      utemp = (unsigned) *uptr++ ;
      if (dbrighten)
         utemp = make_brighter(utemp, dbrighten) ;
      uconv.uc[0] = (u8) utemp ;
      utemp = (unsigned) *uptr++ ;
      if (dbrighten)
         utemp = make_brighter(utemp, dbrighten) ;
      uconv.uc[1] = (u8) utemp ;
      utemp = (unsigned) *uptr++ ;
      if (dbrighten)
         utemp = make_brighter(utemp, dbrighten) ;
      uconv.uc[2] = (u8) utemp ;
      uconv.uc[3] = 0 ;
      uptr++ ; //  skip fourth byte
      // curr_attr[j] = uconv.ul ;
      curr_attr_element = uconv.ul ;
   }
   return 0;
}

//**************************************************************
// void dump_console_data(void)
// {
//    for (unsigned j=0; j<16; j++) 
//       printf("ColorTable%02u: 0x%08X => 0x%08X\n", j, console_attr[j], curr_attr[j]) ;
// }

//**************************************************************
void build_console_list(void)
{
   char path[FULL_KEY_LEN] ;
   add_console_entry(0) ;  //  add root entry to console list
   target_key = HKEY_CURRENT_USER ;
   sprintf(path, "Console") ;
   enum_subkeys(path, 0) ;
   puts("") ;
}

