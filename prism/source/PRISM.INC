{ ========================================================================= }
{ PRISM.INC                                                                 }
{ ========================================================================= }
{ NoVga =================================================================== }

PROCEDURE NoVga;

BEGIN
  WryteLn (ProgramName + ' is capable of setting each of the 16 attributes');
  WryteLn ('of the VGA text mode display to any of 262,144 different colors.');
  WryteLn ('It requires a computer with a VGA card and a compatible monitor.');
  WryteLn ('');
  textcolor (colormono (lightred, white));
  WryteLn ('Sorry.  This system is not running a VGA display.');
  halt;
END;

{ NoBw ==================================================================== }

PROCEDURE NoBw;

BEGIN
  WryteLn (ProgramName + ' is capable of setting each of the 16 attributes');
  WryteLn ('of the VGA text mode display to any of 262,144 different colors.');
  WryteLn ('');
  textcolor (colormono (lightred, white));
  WryteLn ('Sorry.  It cannot be run in monochrome mode.');
  halt;
END;

{ VgaRegisterOb.Accept ==================================================== }

PROCEDURE VgaRegisterOb.Accept (Num, Rval, Gval, Bval : byte);

BEGIN
  With ColorValues do begin
    ColorNumber := Num;
    EgaReg := EgaPal [ColorNumber];
    R := Rval;
    G := Gval;
    B := Bval;

    Saturation := 63 - min (R, min (G, B));      { saturation }
    Intensity := max (R, max (G, B));            { intensity }

    Rreal := R;
    Greal := G;
    Breal := B;
    end;
END;

{ VgaRegisterOb.Get ======================================================= }

PROCEDURE VgaRegisterOb.Get (Color : byte);

BEGIN
  With ColorValues do begin
    ColorNumber := Color;
    EgaReg := EgaPal [ColorNumber];
    GetVgaRegister (EgaReg, R, G, B);

    Saturation := 63 - min (R, min (G, B));      { saturation }
    Intensity := max (R, max (G, B));            { intensity }

    Rreal := R;
    Greal := G;
    Breal := B;
    end;
END;

{ VgaRegisterOb.GetSaturation ============================================= }

PROCEDURE VgaRegisterOb.GetSaturation (Color : byte);

BEGIN
  With ColorValues do begin
    Get (Color);
    if Saturation > 0 then begin
      Rstep := (63 - R)/Saturation;              { step size }
      Gstep := (63 - G)/Saturation;
      Bstep := (63 - B)/Saturation;
      end
    else begin
      Rstep := 1;                                { step size }
      Gstep := 1;
      Bstep := 1;
      end;
    end;
END;

{ VgaRegisterOb.GetIntensity ============================================== }

PROCEDURE VgaRegisterOb.GetIntensity (Color : byte);

BEGIN
  With ColorValues do begin
    Get (Color);
    if Intensity > 0 then begin
      Rstep := R/Intensity;                      { step size }
      Gstep := G/Intensity;
      Bstep := B/Intensity;
      end
    else begin
      Rstep := 1;                                { step size }
      Gstep := 1;
      Bstep := 1;
      end;
    end;
END;

{ VgaRegisterOb.Put ======================================================= }

PROCEDURE VgaRegisterOb.Put;

BEGIN
  With ColorValues do begin
    R := round (Rreal);
    G := round (Greal);
    B := round (Breal);
    Saturation := 63 - min (R, min (G, B));      { saturation }
    Intensity := max (R, max (G, B));            { intensity }
    SetVgaRegister (EgaReg, R, G, B);
    end;
END;

{ VgaRegisterOb.SetSaturation ============================================= }

PROCEDURE VgaRegisterOb.SetSaturation (Vector : integer);

BEGIN
  With ColorValues do begin
    repeat
      Rreal := Rreal + Rstep * Vector;
      Greal := Greal + Gstep * Vector;
      Breal := Breal + Bstep * Vector;
    until
      (round (Rreal) <> R) or (round (Greal) <> G) or (round (Breal) <> B);
    Put;
    end;
END;

{ VgaRegisterOb.SetIntensity ============================================== }

PROCEDURE VgaRegisterOb.SetIntensity (Vector : integer);

BEGIN
  With ColorValues do begin
    repeat
      Rreal := Rreal + Rstep * Vector;
      Greal := Greal + Gstep * Vector;
      Breal := Breal + Bstep * Vector;
    until
      (round (Rreal) <> R) or (round (Greal) <> G) or (round (Breal) <> B);
    Put;
    end;
END;

{ ========================================================================= }
{ VgaPaletteOb.Init ======================================================= }

PROCEDURE VgaPaletteOb.Init;
VAR Loop : byte;
BEGIN

  Get;                                           { P = current palette }
  MaxYank := 15;                                 { how many to store }
  Ctr := 0;
  for Loop := 0 to MaxYank do                    { fill stack with P }
    Stack [Loop] := StoreVgaPal;

END;

{ VgaPaletteOb.Push ======================================================= }

PROCEDURE VgaPaletteOb.Push;
{ Store deleted palette on stack }

BEGIN
  { if current palette is not most recently stored palette ... }
  if
    CompStruct (Stack [Ctr], P, sizeof (VgaRegArray)) <> equal
  then begin
    UpCycle (Ctr, 0, MaxYank);                   { increment counter }
    Stack [Ctr] := P;                            { store on top of stack }
    end;
END;

{ VgaPaletteOb.Pop ======================================================== }

PROCEDURE VgaPaletteOb.Pop (VAR Palette : VgaRegArray);
{ Retrieve deleted palette from stack }

VAR
  Loop : byte;

BEGIN
  Loop := 0;
  While
    (CompStruct (Stack [Ctr], P, sizeof (VgaRegArray)) = equal)
      and
    (Loop <= MaxYank)
  do begin
    DownCycle (Ctr, 0, MaxYank);
    inc (Loop);
    end;

  Palette := Stack [Ctr];                        { pop most recent palette }
  Stack [Ctr] := P;                              { store P at new bottom }
  DownCycle (Ctr, 0, MaxYank);                   { decrement counter }
END;

{ VgaPaletteOb.Get ======================================================== }

PROCEDURE VgaPaletteOb.Get;
BEGIN
  GetVgaPalette (P);
END;

{ VgaPaletteOb.Put ======================================================== }

PROCEDURE VgaPaletteOb.Put;
BEGIN
  SetVgaPalette (P);                             { P is the new palette }
  Push;                                          { push it onto stack }
END;

{ VgaPaletteOb.Swap ======================================================= }

