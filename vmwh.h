/* vmwh.c */
extern int debug;

/* vmware.c */
extern int16_t host_mouse_x;
extern int16_t host_mouse_y;
extern int mouse_grabbed;
void vmware_check_version(void);
int vmware_get_clipboard(char **);
void vmware_set_clipboard(char *);
void vmware_get_mouse_position(void);
void vmware_set_mouse_position(int, int);

/* x11.c */
void x11_init(void);
int x11_get_clipboard(char **);
void x11_set_clipboard(char *);
void x11_set_cursor(int, int);
