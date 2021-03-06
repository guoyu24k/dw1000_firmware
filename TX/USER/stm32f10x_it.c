/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "DW1000.h"
#include "USART.h"
#include "SPI.h"

u8 distance_flag;
u8 ars_counter;
u8 ars_max=3;

u8 usart_buffer[64];
u8 usart_index;
u8 usart_status;
u16 pan;

extern u8 toggle;

extern u8 Sequence_Number;
extern u32 Tx_stp_L;
extern u8 Tx_stp_H;
extern u32 Rx_stp_L;
extern u8 Rx_stp_H;
extern u32 data;
extern u32 tmp1;
extern s32 tmp2;
extern double diff;
extern double distance;
u8 tmpp[14];
u8 i;

extern u16 std_noise;
extern	u16 fp_ampl1;
extern	u16 fp_ampl2;
extern	u16 fp_ampl3;
extern	u16 cir_mxg;
extern	u16 rxpacc;
extern	double fppl;
extern	double rxl;

/*
0:已完成处理
1：正在接收
2：已完成接收，没有完成处理


*/
/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
	/* Go to infinite loop when Hard Fault exception occurs */
	while (1)
	{
	}
}

/**
	* @brief  This function handles Memory Manage exception.
	* @param  None
	* @retval None
*/
void MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1)
	{
	}
}

/**
	* @brief  This function handles Bus Fault exception.
	* @param  None
	* @retval None
*/
void BusFault_Handler(void)
{
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
	}
}

/**
	* @brief  This function handles Usage Fault exception.
	* @param  None
	* @retval None
*/
void UsageFault_Handler(void)
{
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
	}
}

/**
	* @brief  This function handles SVCall exception.
	* @param  None
	* @retval None
*/
void SVC_Handler(void)
{
}

/**
	* @brief  This function handles Debug Monitor exception.
	* @param  None
	* @retval None
*/
void DebugMon_Handler(void)
{
}

/**
	* @brief  This function handles PendSVC exception.
	* @param  None
	* @retval None
*/
void PendSV_Handler(void)
{
}

