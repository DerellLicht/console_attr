@if /I "%~1"=="console" goto :console
@if /I "%~1"=="windows" goto :windows
goto :usage

:usage
   @echo USAGE:
   @echo     domake [console ^<options^>] [windows ^<options^>]
   @echo.
   @echo ARGUMENTS
   @echo    console - run make on cMakefile
   @echo    windows - run make on wMakefile
   @echo.
   @echo    Either console or windows are required
   @echo.
   @goto :eof

:console
make -f cMakefile 
   @goto :eof

:windows
make -f wMakefile 
   @goto :eof

