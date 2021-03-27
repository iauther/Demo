#include "dev/ch101/inc/chirp_bsp.h"
#include "dev/ch101/ch101.h"


#define  DATA_READY_FLAG (1<<0)
#define  IQ_READY_FLAG   (1<<1)
#define	 CHIRP_SENSOR_FW_INIT_FUNC	ch101_gpr_open_init





typedef struct {
	uint32_t		range;						// from ch_get_range()
	uint16_t		amplitude;					// from ch_get_amplitude()
	uint16_t		num_samples;				// from ch_get_num_samples()
#ifdef READ_AMPLITUDE_DATA
	uint16_t		amp_data[DATA_MAX_NUM_SAMPLES];	
												// from ch_get_amplitude_data()
#endif
#ifdef READ_IQ_DATA
	ch_iq_sample_t	iq_data[DATA_MAX_NUM_SAMPLES];	
												// from ch_get_iq_data()
#endif
} ch_data_t;






		
ch_dev_t    ch_dev;		
ch_group_t 	ch_grp;
ch_data_t	ch_data;
volatile U8 data_flag=0;
static void sensor_int_callback(ch_group_t *grp_ptr, uint8_t dev_num)
{
    data_flag |= DATA_READY_FLAG;
    chbsp_io_interrupt_enable(&ch_dev);
}
static void io_complete_callback(ch_group_t *grp_ptr)
{
    data_flag |= IQ_READY_FLAG;
}
static void ch_timer_callback(void)
{
    ch_trigger(&ch_dev);
}


static void ch_config()
{
    uint8_t error;
    ch_config_t dev_config;
    ch_dev_t *dev_ptr = &ch_dev;

    if (ch_sensor_is_connected(dev_ptr)) {
        /* Select sensor mode 
         *   All connected sensors are placed in hardware triggered mode.
         *   The first connected (lowest numbered) sensor will transmit and 
         *   receive, all others will only receive.
         */
        dev_config.mode = CH_MODE_TRIGGERED_TX_RX;

        /* Init config structure with values from app_config.h */
        dev_config.max_range       = 750;//CHIRP_SENSOR_MAX_RANGE_MM;
        dev_config.static_range    = 0;//CHIRP_SENSOR_STATIC_RANGE;

        /* If sensor will be free-running, set internal sample interval */
        if (dev_config.mode == CH_MODE_FREERUN) {
            dev_config.sample_interval = 10;
        } else {
            dev_config.sample_interval = 0;
        }

        dev_config.thresh_ptr = 0;							

        /* Apply sensor configuration */
        error = ch_set_config(dev_ptr, &dev_config);  
    }
}



static uint8_t handle_data_ready(ch_dev_t *dev_ptr)
{
	uint8_t 	dev_num;
	int 		num_samples = 0;
	uint8_t 	r = 0;

    if (ch_sensor_is_connected(dev_ptr)) {

        /* Get measurement results from each connected sensor 
         *   For sensor in transmit/receive mode, report one-way echo 
         *   distance,  For sensor(s) in receive-only mode, report direct 
         *   one-way distance from transmitting sensor 
         */
        
        if (ch_get_mode(dev_ptr) == CH_MODE_TRIGGERED_RX_ONLY) {
            ch_data.range = ch_get_range(dev_ptr, CH_RANGE_DIRECT);
        } else {
            ch_data.range = ch_get_range(dev_ptr, CH_RANGE_ECHO_ONE_WAY);
        }

        if (ch_data.range == CH_NO_TARGET) {
            /* No target object was detected - no range value */
            ch_data.amplitude = 0;  /* no updated amplitude */
            printf("Port %d:          no target found        ", dev_num);

        } else {
            /* Target object was successfully detected (range available) */
             /* Get the new amplitude value - it's only updated if range 
              * was successfully measured.  */
            ch_data.amplitude = ch_get_amplitude(dev_ptr);
            printf("Port %d:  Range: %0.1f mm  Amp: %u  ", dev_num, (float) ch_data.range/32.0f, ch_data.amplitude);
        }

        /* Store number of active samples in this measurement */
        num_samples = ch_get_num_samples(dev_ptr);
        ch_data.num_samples = num_samples;

        /* Optionally read amplitude values for all samples */
#ifdef READ_AMPLITUDE_DATA
        uint16_t 	start_sample = 0;
        ch_get_amplitude_data(dev_ptr, ch_data.amp_data, start_sample, num_samples, CH_IO_MODE_BLOCK);

#ifdef OUTPUT_AMPLITUDE_DATA
        printf("\n");
        for (uint8_t count = 0; count < num_samples; count++) {
            printf("%d\n",  ch_data.amp_data[count]);
        }
#endif
#endif

        /* Optionally read raw I/Q values for all samples */
#ifdef READ_IQ_DATA
        display_iq_data(dev_ptr);
#endif
        /* If more than 2 sensors, put each on its own line */
    }
	
	return r;
}




int ch101_init2(void)
{
    
    uint8_t r;
       
    r = ch_init(&ch_dev, &ch_grp, 1, CHIRP_SENSOR_FW_INIT_FUNC);
    //printf("_____ ch_init %d\n", r);
    if(r) 
        return -1;
       
    r = ch_group_start(&ch_grp);
    //printf("_____ ch_group_start %d\n", r);
    if(r) 
        return -1;
    
    r = ch_sensor_is_connected(&ch_dev);
    //printf("_____ ch_sensor_is_connected %d\n", r);
    if(r!=1) 
        return -1;
    
    ch_io_int_callback_set(&ch_grp, sensor_int_callback);
    ch_io_complete_callback_set(&ch_grp, io_complete_callback);
    
    ch_set_rx_pretrigger(&ch_grp, 1);
    
    //start timer....
    //timer_start(ch_timer_callback);
    
    while(1) {

		if (data_flag & DATA_READY_FLAG) {
			data_flag &= ~DATA_READY_FLAG;		// clear flag
			handle_data_ready(&ch_dev);
		}
        
        //delay_ms(10);
    }
    
    return r;
}