/**
	* @brief  This function handles SysTick Handler.
	* @param  None
	* @retval None
*/
void SysTick_Handler(void)
{
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
	* @brief  This function handles PPP interrupt request.
	* @param  None
	* @retval None
*/
/*void PPP_IRQHandler(void)
{
}*/

/**
* @}
*/

/*
状态转换说明
distance_flag=0  初始状态
distance_flag=1  定位申请已发送
distance_flag=2  ACK已经接受
distance_flag=3  第二次数据已接收
*/
void EXTI1_IRQHandler(void)
{
	u16 status;
	u8 tmp;
	//u8 i;
	EXTI_ClearITPendingBit(EXTI_Line1);

	while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)==0)
{
	Read_DW1000(0x0F,0x00,(u8 *)(&status),4);
		if((status&0x00040000)==0x00040000)			//LDE算法检测标志位
		{
			tmp=0x04;
			Write_DW1000(0x0F,0x02,&tmp,1);		   //强制写入1
			to_IDLE();
			RX_mode_enable();
		}
//	{
		Read_DW1000(0x0F,0x00,(u8 *)(&status),4);
		if((status&0x00006000)==0x00002000)		  //CRC校验如果错误进入IDLE，丢弃
		{
			 to_IDLE();
		}
		Read_DW1000(0x0F,0x00,(u8 *)(&status),2);	  //
		//printf("0x%4x\r\n",status);
		if((status&0x0080)==0x0080)
		{
			tmp=0x80;
			Write_DW1000(0x0F,0x00,&tmp,1);
			if(distance_flag==0)
			{
				//printf("\rTimeStamp_Tx\r\n");
				Read_DW1000(0x17,0x00,(u8 *)(&Tx_stp_L),4);		//写入TX时间戳
				Read_DW1000(0x17,0x04,&Tx_stp_H,1);

				printf("0x%8x\r\n",Tx_stp_L);
				printf("0x%2x\r\n",Tx_stp_H);
				distance_flag=1;					 //定位申请发送完毕

				//计数器TIM3清零
				TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE);
				TIM_SetCounter(TIM3,0x0000);
				TIM_ClearFlag(TIM3, TIM_FLAG_Update);
				TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
				if(ars_counter==0)
				{
					//printf("\r\n=============定位信息=============\r\n");
				}
				//printf("定位请求%d\t\t发送成功\r\n",ars_counter+1);
			}



		}
		Read_DW1000(0x0F,0x00,(u8 *)(&status),2);
		if((status&0x4000)==0x4000)
		{
			tmp=0x60;
			Write_DW1000(0x0F,0x01,&tmp,1);				 //接收回读的数据完成CRC校验
			Read_DW1000(0x11,0x00,tmpp,14);			   //读RX_FRAME_BUFFER的值，写入tmpp
			/*for(i=0;i<14;i++)
			{
				printf("%02x",tmpp[i]);
			}
			printf("\r\n")  ;*/
			if(((tmpp[0]&0x07)==0x04)&&(distance_flag==1)) //如果是ACK
			{
				printf("ACK RX_FRAME：\r\n");
				for(i=0;i<14;i++)
				{
					printf("%02x",tmpp[i]);
				}
				printf("\r\n");
				if(tmpp[2]==Sequence_Number-1)
				{
					//printf("11\r\n")  ;

					Read_DW1000(0x15,0x00,(u8 *)(&Rx_stp_L),4);			   //读取RX时间戳
					Read_DW1000(0x15,0x04,&Rx_stp_H,1);

					//printf("\rRx时间戳\r\n");
					printf("0x%8x\r\n",Rx_stp_L);
					printf("0x%2x\r\n",Rx_stp_H);

					Read_DW1000(0x12,0x00,(u8 *)(&std_noise),2);
					Read_DW1000(0x12,0x02,(u8 *)(&fp_ampl2),2);
					Read_DW1000(0x12,0x04,(u8 *)(&fp_ampl3),2);
					Read_DW1000(0x12,0x06,(u8 *)(&cir_mxg),2);
					Read_DW1000(0x15,0x07,(u8 *)(&fp_ampl1),2);
					Read_DW1000(0x10,0x02,(u8 *)(&rxpacc),2);

					//计数器TIM3清零
					TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE);
					TIM_SetCounter(TIM3,0x0000);
					TIM_ClearFlag(TIM3, TIM_FLAG_Update);
					TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);

					to_IDLE();
					RX_mode_enable();

					distance_flag=2;

					printf("定位应答%d\t\t接收成功\r\n",ars_counter+1);


				}
			}
			else if(((tmpp[0]&0x07)==0x01)&&(distance_flag==2))//数据
			{
				printf("DATA RX_FRAME：\r\n");
				for(i=0;i<14;i++)
				{
					printf("%02x",tmpp[i]);
				}
				printf("\r\n");
				if(tmpp[2]==Sequence_Number-1)
				{

					//ACK_send();
					//计数器TIM3清零,停止工作
					TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE);
					TIM_SetCounter(TIM3,0x0000);
					TIM_Cmd(TIM3, DISABLE);
					TIM_ClearFlag(TIM3, TIM_FLAG_Update);
					TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
					distance_flag=3;
					ars_counter=0;

					//printf("21\r\n")  ;

					Read_DW1000(0x11,0x09,(u8 *)(&data),4);
					printf("0x%08x\r\n",data);
					Read_DW1000(0x13,0x00,(u8 *)(&tmp1),4);
					Read_DW1000(0x14,0x00,(u8 *)(&tmp2),3);

					printf("定位数据%d\t\t接收成功\r\n",ars_counter+1);

				/*for(i=0;i<14;i++)
					{
						printf("%02x",tmpp[i]);
					}
					printf("\r\n");			*/
					if((tmpp[3]==0x74)&&(tmpp[4]==0x10))	          	 //判断PAN_ID
					{
						printf("\r\n    -------节点1测量结果-------    \r\n");
					}
					else if((tmpp[3]==0x74)&&(tmpp[4]==0x89))
					{
						printf("\r\n    -------节点2测量结果-------    \r\n");
					}
					distance_measurement();
					quality_measurement();

					//ACK_send();
					//printf("==================================\r\n");
				}

			}

		}
}
	//printf("%d\r\n",distance_flag);

}

