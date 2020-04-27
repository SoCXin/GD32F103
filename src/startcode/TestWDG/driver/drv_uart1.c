#include "os_public.h"
#include "drv_systick.h"
#include "drv_uart1.h"

static VOID UART1_GpioInit(VOID)
{
    GPIO_InitPara GPIO_InitStructure;
    
    RCC_APB2PeriphClock_Enable(RCC_APB2PERIPH_GPIOA , ENABLE);
    
    GPIO_InitStructure.GPIO_Pin     = GPIO_PIN_9 ;
    GPIO_InitStructure.GPIO_Mode    = GPIO_MODE_AF_PP;
    GPIO_InitStructure.GPIO_Speed   = GPIO_SPEED_50MHZ;
    GPIO_Init( GPIOA , &GPIO_InitStructure); 
    GPIO_InitStructure.GPIO_Pin     = GPIO_PIN_10;
    GPIO_InitStructure.GPIO_Mode    = GPIO_MODE_IN_FLOATING;;
    GPIO_Init( GPIOA , &GPIO_InitStructure); 
}

static VOID UART1_Config(VOID)
{
    USART_InitPara USART_InitStructure;
    
    RCC_APB2PeriphClock_Enable(RCC_APB2PERIPH_USART1 , ENABLE);
    
    USART_DeInit( USART1 );
    USART_InitStructure.USART_BRR = 115200;  /* ������ */
    USART_InitStructure.USART_WL = USART_WL_8B; /* ����λ */
    USART_InitStructure.USART_STBits = USART_STBITS_1; /* ֹͣλ */
    USART_InitStructure.USART_Parity = USART_PARITY_RESET; /* У��λ */
    USART_InitStructure.USART_HardwareFlowControl = USART_HARDWAREFLOWCONTROL_NONE; /* ���� */
    USART_InitStructure.USART_RxorTx = USART_RXORTX_RX | USART_RXORTX_TX; /* �շ�ʹ�� */
    USART_Init(USART1, &USART_InitStructure);
}

VOID DRV_UART1_Init(VOID)
{
    UART1_GpioInit();
    UART1_Config();
    USART_Enable(USART1, ENABLE);
}

PUTCHAR_PROTOTYPE
{
    /* �ȴ�������� */
    while (USART_GetBitState(USART1 , USART_FLAG_TBE) == RESET)
    {
    }
    
    USART_DataSend(USART1 , (U8)ch);
    
    while (USART_GetBitState(USART1 , USART_FLAG_TC) == RESET)
    {
    }
    return ch;
}

GETCHAR_PROTOTYPE
{
    while(USART_GetBitState(USART1 , USART_FLAG_RBNE) == RESET)
    {
    }
    return USART_DataReceive(USART1);
}


