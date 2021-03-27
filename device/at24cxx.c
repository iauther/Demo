#include "drv/i2c.h"
#include "drv/delay.h"
#include "at24cxx.h"


#define AT24CXX     AT24C01

#if (AT24CXX==AT24C01) ||  (AT24CXX==AT24C02)
#define ADDR_SIZE   I2C_MEMADD_SIZE_8BIT
#else
#define ADDR_SIZE   I2C_MEMADD_SIZE_16BIT
#endif

static handle_t i2c2Handle=NULL;


int at24cxx_init(void)
{
    i2c_cfg_t  ic;
    
    ic.pin.scl.grp = GPIOB;
    ic.pin.scl.pin = GPIO_PIN_10;
    
    ic.pin.sda.grp = GPIOB;
    ic.pin.sda.pin = GPIO_PIN_3;
    
    ic.freq = 100000;
    ic.useDMA = 0;;
    ic.callback = NULL;
    ic.finish_flag=0;
    i2c2Handle = i2c_init(&ic);
    
    return 0;
}


int at24cxx_read(U16 addr, U8 *data, U16 len)
{
    int r;
    
    if ( (addr + len) <= MEM_MAX_SIZE ) {/* Check data length */
        return -1;
    }
    
    r = i2c_mem_read(i2c2Handle, DEV_ADDRESS, addr, MEMADD_SIZE, data, len); /* Write data */
    delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */
    
    return r;
}


int at24cxx_write(U16 addr, U8 *data, U16 len)
{
    int r;
    U16  start_page    = addr / MEM_PAGE_SIZE; /* Calculating memory starting page */
	U16  end_page      = (addr + len) / MEM_PAGE_SIZE; /* Calculating memory end page */
	U16  page_capacity = ((start_page + 1) * MEM_PAGE_SIZE) - addr; /* Calculating memory page capacity */
	
	if ( (addr + len) > MEM_MAX_SIZE ) {/* Check data length */
        
    }
    
    if (page_capacity >= len) {/* Check memory page capacity */
        r = i2c_mem_write(i2c2Handle, DEV_ADDRESS,addr, MEMADD_SIZE, data, len); /* Write data */
        delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */
    }
    else {
        r = i2c_mem_write(i2c2Handle, DEV_ADDRESS, addr, MEMADD_SIZE, data, page_capacity); /* Write data */
        delay_ms(MEM_STWC); /* Delay for Self Timed Write Cycle */

        if (r == 0) {/* Check i2c status */
            addr   += page_capacity; /* Set new address */
            data   += page_capacity; /* Set new data address */
            len    -= page_capacity; /* Set new data length */
            start_page++; /* Increase page number */
            
            for (; (start_page <= end_page) && (r == 0) ;start_page++) {/* Loop for write data */
                if (len < MEM_PAGE_SIZE) {/* Check data length */
                    r = i2c_mem_write(i2c2Handle, DEV_ADDRESS, addr, MEMADD_SIZE, data, len); /* Write data */
                }
                else {
                    r = i2c_mem_write(i2c2Handle, DEV_ADDRESS, addr, MEMADD_SIZE, data, MEM_PAGE_SIZE); /* Write data */
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



