#include "lan8720.h"
#include "drv/delay.h"

extern ETH_HandleTypeDef heth;
static int LAN8720_ReadPHY(u16 reg,u32 *regval)
{
    if(HAL_ETH_ReadPHYRegister(&heth,LAN8720_ADDR,reg,regval)!=HAL_OK)
        return -1;
    return 0;
}
static int LAN8720_WritePHY(u16 reg,u16 value)
{
    u32 temp=value;
    if(HAL_ETH_WritePHYRegister(&heth,LAN8720_ADDR,reg,temp)!=HAL_OK)
        return -1;
    return 0;
}
static void LAN8720_EnablePowerDownMode(void)
{
    u32 readval=0;
    LAN8720_ReadPHY(LAN8720_BCR,&readval);
    readval|=LAN8720_BCR_POWER_DOWN;
    LAN8720_WritePHY(LAN8720_BCR,readval);
}
static void LAN8720_DisablePowerDownMode(void)
{
    u32 readval=0;
    LAN8720_ReadPHY(LAN8720_BCR,&readval);
    readval&=~LAN8720_BCR_POWER_DOWN;
    LAN8720_WritePHY(LAN8720_BCR,readval);
}
static void LAN8720_StartAutoNego(void)
{
    u32 readval=0;
    LAN8720_ReadPHY(LAN8720_BCR,&readval);
    readval|=LAN8720_BCR_AUTONEGO_EN;
    LAN8720_WritePHY(LAN8720_BCR,readval);
}
static void LAN8720_EnableLoopbackMode(void)
{
    u32 readval=0;
    LAN8720_ReadPHY(LAN8720_BCR,&readval);
    readval|=LAN8720_BCR_LOOPBACK;
    LAN8720_WritePHY(LAN8720_BCR,readval);
}
static void LAN8720_DisableLoopbackMode(void)
{
    u32 readval=0;
    LAN8720_ReadPHY(LAN8720_BCR,&readval);
    readval&=~LAN8720_BCR_LOOPBACK;
    LAN8720_WritePHY(LAN8720_BCR,readval);
}
static void LAN8720_EnableIT(u32 interrupt)
{
    u32 readval=0;
    LAN8720_ReadPHY(LAN8720_IMR,&readval);
    readval|=interrupt;
    LAN8720_WritePHY(LAN8720_IMR,readval);
}
static void LAN8720_DisableIT(u32 interrupt)
{
    u32 readval=0;
    LAN8720_ReadPHY(LAN8720_IMR,&readval);
    readval&=~interrupt;
    LAN8720_WritePHY(LAN8720_IMR,readval);
}
static void LAN8720_ClearIT(u32 interrupt)
{
    u32 readval=0;
    LAN8720_ReadPHY(LAN8720_ISFR,&readval);
}
u8 LAN8720_GetITStatus(u32 interrupt)
{
    u32 readval=0;
    u32 status=0;
    LAN8720_ReadPHY(LAN8720_ISFR,&readval);
    if(readval&interrupt) status=1;
    else status=0;
    return status;
}
static void lan8720_set_link_state(u32 linkstate)
{
    u32 bcrvalue=0;
    
    LAN8720_ReadPHY(LAN8720_BCR,&bcrvalue);
    //�ر��������ã������Զ�Э�̣��ٶȺ�˫��
    bcrvalue&=~(LAN8720_BCR_AUTONEGO_EN|LAN8720_BCR_SPEED_SELECT|LAN8720_BCR_DUPLEX_MODE);
    if(linkstate==LAN8720_STATUS_100MBITS_FULLDUPLEX)       //100Mȫ˫��
        bcrvalue|=(LAN8720_BCR_SPEED_SELECT|LAN8720_BCR_DUPLEX_MODE);
    else if(linkstate==LAN8720_STATUS_100MBITS_HALFDUPLEX)  //100M��˫��
        bcrvalue|=LAN8720_BCR_SPEED_SELECT;
    else if(linkstate==LAN8720_STATUS_10MBITS_FULLDUPLEX)   //10Mȫ˫��
        bcrvalue|=LAN8720_BCR_DUPLEX_MODE;

    LAN8720_WritePHY(LAN8720_BCR,bcrvalue);
}
static void lan8720_hw_reset(void)
{
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_RESET); // Ӳ����λ
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_2, GPIO_PIN_SET); // ��λ����
    HAL_Delay(100);
}


///////////////////////////////////////////////////////////////////

