#include "task.h"                          // CMSIS RTOS header file
 

void task_misc_fn(void *arg)
{
    int r;
    evt_t e;
    task_handle_t *h=(task_handle_t*)arg;
    
    h->running = 1;
    while(1) {
        r = msg_recv(h->msg, &e, sizeof(e));
        if(r==0) {
            
            switch(e.evt) {
                case EVT_EEPROM:
                {
                    //
                }
                break;
                
                
                default:
                break;
            }
        }
        
    }
    
    
    
    
    
    
    
    
    
    
}