PROCEDURE VgaPaletteOb.Swap (Source, Target : byte);
VAR
  Loop : byte;
  SwapPal : VgaRegArray;
BEGIN
  SwapPal := P;
  for Loop := 1 to 3 do begin
    P [Source, Loop] :=
      SwapPal [Target, Loop];
    P [Target, Loop ] :=
      SwapPal [Source, Loop];
    end;  { loop }
  Put;
END;

{ VgaPaletteOb.Dupe ======================================================= }

PROCEDURE VgaPaletteOb.Dupe (Source, Target : byte);
VAR
  Loop : byte;
BEGIN
  for Loop := 1 to 3 do
    P [Target, Loop] := P [Source, Loop];
  Put;
END;

{ VgaPaletteOb.SetRGB ===================================================== }

PROCEDURE VgaPaletteOb.SetRGB (ColorNumber, Color, Value : byte);

BEGIN

  P [ColorNumber, Color] := Value;
  SetVgaRegister (EgaPal [ColorNumber],
                  P [ColorNumber, 1],
                  P [ColorNumber, 2],
                  P [ColorNumber, 3]);

END;

{ VgaPaletteOb.DissolveTo ================================================= }

PROCEDURE VgaPaletteOb.DissolveTo (NewPal : VgaRegArray);
{ change to a new palette }

BEGIN
  Dissolve (P, NewPal);                          { go from current to new }
  P := NewPal;                                   { new is now current }
  Push;                                          { store it on stack }
END;

{ ========================================================================= }
{ ResetDefaults =========================================================== }

PROCEDURE ResetDefaults;

