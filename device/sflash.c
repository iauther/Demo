#include <string.h>
#include "gd32f4xx_spi.h"
#include "gd32f4xx_gpio.h"
#include "sflash.h"
#include "log.h"

#define PAGE(addr)          ((addr)/SFLASH_PAGE_SIZE)
#define PCOUNT(addr)        ((addr)%SFLASH_PAGE_SIZE)

#define SECTOR(addr)        ((addr)/SFLASH_SECTOR_SIZE)
#define SCOUNT(addr)        ((addr)%SFLASH_SECTOR_SIZE)


#define SPI_PORT                   SPI4
#define SPI_RCU                    RCU_SPI4

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
static int sf_read(U32 addr, void *data, U32 len);
static int sf_write(U32 addr, void *data, U32 len, U8 chk);


static U8 spi_rw_byte(U8 byte)
{
    while(RESET == spi_i2s_flag_get(SPI_PORT,SPI_FLAG_TBE));
    spi_i2s_data_transmit(SPI_PORT,byte);
    while(RESET == spi_i2s_flag_get(SPI_PORT,SPI_FLAG_RBNE));
    return(spi_i2s_data_receive(SPI_PORT));
}

static void write_wait(void)
{
    U8 v;
    
    SFLASH_CS(0);
    spi_rw_byte(RDSR);
    v = spi_rw_byte(DUMMY_BYTE);
     do{
        v = spi_rw_byte(DUMMY_BYTE);
     }while(v&WIP_FLAG);
    SFLASH_CS(1);
}

