#include "spis.h"


#define SPIS_PIN_GRP        2
typedef struct {
    hal_gpio_pin_t          pin;
    U8                      fun;
}pinmux_t;
typedef struct {
    pinmux_t                cs[SPIS_PIN_GRP];
    pinmux_t                clk[SPIS_PIN_GRP];
    pinmux_t                miso[SPIS_PIN_GRP];
    pinmux_t                mosi[SPIS_PIN_GRP];
}spis_pinmuxs_t;

typedef struct {
    pinmux_t                cs;
    pinmux_t                clk;
    pinmux_t                miso;
    pinmux_t                mosi;
}spis_pinmux_t;

#define SPIS_BUF_LEN    1000
static U8 spis_buffer[SPIS_BUF_LEN];

spis_pinmuxs_t spis_pinmuxs[HAL_SPI_SLAVE_MAX]=
{
    {
        {
            {HAL_GPIO_25, HAL_GPIO_25_SLV_SPI0_CS},
            {HAL_GPIO_32, HAL_GPIO_32_SLV_SPI0_CS},
        },
        
        {
            {HAL_GPIO_26, HAL_GPIO_26_SLV_SPI0_SCK},
            {HAL_GPIO_33, HAL_GPIO_33_SLV_SPI0_SCK},
        },
        
        {
            {HAL_GPIO_28, HAL_GPIO_28_SLV_SPI0_MISO},
            {HAL_GPIO_35, HAL_GPIO_35_SLV_SPI0_MISO},
        },
        
        {
            {HAL_GPIO_27, HAL_GPIO_27_SLV_SPI0_MOSI},
            {HAL_GPIO_34, HAL_GPIO_34_SLV_SPI0_MOSI},
        },
    },
};

static spis_callback_t spis_cb[HAL_SPI_SLAVE_MAX];


static int get_pin_info(spis_pin_t *pin, spis_pinmux_t *pfun, spis_info_t *info)
{
    int find=0,j;
    hal_spi_slave_port_t i;
    
    for (i=HAL_SPI_SLAVE_0; i<HAL_SPI_SLAVE_MAX; i++) {
        for (j=0; j<SPIS_PIN_GRP; j++) {
            if(spis_pinmuxs[i].cs[j].pin==pin->cs) {
                if(find && (info->port!=i)) {
                    return -1;
                }
                pfun->cs = spis_pinmuxs[i].cs[j];
                info->port = i;
                find = 1;
            }
            
            if(spis_pinmuxs[i].clk[j].pin==pin->clk) {
                if(find && (info->port!=i)) {
                    return -1;
                }
                pfun->clk = spis_pinmuxs[i].clk[j];
                info->port = i;
                find = 1;
            }
            
            if(spis_pinmuxs[i].mosi[j].pin==pin->cs) {
                if(find && (info->port!=i)) {
                    return -1;
                }
                pfun->mosi = spis_pinmuxs[i].mosi[j];
                info->port = i;
                find = 1;
            }
            
            if(spis_pinmuxs[i].miso[j].pin==pin->cs) {
                if(find && (info->port!=i)) {
                    return -1;
                }
                pfun->miso = spis_pinmuxs[i].miso[j];
                info->port = i;
                find = 1;
            }
        }
    }
    
    return find?0:-1;
}
static int set_pinmux(spis_pin_t *pins, spis_info_t *info)
{
    int r;
    spis_info_t   inf;
    spis_pinmux_t mux;
    
    r = get_pin_info(pins, &mux, &inf);
    if(r==0) {
        hal_gpio_init(mux.cs.pin);
        hal_pinmux_set_function(mux.cs.pin, mux.cs.fun);
        
        hal_gpio_init(mux.clk.pin);
        hal_pinmux_set_function(mux.clk.pin, mux.clk.fun);
        
        hal_gpio_init(mux.miso.pin);
        hal_pinmux_set_function(mux.miso.pin, mux.miso.fun);
        
        hal_gpio_init(mux.mosi.pin);
        hal_pinmux_set_function(mux.mosi.pin, mux.mosi.fun);
        
        if(info) *info = inf;
    }
    
    return r;
    
}

