/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include	"project_config.h"


/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/
volatile uint32_t BitFlag = 0;
volatile uint32_t counter_tick = 0;

STR_CANMSG_T rrMsg;
/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

void tick_counter(void)
{
	counter_tick++;
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void compare_buffer(uint8_t *src, uint8_t *des, int nBytes)
{
    uint16_t i = 0;	
	
    for (i = 0; i < nBytes; i++)
    {
        if (src[i] != des[i])
        {
            printf("error idx : %4d : 0x%2X , 0x%2X\r\n", i , src[i],des[i]);
			set_flag(flag_error , ENABLE);
        }
    }

	if (!is_flag_set(flag_error))
	{
    	printf("%s finish \r\n" , __FUNCTION__);	
		set_flag(flag_error , DISABLE);
	}

}

void reset_buffer(void *dest, unsigned int val, unsigned int size)
{
    uint8_t *pu8Dest;
//    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;

	#if 1
	while (size-- > 0)
		*pu8Dest++ = val;
	#else
	memset(pu8Dest, val, size * (sizeof(pu8Dest[0]) ));
	#endif
	
}

void copy_buffer(void *dest, void *src, unsigned int size)
{
    uint8_t *pu8Src, *pu8Dest;
    unsigned int i;
    
    pu8Dest = (uint8_t *)dest;
    pu8Src  = (uint8_t *)src;


	#if 0
	  while (size--)
	    *pu8Dest++ = *pu8Src++;
	#else
    for (i = 0; i < size; i++)
        pu8Dest[i] = pu8Src[i];
	#endif
}

void dump_buffer(uint8_t *pucBuff, int nBytes)
{
    uint16_t i = 0;
    
    printf("dump_buffer : %2d\r\n" , nBytes);    
    for (i = 0 ; i < nBytes ; i++)
    {
        printf("0x%2X," , pucBuff[i]);
        if ((i+1)%8 ==0)
        {
            printf("\r\n");
        }            
    }
    printf("\r\n\r\n");
}

void  dump_buffer_hex(uint8_t *pucBuff, int nBytes)
{
    int     nIdx, i;

    nIdx = 0;
    while (nBytes > 0)
    {
        printf("0x%04X  ", nIdx);
        for (i = 0; i < 16; i++)
            printf("%02X ", pucBuff[nIdx + i]);
        printf("  ");
        for (i = 0; i < 16; i++)
        {
            if ((pucBuff[nIdx + i] >= 0x20) && (pucBuff[nIdx + i] < 127))
                printf("%c", pucBuff[nIdx + i]);
            else
                printf(".");
            nBytes--;
        }
        nIdx += 16;
        printf("\n");
    }
    printf("\n");
}

void CAN_ShowMsg(STR_CANMSG_T* Msg)
{
    uint8_t i;
    printf("Read ID=%8X, Type=%s, DLC=%d,Data=",Msg->Id,Msg->IdType?"EXT":"STD",Msg->DLC);
    for(i=0; i<Msg->DLC; i++)
        printf("%02X,",Msg->Data[i]);
    printf("\n\n");
}

void CAN_MsgInterrupt(CAN_T *tCAN, uint32_t u32IIDR)
{
    if(u32IIDR==1)
    {
        printf("Msg-0 INT and Callback\n");
        CAN_Receive(tCAN, 0,&rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
    if(u32IIDR==5+1)
    {
        printf("Msg-5 INT and Callback \n");
        CAN_Receive(tCAN, 5,&rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
    if(u32IIDR==31+1)
    {
        printf("Msg-31 INT and Callback \n");
        CAN_Receive(tCAN, 31,&rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
}

void CAN0_IRQHandler(void)
{
    uint32_t u8IIDRstatus;

    u8IIDRstatus = CAN0->IIDR;

    if(u8IIDRstatus == 0x00008000)        /* Check Status Interrupt Flag (Error status Int and Status change Int) */
    {
        /**************************/
        /* Status Change interrupt*/
        /**************************/
        if(CAN0->STATUS & CAN_STATUS_RXOK_Msk)
        {
            CAN0->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status*/

            printf("RX OK INT\n") ;
        }

        if(CAN0->STATUS & CAN_STATUS_TXOK_Msk)
        {
            CAN0->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status*/

            printf("TX OK INT\n") ;
        }

        /**************************/
        /* Error Status interrupt */
        /**************************/
        if(CAN0->STATUS & CAN_STATUS_EWARN_Msk)
        {
            printf("EWARN INT\n") ;
        }

        if(CAN0->STATUS & CAN_STATUS_BOFF_Msk)
        {
            printf("BOFF INT\n") ;

            /* Do Init to release busoff pin */
            CAN0->CON = (CAN_CON_INIT_Msk | CAN_CON_CCE_Msk);
            CAN0->CON &= (~(CAN_CON_INIT_Msk | CAN_CON_CCE_Msk));
            while(CAN0->CON & CAN_CON_INIT_Msk);
        }
    }
    else if (u8IIDRstatus!=0)
    {
        printf("=> Interrupt Pointer = %d\n",CAN0->IIDR -1);

        CAN_MsgInterrupt(CAN0, u8IIDRstatus);

        CAN_CLR_INT_PENDING_BIT(CAN0, ((CAN0->IIDR) -1));      /* Clear Interrupt Pending */

    }
    else if(CAN0->WU_STATUS == 1)
    {
        printf("Wake up\n");

        CAN0->WU_STATUS = 0;                       /* Write '0' to clear */
    }

}

void CAN_Init(CAN_T  *tCAN)
{
    if(tCAN == CAN0)
    {
        // Enable IP clock
        CLK_EnableModuleClock(CAN0_MODULE);	//CLK->APBCLK0 |= CLK_APBCLK0_CAN0CKEN_Msk;    

	    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB10MFP_Msk | SYS_GPB_MFPH_PB11MFP_Msk);
	    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB10MFP_CAN0_RXD | SYS_GPB_MFPH_PB11MFP_CAN0_TXD);	

//	    SYS->GPA_MFPL &= ~(SYS_GPA_MFPL_PA4MFP_Msk | SYS_GPA_MFPL_PA5MFP_Msk);
//	    SYS->GPA_MFPL |= (SYS_GPA_MFPL_PA4MFP_CAN0_RXD | SYS_GPA_MFPL_PA5MFP_CAN0_TXD);	

    }
    else if(tCAN == CAN1)
    {
        // Enable IP clock
        CLK_EnableModuleClock(CAN1_MODULE);	//CLK->APBCLK0 |= CLK_APBCLK0_CAN1CKEN_Msk;

	    SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB6MFP_Msk | SYS_GPB_MFPL_PB7MFP_Msk);
	    SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB6MFP_CAN1_RXD | SYS_GPB_MFPL_PB7MFP_CAN1_TXD);	
    }

	/*
		1000Kbps
		500Kbps
		250Kbps
		125Kbps
		100Kbps
		50Kbps		
	*/
	//500Kbps
    CAN_Open(tCAN,  500000, CAN_NORMAL_MODE);	
}

void CAN_STOP(CAN_T  *tCAN)
{
    if(tCAN == CAN0)
    {
        /* Disable CAN0 Clock and Reset it */
        SYS->IPRST1 |= SYS_IPRST1_CAN0RST_Msk;
        SYS->IPRST1 &= ~SYS_IPRST1_CAN0RST_Msk;
        CLK->APBCLK0 &= ~CLK_APBCLK0_CAN0CKEN_Msk;
    }
    else if(tCAN == CAN1)
    {
        /* Disable CAN0 Clock and Reset it */
        SYS->IPRST1 |= SYS_IPRST1_CAN1RST_Msk;
        SYS->IPRST1 &= ~SYS_IPRST1_CAN1RST_Msk;
        CLK->APBCLK0 &= ~CLK_APBCLK0_CAN1CKEN_Msk;
    }
}

void Test_NormalMode_Tx(CAN_T *tCAN)
{
    STR_CANMSG_T tMsg;
    uint32_t i;

	#if 0
    /* Send a 11-bits message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_STD_ID;
    tMsg.Id       = 0x7FF;
    tMsg.DLC      = 2;
    tMsg.Data[0]  = 7;
    tMsg.Data[1]  = 0xFF;

    if(CAN_Transmit(tCAN, MSG(0),&tMsg) == FALSE)   // Configure Msg RAM and send the Msg in the RAM
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    printf("MSG(0).Send STD_ID:0x7FF, Data[07,FF]done\n");
	#endif

	#if 0
    /* Send a 29-bits message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = 0x12345;
    tMsg.DLC      = 3;
    tMsg.Data[0]  = 1;
    tMsg.Data[1]  = 0x23;
    tMsg.Data[2]  = 0x45;

    if(CAN_Transmit(tCAN, MSG(1),&tMsg) == FALSE)
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    printf("MSG(1).Send EXT:0x12345 ,Data[01,23,45]done\n");
	#endif

	#if 1
    /* Send a data message */
    tMsg.FrameType= CAN_DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = 0x7FF01;
    tMsg.DLC      = 4;
    tMsg.Data[0]  = 0xA1;
    tMsg.Data[1]  = 0xB2;
    tMsg.Data[2]  = 0xC3;
    tMsg.Data[3]  = 0xD4;

    if(CAN_Transmit(tCAN, MSG(3),&tMsg) == FALSE)
    {
        printf("Set Tx Msg Object failed\n");
        return;
    }

    printf("MSG(3).Send EXT:0x7FF01 ,Data[A1,B2,C3,D4]done\n");
	#endif

    for(i=0; i < 10000; i++);

    printf("Trasmit Done!\nCheck the receive host received data\n\n");

}

/*----------------------------------------------------------------------------*/
/*  Receive Rx Msg by Normal Mode Function (With Message RAM)                    */
/*----------------------------------------------------------------------------*/
void Test_NormalMode_SetRxMsg(CAN_T *tCAN)
{
    if(CAN_SetRxMsg(tCAN, MSG(0),CAN_STD_ID, 0x7FF) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg(tCAN, MSG(5),CAN_EXT_ID, 0x12345) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

    if(CAN_SetRxMsg(tCAN, MSG(31),CAN_EXT_ID, 0x7FF01) == FALSE)
    {
        printf("Set Rx Msg Object failed\n");
        return;
    }

}

void Test_NormalMode_WaitRxMsg(CAN_T *tCAN)
{
    /*Choose one mode to test*/
#if 1
    /* Polling Mode */
    while(1)
    {
        while(tCAN->IIDR ==0);            /* Wait IDR is changed */
        CAN_Receive(tCAN, tCAN->IIDR -1, &rrMsg);
        CAN_ShowMsg(&rrMsg);
    }
#else
    /* INT Mode */
    CAN_EnableInt(tCAN, CAN_CON_IE_Msk);
    NVIC_SetPriority(CAN1_IRQn, (1<<__NVIC_PRIO_BITS) - 2);
    NVIC_EnableIRQ(CAN1_IRQn);

    printf("Enter any key to exit\n");
    getchar();
#endif
}


void TMR1_IRQHandler(void)
{
//	static uint32_t LOG = 0;

	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
//        	printf("%s : %4d\r\n",__FUNCTION__,LOG++);
			#if 0	//M487 EVM LED
			PH0 ^= 1;
			#else	//M483 EVM LED
			PH4 ^= 1;
			#endif
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}


void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res == 'x' || res == 'X')
	{
		NVIC_SystemReset();
	}

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		switch(res)
		{
			case '1':
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
				NVIC_SystemReset();		
				break;
		}
	}
}


void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART0_IRQn);

	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());	
	#endif
}