BEGIN
{
  If no Config file, then clone the EXE file.
}
  if not ExistAnyFile (ConfigFileName) then 
    if CloneArranger.InitCustom (ExeFileName, UpdateAll, DefBufSize) then begin
      if CloneArranger.FindDefaultsEnd (Id, SizeOf (Id), 0) then begin 

        { write new values }
        CloneArranger.StoreDefaults (CloneArranger.GetPos,
                                     Id,
                                     Ofs (CfgEnd) - Ofs (Id));
        { check for errors }
        if CloneArranger.GetLastError = 0 then begin
          CloneArranger.Done;                    { close the EXE file }
          exit;
          end;
        end;

      { Oops, can't clone the EXE file. }
      CloneArranger.Done;                        { close the EXE file }
      end;

{
  Can't clone EXE file, so write to ConfigFile. 
}
  Rewrite (ConfigFile, sizeof (Block));          { write to config file }
  BlockWrite (ConfigFile, Block, 1);
  Close (ConfigFile);
END;

{ SliderSound ============================================================= }

PROCEDURE SliderSound;
VAR
  S          : string;
  Loop       : byte;
BEGIN
  if not Sfx (SfxCues) then exit;
  FastRead (80, SliderOption + SaturationLine, 1, S);
  Case SliderOption of
    0    : Loop := Pos (SaturationChar, S);
    1..3 : Loop := Pos (SliderChar, S);
    4    : Loop := Pos (IntensityChar, S);
    end;
  sound (Loop * 100);
END;

{ SliderBeep ============================================================== }

PROCEDURE SliderBeep;
VAR
  Loop : byte;
BEGIN
  if not Sfx (SfxCues) then exit;
  SliderSound;
  delay (3);
  Loop := 0;
  repeat
    inc (Loop)
  until
    (Loop = 50)
      or
    KeyOrButtonPressed;
  nosound;
END;

{ DrawAttributeBox ======================================================== }

PROCEDURE DrawAttributeBox (ColorNumber : byte);
CONST
  S : string [10] = ' ' +
                    BxChar + BxChar + BxChar + BxChar +
                    BxChar + BxChar + BxChar + BxChar + ' ';

VAR
  Loop : byte;
BEGIN
  for Loop := 1 to 4 do
    FastWrite (S,                                               { string }
               trunc (1 + ((ColorNumber div 8) * 5)) + Loop,    { row }
               succ ((ColorNumber mod 8) * 10),                 { column }
               ColorNumber);                                    { attr }
END;

{ QuickStr ================================================================ }

FUNCTION QuickStr (V : byte) : string;           { formatted num to str }
BEGIN
  Write (TpStr, V:2);
  QuickStr := ReturnStr;
END;

{ QuickMark =============================================================== }

FUNCTION QuickMark (M : string;  V : byte;  S : char) : string;
VAR
  MeterString : string [70];
  Mark : string [3];

BEGIN
  Mark := ^A + S + ^A;
  MeterString := M;                              { string with mark }
  Insert (Mark, MeterString, V + 2);
  QuickMark := MeterString;
END;

{ ShowKernel ============================================================== }

PROCEDURE ShowKernel (Vgr : VgaRegisterOb);
{ Writes stars and bars. }
CONST
  Fattrs : FlexAttrs = (Black, White, Green, Blue);
VAR
  Loop   : byte;

BEGIN
  if (CurrentColor = Black) or (CurrentColor = White) then
    Fattrs [0] := LightGray
  else
    Fattrs [0] := CurrentColor;

  HideMouse;
  With Vgr, ColorValues do begin
    MeterString2 [1] := #201;                              { top }
    MeterString2 [65] := #187;
    FlexWrite (QuickMark (MeterString2, Saturation, SaturationChar),
               SaturationLine, 12, Fattrs);

    FlexWrite (QuickMark (MeterString1, R, SliderChar),
               RedLine, 12, Fattrs);                       { red }
    FlexWrite (QuickMark (MeterString1, G, SliderChar),
               GreenLine, 12, Fattrs);                     { green }
    FlexWrite (QuickMark (MeterString1, B, SliderChar),
               BlueLine, 12, Fattrs);                      { blue }

    { show actual number values }
    For Loop := 0 to 4 do
      FastWrite (QuickStr (ValArray [Loop]),
                 SaturationLine + Loop, 79, White);

    MeterString2 [1] := #200;                              { bottom }
    MeterString2 [65] := #188;
    FlexWrite (QuickMark (MeterString2, Intensity, IntensityChar),
               IntensityLine, 12, Fattrs);
    end;
  ShowMouse;
END;

{ ShowPercentages ========================================================= }

PROCEDURE ShowPercentages (ColorNum : byte);
{ Shows percentages of particular color. }
VAR
  Vgr : VgaRegisterOb;

BEGIN
  With Pal do
    Vgr.Accept (ColorNum, P [ColorNum, 1], P [ColorNum, 2], P [ColorNum, 3]);
  ShowKernel (Vgr);
END;

{ SlidePercentages ======================================================== }

{$F+} PROCEDURE SlidePercentages; {$F-}
{ Shows percentage of CurrentColor.  Slides during dissolves. }

VAR
  Vgr : VgaRegisterOb;

BEGIN
  Vgr.Get (CurrentColor);
  ShowKernel (Vgr);
END;

{ DrawBox ================================================================= }

PROCEDURE DrawBox;
CONST
  BoxTop         : string [11] = #201#205#205#205#205#205#205#205#205#187;
  BoxSide        : string [4]  = #186#186#186#186;
  BoxBottom      : string [11] = #200#205#205#205#205#205#205#205#205#188;
  EmptyBoxTop    : string [11] = '          ';
  EmptyBoxSide   : string [4]  = '    ';

VAR
  TopRow   : byte;
  LeftCol  : byte;
  Top, Side, Bottom : string [11];
  BoxColor : byte;

BEGIN
  { outline box is blank }
  Top := EmptyBoxTop;
  Side := EmptyBoxSide;
  Bottom := EmptyBoxTop;
  TopRow := trunc (1 + ((LastColor div 8) * 5));
  LeftCol := succ ((LastColor mod 8) * 10);

  { erase old outline box }
  if CurrentColor <> LastColor then begin
    FastWrite (Top, TopRow, LeftCol, Black);
    inc (TopRow);
    FastVert (Side, TopRow, LeftCol, Black);
    FastVert (Side, TopRow, LeftCol + 9, Black);
    FastWrite (Bottom, TopRow + 4, LeftCol, Black);
    end;

  { outline box is frame }
  Top := BoxTop;
  Side := BoxSide;
  Bottom := BoxBottom;
  TopRow := trunc (1 + ((CurrentColor div 8) * 5));
  LeftCol := succ ((CurrentColor mod 8) * 10);
  if Pending.Status > -1 then                              { color of box }
    BoxColor := LightRed
  else
    BoxColor := White;

  { draw new outline box }
  FastWrite (Top, TopRow, LeftCol, BoxColor);              { draw box }
  inc (TopRow);
  FastVert (Side, TopRow, LeftCol, BoxColor);
  FastVert (Side, TopRow, LeftCol + 9, BoxColor);
  FastWrite (Bottom, TopRow + 4, LeftCol, BoxColor);

  LastColor := CurrentColor;
END;

{ ShowSelectedColor ======================================================= }

PROCEDURE ShowSelectedColor;
VAR
  Loop : integer;
  StoreMouseCursor : boolean;
BEGIN

  HideMousePrim (StoreMouseCursor);
  for Loop := 1 to 5 do
    FastWrite (CharStr (BxChar, 38), 11 + Loop, 22, CurrentColor);
  DrawBox;
  ShowPercentages (CurrentColor);
  ShowMousePrim (StoreMouseCursor);

END;

{ ShowSliderOption ======================================================== }

PROCEDURE ShowSliderOption;
VAR
  LocalColor : byte;
  StoreMouseCursor : boolean;

BEGIN
  if Pending.Status > -1 then exit;              { uh oh, operation pending }
  if SliderOption = LastSliderOption then exit;  { no need to change option }

  HideMousePrim (StoreMouseCursor);
  FastWrite ('  Saturate ', SaturationLine, 1, White);
  FastWrite ('       Red ',        RedLine, 1, White);
  FastWrite ('     Green ',      GreenLine, 1, White);
  FastWrite ('      Blue ',       BlueLine, 1, White);
  FastWrite (' Intensity ',  IntensityLine, 1, White);
  ChangeAttribute (11,
                   SaturationLine + SliderOption, 1,
                   BlackOnLtGray + 128);
  ShowMousePrim (StoreMouseCursor);
  LastSliderOption := SliderOption;
END;

{ SetSlider =============================================================== }

PROCEDURE SetSlider (CurrentColor, SliderOption, NewVal : integer);
{ shell for SetVgaRegister. }
VAR
  Vector  : integer;

BEGIN
  if Pending.Status > -1 then exit;              { uh oh, operation pending }

  NewVal := min (max (NewVal, 0), 63);           { check range }
  With VgaReg, ColorValues do
    case SliderOption of
      0    : begin
             While
               (Saturation <> NewVal)
             do begin
               if Saturation > NewVal then Vector := 1 else Vector := -1;
               SetSaturation (Vector);
               end;
             Pal.Get;
             ShowPercentages (CurrentColor);
             end;
      1..3 : With Pal do begin
               SetRGB (CurrentColor,
                       SliderOption,
                       round (MinReal (MaxReal (NewVal, 0), 63)));
               ShowPercentages (CurrentColor);
               end;
      4    : begin
             While
               (Intensity <> NewVal)
             do begin
               if Intensity < NewVal then Vector := 1 else Vector := -1;
               SetIntensity (Vector);
               end;
             Pal.Get;
             ShowPercentages (CurrentColor);
             end;
      end;  { case }
END;

{ GoBack ================================================================== }

PROCEDURE GoBack;
VAR
  P : VgaRegArray;
BEGIN

  if Pending.Status > -1 then                    { if operation pending }
    Pending.Erase                                { then cancel it }
  else begin                                     { else }

    Pal.Pop (P);                                 { pop last palette }
    Pal.DissolveTo (P);                          { and dissolve to it }
    end;
END;

{ RandomPalette =========================================================== }

PROCEDURE RandomPalette;
VAR
  Loop, Color : byte;
  NewPal     : VgaRegArray;

BEGIN
  NewPal := Pal.P;
  for Loop := 0 to 15 do
    for Color := 1 to 3 do
      NewPal [Loop, succ (random (3))] := random (10) * 7;
  Pal.DissolveTo (NewPal);
END;

{ HandleIsDevice ========================================================== }

FUNCTION HandleIsDevice (H : word) : boolean;
VAR
  R : registers;
BEGIN
  HandleIsDevice := false;
  with R do begin
    AH := $44;                                   { IOCTL }
    AL := $00;                                   { subfunction 0 }
    BX := H;
    MsDos (R);
    if not Odd (Flags) then
      HandleIsDevice := DX and $80 <> 0;
    end;
END;

{ GetUserFileName ========================================================= }

CONST
GetFileNameFlag : boolean = false;

PROCEDURE GetUserFileName (VAR S : string);
VAR
  Len     : byte absolute S;
  Le      : LineEditor;                          { line editor object }

  Loop    : byte;
  F       : file;
  Created : boolean;

BEGIN
  GetFileNameFlag := true;
  Le.Init (MenuColors);
  EditCommands.cpOptionsOn (cpEnableMouse);         { turn mouse on }
  EditCommands.AddCommand (ccQuit, 1, KcCtrlU, 0);  { exit on Ctrl-U }

  S := '';                                       { S = null string }
  While S = '' do begin                          { while no valid file name }

    Created := false;
    Le.ReadString (' Palette Name? ',
                   14, 29, 8, 8, S);             { read S }
    Case Le.GetLastCommand of
      ccQuit : S := '';                          { cancel name }
      end;

    if
      (S = '')                                   { if no name }
    then begin                                   { exit without name }
      Le.Done;
      ShowSelectedColor;
      GetFileNameFlag := false;
      exit;
      end;

    S := StUpCase (S);                           { upcase S }

    Assign (F, S);                               { assign file name }
    {$I-} Reset (F); {$I+}                       { open it }
    if IOresult <> 0 then begin                  { IO error }
      {$I-} Rewrite (F);  {$I+}
      if IOresult <> 0 then begin                { IO error }
        { failed to create file }
        end
      else begin                                 { file created }
        { created the file }
        Created := true;
        end;
      end

    else begin                                   { file already exists }
      end;

    if FileRec (F).mode = fmClosed then begin    { file is closed }
      end
    else begin
      if HandleIsDevice (FileRec(F).Handle) then begin
        PauseMsgBox ('Sorry, DOS will not allow ''' + S +
                     ''' to be used as a palette file name.  ' +
                     'Please enter another name.',
                     ReddbColorSet, dbJustify + dbShadow, 40);
        S := '';                                 { cancel S, try again }
        end;
      Close (F);                                 { close file }
      if Created then erase (F);                 { if created, erase file }
      end;

    For Loop := 1 to Len do
      if S [Loop] = ' ' then S [Loop] := '_';    { translate spaces }
    end;                                         { while S = '' do begin }

  Le.Done;
  ShowSelectedColor;
  GetFileNameFlag := false;
END;

{ WritePalette ============================================================ }

PROCEDURE WritePalette;
VAR
  Pfile     : file of VgaRegArray;
  PfileName : string;
  PfileAttr : word;

BEGIN
  PaletteFileName := JustName (PaletteFileName);

  if
    (PaletteFileName = '')
      or
    ((PaletteFileName > '')
      and
    (not YornBox ('Store as ''' + PaletteFileName + ''' palette?  (Y/N)')))
  then
    GetUserFileName (PaletteFileName);

  if PaletteFileName = '' then exit;             { escape if no file name }
  PfileName := ProgramPath + ForceExtension (PaletteFileName, 'PAL');

  if
    not ExistFile (PFileName)
      or
    (ExistFile (PFileName)
      and
    YornBox ('The ''' + PaletteFileName +
             ''' palette already exists as a file.  Overwrite?  (Y/N)'))
  then begin
    Assign (Pfile, PFileName);
    GetFattr (Pfile, PfileAttr);
    if PfileAttr and ReadOnly = ReadOnly then
      PauseMsgBox ('Sorry.  The ''' + PaletteFileName +
                   ''' palette has been stored as a read-only file.' +
                   '  It cannot be overwritten.',
                   RedDbColorSet, dbJustify + dbShadow, 40)
    else begin
      Rewrite (Pfile);
      Write (Pfile, Pal.P);
      Close (Pfile);
      TimedPauseMsg ('The ''' + PaletteFileName + ''' palette has been saved.',
                        GreenDbColorSet, dbShadow, 60, 1500);
      end;
    end;
END;

{ ReadDiskPalette ========================================================= }

PROCEDURE ReadDiskPalette;
{ Read palette from disk file . }

VAR
  Pfile     : file of VgaRegArray;
  PfileAttr : word;
  NewPal    : VgaRegArray;

BEGIN
  PaletteFileName := PickFile;
  if PaletteFileName > '' then begin

    Assign (Pfile, PaletteFileName);
    GetFattr (PFile, PfileAttr);
    if PfileAttr and ReadOnly = ReadOnly then
      SetFattr (PFile, 0);                       { workaround Turbo quirk }
    Assign (Pfile, PaletteFileName);
    Reset (Pfile);
    Read (Pfile, NewPal);
    Close (Pfile);
    SetFattr (Pfile, PfileAttr);

    Pal.DissolveTo (NewPal);
    end;
END;

{ LoadNewPalette ========================================================== }

PROCEDURE LoadNewPalette;
{ Load a palette from disk without running editor. }

VAR
  Pfile     : file of VgaRegArray;
  PfileAttr : word;
  NewPal    : VgaRegArray;
  Pname     : string [12];

BEGIN
  Pal.Get;                                       { get active palette }
  Wryte (ProgramName);

  Pname := ForceExtension (StUpCase (ParamStr (1)), 'PAL');
  PaletteFileName := ProgramPath + Pname;
  if not ExistAnyFile (PaletteFileName) then begin
    WryteLn (' cannot locate ''' + Pname + '''');
    exit;
    end;

  WryteLn (' loading ' + PaletteFileName);

  Assign (Pfile, PaletteFileName);
  GetFattr (PFile, PfileAttr);
  if PfileAttr and ReadOnly = ReadOnly then
    SetFattr (PFile, 0);                         { workaround Turbo quirk }
  Assign (Pfile, PaletteFileName);
  Reset (Pfile);
  Read (Pfile, NewPal);
  Close (Pfile);
  SetFattr (Pfile, PfileAttr);
  Pal.DissolveTo (NewPal);                       { dissolve to new palette }
END;

{ ========================================================================= }
{ MouseEventDeclarations ================================================== }

CONST
  MouseEventReentryFlag : boolean = false;
  RightButtonFlag       : boolean = false;
  RandomPaletteFlag     : boolean = false;

VAR
  TempStack : array [1..4096] of byte;           { temporary stack }

{ UserHook ================================================================ }

{$F+}
PROCEDURE UserHook (CPP : CommandProcessorPtr;
                    MT  : MatchType;
                    Key : word);
{$F-}
BEGIN
{
  Can't do a dissolve inside the mouse event handler.  It screws things up.
  Instead, look to see if the flag is set;  if it is, do the dissolve here.
}
  if not RandomPaletteFlag then exit;
  RandomPaletteFlag := false;
  RandomPalette;
END;

{ MouseEventKernel ======================================================== }

{$F+} PROCEDURE MouseEventKernel (Var Dummy : IntRegisters); {$F-}
{
  This procedure contains the real work of the mouse event handler.
}

CONST
  MinCol = 13;
  MaxCol = 76;
  WindowRestricted : boolean = false;

VAR
  CharAtMouseCursor : char;                      { what char under mouse }
  AttrAtMouseCursor : byte;                      { what attr under mouse }

{ ------------------------- }

PROCEDURE BoxCharClicked;
BEGIN
  { if mouse was clicked on any of 16 little boxes... }
  if MouseLastY < 11 then begin
    if CurrentColor <> AttrAtMouseCursor then begin
      CueClick;
      CurrentColor := AttrAtMouseCursor;         { change active color }

      { If flag set then swap color with current color }
      if Pending.Status > -1 then begin          { if operation pending }
        Pal.Swap (SelectColor, CurrentColor);
        Pending.Erase;                           { zap pending box }
        end;
      ShowSelectedColor;
      end;
    end

  { User has clicked on large current color box in center of the screen. }
  else begin
    CueClick;
    { get ready for a swap, dupe, or random palette }
    if (TimeMs - MouseStoreTime) < 333 then begin
      Pending.Erase;
      { Can't do a dissolve inside the event handler, it interferes
        with the mouse, so set a flag instead and trigger it in
        the UserHook procedure. }
      RandomPaletteFlag := true;
      end
    else begin
      MouseStoreTime := TimeMs;
      Pending.Draw;                              { operation pending }
      SelectColor := CurrentColor;
      end;
    end;
END;

{ ------------------------- }

  PROCEDURE EndSliders;
  BEGIN
    { end slider move, restore mouse window }
    if not WindowRestricted then exit;

    FullMouseWindow;
    WindowRestricted := false;
    { restore mouse cursor shape }
    With MenuColors do
      SoftMouseCursor($0000, (ColorMono (MouseColor, MouseMono) shl 8) +
                              Byte (MouseChar));
    ShowMouse;
    Pal.Push;                                    { store it on pal stack }
  END;

{ ------------------------- }

  PROCEDURE DoLeftButtonReleased;
  BEGIN
{
  If the Pending msg is active, then a color operation is pending.
  Left button down sends msg that color duplication is pending,
  left button up means color swap is pending.
}
    if Pending.Status > -1 then begin            { if operation pending }
      Pending.SetStatus (succ (ord (MouseStatus = LeftButton)));
{
  If mouse has been dragged to target color box, then
  duplicate the current color into the target color.
}
      if
        (CharAtMouseCursor = BxChar) and         { if target color box }
        (MouseLastY < 11)
      then begin
        CueClick;
        CurrentColor := AttrAtMouseCursor;       { get current color }
        Pal.Dupe (SelectColor, CurrentColor);    { duplicate }
        Pending.Erase;                           { zap pending msg }
        ShowSelectedColor;                       { update screen }
        end;
      end
    else
      EndSliders;                                { if WindowRestricted }
  END;

{ ------------------------- }

  PROCEDURE DoMouseMoved;
  BEGIN

{ If the Pending window is open then a color operation is pending.
  Left button down sends msg that color duplication is pending,
  left button up means color swap is pending.  }

    if Pending.Status > -1 then begin
      if CharAtMouseCursor = BxChar then
        Pending.SetStatus (succ (ord (MouseStatus = LeftButton)))
      else begin
        Case Pending.Status of
          0, 2 : Pending.SetStatus (succ (ord (MouseStatus = LeftButton)));
          end;  { case}
        end;
      end

{ Any other mouse move would be a slider.  If user is not
  pressing the left button, then end all slider operations now. }

    else
      if MouseStatus <> LeftButton then
        EndSliders

{ If the left button is down and the window is restricted, then
  user is dragging a slider.  If he's not dragging a slider, then
  he clicked while moving the mouse. }

      else
        if WindowRestricted then begin           { drag a slider }
          SliderSound;
          delay (3);
          HideMouse;
          SetSlider (CurrentColor, SliderOption, MouseLastX - 1);
          ShowMouse;
          NoSound;
          end;
  END;

{ ------------------------- }

  PROCEDURE DoLeftButtonPressed;
  BEGIN
    if
      (MouseLastY >= SaturationLine)             { if mouse in slider frame }
        and
      (MouseLastY <= IntensityLine)
    then begin                                   { then set sliders }
      SliderOption := MouseLastY - SaturationLine;
      ShowSliderOption;
      CueClick;
      end;

    case CharAtMouseCursor of
      BxChar       : BoxCharClicked;

      SliderChar,
      LineChar     : begin
                     Pending.Erase;
                     { force window around mouse }
                     MouseWindow (MinCol, SaturationLine + SliderOption,
                                  MaxCol, SaturationLine + SliderOption);
                     WindowRestricted := true;
                     SliderSound;
                     { change Mouse cursor shape }
                     With MenuColors do
                       SoftMouseCursor($0000,
                         (ColorMono (MouseColor, MouseMono) shl 8) +
                           Byte (SliderChar));
                     { update the slider }
                     HideMouse;
                     SetSlider (CurrentColor, SliderOption, MouseLastX - 13);
                     ShowMouse;
                     nosound;
                     end;

      SaturationChar,
      IntensityChar,
      FrameChar    : begin
                     Pending.Erase;
                     if (MouseLastY = SaturationLine) or
                        (MouseLastY = IntensityLine)
                     then
                       With VgaReg, ColorValues do begin
                         Case SliderOption of
                           0 : begin
                               GetSaturation (CurrentColor);
                               With MenuColors do
                                 SoftMouseCursor ($0000,
                                   (ColorMono (MouseColor, MouseMono) shl 8)
                                     + Byte (SaturationChar));
                               end;
                           4 : begin
                               GetIntensity (CurrentColor);
                               With MenuColors do
                                 SoftMouseCursor ($0000,
                                   (ColorMono (MouseColor, MouseMono) shl 8)
                                     + Byte (IntensityChar));
                               end;
                           end; { case }

                         if
                           (SliderOption = 0)
                             or
                           (SliderOption = 4)
                         then begin
                           { force window around mouse }
                           MouseWindow (MinCol,
                                        SaturationLine + SliderOption,
                                        MaxCol,
                                        SaturationLine + SliderOption);
                           WindowRestricted := true;
                           SliderSound;
                           { update the slider }
                           HideMouse;
                           SetSlider
                             (CurrentColor, SliderOption, MouseLastX - 13);
                           ShowMouse;
                           nosound;
                           end;
                         end;  { with VgaReg, ColorValues do begin }
                     end;  { begin }
      end; { case }
  END;

{ ------------------------- }

BEGIN;
  { If right button is pressed, do not allow left button events. }
  if MouseEvent and RightButtonReleased <> 0 then
    RightButtonFlag := false;
  if MouseEvent and RightButtonPressed <> 0 then
    RightButtonFlag := true;
  if RightButtonFlag then exit;

  GotoxyAbs (MouseLastX, MouseLastY);            { send cursor to mouse }
  HideMouse;                                     { no mouse }
  ReadAtCursor
    (CharAtMouseCursor, AttrAtMouseCursor);      { read screen }
  ShowMouse;                                     { return mouse }

  Case MouseEvent of
    MouseMoved + LeftButtonReleased,
    LeftButtonReleased               : DoLeftButtonReleased;
    MouseMoved + LeftButtonPressed,
    LeftButtonPressed                : DoLeftButtonPressed;
    MouseMoved                       : DoMouseMoved;
    end;  { case }
END;

{ MouseEventHandler ======================================================= }

{$F+} PROCEDURE MouseEventHandler; {$F-}

VAR
  Dummy : IntRegisters;

BEGIN;
  if M.ActiveSubPtr <> nil then                  { if submenus are active }
    exit;                                        { don't do anything }
  if PrismHelp.IsActive then exit;               { if help window open }
  if GetFileNameFlag then exit;                  { if getting a file name }

  if MouseEventReentryFlag then exit;            { don't enter here twice }
  MouseEventReentryFlag := true;                 { set reentry flag }

  SwapStackAndCall (@MouseEventKernel,
                    @TempStack [sizeof (TempStack)],
                    Dummy);                      { get real event handler }

  MouseEventReentryFlag := false;                { reset reentry flag }
END;

{ ========================================================================= }
{ SetMouseSpeed =========================================================== }

PROCEDURE SetMouseSpeed (NewSpeed : byte);
BEGIN
  Case NewSpeed of
    0 : SetMickeyToPixelRatio (16, 32);
    1 : SetMickeyToPixelRatio (8, 16);
    2 : SetMickeyToPixelRatio (4, 8);
    3 : SetMickeyToPixelRatio (2, 4);
    end; { case }
  MouseSpeed := NewSpeed;
  { save new default }
END;

{ PostInstructions ======================================================== }

PROCEDURE PostInstructions;
VAR
  Left, Right : byte;
CONST
  LocalColor : byte = LightGray;

BEGIN
  if MouseInstalled then begin
    Left := 3;  Right := 63;
    FastWrite ('Click on square', 13, Left, LocalColor);
    FastWrite ('to change the',   14, Left, LocalColor);
    FastWrite ('active color.',   15, Left, LocalColor);
    FastWrite ('Click on slider', 13, Right, LocalColor);
    FastWrite ('and hue options', 14, Right, LocalColor);
    FastWrite ('to alter shade. ', 15, Right, LocalColor);
    end
  else begin
    Left := 1;  Right := 63;
    FastWrite ('Use Ctrl-Left-Arrow', 13, Left, LocalColor);
    FastWrite ('or Ctrl-Right-Arrow', 14, Left, LocalColor);
    FastWrite ('to select a color.', 15, Left, LocalColor);
    FastWrite ('Use Shift-Arrows', 13, Right, LocalColor);
    FastWrite ('to move sliders ',  14, Right, LocalColor);
    FastWrite ('and hue options.', 15, Right, LocalColor);
    end;
END;

{ ShowMainScreen ========================================================== }

{$F+} PROCEDURE ShowMainScreen; {$F-}
VAR
  Loop : byte;

BEGIN
  MouseStoreTime := TimeMs;                      { mouse click time delay }

  SetBlink (false);                              { no blinking }
  ClrScr;
  for Loop := 0 to 15 do
    DrawAttributeBox (Loop);                     { draw attribute boxes }

  PostInstructions;                              { normal instructions }
  ShowSelectedColor;                             { current color }
  ShowSliderOption;                              { which slider }

{ Menu Initializations ---------------------------------------------------- }

  Status := InitMenu (M);
  if Status <> 0 then begin
    WriteLn('Error initializing menu: ', Status);
    Halt(1);
  end;

  Status := InitHelpLine (H);
  if Status <> 0 then begin
    WriteLn('Error initializing help line: ', Status);
    Halt(1);
  end;
  M.SetCurrentItemProc (UpdateHelpLine);

  H.Draw;                                        { draw help }
  M.Draw;                                        { draw menu }

  if MouseInstalled then
    with MenuColors do begin
      DisableEventHandling;                      { no mouse events yet }

      {activate mouse cursor}
      SoftMouseCursor($0000, (ColorMono (MouseColor, MouseMono) shl 8)+
                             Byte (MouseChar));
      ShowMouse;
      { enable mouse support }
      MenuCommands.cpOptionsOn (cpEnableMouse);
      SetMouseSpeed (MouseSpeed);
      MouseGotoxy (80, 25);                      { go to your corner }
      KeyStateByte := 0;
      end
  else begin
    M.ProtectItem (miMouse3);                    { no mouse help }
    M.ProtectItem (miMouse11);                   { no mouse speed reset }
    KeyStateByte := NumLock;                     { turn editing keys on }
    end;

  CW.InitCustom  (23, 13, 58, 15, MenuColors, wClear + wBordered);
  CW.SetCursor (CuHidden);
  CW.Draw;
  CW.wFastCenter (ProgramName, 1, WhiteOnCyan);
  CW.wFastCenter ('a VGA palette editor', 2, BlackOnCyan);
  CW.wFastCenter ('by David Gerrold', 3, BlackOnCyan);

END;

{ ========================================================================= }
{ PendOb.Init ============================================================= }

PROCEDURE PendOb.Init;
BEGIN
  Status := -1;
END;

{ PendOb.Draw ============================================================= }

PROCEDURE PendOb.Draw;
VAR
  Left, Right : byte;
BEGIN
  Status := 0;
  DrawBox;

  Left := 1;  Right := 62;
  FastWrite (PadCenter ('Color', 18),          13, Left, LightRed);
  FastWrite (PadCenter ('Operation', 18),      14, Left, LightRed);
  FastWrite (PadCenter ('Pending', 18),        15, Left, LightRed);
  FastWrite (PadCenter ('Click to swap ', 16),  13, Right, LightRed);
  FastWrite (PadCenter ('Drag to dupe  ', 16),   14, Right, LightRed);
  FastWrite (PadCenter ('Undo to cancel', 16), 15, Right, LightRed);
END;

{ PendOb.SetStatus ======================================================== }

PROCEDURE PendOb.SetStatus (NewStatus : integer);
CONST
  StatusMsg : array [1 .. 2] of string [12] =
              ('Swap ',
               'Duplication');
VAR
  Left : byte;
BEGIN
  if Status < 0 then exit;
  if Status = NewStatus then exit;

  Status := NewStatus;
  Left := 1;
  FastWrite (PadCenter (StatusMsg [NewStatus], 18), 14, Left, LightRed);
END;

{ PendOb.Erase ============================================================ }

PROCEDURE PendOb.Erase;
BEGIN
  PostInstructions;
  Status := -1;
END;

{ ========================================================================= }
{ EndProc ================================================================= }

{$F+}
PROCEDURE EndProc;
BEGIN
  PrismHelp.Done;                                { no more help object }
  H.Done;                                        { end help }
  M.Done;                                        { end menu }
END;
{$F-}

{ ========================================================================= }
{ RunEditor =============================================================== }

PROCEDURE RunEditor;
VAR
  SliderFlag : boolean;                          { for ccUser4 & ccuser6 }
  StoreTime  : longint;

CONST
  LastccUser : byte = 0;                         { last cursorpad char }

BEGIN
  Pal.Init;                                      { start palette }
  Pending.Init;                                  { initialize pending box }

  LastColor    := 0;
  CurrentColor := succ (Random (15));            { pick a color }
  LastSliderOption := 4;
  SliderOption := 0;                             { which slider }

{ Menu Initializations ---------------------------------------------------- }

  MenuCommands.SetUserHookProc (UserHook);

  MenuCommands.AddCommand (ccQuit, 1, KcCtrlU, 0);

  MenuCommands.AddCommand (ccUser2, 1, KcNumpad2, 0);
  MenuCommands.AddCommand (ccUser4, 1, KcNumpad4, 0);
  MenuCommands.AddCommand (ccUser6, 1, KcNumpad6, 0);
  MenuCommands.AddCommand (ccUser8, 1, KcNumpad8, 0);
  MenuCommands.AddCommand (ccUser10, 1, KcCtrlD, 0);
  MenuCommands.AddCommand (ccUser11, 1, KcCtrlN, 0);
  MenuCommands.AddCommand (ccUser12, 1, KcCtrlR, 0);
  MenuCommands.AddCommand (ccUser13, 1, KcCtrlS, 0);

  MenuCommands.AddCommand (ccUser15, 1, KcCtrlLeftArrow, 0);
  MenuCommands.AddCommand (ccUser16, 1, KcCtrlRightArrow, 0);
  MenuCommands.AddCommand (ccUser17, 1, KcNumPadDot, 0);
  MenuCommands.AddCommand (ccUser18, 1, KcCtrlHomeKey, 0);

  MenuCommands.AddCommand (ccUser20, 1, MouseBoth, 0);

  Status := MenuCommands.GetLastError;
  if Status <> 0 then begin
    WryteLn ('Failed to add commands.  Error: ' + Num2Str (Status));
    halt;
    end;

{ Help Initialization ----------------------------------------------------- }

  { Make a help window with custom options }
  if not PrismHelp.InitMemCustom (9, 8, 72, 18,
                                 MenuColors,
                                 wBordered,
                                 @HelpText,
                                 PickVertical)
  then begin
    WryteLn ('Failed to initialize Help System.');
    halt;
    end;

  { Add some features }
  PrismHelp.EnableExplosions (6);
  PrismHelp.wFrame.AddHeader (' Topic Index ', heTC);
  PrismHelp.AddMoreHeader (' || for more ', heBR, #24, #25, '', 2, 3, 0);
  PrismHelp.AddTopicHeader (1, 60, heTC);
  PrismHelp.AddMoreHelpHeader (
    ' PgUp/PgDn for more ', heBR, 'PgUp', 'PgDn', '/', 2, 7, 6);

  PrismHelp.wFrame.AddShadow (shBr, shSeeThru);
  PrismHelp.hwFrame.AddShadow (shBr, shSeeThru);

  if SfxFlag then
    PrismHelp.wOptionsOn (wSoundEffects);
  HelpCommands.cpOptionsOn (cpEnableMouse);      { Add mouse support }

{ Fade out Dos, Fade in Program ------------------------------------------- }

  FadeStart (ShowMainScreen, co80);              { put up display }
  DissolveProc := SlidePercentages;
  SetMouseEventHandler (AllMouseEvents,
                        @MouseEventHandler);
  EnableEventHandling;

{
  Certain processes need to occur within the ShowMainScreen procedure
  because they need to happen after the DOS screen fades out, but before
  the program screen fades in.  In particular, H.Draw and M.Draw, which
  draw the help line and menu on the screen.

  Mouse event handling must be disabled during fade in and fade out of
  program, because a mouse event may trigger a crash during the dissolve
  process.
}
{ Run program ------------------------------------------------------------- }

  StoreTime := TimeMs;                           { get time }
  repeat
    if StoreTime > TimeMs then                   { allow for midnight }
      StoreTime := TimeMs;
  until                                          { wait until }
    KeyOrButtonPressed or                        { key event }
    ((TimeMs - StoreTime) > 3000);               { or 3 seconds }
  CW.done;                                       { erase colophon }
  ShowSelectedColor;                             { show correct color }

  ExitFlag := false;
  repeat
    M.Process;
    if M.GetLastCommand = ccSelect then begin
      case M.MenuChoice of
        miKeypad2       : GetHelp (miKeypad2);
        miMouse3        : GetHelp (miMouse3);
        miAbout4        : GetHelp (miAbout4);
        miAbout5        : GetHelp (miAbout5);
        miUsing6        : GetHelp (miUsing6);
        miReferences7   : GetHelp (miReferences7);
        miCopyright8    : GetHelp (miCopyright8);
        miSound10       : begin
                          CueClick;
                          SfxFlag := not SfxFlag;
                          if SfxFlag then
                            PrismHelp.wOptionsOn (wSoundEffects)
                          else
                            PrismHelp.wOptionsOff (wSoundEffects);
                          SfxOptions := byte (SfxFlag);
                          end;
        miMouse11       : begin
                          CueClick;
                          UpCycle (MouseSpeed, 0, 3);
                          SetMouseSpeed (MouseSpeed);
                          end;
        miDissolve12    : begin
                          CueClick;
                          UpCycle (DissolveDelay, 0, 6);
                          FadeRate := FadeRateArray [DissolveDelay];
                          end;
        miUndo14        : begin
                          CueClick;
                          M.EraseCurrentSubMenu;
                          GoBack;
                          end;
        miSwap15        : begin
                          CueClick;
                          M.EraseCurrentSubMenu;
                          if
                            (Pending.Status = 1) and
                            (CurrentColor <> SelectColor)
                          then begin
                            Pending.Erase;
                            Pal.Swap (SelectColor, CurrentColor);
                            ShowPercentages (CurrentColor);
                            end
                          else begin
                            Pending.Draw;
                            Pending.SetStatus (1);
                            SelectColor := CurrentColor;
                            end;
                          end;
        miRestore16     : begin;
                          CueClick;
                          M.EraseCurrentSubMenu;
                          Pal.DissolveTo (StoreVgaPal);
                          end;
        miNew17         : begin
                          CueClick;
                          M.EraseCurrentSubMenu;
                          RandomPalette;
                          end;
        miDuplicate18   : begin
                          CueClick;
                          M.EraseCurrentSubMenu;
                          if
                            (Pending.Status = 2) and
                            (CurrentColor <> SelectColor)
                          then begin
                            Pending.Erase;
                            Pal.Dupe (SelectColor, CurrentColor);
                            ShowPercentages (CurrentColor);
                            end
                          else begin
                            Pending.Draw;
                            Pending.SetStatus (2);
                            SelectColor := CurrentColor;
                            end;
                          end;
        miLoad19        : begin
                          CueClick;
                          M.EraseCurrentSubMenu;
                          ReadDiskPalette;
                          end;
        miSave20        : begin
                          CueClick;
                          M.EraseCurrentSubMenu;
                          WritePalette;
                          end;
        miYesExit22     : begin
                          CueClick;
                          M.EraseCurrentSubMenu;
                          DissolveProc := zen;
                          ExitFlag := true;
                          end;
        miNoResume23    : begin
                          CueClick;
                          M.EraseCurrentSubMenu;
                          end;
        end;  { case }
      end
    else

      if M.ActiveSubPtr <> nil then begin        { if active submenu }
        case M.GetLastCommand of
          { Esc, MouseRt, Ctrl-U }
          ccQuit : begin                         { if Quit }
                   M.EraseCurrentSubMenu;        { erase submenu }
                   CueClick;                     { make noise }
                   end;
          end;  { case }
        end

      else begin                                 { if no active submenus }
                                                 { edit the palette }
{
  If the last command was a slider change the the current command
  is not a slider change, then Push the current palette onto the undo
  stack so it can be restored.
}
        if
          not SliderFlag and
          (M.GetLastCommand <> ccUser4) and (M.GetLastCommand <> ccUser6)
        then
          Pal.Push;

{
  Case statement for processing user commands.
}
        case M.GetLastCommand of

          { Undo:  Esc, ^U or Mouse right button }
          ccQuit          : begin
                            CueClick;            { make noise first }
                            if Pending.Status > -1 then
                              Pending.Erase      { cancel pending operation }
                            else                 { or }
                              GoBack;            { restore previous palette }
                            end;

          { NumPad 2 }
          ccUser2         : begin
                            UpCycle (SliderOption, 0, 4);
                            ShowSliderOption;
                            CueClick;
                            end;

          { NumPad 4, 6 }
          ccUser4,
          ccUser6         : With VgaReg, ColorValues do begin
                            SliderFlag := (LastccUser <> ccUser4) and
                                          (LastccUser <> ccUser6);
                            Case SliderOption of
                              0    : if SliderFlag then
                                       GetSaturation (CurrentColor);
                              1..3 : Get (CurrentColor);
                              4    : if SliderFlag then
                                        GetIntensity (CurrentColor);
                              end;  { case }
                            if
                              ((SliderOption > 0) and (SliderOption < 4))
                                or
                              not (SliderFlag
                                and
                              (((SliderOption = 0) and (Saturation = 0))
                                or
                              ((SliderOption = 4) and (Intensity = 0))))
                            then begin
                              { update the slider }
                              Case M.GetLastCommand of
                                ccUser4 : SetSlider
                                            (CurrentColor, SliderOption,
                                               ValArray [SliderOption] - 1);
                                ccUser6 : SetSlider
                                            (CurrentColor, SliderOption,
                                               ValArray [SliderOption] + 1);
                                end;  { case }
                              SliderBeep;
                              end;
                            end;

          { NumPad 8 }
          ccUser8         : begin
                            DownCycle (SliderOption, 0, 4);
                            ShowSliderOption;
                            CueClick;
                            end;
          { ^D for duplicate }
          ccUser10        : begin
                            CueClick;
                            if
                              (Pending.Status = 2) and
                              (CurrentColor <> SelectColor)
                            then begin
                              Pending.Erase;
                              Pal.Dupe (SelectColor, CurrentColor);
                              ShowPercentages (CurrentColor);
                              end
                            else begin
                              Pending.Draw;
                              Pending.SetStatus (2);
                              SelectColor := CurrentColor;
                              end;
                            end;

          { 'N' for new }
          ccUser11        : begin
                            CueClick;
                            RandomPalette;
                            end;

          { 'R', or MouseBoth for restore }
          ccUser12,
          ccUser20        : begin
                            CueClick;
                            Pending.Erase;
                            RightButtonFlag := false;
                            Pal.DissolveTo (StoreVgaPal);
                            end;

          { 'S' for swap }
          ccUser13        : begin
                            CueClick;
                            if
                              (Pending.Status = 1) and
                              (CurrentColor <> SelectColor)
                            then begin
                              Pending.Erase;
                              Pal.Swap (SelectColor, CurrentColor);
                              ShowPercentages (CurrentColor);
                              end
                            else begin
                              Pending.Draw;
                              Pending.SetStatus (1);
                              SelectColor := CurrentColor;
                              end;
                            end;

          { Ctrl Left Arrow }
          ccUser15        : begin
                            DownCycle (CurrentColor, 0, 15);
                            ShowSelectedColor;
                            CueClick;
                            end;

          { Ctrl Right Arrow }
          ccUser16        : begin
                            UpCycle   (CurrentColor, 0, 15);
                            ShowSelectedColor;
                            CueClick;
                            end;

          { Ctrl Home }
          ccUser18        : begin;
                            if CurrentColor > 7 then
                              CurrentColor := CurrentColor - 8
                            else
                              CurrentColor := CurrentColor + 8;
                            ShowSelectedColor;
                            CueClick;
                            end;

          end;  { case }

        LastccUser := M.GetLastCommand;
        end;  { begin }
  until
    ExitFlag;

  DisableEventHandling;                          { no mousing allowed }
  FinalFadeOutProc := EndProc;                   { dispose objects }

  if                                             { if any defaults changed }
    (StoreSfxFlag <> SfxFlag) or
    (StoreMouseSpeed <> MouseSpeed) or
    (StoreDissolveDelay <> DissolveDelay)
  then                                           
    ResetDefaults;                               { reset default settings }
END;

{ ========================================================================= }
{ ========================================================================= }
