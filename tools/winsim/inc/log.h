#ifndef __LOG_Hx__
#define __LOG_Hx__


int logx(const char* fmt,...);
int logx_clr(void);
int logx_en(int flag);
int logx_get(void);
int logx_save(char *path);


#define LOG  logx

#endif

