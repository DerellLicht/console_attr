//****************************************************************************
//  Copyright (c) 2008-2026  Derell Licht
//  config.cpp - manage configuration data file
//****************************************************************************

// [dialog]
// cmdprog=C:\WINDOWS\system32\cmd.exe
// palette=D:\SourceCode\Git\console_attr\palettes\DOS.PLT
// startpath=C:\download
// brighten=3.000000
// window_top=370
// window_left=977

//  config.cpp
extern uint dbg_flags ;
extern uint window_top ;
extern uint window_left ;

LRESULT save_cfg_file(void);
LRESULT init_config(void);

