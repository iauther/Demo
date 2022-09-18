#ifndef __H_LAN8720_Hx__
#define __H_LAN8720_Hx__

#include "types.h"
#include "platform.h"


#define LAN8720_ADDR                           0               //LAN8720地址为0
#define LAN8720_TIMEOUT                        ((uint32_t)50)  //LAN8720超时时间5s
#define LAN8720_SoftRese_TIMEOUT               ((uint32_t)10)  //reset process will be completed within 0.5s from the setting of this bit
                                               
//LAN8720寄存器                                
#define LAN8720_BCR                            ((uint16_t)0x0000U)
#define LAN8720_BSR                            ((uint16_t)0x0001U)
#define LAN8720_PHYI1R                         ((uint16_t)0x0002U)
#define LAN8720_PHYI2R                         ((uint16_t)0x0003U)
#define LAN8720_ANAR                           ((uint16_t)0x0004U)
#define LAN8720_ANLPAR                         ((uint16_t)0x0005U)
#define LAN8720_ANER                           ((uint16_t)0x0006U)
#define LAN8720_ANNPTR                         ((uint16_t)0x0007U)
#define LAN8720_ANNPRR                         ((uint16_t)0x0008U)
#define LAN8720_MMDACR                         ((uint16_t)0x000DU)
#define LAN8720_MMDAADR                        ((uint16_t)0x000EU)
#define LAN8720_ENCTR                          ((uint16_t)0x0010U)
#define LAN8720_MCSR                           ((uint16_t)0x0011U)
#define LAN8720_SMR                            ((uint16_t)0x0012U)
#define LAN8720_TPDCR                          ((uint16_t)0x0018U)
#define LAN8720_TCSR                           ((uint16_t)0x0019U)
#define LAN8720_SECR                           ((uint16_t)0x001AU)
#define LAN8720_SCSIR                          ((uint16_t)0x001BU)
#define LAN8720_CLR                            ((uint16_t)0x001CU)
#define LAN8720_ISFR                           ((uint16_t)0x001DU)
#define LAN8720_IMR                            ((uint16_t)0x001EU)
#define LAN8720_PHYSCSR                        ((uint16_t)0x001FU)
                                               
//LAN8720 BCR寄存器各位描述                    
#define LAN8720_BCR_SOFT_RESET                 ((uint16_t)0x8000U)
#define LAN8720_BCR_LOOPBACK                   ((uint16_t)0x4000U)
#define LAN8720_BCR_SPEED_SELECT               ((uint16_t)0x2000U)
#define LAN8720_BCR_AUTONEGO_EN                ((uint16_t)0x1000U)
#define LAN8720_BCR_POWER_DOWN                 ((uint16_t)0x0800U)
#define LAN8720_BCR_ISOLATE                    ((uint16_t)0x0400U)
#define LAN8720_BCR_RESTART_AUTONEGO           ((uint16_t)0x0200U)
#define LAN8720_BCR_DUPLEX_MODE                ((uint16_t)0x0100U) 
                                               
//LAN8720的BSR寄存器各位描述                   
#define LAN8720_BSR_100BASE_T4                 ((uint16_t)0x8000U)
#define LAN8720_BSR_100BASE_TX_FD              ((uint16_t)0x4000U)
#define LAN8720_BSR_100BASE_TX_HD              ((uint16_t)0x2000U)
#define LAN8720_BSR_10BASE_T_FD                ((uint16_t)0x1000U)
#define LAN8720_BSR_10BASE_T_HD                ((uint16_t)0x0800U)
#define LAN8720_BSR_100BASE_T2_FD              ((uint16_t)0x0400U)
#define LAN8720_BSR_100BASE_T2_HD              ((uint16_t)0x0200U)
#define LAN8720_BSR_EXTENDED_STATUS            ((uint16_t)0x0100U)
#define LAN8720_BSR_AUTONEGO_CPLT              ((uint16_t)0x0020U)
#define LAN8720_BSR_REMOTE_FAULT               ((uint16_t)0x0010U)
#define LAN8720_BSR_AUTONEGO_ABILITY           ((uint16_t)0x0008U)
#define LAN8720_BSR_LINK_STATUS                ((uint16_t)0x0004U)
#define LAN8720_BSR_JABBER_DETECT              ((uint16_t)0x0002U)
#define LAN8720_BSR_EXTENDED_CAP               ((uint16_t)0x0001U)
                                               
//LAN8720 IMR/ISFR寄存器各位描述               
#define LAN8720_INT_7                          ((uint16_t)0x0080U)
#define LAN8720_INT_6                          ((uint16_t)0x0040U)
#define LAN8720_INT_5                          ((uint16_t)0x0020U)
#define LAN8720_INT_4                          ((uint16_t)0x0010U)
#define LAN8720_INT_3                          ((uint16_t)0x0008U)
#define LAN8720_INT_2                          ((uint16_t)0x0004U)
#define LAN8720_INT_1                          ((uint16_t)0x0002U)
                                               
//LAN8720 PHYSCSR寄存器各位描述                
#define LAN8720_PHYSCSR_AUTONEGO_DONE          ((uint16_t)0x1000U)
#define LAN8720_PHYSCSR_HCDSPEEDMASK           ((uint16_t)0x001CU)
#define LAN8720_PHYSCSR_10BT_HD                ((uint16_t)0x0004U)
#define LAN8720_PHYSCSR_10BT_FD                ((uint16_t)0x0014U)
#define LAN8720_PHYSCSR_100BTX_HD              ((uint16_t)0x0008U)
#define LAN8720_PHYSCSR_100BTX_FD              ((uint16_t)0x0018U) 
  
//LAN8720状态相关定义
#define  LAN8720_STATUS_READ_ERROR             ((int32_t)-5)
#define  LAN8720_STATUS_WRITE_ERROR            ((int32_t)-4)
#define  LAN8720_STATUS_ADDRESS_ERROR          ((int32_t)-3)
#define  LAN8720_STATUS_RESET_TIMEOUT          ((int32_t)-2)
#define  LAN8720_STATUS_ERROR                  ((int32_t)-1)
#define  LAN8720_STATUS_OK                     ((int32_t) 0)
#define  LAN8720_STATUS_LINK_DOWN              ((int32_t) 1)
#define  LAN8720_STATUS_100MBITS_FULLDUPLEX    ((int32_t) 2)
#define  LAN8720_STATUS_100MBITS_HALFDUPLEX    ((int32_t) 3)
#define  LAN8720_STATUS_10MBITS_FULLDUPLEX     ((int32_t) 4)
#define  LAN8720_STATUS_10MBITS_HALFDUPLEX     ((int32_t) 5)
#define  LAN8720_STATUS_AUTONEGO_NOTDONE       ((int32_t) 6)
                                               
//LAN8720中断标志位定义                        
#define  LAN8720_ENERGYON_IT                   LAN8720_INT_7
#define  LAN8720_AUTONEGO_COMPLETE_IT          LAN8720_INT_6
#define  LAN8720_REMOTE_FAULT_IT               LAN8720_INT_5
#define  LAN8720_LINK_DOWN_IT                  LAN8720_INT_4
#define  LAN8720_AUTONEGO_LP_ACK_IT            LAN8720_INT_3
#define  LAN8720_PARALLEL_DETECTION_FAULT_IT   LAN8720_INT_2
#define  LAN8720_AUTONEGO_PAGE_RECEIVED_IT     LAN8720_INT_1


int lan8720_init(void);
int lan8720_get_link_state(void);

#endif 