static void spis_slave_callback(hal_spi_slave_transaction_status_t status, void *user_data)
{
    int r;
    U32 rlen;
    hal_spi_slave_callback_event_t slave_status;
    hal_spi_slave_port_t port=*(hal_spi_slave_port_t*)user_data;

    if (HAL_SPI_SLAVE_FSM_SUCCESS_OPERATION == (status.fsm_status)) {
        /* Normal fsm behavior */
        slave_status = status.interrupt_status;
        switch (slave_status) {
            case HAL_SPI_SLAVE_EVENT_POWER_ON:
                /* PDN is turned on, initializes spi slave controller here */
                //spi_slave_poweron_flag = true;
                printf("  ---Receive POWERON command----\r\n");
                break;
            case HAL_SPI_SLAVE_EVENT_POWER_OFF:
                //hal_spi_slave_deinit(SPIS_TEST_PORT);
                //spi_slave_poweron_flag = false;
                printf("  ---Receive POWEROFF command----\r\n");
                break;
            case HAL_SPI_SLAVE_EVENT_CRD_FINISH:
                if(spis_cb[port].callback) {
                    spis_cb[port].callback(port, SPIS_SEND, spis_buffer, rlen);
                }
                r = spis_send(port, spis_buffer, SPIS_BUF_LEN, &rlen);
                break;
            case HAL_SPI_SLAVE_EVENT_CWR_FINISH:
                r = spis_recv(port, spis_buffer, SPIS_BUF_LEN, &rlen);
                if(spis_cb[port].callback) {
                    spis_cb[port].callback(port, SPIS_RECV, spis_buffer, rlen);
                }
                //printf("  ---Receive CWR_FINISH command----\r\n");
                break;
            case HAL_SPI_SLAVE_EVENT_RD_FINISH:
                //spi_slave_read_data_flag = true;
                printf("  ---Receive RD_FINISH command----\r\n");
                break;
            case HAL_SPI_SLAVE_EVENT_WR_FINISH:
                /* User can now get the data from the address set before */
                //spi_slave_write_data_flag = true;
                printf("  ---Receive WR_FINISH command----\r\n");
                break;
            case HAL_SPI_SLAVE_EVENT_RD_ERR:
                /* Data buffer may be reserved for retransmit depending on user's usage */
                //spi_slave_read_error_flag = true;
                printf("  ---detect RD_ERR----\r\n");
                break;
            case HAL_SPI_SLAVE_EVENT_WR_ERR:
                /* Data in the address set before isn't correct, user may abandon them */
                //spi_slave_write_error_flag = true;
                printf("  ---detect WR_ERR----\r\n");
                break;
            case HAL_SPI_SLAVE_EVENT_TIMEOUT_ERR:
                /* timeout happen */
                //spi_slave_timeout_flag = true;
                printf("  ---detect TMOUT_ERR----\r\n");
                break;
            default:
                break;
        }
    } else if (HAL_SPI_SLAVE_FSM_INVALID_OPERATION != (status.fsm_status)) {
        switch (status.fsm_status) {
            case HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CR:
                printf("  HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CR, fsm is poweroff\r\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CW:
                printf("  HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CW, fsm is poweroff\r\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CR:
                printf("  HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CR, fsm is CR\r\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_CR_AFTER_CW:
                printf("  HAL_SPI_SLAVE_FSM_ERROR_CR_AFTER_CW, fsm is CR\r\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CW:
                printf("  HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CW, fsm is CW\r\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_CW_AFTER_CR:
                printf("  HAL_SPI_SLAVE_FSM_ERROR_CW_AFTER_CR, fsm is CW\r\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_WRITE_AFTER_CR:
                printf("  HAL_SPI_SLAVE_FSM_ERROR_WRITE_AFTER_CR, fsm is poweron\r\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_READ_AFTER_CW:
                printf("  HAL_SPI_SLAVE_FSM_ERROR_READ_AFTER_CW, fsm is poweron\r\n");
                break;
            default:
                break;
        }
    } else {
        printf("  HAL_SPI_SLAVE_FSM_INVALID_OPERATION, fsm is poweron\r\n");
    }
}


int spis_init(spis_cfg_t *cfg, spis_info_t *info)
{
    int r;
    spis_info_t inf;
    hal_spi_slave_config_t spi_config;
    
    if(!cfg || !info) {
        return -1;
    }
    
    r = set_pinmux(&cfg->pin, &inf);
    if(r!=0) {
        return r;
    }
    
    spi_config.bit_order = (hal_spi_slave_bit_order_t)cfg->para.lsb;
    spi_config.phase = (hal_spi_slave_clock_phase_t)cfg->para.cpha;
    spi_config.polarity = (hal_spi_slave_clock_polarity_t)cfg->para.cpol;
    spi_config.timeout_threshold = 0xFFFFFFFF;
    if (HAL_SPI_SLAVE_STATUS_OK != hal_spi_slave_init(inf.port, &spi_config)) {
        //printf("SPI slave init fail\r\n");
        return -1;
    }
    
    if (HAL_SPI_SLAVE_STATUS_OK != hal_spi_slave_register_callback(inf.port, spis_slave_callback, spis_cb[inf.port].user_data)) {
        //printf("SPI slave register callback fail\r\n");
        return -1;
    }
    
    spis_cb[inf.port] = cfg->cb;
    if(info) *info = inf;
    
    return 0;
}

    
int spis_set_callback(hal_spi_slave_port_t port, spis_callback_t *cb)
{
    if(!cb) {
        return -1;
    }
    
    if (HAL_SPI_SLAVE_STATUS_OK != hal_spi_slave_register_callback(port, spis_slave_callback, spis_cb[port].user_data)) {
        //printf("SPI slave register callback fail\r\n");
        return -1;
    }
    spis_cb[port] = *cb;
    
    return 0;
}


int spis_recv(hal_spi_slave_port_t port, U8 *data, U32 len, U32 *realen)
{
    uint32_t req_addr,req_len,real_len;
    hal_spi_slave_get_master_write_config(port, &req_addr, &req_len);
    real_len = (req_len>len)?len:req_len;
    hal_spi_slave_receive(port, data, real_len);
    if(realen) *realen = real_len;
    
    return 0;
}


int spis_send(hal_spi_slave_port_t port, U8 *data, U32 len, U32 *realen)
{
    uint32_t req_addr,req_len,real_len;
    hal_spi_slave_get_master_read_config(port, &req_addr, &req_len);
    real_len = (req_len>len)?len:req_len;
    hal_spi_slave_send(port, data, real_len);
    if(realen) *realen = real_len;
    
    return 0;
}






