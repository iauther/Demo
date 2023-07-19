#include <string.h>
#include "gd32f4xx_spi.h"
#include "gd32f4xx_gpio.h"
#include "sflash.h"
#include "log.h"


#define SPI_PORT                   SPI4
#define SPI_RCU                    RCU_SPI4

#define SFLASH_SECTOR_SIZE         0x1000
#define SFLASH_PAGE_SIZE           0x100
#define SFLASH_CS(hl)              gpio_bit_write(GPIOF,GPIO_PIN_6,hl?SET:RESET)


#define WRITE                      0x02     /* write to memory instruction */
#define QWRITE                     0x32     /* quad write to memory instruction */
#define WRSR                       0x01     /* write status register instruction */
#define WREN                       0x06     /* write enable instruction */
                                   
#define READ                       0x03     /* read from memory instruction */
#define QREAD                      0x6B     /* read from memory instruction */
#define RDSR                       0x05     /* read status register instruction */
#define RDID                       0x9F     /* read identification */
#define RDUID                      0x4B     /* read unique ID number */
                                   
#define SE                         0x20     /* sector erase instruction */
#define BE                         0xC7     /* bulk erase instruction */
                                   
#define WIP_FLAG                   0x01     /* write in progress(wip) flag */
#define DUMMY_BYTE                 0xA5


//////////////////////////////////////////////////////////////////////////////
static U8 sf_tmp[SFLASH_SECTOR_SIZE];

static U8 read_reg(U8 reg);
static int page_write(U32 addr, void* buf, U16 len, U8 check);


static U8 spi_rw_byte(U8 byte)
{
    while(RESET == spi_i2s_flag_get(SPI_PORT,SPI_FLAG_TBE));
    spi_i2s_data_transmit(SPI_PORT,byte);
    while(RESET == spi_i2s_flag_get(SPI_PORT,SPI_FLAG_RBNE));
    return(spi_i2s_data_receive(SPI_PORT));
}

static void wait_busy(void)
{
    while(read_reg(RDSR)&WIP_FLAG);
}

static void write_en(void)
{
    SFLASH_CS(0);
    spi_rw_byte(WREN);  //send "write enable" instruction
    SFLASH_CS(1);
    
    wait_busy();
}

static U8 read_reg(U8 reg)
{
    U8 v;
    
    SFLASH_CS(0);
    spi_rw_byte(reg);
    v = spi_rw_byte(DUMMY_BYTE);
    SFLASH_CS(1);
    
    return v;
}

static int page_write(U32 addr, void* data, U16 len, U8 check)
{
    U16 i;
    U8 *p=(U8*)data;
    
    write_en();

    SFLASH_CS(0);
    spi_rw_byte(WRITE);
    spi_rw_byte((addr & 0xFF0000) >> 16);
    spi_rw_byte((addr & 0xFF00) >> 8);
    spi_rw_byte(addr & 0xFF);

    for(i=0; i<len; i++) {
        spi_rw_byte(p[i]);
    }
    SFLASH_CS(1);

    wait_busy();
    
    if(check) {
        sflash_read(addr, sf_tmp, len);
        if(memcmp(p, sf_tmp, len)) {
            return -1;
        }
    }
    
    return 0;
}

static int pages_write(U32 addr, void* data, U32 len, U8 check)
{
    int r=0;
    U32 i,page1,pages,wlen=0;
    U8 *p=(U8*)data;
    
    if((addr%SFLASH_PAGE_SIZE) || (len%SFLASH_PAGE_SIZE)) {
        LOGE("pages_write, addr or len not align to page size!\n");
        return -1;
    }
    
    page1 = addr/SFLASH_PAGE_SIZE;
    pages = len/SFLASH_PAGE_SIZE;
    for(i=0; i<pages; i++) {
        r = page_write(addr+wlen, p+wlen, SFLASH_PAGE_SIZE, check);
        if(r) {
            break;
        }
        wlen += SFLASH_PAGE_SIZE;
    }
    
    return r;
}


static int is_erased(U32 addr, U32 len)
{
    int r=1;
    U32 i,v;
    
    SFLASH_CS(0);
    spi_rw_byte(READ);
    spi_rw_byte((addr & 0xFF0000) >> 16);
    spi_rw_byte((addr& 0xFF00) >> 8);
    spi_rw_byte(addr & 0xFF);

    for(i=0; i<len; i++) {
        v = spi_rw_byte(DUMMY_BYTE);
        if(v!=0xff) {
            r = 0;
            break;
        }
    }
    SFLASH_CS(1);
    
    return r;
}


