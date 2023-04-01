#ifndef __PCM186x_Hx__
#define __PCM186x_Hx__

#include "types.h"

enum {
    PCM186X_ID_1=0,
    PCM186X_ID_2,
    
    PCM186X_ID_MAX
};


// 主板ADC I2C地址(7位)
#define ADC1_I2CADDR_TI 	            (0x94>>1) // MS/AD = 0
#define ADC2_I2CADDR_TI 	            (0x96>>1) // MS/AD = 1
// 扩展板ADC I2C地址(7位)
#define ADC3_I2CADDR_TI 	            (0x94>>1) // MS/AD = 0
#define ADC4_I2CADDR_TI 	            (0x96>>1) // MS/AD = 1

/* The page selection register address is the same on all pages */
#define PCM186X_PAGE			        0x00

#define PCM186X_RESET			        0xFE

/* Register Definitions - Page 0 */
#define PCM186X_PGA_VAL_CH1_L		    (1)
#define PCM186X_PGA_VAL_CH1_R		    (2)
#define PCM186X_PGA_VAL_CH2_L		    (3)
#define PCM186X_PGA_VAL_CH2_R		    (4)
#define PCM186X_PGA_CTRL		        (5)
#define PCM186X_ADC1_INPUT_SEL_L	    (6)
#define PCM186X_ADC1_INPUT_SEL_R	    (7)
#define PCM186X_ADC2_INPUT_SEL_L	    (8)
#define PCM186X_ADC2_INPUT_SEL_R	    (9)
#define PCM186X_AUXADC_INPUT_SEL	    (10)
#define PCM186X_PCM_CFG			        (11)
#define PCM186X_TDM_TX_SEL		        (12)
#define PCM186X_TDM_TX_OFFSET		    (13)
#define PCM186X_TDM_RX_OFFSET		    (14)
#define PCM186X_DPGA_VAL_CH1_L		    (15)
#define PCM186X_GPIO1_0_CTRL		    (16)
#define PCM186X_GPIO3_2_CTRL		    (17)
#define PCM186X_GPIO1_0_DIR_CTRL	    (18)
#define PCM186X_GPIO3_2_DIR_CTRL	    (19)
#define PCM186X_GPIO_IN_OUT		        (20)
#define PCM186X_GPIO_PULL_CTRL		    (21)
#define PCM186X_DPGA_VAL_CH1_R		    (22)
#define PCM186X_DPGA_VAL_CH2_L		    (23)
#define PCM186X_DPGA_VAL_CH2_R		    (24)
#define PCM186X_PGA_GAIN_CTRL		    (25)
#define PCM186X_DPGA_MIC_CTRL		    (26)
#define PCM186X_DIN_RESAMP_CTRL		    (27)
#define PCM186X_CLK_CTRL		        (32)
#define PCM186X_DSP1_CLK_DIV		    (33)
#define PCM186X_DSP2_CLK_DIV		    (34)
#define PCM186X_ADC_CLK_DIV		        (35)
#define PCM186X_PLL_SCK_DIV		        (37)
#define PCM186X_BCK_DIV			        (38)
#define PCM186X_LRK_DIV			        (39)
#define PCM186X_PLL_CTRL		        (40)
#define PCM186X_PLL_P_DIV		        (41)
#define PCM186X_PLL_R_DIV		        (42)
#define PCM186X_PLL_J_DIV		        (43)
#define PCM186X_PLL_D_DIV_LSB		    (44)
#define PCM186X_PLL_D_DIV_MSB		    (45)
#define PCM186X_SIGDET_MODE		        (48)
#define PCM186X_SIGDET_MASK		        (49)
#define PCM186X_SIGDET_STAT		        (50)
#define PCM186X_SIGDET_LOSS_TIME	    (51)
#define PCM186X_SIGDET_SCAN_TIME	    (52)
#define PCM186X_SIGDET_INT_INTVL	    (54)
#define PCM186X_SIGDET_DC_REF_CH1_L	    (64)
#define PCM186X_SIGDET_DC_DIFF_CH1_L	(65)
#define PCM186X_SIGDET_DC_LEV_CH1_L	    (66)
#define PCM186X_SIGDET_DC_REF_CH1_R	    (67)
#define PCM186X_SIGDET_DC_DIFF_CH1_R	(68)
#define PCM186X_SIGDET_DC_LEV_CH1_R	    (69)
#define PCM186X_SIGDET_DC_REF_CH2_L	    (70)
#define PCM186X_SIGDET_DC_DIFF_CH2_L	(71)
#define PCM186X_SIGDET_DC_LEV_CH2_L	    (72)
#define PCM186X_SIGDET_DC_REF_CH2_R	    (73)
#define PCM186X_SIGDET_DC_DIFF_CH2_R	(74)
#define PCM186X_SIGDET_DC_LEV_CH2_R	    (75)
#define PCM186X_SIGDET_DC_REF_CH3_L	    (76)
#define PCM186X_SIGDET_DC_DIFF_CH3_L	(77)
#define PCM186X_SIGDET_DC_LEV_CH3_L	    (78)
#define PCM186X_SIGDET_DC_REF_CH3_R	    (79)
#define PCM186X_SIGDET_DC_DIFF_CH3_R	(80)
#define PCM186X_SIGDET_DC_LEV_CH3_R	    (81)
#define PCM186X_SIGDET_DC_REF_CH4_L	    (82)
#define PCM186X_SIGDET_DC_DIFF_CH4_L	(83)
#define PCM186X_SIGDET_DC_LEV_CH4_L	    (84)
#define PCM186X_SIGDET_DC_REF_CH4_R	    (85)
#define PCM186X_SIGDET_DC_DIFF_CH4_R	(86)
#define PCM186X_SIGDET_DC_LEV_CH4_R	    (87)
#define PCM186X_AUXADC_DATA_CTRL	    (88)
#define PCM186X_AUXADC_DATA_LSB		    (89)
#define PCM186X_AUXADC_DATA_MSB		    (90)
#define PCM186X_INT_ENABLE		        (96)
#define PCM186X_INT_FLAG		        (97)
#define PCM186X_INT_POL_WIDTH		    (98)
#define PCM186X_POWER_CTRL		        (112)
#define PCM186X_FILTER_MUTE_CTRL	    (113)
#define PCM186X_DEVICE_STATUS		    (114)
#define PCM186X_FSAMPLE_STATUS		    (115)
#define PCM186X_DIV_STATUS		        (116)
#define PCM186X_CLK_STATUS		        (117)
#define PCM186X_SUPPLY_STATUS		    (120)

/* Register Definitions - Page 1 */
#define PCM186X_MMAP_STAT_CTRL		    (1)
#define PCM186X_MMAP_ADDRESS		    (2)
#define PCM186X_MEM_WDATA0		        (4)
#define PCM186X_MEM_WDATA1		        (5)
#define PCM186X_MEM_WDATA2		        (6)
#define PCM186X_MEM_WDATA3		        (7)
#define PCM186X_MEM_RDATA0		        (8)
#define PCM186X_MEM_RDATA1		        (9)
#define PCM186X_MEM_RDATA2		        (10)
#define PCM186X_MEM_RDATA3		        (11)

/* Register Definitions - Page 3 */
#define PCM186X_OSC_PWR_DOWN_CTRL	    (18)
#define PCM186X_MIC_BIAS_CTRL		    (21)

/* Register Definitions - Page 253 */
#define PCM186X_CURR_TRIM_CTRL		    (20)

int pcm186x_init(void);
int pcm186x_config(void);
int pcm186x_get_status(U8 *status);

#endif



