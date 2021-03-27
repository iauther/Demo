#ifndef __TCA6424A_Hx__
#define __TCA6424A_Hx__

#include "types.h"

enum {
    TCA6424A_PORT_0=0,
    TCA6424A_PORT_1,
    TCA6424A_PORT_2,
    
    TCA6424A_PORT_MAX
};

typedef enum
{
	TCA6424A_ADDR_L = 0x22,
	TCA6424A_ADDR_H = 0x23,
}TCA6424A_ADDR;


#pragma pack (1)  
typedef struct {
	TCA6424A_ADDR   addr;
    U8              index;
    U8              bit;
}tca6424a_set_t;

typedef struct {
    U8 input[TCA6424A_PORT_MAX];
    U8 output[TCA6424A_PORT_MAX];
    U8 polarity[TCA6424A_PORT_MAX];
    U8 config[TCA6424A_PORT_MAX];
}tca6424a_reg_t;

typedef struct {
    tca6424a_reg_t  l;
    tca6424a_reg_t  h;
}tca6424a_regs_t;

#pragma pack ()  



#define TCA6424A_INPUT_REG0		0x00		// Input status register
#define TCA6424A_INPUT_REG1		0x01		// Input status register
#define TCA6424A_INPUT_REG2		0x02		// Input status register
#define TCA6424A_OUTPUT_REG0	0x04		// Output register to change state of output BIT set to 1, output set HIGH
#define TCA6424A_OUTPUT_REG1	0x05		// Output register to change state of output BIT set to 1, output set HIGH
#define TCA6424A_OUTPUT_REG2	0x06		// Output register to change state of output BIT set to 1, output set HIGH
#define TCA6424A_POLARITY_REG0 	0x08		// Polarity inversion register. BIT '1' inverts input polarity of register 0x00
#define TCA6424A_POLARITY_REG1 	0x09		// Polarity inversion register. BIT '1' inverts input polarity of register 0x00
#define TCA6424A_POLARITY_REG2 	0x0A		// Polarity inversion register. BIT '1' inverts input polarity of register 0x00
#define TCA6424A_CONFIG_REG0   	0x0C		// Configuration register. BIT = '1' sets port to input BIT = '0' sets port to output
#define TCA6424A_CONFIG_REG1   	0x0D		// Configuration register. BIT = '1' sets port to input BIT = '0' sets port to output
#define TCA6424A_CONFIG_REG2   	0x0E		// Configuration register. BIT = '1' sets port to input BIT = '0' sets port to output


int tca6424a_init(void);

int tca6424a_reset(TCA6424A_ADDR addr);

int tca6424a_read_reg(TCA6424A_ADDR addr, tca6424a_reg_t *reg);

int tca6424a_write_reg(TCA6424A_ADDR addr, tca6424a_reg_t *reg);

void tca6424a_print(char *s, tca6424a_reg_t *reg);

void tca6424a_print_reg(char *s, TCA6424A_ADDR addr);

int tca6424a_read(TCA6424A_ADDR addr, U8 reg, U8 *data, U32 len);

int tca6424a_write(TCA6424A_ADDR addr, U8 reg, U8 *data, U32 len);

int tca6424a_check(void);

#endif