static void write_en(void)
{
    SFLASH_CS(0);
    spi_rw_byte(WREN);  //send "write enable" instruction
    SFLASH_CS(1);
}
static int do_check(U32 addr, void *data, U32 len)
{
    int r=0;
    U8 *p=(U8*)data,*tmp;
    U32 i,cnt,rem,tlen=0;
    U32 ulen=SFLASH_SECTOR_SIZE;
    
    tmp = (U8*)malloc(ulen);
    if(!tmp) {
        return -1;
    }
    cnt = len/ulen;
    rem = len%ulen;

    for(i=0; i<cnt; i++) {
        sf_read(addr+tlen, tmp, ulen);
        if(memcmp(p+tlen, tmp, ulen)) {
            LOGE("___ sf_check failed 1, addr: 0x%x, len: %d\n", addr+tlen, ulen);
            r = -1;
            break;
        }
        tlen += ulen;
    }
    if(r==0 && rem) {
        sf_read(addr+tlen, tmp, rem);
        if(memcmp(p+tlen, tmp, rem)) {
            LOGE("___ sf_check failed 2, addr: 0x%x, len: %d\n", addr+tlen, rem);
            r = -1; 
        }
    }
    free(tmp);
    
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
static int spi_write(U32 addr, void *data, U32 len)
{
    U32 i;
    U8 *p=(U8*)data;
    
    if(len>SFLASH_PAGE_SIZE) {
        return -1;
    }
    
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
    
    write_wait();
    
    return 0;
}


static int page_write(U32 page, void *data, U32 len)
{
    U32 addr=(page*SFLASH_PAGE_SIZE);
    
    if(len>SFLASH_PAGE_SIZE) {
        return -1;
    }
    
    return spi_write(addr, data, len);
}

static int is_sec_erased(U32 sec)
{
    U32 addr=sec*SFLASH_SECTOR_SIZE;
    
    return is_erased(addr, SFLASH_SECTOR_SIZE);
}
static int sec_erase(U32 sec)
{
    U32 addr=sec*SFLASH_SECTOR_SIZE;
    
    if(is_sec_erased(sec)) {
        return 0;
    }
    
    write_en();

    SFLASH_CS(0);
    spi_rw_byte(SE);
    spi_rw_byte((addr & 0xFF0000) >> 16);
    spi_rw_byte((addr & 0xFF00) >> 8);
    spi_rw_byte(addr & 0xFF);
    SFLASH_CS(1);

    write_wait();
    
    return 0;
}
static int sec_read(U32 sec, void *data, U32 len)
{
    U32 addr=sec*SFLASH_SECTOR_SIZE;
    
    return sf_read(addr, data, SFLASH_SECTOR_SIZE);
}
static int sec_write(U32 sec, void *data, U32 len, U8 chk)
{
    int r;
    U32 addr=sec*SFLASH_SECTOR_SIZE;
    
    r = sec_erase(sec);
    if(r) {
        return r;
    }
    
    return sf_write(addr, data, len, chk);
}

static int sf_read(U32 addr, void *data, U32 len)
{
    U32 i;
    U8 *p=(U8*)data;
    
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

static int sf_write(U32 addr, void *data, U32 len, U8 chk)
{
    int r=0;
    U32 i,s,s1,e,e1,tlen=0;
    U8 *p=(U8*)data;
    U32 ulen=SFLASH_PAGE_SIZE;
    
    s  = PAGE(addr);     s1 = PCOUNT(addr);
    e  = PAGE(addr+len); e1 = PCOUNT(addr+len);
    if(s1) {
        r = spi_write(addr, p, ulen-s1);
        if(r) {
            return -1;
        }
        s++;
        tlen += ulen-s1;
    }
    
    for(i=s; i<e; i++) {
        r = page_write(i, p+tlen, ulen);
        if(r) {
            break;
        }
        tlen += ulen;
    }
    
    if(r==0 && e1) {
        r = page_write(e, p+tlen, e1);
    }
    
    if(r==0 && chk) {
        r = do_check(addr, data, len);
    }
    
    return r;
}


static int sf_erase(U32 addr, U32 len)
{
    int r=0;
    U32 i,s,s1,e,e1;
    U32 ulen=SFLASH_SECTOR_SIZE;
    
    s = SECTOR(addr);      s1 = SCOUNT(addr);
    e = SECTOR(addr+len);  e1 = SCOUNT(addr+len);
    if(s1) {
        U8 *p=(U8*)malloc(ulen);
        if(!p) {
            LOGE("___ sf_erase 1, malloc %d failed\n", ulen);
            return -1;
        }
        sec_read(s, p, s1); 
        r = sec_write(s, p, s1, 1);
        free(p);
        
        if(r) {
            return -1;
        }
    }
    
    for(i=s; i<e; i++) {
        r = sec_erase(i);
        if(r) {
            return -1;
        }
    }
    
    if((s1==0 || e>s) && e1) {
        U8 *p=(U8*)malloc(ulen);
        if(!p) {
            LOGE("___ sf_erase 2, malloc %d failed\n", ulen);
            return -1;
        }
        sf_read(addr+len, p, ulen-e1);
        r = sec_erase(e);
        if(r==0) {
            r = sf_write(addr+len, p, ulen-e1, 1);
        }
        free(p);
        
        if(r) {
            return -1;
        }
    }
    
    return r;
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

    write_wait();
    
    return 0;
}


int sflash_erase(U32 addr, U32 len)
{
    return sf_erase(addr, len);
}



int sflash_erase_sector(U32 sec)
{
    return sec_erase(sec);
}


int sflash_write_sector(U32 sec, void *data)
{
    return sec_write(sec, data, SFLASH_SECTOR_SIZE, 1);
}


int sflash_read(U32 addr, void *data, U32 len)
{
    return sf_read(addr, data, len);
}


int sflash_write(U32 addr, void *data, U32 len, U8 erase, U8 chk)
{
    int r=0;

    if(erase) {
        r = sf_erase(addr, len);
        if(r) {
            LOGE("sflash_write, erase failed\n");
            return -1;
        }
    }
    
    r = sf_write(addr, data, len, chk);
    
    return r;
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
    U32 i,tlen=0;
    #define TCNT        5000
    #define TLEN        (TCNT*4)
    #define ONELEN      2000
    U32 addr;
    U32 *pbuf=(U32*)malloc(TLEN);
    
    sflash_init();
    
    addr = TMP_APP_OFFSET+0x1234;
    if(pbuf) {
        sflash_erase(addr, TLEN);
        //sflash_read(addr, pbuf, TLEN);
        
        for(i=0; i<TCNT; i++) {
            pbuf[i] = i;
        }
        
        r = sflash_write(addr, pbuf, TLEN, 0, 1);
        if(r==0) {
            memset(pbuf, 0, TLEN);
            r = sflash_read(addr, pbuf, TLEN);
        }
        
        free(pbuf);
    }
    
    return 0;
}

