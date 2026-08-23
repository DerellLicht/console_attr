extern unsigned curr_attr[16] ;

// console.attr.cpp
extern char palette_filename[MAX_PATH_LEN+1] ;
extern char cmd_proc_filename[MAX_PATH_LEN+1] ;
extern char starting_path[MAX_PATH_LEN+1] ;
extern double brighten ;

int setup_palette_filename();
void build_console_list(void);
int  enum_all_consoles(void);
void restore_default_colors(void);
int read_palette_file(char *palette_name, double dbrighten);
int write_palette_file(char *palette_name, double dbrighten);
int write_all_consoles(void);
