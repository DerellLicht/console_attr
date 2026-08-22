//****************************************************************************
//  Copyright (c) 2008-2026  Derell Licht
//  config.cpp - manage configuration data file
//****************************************************************************
//  Filename will be same as executable, but will have .ini extensions.
//  Config file will be stored in same location as executable file.
//  Comments will begin with '#'
//  First line:
//  device_count=%u
//  Subsequent file will have a section for each device.
//****************************************************************************
#include <windows.h>
#include <cstdio>   //  fopen, etc
#include <cstdlib>  //  atoi()
#include <memory>

#include "common.h"
#include "console.attr.h"
#include "config.h"

// [dialog]
// cmdprog=C:\WINDOWS\system32\cmd.exe
// palette=D:\SourceCode\Git\console_attr\palettes\DOS.PLT
// startpath=C:\download
// brighten=3.000000
// window_top=370
// window_left=977

//****************************************************************************
//  debug: message-reporting data
//  NOTE: setting constants here, won't work!!
//        This value is over-written by INI file!!
//****************************************************************************
//    if (dbg_flags & DBG_WINMSGS) {
uint dbg_flags =
               // DBG_WINMSGS |
               0 ;

uint window_left = 500 ;
uint window_top = 200 ;

static char ini_name[MAX_PATH_LEN+1] = "" ;

//****************************************************************************
static void strip_comments(char *bfr)
{
   char *hd = strchr(bfr, '#') ;
   if (hd != 0)
      *hd = 0 ;
}

//****************************************************************************
LRESULT save_cfg_file(void)
{
   // first, make sure palette path/filename is set up
   setup_palette_filename();

   char *fname = ini_name ;
   // FILE *fd = fopen(fname, "wt") ;
   unique_file fd(fopen(fname, "wt")) ;
   if (fd == nullptr) {
      LRESULT result = (LRESULT) GetLastError() ;
      syslog("%s open: %s\n", fname, get_system_message(result)) ;
      return result;
   }
   //  save any global vars
   fprintf(fd.get(), "dbg_flags=0x%X\n", dbg_flags) ;
   fprintf(fd.get(), "cmdprog=%s\n", cmd_proc_filename) ;
   fprintf(fd.get(), "palette=%s\n", palette_filename) ;
   fprintf(fd.get(), "startpath=%s\n", starting_path) ;
   fprintf(fd.get(), "brighten=%.2f\n", brighten) ;
   fprintf(fd.get(), "window_top=%u\n", window_top) ;
   fprintf(fd.get(), "window_left=%u\n", window_left) ;
   // fclose(fd) ;
   return ERROR_SUCCESS;
}
// extern char palette_filename[MAX_PATH_LEN] ;
// extern char cmd_proc_filename[MAX_PATH_LEN] ;
// extern char starting_path[MAX_PATH_LEN] ;

//****************************************************************************
//  - derive ini filename from exe filename
//  - attempt to open file.
//  - if file does not exist, create it, with device_count=0
//    no other data.
//  - if file *does* exist, open/read it, create initial configuration
//****************************************************************************
LRESULT init_config(void)
{
   char inpstr[128] ;
   // int ivalue ;
   // uint uvalue ;
   LRESULT result ;
   
   result = derive_filename_from_exec(ini_name, ".ini") ;
   if (result != 0) {
      return result;
   }
   // if (dbg_flags & DBG_VERBOSE)
   //    syslog("INI fname=%s\n", ini_name) ;

   // FILE *fd = fopen(ini_name, "rt") ;
   unique_file fd(fopen(ini_name, "rt")) ;
   if (fd == 0) {
      return save_cfg_file() ;
   }

   // uint local_max_devs = 0 ;
   while (fgets(inpstr, sizeof(inpstr), fd.get()) != 0) {
      strip_comments(inpstr) ;
      strip_newlines(inpstr) ;
      if (strlen(inpstr) == 0)
         continue;

      char *tl = strchr(inpstr, '=') ;
      //  if '=' not found, try ':' as separator
      if (tl == NULL) {
         tl = strchr(inpstr, ':') ;
         if (tl == NULL) 
            continue;
      }
      *tl++ = 0 ; //  split field name from value ;

      // fprintf(fd.get(), "dbg_flags=0x%X\n", dbg_flags) ;
      // fprintf(fd.get(), "cmdprog=%s\n", cmd_proc_filename) ;
      // fprintf(fd.get(), "palette=%s\n", palette_filename) ;
      // fprintf(fd.get(), "startpath=%s\n", starting_path) ;
      // fprintf(fd.get(), "brighten=%.2f\n", brighten) ;
      // fprintf(fd.get(), "window_top=%u\n", window_top) ;
      // fprintf(fd.get(), "window_left=%u\n", window_left) ;
   
      if (strcmp(inpstr, "dbg_flags") == 0) {
         dbg_flags = strtoul(tl, nullptr, 0);    
      } else
      if (strcmp(inpstr, "cmdprog") == 0) {
          strncpy(cmd_proc_filename, tl, MAX_PATH_LEN);
      } else
      if (strcmp(inpstr, "palette") == 0) {
          strncpy(palette_filename, tl, MAX_PATH_LEN);
      } else
      if (strcmp(inpstr, "startpath") == 0) {
          strncpy(starting_path, tl, MAX_PATH_LEN);
      } else
      if (strcmp(inpstr, "brighten") == 0) {
         brighten = strtod(tl, 0);    
      } else
      if (strcmp(inpstr, "window_top") == 0) {
         window_top = (unsigned) strtol(tl, NULL, 10) ;
      } else
      if (strcmp(inpstr, "window_left") == 0) {
         window_left = (unsigned) strtol(tl, NULL, 10) ;
      } else
      {
         // syslog("unknown: [%s]\n", inpstr) ;
      }

   }
   // fclose(fd) ;
   return 0;
}

