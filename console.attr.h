extern char tempstr[1024] ;
extern unsigned curr_attr[16] ;
   
void build_console_list(void);
int  enum_all_consoles(void);
void restore_default_colors(void);
int read_palette_file(char *palette_name, double brighten);
int write_palette_file(char *palette_name, double brighten);
int write_all_consoles(void);