void Custom_Init(void)
{
	
	#if 0	//M487 EVM LED
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH0MFP_Msk)) | (SYS_GPH_MFPL_PH0MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH1MFP_Msk)) | (SYS_GPH_MFPL_PH1MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH2MFP_Msk)) | (SYS_GPH_MFPL_PH2MFP_GPIO);
	
	GPIO_SetMode(PH,BIT0,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT1,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT2,GPIO_MODE_OUTPUT);
	#else	//M483 EVM
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH4MFP_Msk)) | (SYS_GPH_MFPL_PH4MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH5MFP_Msk)) | (SYS_GPH_MFPL_PH5MFP_GPIO);
	SYS->GPH_MFPL = (SYS->GPH_MFPL & ~(SYS_GPH_MFPL_PH6MFP_Msk)) | (SYS_GPH_MFPL_PH6MFP_GPIO);
	
	GPIO_SetMode(PH,BIT4,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT5,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT6,GPIO_MODE_OUTPUT);
	
	#endif
	
}


void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);

    /* Enable External XTAL (4~24 MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk|CLK_PWRCTL_HIRCEN_Msk);

    /* Waiting for 12MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk|CLK_STATUS_HIRCSTB_Msk);

    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk|CLK_PWRCTL_LIRCEN_Msk);

    /* Waiting for 12MHz clock ready */
    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk|CLK_STATUS_LIRCSTB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(FREQ_192MHZ);
    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SYS->GPB_MFPH &= ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk);
    SYS->GPB_MFPH |= (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
	
    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

int main()
{
    CAN_T *targetCAN;
    targetCAN = (CAN_T *) CAN0;

    SYS_Init();

	UART0_Init();
	Custom_Init();	
	TIMER1_Init();

    CAN_Init(targetCAN);
    Test_NormalMode_Tx(targetCAN);
	
    /* Got no where to go, just loop forever */
    while(1)
    {


    }
}

/*** (C) COPYRIGHT 2014 Nuvoton Technology Corp. ***/