static int sec_erased(U32 sec)
{
    U32 addr=sec*SFLASH_SECTOR_SIZE;
    
    return is_erased(addr, SFLASH_SECTOR_SIZE);
}



////////////////////////////////////////////////////////////

int sflash_init(void)
{
    U32 pin;
    static U8 sf_inited=0;
    spi_parameter_struct init;

    if(sf_inited) {
        return 0;
    }
    
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(SPI_RCU);
    
    pin = GPIO_PIN_7|GPIO_PIN_8| GPIO_PIN_9;
    gpio_af_set(GPIOF, GPIO_AF_5, pin);
    gpio_mode_set(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, pin);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, pin);

    gpio_mode_set(GPIOF, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_6);
    gpio_output_options_set(GPIOF, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    SFLASH_CS(1);

    init.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    init.device_mode          = SPI_MASTER;
    init.frame_size           = SPI_FRAMESIZE_8BIT;
    init.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    init.nss                  = SPI_NSS_SOFT;
    init.prescale             = SPI_PSC_8;
    init.endian               = SPI_ENDIAN_MSB;
    spi_init(SPI_PORT, &init);

    spi_enable(SPI_PORT);
    sf_inited = 1;
    
    return 0;
}


int sflash_deinit(void)
{
    spi_i2s_deinit(SPI_PORT);
    return 0;
}


int sflash_erase_all(void)
{
    write_en();

    SFLASH_CS(0);
    spi_rw_byte(BE);
    SFLASH_CS(1);

    wait_busy();
    
    return 0;
}


static int sector_erase(U32 sec)
{
    U32 addr=sec*SFLASH_SECTOR_SIZE;

    if(sec_erased(sec)) {
        return 0;
    }
    
    write_en();

    SFLASH_CS(0);
    spi_rw_byte(SE);
    spi_rw_byte((addr & 0xFF0000) >> 16);
    spi_rw_byte((addr & 0xFF00) >> 8);
    spi_rw_byte(addr & 0xFF);
    SFLASH_CS(1);

    wait_busy();
    
    return 0;
}


int sflash_erase(U32 addr, U32 length)
{
    int r=0,erased;
    U32 i,sec1,sec2;
    U16 rem1,rem2;
    U32 t1,t2,tlen=0;

    sec1 = addr/SFLASH_SECTOR_SIZE;
    rem1 = addr%SFLASH_SECTOR_SIZE;
    
    sec2 = (addr+length)/SFLASH_SECTOR_SIZE;
    rem2 = (addr+length)%SFLASH_SECTOR_SIZE;
    
    LOGD("sflash erase sector from %d to %d\n", sec1, sec2);
    
    if(rem1>0) {
        t1 = rem1;
        t2 = SFLASH_SECTOR_SIZE-t1;
        
        erased = is_erased(addr, t2);
        if(!erased) {
            LOGD("rem1 is not erased, read %d out.\n", t1);
            
            sflash_read(addr-t1, sf_tmp, t1);
            sector_erase(sec1);
            r = page_write(addr-t1, sf_tmp, t1, 1);
            if(r) {
                return -1;
            }
        }
        
        tlen += t2;
        sec1++;
    }
    
    for(i=sec1; i<sec2; i++) {
        sector_erase(i);
        tlen += SFLASH_SECTOR_SIZE;
    }
    
    if(rem2>0) {
        t1 = rem2;
        t2 = SFLASH_SECTOR_SIZE-t1;
        
        erased = is_erased(addr+tlen, t1);
        if(!erased) {
            LOGD("rem2 is not erased, read %d out.\n", t2);
            
            sflash_read(addr+tlen+t1, sf_tmp, t2);
            sector_erase(sec2);
            r = page_write(addr+tlen+t1, sf_tmp, t2, 1);
            if(r) {
                return -1;
            }
        }
    }
    
    return 0;
}



int sflash_erase_sector(U32 sec)
{
    return sector_erase(sec);
}


