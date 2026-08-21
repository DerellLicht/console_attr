//  undocumented console font info

typedef struct _CONSOLE_FONT_INFO {
  DWORD nFont;
  COORD dwFontSize;
} CONSOLE_FONT_INFO, *PCONSOLE_FONT_INFO;

// only in Win2k+  (use FindWindow for NT4)
HWND WINAPI GetConsoleWindow(void);
BOOL WINAPI GetCurrentConsoleFont(
  HANDLE hConsoleOutput,
  BOOL bMaximumWindow,
  PCONSOLE_FONT_INFO lpConsoleCurrentFont
);
COORD WINAPI GetConsoleFontSize(
  HANDLE hConsoleOutput,
  DWORD nFont
);


