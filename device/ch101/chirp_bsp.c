/*!
 * \file chbsp_dummy.c
 *
 * \brief Dummy implementations of optional board support package IO functions allowing 
 * platforms to selectively support only needed functionality.  These are placeholder
 * routines that will satisfy references from other code to avoid link errors, but they 
 * do not peform any actual operations.
 *
 * See chirp_bsp.h for descriptions of all board support package interfaces, including 
 * details on these optional functions.
 */

/*
 * Copyright © 2017-2019 Chirp Microsystems.  All rights reserved.
 */

#include "dev/ch101/inc/chirp_bsp.h"
#include "inc/types.h"
#include "drv/i2c.h"
#include "drv/gpio.h"
#include "drv/delay.h"

#define CH101_ADDR          CH_I2C_ADDR_PROG    //{45, 43, 44, 42 }
#define CH101_INT_PIN       {GPIOB, GPIO_PIN_3}
#define CH101_PROG_PIN      {GPIOB, GPIO_PIN_4}
#define CH101_RESET_PIN     {GPIOB, GPIO_PIN_5}


void chbsp_debug_toggle(uint8_t dbg_pin_num){}
void chbsp_debug_on(uint8_t dbg_pin_num){}
void chbsp_debug_off(uint8_t dbg_pin_num){}
    
    
void chbsp_board_init(ch_group_t *grp_ptr)
{
    gpio_pin_t pin_int=CH101_RESET_PIN;
    gpio_pin_t pin_prog=CH101_RESET_PIN;
    gpio_pin_t pin_reset=CH101_RESET_PIN;
    
	grp_ptr->num_ports = 1;
	grp_ptr->num_i2c_buses = CHIRP_NUM_I2C_BUSES;
	grp_ptr->rtc_cal_pulse_ms = 100;
	
    gpio_init(&pin_int,   MODE_IT_RISING);
    gpio_init(&pin_prog,  MODE_OUTPUT);
    gpio_init(&pin_reset, MODE_OUTPUT);
}
void chbsp_reset_assert(void)
{
    gpio_pin_t pin=CH101_RESET_PIN;
    gpio_set_hl(&pin, 0);
}
void chbsp_reset_release(void)
{
    gpio_pin_t pin=CH101_RESET_PIN;
    gpio_set_hl(&pin, 1);
}

void chbsp_program_enable(ch_dev_t *dev_ptr)
{
    gpio_pin_t pin=CH101_PROG_PIN;
    gpio_set_hl(&pin, 1);
}


void chbsp_program_disable(ch_dev_t *dev_ptr)
{
    gpio_pin_t pin=CH101_PROG_PIN;
    gpio_set_hl(&pin, 0);
}


void chbsp_set_io_dir_out(ch_dev_t *dev_ptr)
{
    gpio_pin_t pin=CH101_INT_PIN;
    gpio_set_dir(&pin, OUTPUT);
}
void chbsp_set_io_dir_in(ch_dev_t *dev_ptr)
{
    gpio_pin_t pin=CH101_INT_PIN;
    gpio_set_dir(&pin, INPUT);
}


void chbsp_io_clear(ch_dev_t *dev_ptr)
{
    gpio_pin_t pin=CH101_INT_PIN;
    gpio_set_hl(&pin, 0);
}

void chbsp_io_set(ch_dev_t *dev_ptr)
{
    gpio_pin_t pin=CH101_INT_PIN;
    gpio_set_hl(&pin, 1);
}


void chbsp_io_interrupt_enable(ch_dev_t *dev_ptr)
{
    gpio_pin_t pin=CH101_INT_PIN;
    if (ch_sensor_is_connected(dev_ptr)) {
        gpio_irq_en(&pin, 1);
    }
}


void chbsp_io_interrupt_disable(ch_dev_t *dev_ptr)
{
    gpio_pin_t pin=CH101_INT_PIN;
    if (dev_ptr->sensor_connected) {
        gpio_irq_en(&pin, 0);
    }
}

static ch_io_int_callback_t ch_io_func=NULL;
static void io_func(U16 pin)
{
    if(ch_io_func) {
        ch_io_func(NULL, 0);
    }
}
void chbsp_io_callback_set(ch_io_int_callback_t callback_func_ptr)
{
    gpio_pin_t pin=CH101_INT_PIN;
    gpio_set_callback(&pin, io_func);
    ch_io_func = callback_func_ptr;
}


