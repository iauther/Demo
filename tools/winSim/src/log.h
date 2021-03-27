#ifndef __LOG_Hx__
#define __LOG_Hx__


int log(const char* fmt, ...);
int log_clr(void);
int log_en(int flag);
int log_get(void);
int log_save(char *path);

#endif

