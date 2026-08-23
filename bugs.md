- Setting console colors isn't quite working right for ConsoleZ

- update help file
  > document the settings for manually setting the screen position of the color dialog:
  
  position of color dialog for "Palette Edit Buttons" functions, 
  can now be manually controlled offline, 
  by editing the `color_x0` and `color_y0` in `console_attr.ini`.
  
  This should *only* be done when `console_attr` is not currently running.
  
  > setting console attributes in `ConsoleZ` consoles
  This can easily be done by using "Launch console"
  1. close ConsoleZ
  2. open a console via "Launch console" in `console_attr.exe`
  3. run `consattr -x<path to ConsoleZ folder> palette_name`
  4. open ConsoleZ
  
- update comments about this project, on web site


