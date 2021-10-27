#include "drv/i2c.h"
#include "drv/delay.h"
#include "at24cxx.h"
#include "drv/gpio.h"


extern handle_t i2c2Handle;
static handle_t atHandle=NULL;


int at24cxx_init(void)
{
    gpio_pin_t p=GPIO_AT24CXX_PIN;
    
    gpio_init(&p, MODE_OUTPUT);
    gpio_set_hl(&p, 1);
    
    atHandle = i2c2Handle;
    //at24cxx_test();
    
    return 0;
}


int at24cxx_read_byte(U16 addr, U8 *data)
{
    return i2c_at_read(atHandle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, 1, 10); 
}


int at24cxx_write_byte(U16 addr, U8 *data)
{
    int r;
    gpio_pin_t p=GPIO_AT24CXX_PIN;
    
    gpio_set_hl(&p, 0);
    r = i2c_at_write(atHandle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, 1, 10); 
    gpio_set_hl(&p, 1);
    
    return r;
}




int at24cxx_read(U16 addr, U8 *data, U16 len)
{
    int r;
    gpio_pin_t p=GPIO_AT24CXX_PIN;
    
    if ( !atHandle || (addr + len) >= MEM_MAX_SIZE ) {/* Check data length */
        return -1;
    }
    
    r = i2c_at_read(atHandle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, len, len); /* Write data */
    //delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */
    
    return r;
}


int at24cxx_write(U16 addr, U8 *data, U16 len)
{
    int r;
    gpio_pin_t p=GPIO_AT24CXX_PIN;
    U16  start_page    = addr / MEM_PAGE_SIZE; /* Calculating memory starting page */
	U16  end_page      = (addr + len) / MEM_PAGE_SIZE; /* Calculating memory end page */
	U16  page_capacity = ((start_page + 1) * MEM_PAGE_SIZE) - addr; /* Calculating memory page capacity */
	
	if ( !atHandle || (addr + len) >= MEM_MAX_SIZE ) {/* Check data length */
        return -1;
    }
    
    gpio_set_hl(&p, 0);
    if (page_capacity >= len) {/* Check memory page capacity */
        r = i2c_at_write(atHandle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, len, 20); /* Write data */
        delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */
    }
    else {
        r = i2c_at_write(atHandle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, page_capacity, 20); /* Write data */
        delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */

        if (r == 0) {/* Check i2c status */
            addr   += page_capacity; /* Set new address */
            data   += page_capacity; /* Set new data address */
            len    -= page_capacity; /* Set new data length */
            start_page++; /* Increase page number */
            
            for (; (start_page <= end_page) && (r == 0) ;start_page++) {/* Loop for write data */
                if (len < MEM_PAGE_SIZE) {/* Check data length */
                    r = i2c_at_write(atHandle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, len, 20); /* Write data */
                }
                else {
                    r = i2c_at_write(atHandle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, MEM_PAGE_SIZE, 20); /* Write data */
                    addr    += MEM_PAGE_SIZE; /* Set new address */
                    data    += MEM_PAGE_SIZE; /* Set new data address */
                    len     -= MEM_PAGE_SIZE; /* Set new data length */
                }
                if(r) {
                    break;
                }
                
                delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */
            }
        }
    }
    gpio_set_hl(&p, 1);
    
    return r;
}



///////////////////////////////////////////////////////////////////////////
//static U16 gtmp[1000];
int at24cxx_test(void)
{
    int r;
    U16 i=0;
    U8 tmp=0x38;

#if 0   
    for(i=0; i<sizeof(gtmp)/sizeof(U16); i++) {
        gtmp[i] = i;
    }    
    
    r = at24cxx_write(0, (U8*)gtmp, sizeof(gtmp));
    
    memset(gtmp, 0, sizeof(gtmp));
    r = at24cxx_read(0, (U8*)gtmp, sizeof(gtmp));
    r = 0;
//#else
    while(1) {
        r = at24cxx_read(0, &tmp, sizeof(tmp));
        delay_ms(50);
    }
#endif
    
    
    return r;
}


