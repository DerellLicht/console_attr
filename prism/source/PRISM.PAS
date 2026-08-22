{
 ‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹‹
 €                                                                         €
 €        TITLE :      PRISM.EXE                                           €
 €      PURPOSE :      Video Palette Editor.                               €
 €       AUTHOR :      David Gerrold, CompuServe ID:  70307,544            €
 €  _____________________________________________________________________  €
 €                                                                         €
 €   Written in Turbo Pascal, Version 5.5,                                 €
 €   with routines from TurboPower, Object Professional.                   €
 €                                                                         €
 €   Turbo Pascal is a product of Borland International.                   €
 €   Object Professional is a product of TurboPower Software.              €
 €  _____________________________________________________________________  €
 €                                                                         €
 €   This is not public domain software.                                   €
 €   This version of PRISM is copyright (c) 1990, Ziff Communications Co.  €
 €                                                                         €
 ﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂﬂ
                                                                            }
{ Compiler Directives ===================================================== }

{$A-}    {Switch word alignment off, necessary for cloning}
{$R-}    {Range checking off}
{$B-}    {Boolean complete evaluation off}
{$S-}    {Stack checking off}
{$I-}    {I/O checking off}
{$N+,E+} {Simulate numeric coprocessor}
{$M 16384,0,327680} {stack and heap}
{$V-}    {Variable range checking off}

{ Name ==================================================================== }

PROGRAM Prism;

{ ========================================================================= }
{ Uses ==================================================================== }

USES
{ Turbo Pascal 5.5 Units }
  Dos,

{ Object Professional Units }
  OpClone,
  OpCmd,
  OpColor,
  OpCrt,
  OpDir,
  OpDos,
  OpEdit,
  OpEntry,
  OpErrHan,
  OpFrame,
  OpHelp,
  OpInt,
  OpMenu,
  OpMouse,
  OpPick,
  OpRoot,
  OpStrDev,
  OpString,
  OpWindow,

{ DgUnits }
  DgCrt,
  DgDec,
  DgDialog,
  DgFile,
  DgMath,
  DgParam,
  DgSound,
  DgStr,
  DgWryte;

{ ========================================================================= }
{ Clone Declarations ====================================================== }

CONST
  { start of installation area }
  Id : string [30]      = 'Copyright 1990, David Gerrold';

  { configuration data }
  SfxFlag         : boolean = true;              { sound effects on }
  MouseSpeed      : integer = 2;                 { fast speed }
  DissolveDelay   : integer = 2;                 { 12 steps to dissolve }

  { end of installation area }
  CfgEnd : byte = 0;

VAR
  { for resetting defaults in a config file: }
  Block : array [0..10] of byte absolute SfxFlag;
  StoreSfxFlag : boolean;
  StoreMouseSpeed : integer;
  StoreDissolveDelay : integer;

  CloneArranger : Cloner;                        { Clone Object }
  PrismHelp     : ScrollingHelpWindow;           { Help Object }


{ ========================================================================= }
{ Palette Object Declarations ============================================= }

TYPE
  VgaRegisterOb = Object
    ColorNumber : byte;                          { 0..15 }
    EgaReg      : byte;                          { ega register 0..63 }
    ColorValues : record case boolean of
      true  : (Saturation, R, G, B, Intensity : byte);
      false : (ValArray : array [0..4] of byte);
      end;
    Rreal,
    Greal,
    Breal       : real;                          { for stepping }
    Rstep,
    Gstep,
    Bstep       : real;                          { how big a step }

    Procedure Accept (Num, Rval, Gval, Bval : byte);  { accept color vals }
    Procedure Get (Color : byte);                { read register values }
    Procedure Put;                               { poke register values }
    Procedure GetSaturation (Color : byte);      { get Saturation steps }
    Procedure GetIntensity (Color : byte);       { get Intensity steps }
    Procedure SetSaturation (Vector : integer);  { Saturation up or down }
    Procedure SetIntensity (Vector : integer);   { Intensity up or down }
    end;

  VgaPaletteOb = Object
    P : VgaRegArray;                             { array [0..16, 1 ..3] }

    { undo variables }
    MaxYank  : integer;
    Ctr      : integer;
    Stack    : array [0..15] of VgaRegArray;

    { undo procedures }
    Procedure Init;                              { Gets }
    Procedure Push;                              { store for undo }
    Procedure Pop (VAR Palette : VgaRegArray);   { undo }

    { palette procedures }
    Procedure Get;                               { read register values }
    Procedure Put;                               { poke register values }
    Procedure Swap (Source, Target : byte);      { swap two colors }
    Procedure Dupe (Source, Target : byte);      { duplicate a color }
    Procedure SetRGB (ColorNumber, Color, Value : byte);
                                                 { poke a color value }
    Procedure DissolveTo (NewPal : VgaRegArray); { change to a new palette }
    end;

  PendOb = Object
    Status : integer;                            { status }

    Procedure Init;                              { status := -1 }
    Procedure Draw;                              { display msg }
    Procedure SetStatus (NewStatus : integer);   { update msg }
    Procedure Erase;                             { erase msg }
    end;