void TIM2_IRQHandler(void)
{
	if ( TIM_GetITStatus(TIM2 , TIM_IT_Update) != RESET )
	{
		TIM_ClearITPendingBit(TIM2 , TIM_FLAG_Update);
		Location_polling();//申请定位
		//printf("TIM2\r\n");
	}
}
void TIM3_IRQHandler(void)
{
	//u32 tmp;
	if ( TIM_GetITStatus(TIM3 , TIM_IT_Update) != RESET )
	{
		//Read_DW1000(0x1e,0x00,(u8 *)(&tmp),4);
	//	printf("%x\r\n",tmp);
		TIM_ClearITPendingBit(TIM3 , TIM_FLAG_Update);

		if(distance_flag==0) //发送失败，处理不同状态
		{
			if(ars_counter==0)
			{
				//Read_DW1000(0x24,0x04,(u8 *)(&tmp),4);
				//printf("%x\r\n",tmp);
				printf("\r\n=============定位信息=============\r\n");
			}
			printf("定位请求%d\t\t发送异常\r\n",ars_counter+1);
		}
		else if(distance_flag==1)  //发送成功.但ACK失败
		{
			printf("定位应答%d\t\t接收异常\r\n",ars_counter+1);
		}
		else if(distance_flag==2) //ACK接收成功，但数据接收失败
		{
			printf("定位数据%d\t\t接收异常\r\n",ars_counter+1);
		}

		ars_counter++;
		if(ars_counter<ars_max)				//小于最大值时，继续进行申请定位，最大重发三次
		{
			Location_polling();
		}
		else
		{
			TIM_ITConfig(TIM3,TIM_IT_Update,DISABLE);
			TIM_SetCounter(TIM3,0x0000);
			TIM_Cmd(TIM3, DISABLE);
			TIM_ClearFlag(TIM3, TIM_FLAG_Update);
			TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);

			ars_counter=0;
			printf("==================================\r\n");
		}
		//printf("TIM3\r\n");
	}
}
void TIM4_IRQHandler(void)
{
	if ( TIM_GetITStatus(TIM4 , TIM_IT_Update) != RESET )
	{
		TIM_ClearITPendingBit(TIM4 , TIM_FLAG_Update);
		usart_status=2;
		//计数器TIM4清零,停止工作
		TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
		TIM_SetCounter(TIM4,0x0000);
		TIM_Cmd(TIM4, DISABLE);

		usart_handle();

	}
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		if(usart_status==0)
		{
			usart_status=1;

			//开启计数器TIM4
			TIM_ClearFlag(TIM4, TIM_FLAG_Update);
			TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);
			TIM_Cmd(TIM4, ENABLE);

			usart_buffer[usart_index++]=USART1->DR;
			if(usart_index==64)
			{
				usart_index=0;
			}
		}
		else if(usart_status==1)
		{
			//计数器TIM3清零,
			TIM_ITConfig(TIM4,TIM_IT_Update,DISABLE);
			TIM_SetCounter(TIM4,0x0000);
			TIM_ClearFlag(TIM4, TIM_FLAG_Update);
			TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);

			usart_buffer[usart_index++]=USART1->DR;
			if(usart_index==64)
			{
				usart_index=0;
			}
		}
	}

}
/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
