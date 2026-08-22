//  convert prism palette files to 32-bit palette files
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
// #include <sys/types.h>
// #include <sys/stat.h>

typedef unsigned char u8 ;

//*************************************************************
char *get_system_message(void)
{
   static char msg[261] ;
   int slen ;

   LPVOID lpMsgBuf;
   FormatMessage( 
      FORMAT_MESSAGE_ALLOCATE_BUFFER | 
      FORMAT_MESSAGE_FROM_SYSTEM | 
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
      (LPTSTR) &lpMsgBuf,
      0, 0);
   // Process any inserts in lpMsgBuf.
   // ...
   // Display the string.
   strncpy(msg, (char *) lpMsgBuf, 260) ;

   // Free the buffer.
   LocalFree( lpMsgBuf );

   //  trim the newline off the message before copying it...
   slen = strlen(msg) ;
   if (msg[slen-1] == 10  ||  msg[slen-1] == 10) {
      msg[slen-1] = 0 ;
   }

   return msg;
}

//*************************************************************
int main(void)
{
   WIN32_FIND_DATA fdata ;
   char outfile[MAX_PATH_LEN] ;
   u8 inbfr[51] ;
   u8 outbfr[48] ;

   HANDLE handle = FindFirstFile ("*.pal", &fdata);
   if (handle == INVALID_HANDLE_VALUE) {
      printf("*.pal: %s\n", get_system_message()) ;
      return 1;
   }
   while (1) {
      unsigned j, inidx, outidx ;
      int infd, outfd, rdbytes, wrbytes ;

      strcpy(outfile, fdata.cFileName) ;
      char *sptr = strrchr(outfile, '.') ;
      if (sptr == 0) 
         goto next_file;
      sprintf(sptr, ".PLT") ;

      printf("%s => %s: ", fdata.cFileName, outfile) ;
      //  read input file
      infd = open(fdata.cFileName, O_BINARY | O_RDONLY) ;
      if (infd < 0) {
         perror(fdata.cFileName) ;
         goto next_file;
      }
      rdbytes = read(infd, inbfr, 51) ;
      if (rdbytes != 51) {
         printf("read failed, %d bytes\n", rdbytes) ;
         goto next_file;
      }
      close(infd) ;

      //  port data out output format
      inidx = 0 ;
      outidx = 0 ;
      for (j=0; j<16; j++) {
         outbfr[outidx++] = inbfr[inidx++] ;
         outbfr[outidx++] = inbfr[inidx++] ;
         outbfr[outidx++] = inbfr[inidx++] ;
         outbfr[outidx++] = 0 ;
      }
      if (outidx != 64) {
         printf("something went wrong, outidx=%u, inidx=%u\n", outidx, inidx) ;
         goto next_file;
      }

      //  write output file
      outfd = open(outfile, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, 0666) ;
      if (outfd < 0) {
         perror(outfile) ;
         goto next_file;
      }
      wrbytes = write(outfd, outbfr, 64) ;
      if (wrbytes != 64) {
         printf("write failed, %d bytes\n", wrbytes) ;
         goto next_file;
      }
      close(outfd) ;
      puts("success... ") ;
      
next_file:
      //  goto next file
      if (FindNextFile (handle, &fdata) == 0)
         break;
   }
   FindClose (handle);
   return 0;
}
