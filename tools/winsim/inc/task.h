#ifndef __TASK_Hx__
#define __TASK_Hx__

#include <windows.h>
#include "win.h"
#include "pkt.h"
#include "port.h"
#include "log.h"
#include "data.h"


int task_app_start(void);
int task_app_stop(void);

int task_dev_start(void);
int task_dev_stop(void);

int task_quit(void);

#endif
