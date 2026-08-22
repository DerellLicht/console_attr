//****************************************************************************
//  Copyright (c) 2008-2026  Derell Licht
//  config.cpp - manage configuration data file
//****************************************************************************

// must be defined by top-level code
// *or* top-level code could assign client_width/client_height 
// whenever cxClient/cyClient change
extern uint cxClient ;
extern uint cyClient ;

//  config.cpp
extern uint dbg_flags ;
extern uint window_top ;
extern uint window_left ;
extern uint client_height ;
extern uint client_width ;

LRESULT save_cfg_file(void);
LRESULT init_config(void);

