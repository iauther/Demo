#include "app.h"
#include "boot.h"

int main(void)
{   
#ifdef BOOTLOADER
    boot_start();
#else
    app_start();
#endif
    
    while(1);
}