void chbsp_group_set_io_dir_out(ch_group_t *grp_ptr)
{
    chbsp_set_io_dir_out(ch_get_dev_ptr(grp_ptr, 0));
}
void chbsp_group_set_io_dir_in(ch_group_t *grp_ptr)
{
    chbsp_set_io_dir_in(ch_get_dev_ptr(grp_ptr, 0));
}
void chbsp_group_pin_init(ch_group_t *grp_ptr)
{
    chbsp_reset_assert();
    chbsp_program_disable(ch_get_dev_ptr(grp_ptr, 0));
    chbsp_program_enable(ch_get_dev_ptr(grp_ptr, 0));
}
void chbsp_group_io_clear(ch_group_t *grp_ptr)
{
    chbsp_io_clear(ch_get_dev_ptr(grp_ptr, 0));
}
void chbsp_group_io_set(ch_group_t *grp_ptr)
{
    chbsp_io_set(ch_get_dev_ptr(grp_ptr, 0));
}
void chbsp_group_io_interrupt_enable(ch_group_t *grp_ptr)
{
    chbsp_io_interrupt_enable(ch_get_dev_ptr(grp_ptr, 0));
}
void chbsp_group_io_interrupt_disable(ch_group_t *grp_ptr)
{
    chbsp_io_interrupt_disable(ch_get_dev_ptr(grp_ptr, 0));
}


void chbsp_delay_us(uint32_t us)
{
    delay_us(us);
}


void chbsp_delay_ms(uint32_t ms)
{
    delay_ms(ms);
}


uint32_t chbsp_timestamp_ms(void)
{
    return HAL_GetTick();
}


int chbsp_i2c_init(void)
{
    return 0;
}


int chbsp_i2c_deinit(void)
{
    return 0;
}


uint8_t chbsp_i2c_get_info(ch_group_t *grp_ptr, uint8_t dev_num, ch_i2c_info_t *info_ptr)
{
    if(!info_ptr) {
        return 1;
    }
    
    info_ptr->address = 0x55;
    info_ptr->bus_num = 0;
    info_ptr->drv_flags = 0;

    return 0;
}


int chbsp_i2c_write(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes)
{
    return i2c_write(I2C_PORT, dev_ptr->app_i2c_address, data, num_bytes);
}
int chbsp_i2c_mem_write(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes)
{
    i2c_write(I2C_PORT, dev_ptr->app_i2c_address, (U8*)&mem_addr, 2);
    return i2c_write(I2C_PORT, dev_ptr->app_i2c_address, data, num_bytes);
}

int chbsp_i2c_write_nb(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes)
{
    return chbsp_i2c_write(dev_ptr, data, num_bytes);
}
int chbsp_i2c_mem_write_nb(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes)
{
    return chbsp_i2c_mem_write(dev_ptr, mem_addr, data, num_bytes);
}
////////////////////////////////////////////////////////////////////////////////////////

int chbsp_i2c_read(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes)
{
    return i2c_read(I2C_PORT, dev_ptr->app_i2c_address, data, num_bytes);
}
int chbsp_i2c_mem_read(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes)
{
    i2c_write(I2C_PORT, dev_ptr->app_i2c_address, (U8*)&mem_addr, 2);
    return i2c_read(I2C_PORT, dev_ptr->app_i2c_address, data, num_bytes);
}

int chbsp_i2c_read_nb(ch_dev_t *dev_ptr, uint8_t *data, uint16_t num_bytes)
{
    return i2c_read(I2C_PORT, dev_ptr->app_i2c_address, data, num_bytes);
}
int chbsp_i2c_mem_read_nb(ch_dev_t *dev_ptr, uint16_t mem_addr, uint8_t *data, uint16_t num_bytes)
{
    return chbsp_i2c_mem_read(dev_ptr, mem_addr, data, num_bytes);
}



void chbsp_external_i2c_irq_handler(chdrv_i2c_transaction_t *trans){}


uint8_t chbsp_periodic_timer_init(uint16_t interval_ms, ch_timer_callback_t callback_func_ptr){return 0;}
void chbsp_periodic_timer_irq_enable(void){}
void chbsp_periodic_timer_irq_disable(void){}
uint8_t chbsp_periodic_timer_start(void){return 0;}
uint8_t chbsp_periodic_timer_stop(void){return 0;}
void chbsp_i2c_reset(ch_dev_t *dev_ptr){}
void chbsp_periodic_timer_handler(void){}
void chbsp_periodic_timer_change_period(uint32_t new_period_us){}
void chbsp_proc_sleep(void){}
void chbsp_led_on(uint8_t dev_num){}
void chbsp_led_off(uint8_t dev_num){}
void chbsp_led_toggle(uint8_t dev_num){}
void chbsp_print_str(char *str){}