VAR
  Pal            : VgaPaletteOb;                 { active object }
  Pending        : PendOb;                       { pending window }

{ ========================================================================= }
{ Screen Declarations ===================================================== }

CONST
  PaletteFileName : string = '';                 { read/write to file }

  NormalMouseChar  = #04;
  MouseChar : Char = NormalMouseChar;            { mouse cursor }

  BxChar         = #219;                         { for drawing boxes }
  SliderChar     = #15;                          { sliders }
  SaturationChar = #31;                          { saturation marker }
  IntensityChar  = #30;                          { intensity marker }
  LineChar       = #196;                         { slider lines }
  FrameChar      = #205;                         { slider frame }

  MeterString1 : string [65] = #199 +
                               #196#196#196#196#196#196#196#196 +
                               #196#196#196#196#196#196#196#196 +
                               #196#196#196#196#196#196#196#196 +
                               #196#196#196#196#196#196#196#196 +
                               #196#196#196#196#196#196#196#196 +
                               #196#196#196#196#196#196#196#196 +
                               #196#196#196#196#196#196#196#196 +
                               #196#196#196#196#196#196#196 +
                               #182;

  MeterString2 : string [66] = #199 +
                               #205#205#205#205#205#205#205#205 +
                               #205#205#205#205#205#205#205#205 +
                               #205#205#205#205#205#205#205#205 +
                               #205#205#205#205#205#205#205#205 +
                               #205#205#205#205#205#205#205#205 +
                               #205#205#205#205#205#205#205#205 +
                               #205#205#205#205#205#205#205#205 +
                               #205#205#205#205#205#205#205 +
                               #182;
  SaturationLine = 18;
  RedLine        = SaturationLine + 1;
  GreenLine      = RedLine + 1;
  BlueLine       = GreenLine + 1;
  IntensityLine  = BlueLine + 1;

  FadeRateArray  : array [0..6] of byte = (0,6,12,18,24,30,36);


{ ========================================================================= }
{ Program Declarations ==================================================== }

VAR
  ExitFlag       : boolean;                      { leave program? }

  LastColor,                                     { last active color }
  CurrentColor   : integer;                      { active color }
  LastSliderOption,                              { last active slider }
  SliderOption   : integer;                      { active slider }

  MouseStoreTime : longint;                      { for double click }

  SelectColor    : byte;                         { for swapping & duping }
  VgaReg         : VgaRegisterOb;                { holder for Sat & Int }

VAR
  Status : Word;                                 { used by menu system }
  M      : Menu;                                 { menu system }
  H      : RawWindow;                            { item help window }
  CW     : RawWindow;                            { colophon window }

{ Linking ================================================================= }

{$L Prism.Obj }
PROCEDURE HelpText ; external;                   { help text }

{$I PrisMenu.Inc }                               { menu & help system }
{$I Prism.Inc }                                  { actual palette editor }

{ ========================================================================= }
{ Main  =================================================================== }

BEGIN
  { Program can't clone unless it knows itself. }
  SetProgramName ('Prism', 1.0);                 { PC-Mag version 1.0 }

  if CurrentDisplay < VGA then                   { check for VGA }
    NoVga;                                       { halt }
  if DefColorChoice = ForceMono then             { no /bw param }
    NoBw;                                        { halt }

  if ExistAnyFile (ConfigFileName) then begin
    Reset (ConfigFile, sizeof (Block));
    BlockRead (ConfigFile, Block, 1);
    Close (ConfigFile);
    end;

  SfxOptions := ord (SfxFlag);                   { sound cues only }
  StoreSfxFlag := SfxFlag;                       { store for comparisons }
  StoreMouseSpeed := MouseSpeed;
  StoreDissolveDelay := DissolveDelay;

  FadeRate := FadeRateArray [DissolveDelay];     { Set FadeRate }
  DoBoxes;                                       { set msg appearance }

  if ParamCount = 1 then
    LoadNewPalette                               { load a palette }
  else
    RunEditor;                                   { edit a palette }

  StoreVgaPal := Pal.P;                          { reset stored system pal }
{ 
  StoreVgaPal holds the palette that was running on the system when 
  the program booted.  When the system goes to restore the previous  
  palette, it will now reset it to the user's chosen (or redefined) 
  set of colors.

  Because DgCrt.FadeStart sets a flag, the exit procedure will automatically
  fade out and fade back in to the saved DOS screen.
} 
END.

{ ========================================================================= }
{ ========================================================================= }

VERSION HISTORY:

  9009.18
    First release version (1.0) completed.

{ ========================================================================= }
{ ========================================================================= }