int lan8720_init(void)
{
    u32 timeout=0;
    u32 regval=0;
    u32 phylink=0;
    int32_t status=LAN8720_STATUS_OK;

    lan8720_hw_reset();
    
    // LAN8720�����λ
    if(LAN8720_WritePHY(LAN8720_BCR,LAN8720_BCR_SOFT_RESET) >= 0) {
        //�ȴ������λ���
        if(LAN8720_ReadPHY(LAN8720_BCR,&regval) >= 0) {
            while(regval&LAN8720_BCR_SOFT_RESET) {
                HAL_Delay(100);
                timeout++;
                if(timeout >= LAN8720_SoftRese_TIMEOUT) {
                    status = LAN8720_STATUS_RESET_TIMEOUT; break;
                }

                if(LAN8720_ReadPHY(LAN8720_BCR,&regval) < 0) {
                    status = LAN8720_STATUS_READ_ERROR; break;
                }
            }
        }
        else {
            status=LAN8720_STATUS_READ_ERROR;
        }
    }
    else {
        status=LAN8720_STATUS_WRITE_ERROR;
    }

    if (status != LAN8720_STATUS_OK)
        return status; // LAN8720 Reset Fail

    LAN8720_StartAutoNego();                //�����Զ�Э�̹���

    if(status==LAN8720_STATUS_OK)           //���ǰ��������������ʱ1s
        HAL_Delay(1000);

    //�ȴ��������ӳɹ�
    timeout=0;
    while(lan8720_get_link_state()<=LAN8720_STATUS_LINK_DOWN) {
        HAL_Delay(100);
        timeout++;
        if(timeout>=LAN8720_TIMEOUT) {
            status=LAN8720_STATUS_LINK_DOWN;
            break; //��ʱ����5s
        }
    }
    phylink=lan8720_get_link_state();

    return status;
}


int lan8720_get_link_state(void)
{
    u32 readval=0;

    //��ȡ���飬ȷ����ȡ��ȷ������
    LAN8720_ReadPHY(LAN8720_BSR,&readval);
    LAN8720_ReadPHY(LAN8720_BSR,&readval);

    //��ȡ����״̬(Ӳ�������ߵ����ӣ�����TCP��UDP��������ӣ�)
    if((readval&LAN8720_BSR_LINK_STATUS)==0)
        return LAN8720_STATUS_LINK_DOWN;

    //��ȡ�Զ�Э��״̬
    LAN8720_ReadPHY(LAN8720_BCR,&readval);
    if((readval&LAN8720_BCR_AUTONEGO_EN)!=LAN8720_BCR_AUTONEGO_EN)  //δʹ���Զ�Э��
    {
        if(((readval&LAN8720_BCR_SPEED_SELECT)==LAN8720_BCR_SPEED_SELECT)&&
                ((readval&LAN8720_BCR_DUPLEX_MODE)==LAN8720_BCR_DUPLEX_MODE))
            return LAN8720_STATUS_100MBITS_FULLDUPLEX;
        else if((readval&LAN8720_BCR_SPEED_SELECT)==LAN8720_BCR_SPEED_SELECT)
            return LAN8720_STATUS_100MBITS_HALFDUPLEX;
        else if((readval&LAN8720_BCR_DUPLEX_MODE)==LAN8720_BCR_DUPLEX_MODE)
            return LAN8720_STATUS_10MBITS_FULLDUPLEX;
        else
            return LAN8720_STATUS_10MBITS_HALFDUPLEX;
    }
    else                                                            //ʹ�����Զ�Э��
    {
        LAN8720_ReadPHY(LAN8720_PHYSCSR,&readval);
        if((readval&LAN8720_PHYSCSR_AUTONEGO_DONE)==0)
            return LAN8720_STATUS_AUTONEGO_NOTDONE;
        if((readval&LAN8720_PHYSCSR_HCDSPEEDMASK)==LAN8720_PHYSCSR_100BTX_FD)
            return LAN8720_STATUS_100MBITS_FULLDUPLEX;
        else if ((readval&LAN8720_PHYSCSR_HCDSPEEDMASK)==LAN8720_PHYSCSR_100BTX_HD)
            return LAN8720_STATUS_100MBITS_HALFDUPLEX;
        else if ((readval&LAN8720_PHYSCSR_HCDSPEEDMASK)==LAN8720_PHYSCSR_10BT_FD)
            return LAN8720_STATUS_10MBITS_FULLDUPLEX;
        else
            return LAN8720_STATUS_10MBITS_HALFDUPLEX;
    }
}



