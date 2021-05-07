#include "drv/i2c.h"
#include "drv/delay.h"
#include "at24cxx.h"
#include "drv/gpio.h"



static handle_t i2c2Handle=NULL;


int at24cxx_init(void)
{
    i2c_cfg_t  ic;
    
    ic.pin.scl.grp = GPIOF;
    ic.pin.scl.pin = GPIO_PIN_1;
    
    ic.pin.sda.grp = GPIOF;
    ic.pin.sda.pin = GPIO_PIN_0;
    
    ic.freq = 100000;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    
    i2c2Handle = i2c_init(&ic);
    
    at24cxx_test();
    
    return 0;
}


int at24cxx_read_byte(U16 addr, U8 *data)
{
    return i2c_mem_read(i2c2Handle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, 1); 
}


int at24cxx_write_byte(U16 addr, U8 *data)
{
    return i2c_mem_write(i2c2Handle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, 1); 
}




int at24cxx_read(U16 addr, U8 *data, U16 len)
{
    int r;
    
    if ( (addr + len) >= MEM_MAX_SIZE ) {/* Check data length */
        return -1;
    }
    
    r = i2c_mem_read(i2c2Handle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, len); /* Write data */
    //delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */
    
    return r;
}


int at24cxx_write(U16 addr, U8 *data, U16 len)
{
    int r;
    U16  start_page    = addr / MEM_PAGE_SIZE; /* Calculating memory starting page */
	U16  end_page      = (addr + len) / MEM_PAGE_SIZE; /* Calculating memory end page */
	U16  page_capacity = ((start_page + 1) * MEM_PAGE_SIZE) - addr; /* Calculating memory page capacity */
	
	if ( (addr + len) >= MEM_MAX_SIZE ) {/* Check data length */
        return -1;
    }
    
    if (page_capacity >= len) {/* Check memory page capacity */
        r = i2c_mem_write(i2c2Handle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, len); /* Write data */
        delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */
    }
    else {
        r = i2c_mem_write(i2c2Handle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, page_capacity); /* Write data */
        delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */

        if (r == 0) {/* Check i2c status */
            addr   += page_capacity; /* Set new address */
            data   += page_capacity; /* Set new data address */
            len    -= page_capacity; /* Set new data length */
            start_page++; /* Increase page number */
            
            for (; (start_page <= end_page) && (r == 0) ;start_page++) {/* Loop for write data */
                if (len < MEM_PAGE_SIZE) {/* Check data length */
                    r = i2c_mem_write(i2c2Handle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, len); /* Write data */
                }
                else {
                    r = i2c_mem_write(i2c2Handle, AT24CXX_ADDR, addr, MEMADD_SIZE, data, MEM_PAGE_SIZE); /* Write data */
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
    
    return r;
}

int at24cxx_test(void)
{
    int i,r;
    U8 x,tmp[100];
  
    x = 0x78;
    r = at24cxx_write_byte(0, &x);
    
    x = 0;
    r = at24cxx_read_byte(0, &x);
    
    for(i=0; i<sizeof(tmp); i++) {
        tmp[i] = i;
    }
    
    r = at24cxx_write(0, tmp, sizeof(tmp));
    
    memset(tmp, 0, sizeof(tmp));
    r = at24cxx_read(0, tmp, sizeof(tmp));
    r = 0;
    
    return r;
}