int sflash_read(U32 addr, void* buf, U32 len)
{
    U32 i;
    U8 *p=(U8*)buf;
    
    SFLASH_CS(0);

    spi_rw_byte(READ);
    spi_rw_byte((addr & 0xFF0000) >> 16);
    spi_rw_byte((addr& 0xFF00) >> 8);
    spi_rw_byte(addr & 0xFF);

    for(i=0; i<len; i++) {
        p[i] = spi_rw_byte(DUMMY_BYTE);
    }

    SFLASH_CS(1);
    
    return 0;
}


int sflash_write(U32 addr, void* buf, U32 len, U8 check)
{
    int r=0;
    U8 *p=(U8*)buf;
    U32 wlen,tlen=0;
    U32 i,page1,page2;
    U16 remain1,remain2;
    U32 waddr=addr;

#if 0
    r = sflash_erase(addr, len);
    if(r) {
        LOGE("sflash_write, erase failed\n");
        return -1;
    }
#endif
    
    page1   = addr/SFLASH_PAGE_SIZE;
    remain1 = addr%SFLASH_PAGE_SIZE;
    
    page2   = (addr+len)/SFLASH_PAGE_SIZE;
    remain2 = (addr+len)%SFLASH_PAGE_SIZE;
    
    if(remain1>0) {
        wlen = SFLASH_PAGE_SIZE-remain1;
        r = page_write(waddr+tlen, p+tlen, wlen, 1);
        if(r) {
            LOGE("sflash_write 1, 0x%x check len %d failed\n", waddr+tlen, wlen);
            return -1;
        }
        
        tlen += wlen;
        page1++;
    }
    
    for(i=page1; i<page2; i++) {
        wlen = SFLASH_PAGE_SIZE;
        r = page_write(waddr+tlen, p+tlen, wlen, check);
        if(r) {
            LOGE("sflash_write 2, 0x%x check len %d failed\n", waddr+tlen, wlen);
            return -1;
        }
        
        tlen += wlen;
    }
    
    if(remain2>0) {
        wlen = remain2;
        r = page_write(waddr+tlen, p+tlen, wlen, check);
        if(r) {
            LOGE("sflash_write 3, 0x%x check len %d failed\n", waddr+tlen, wlen);
            return -1;
        }
        
        tlen += wlen;
    }
    
    return 0;
}



U32 sflash_read_id(void)
{
    U32 temp,temp0,temp1,temp2;

    SFLASH_CS(0);

    spi_rw_byte(RDID);
    temp0 = spi_rw_byte(DUMMY_BYTE);
    temp1 = spi_rw_byte(DUMMY_BYTE);
    temp2 = spi_rw_byte(DUMMY_BYTE);

    SFLASH_CS(1);

    temp = (temp0 << 16) | (temp1 << 8) | temp2;

    return temp;
}

U32 sflash_read_uid(void)
{
    U32 temp,temp0,temp1,temp2;

    SFLASH_CS(0);

    spi_rw_byte(RDUID);
    temp0 = spi_rw_byte(DUMMY_BYTE);
    temp1 = spi_rw_byte(DUMMY_BYTE);
    temp2 = spi_rw_byte(DUMMY_BYTE);

    SFLASH_CS(1);

    temp = (temp0 << 16) | (temp1 << 8) | temp2;

    return temp;
}


#include "cfg.h"
int sflash_test(void)
{
    int r;
    U32 i,id,tlen=0;
    #define TCNT        50000
    #define TLEN        (TCNT*4)
    #define ONELEN      2000
    U32 addr;
    U32 *pbuf=(U32*)malloc(TLEN);
    
    sflash_init();
    
    addr = TMP_APP_OFFSET;
    if(pbuf) {
        sflash_erase(addr, TLEN);
        //sflash_read(addr, pbuf, TLEN);
        
        for(i=0; i<TCNT; i++) {
            pbuf[i] = i+100;
        }
        
        while(1) {
            r = sflash_write(addr+tlen, ((U8*)pbuf)+tlen, ONELEN, 1);
            if(r) {
                LOGE("____ sflash_write failed\n");
                break;
            }
            
            tlen += ONELEN;
            if(tlen>=TLEN) {
                break;
            }
        }
        
        if(r==0) {
            memset(pbuf, 0, TLEN);
            r = sflash_read(addr, pbuf, TLEN);
        }
        
        free(pbuf);
    }
    
    return 0;
}

