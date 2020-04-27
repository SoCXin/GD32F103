#include "os_public.h"
#include "os_task.h"

#define OS_TASK_MAX 3
#define OS_TASK_SWITCH_INTERVAL 10 /* ��λms */

#define TASK_NVIC_INT_CTRL_REG		( * ( ( volatile uint32_t * ) 0xe000ed04 ) )
#define TASK_NVIC_PENDSVSET_BIT		( 1UL << 28UL )
/* ICSR�Ĵ���bit28��1������PendSV�ж� */
#define OS_TASK_SWITCH  TASK_NVIC_INT_CTRL_REG = TASK_NVIC_PENDSVSET_BIT

typedef S32 StackSize_t;

/* PSR��EPSRִ��״̬�Ĵ���������Tλ��bit24����Ϊ1 */
#define OS_TASK_INITIAL_XPSR			( 0x01000000 ) 
#define OS_TASK_START_ADDRESS_MASK				( ( StackSize_t ) 0xfffffffeUL )

typedef enum
{
    TASK_INIT = 0,
    TASK_READY,    
    TASK_RUNNING,    
    TASK_SUSPENDED,
    TASK_DELETED,
    TASK_BUTT,
}TaskState_E;

typedef struct
{
    volatile StackSize_t *topStack; /* ջ������������ */
    TaskState_E state;
    U16 delay;
    U16 delayMax;
    U32 *eventBitMap;
    U8 id;
}TCB_S;

__attribute__((section("privileged_data"))) volatile TCB_S *gCurrentTCB = NULL;

static TCB_S *gTaskTcbList[OS_TASK_MAX] = {0};

static volatile  U32 gTaskSysTickCount = 0;

/* ��ʼ�����̶�ջ���� */
static StackSize_t* TASK_TaskStackFirstInit(IN StackSize_t *topStack, IN TaskFunction_t func)
{
    /* ����ջ��ַ˳����ջ�����ǼĴ�����ջ˳��
     * PSR,PC,LR,R12,R3,R2,R1,R0 ������оƬ�Զ���ջ��
     * R4,R5,R6,R7,R8,R9,R10,R11 �����ֹ���ջ�����ջ˳��ע�Ᵽ��һ��
     * �˴�Ҳ�������Ӽ��������ڶ�ջ������
     */
    topStack--;
    *topStack = OS_TASK_INITIAL_XPSR;
    topStack--;
    *topStack = (((StackSize_t)func) & OS_TASK_START_ADDRESS_MASK);
    topStack--; /* ����ջ���γ�ʼ�����������ϲ��ˣ����ؼ�����˿������ӷ��غ����û����� */
    topStack -= 5; /* �����ں������ */
    topStack -= 8;
    return topStack;
}

VOID TASK_GetCurrentTask(VOID)
{
    volatile TCB_S *tmpTcb = NULL;
    U8 id = 0;

    for (id = 0; id < OS_TASK_MAX; id++)
    {
        tmpTcb = gTaskTcbList[id];
        if ((TASK_READY == tmpTcb->state) || (TASK_RUNNING == tmpTcb->state))
        {
            tmpTcb->state = TASK_RUNNING;
            gCurrentTCB = tmpTcb;
            break;
        }
    }
    
    return;
}

__asm static VOID TASK_StartFirstTask(VOID)
{
    /* ����svc����svc�ж���ͨ���޸�LD�Ĵ���ֵ�ķ�ʽ�����߳�ģʽ */
    svc 0
    nop
    nop
}

__asm static VOID TASK_SvcHandler(VOID)
{
    extern gCurrentTCB;

    /* �����������ӳ�䵽���߳�ջ */
	ldr	r3, =gCurrentTCB
	ldr r1, [r3]
	ldr r0, [r1]
	ldmia r0!, {r4-r11}
    msr psp, r0
    isb

    /* ����LR�Ĵ����쳣���ؽ����߳�ģʽ���� */
	mov r14, #0xfffffffd
	bx r14
    nop
}

void SVC_Handler(void)
{
    TASK_GetCurrentTask();
    TASK_SvcHandler();
}

__asm VOID PendSV_Handler(VOID)
{
    extern gCurrentTCB;
    extern TASK_GetCurrentTask;
            
    /* �ѵ�ǰ������ջ,��Ҫ��R4-R11����Ϊ�������Զ���ջ */
    mrs r0, psp
    isb
    stmdb r0!, {r4-r11}
    dsb
    isb

    /* �Ѷ�ջ��ַӳ�䵽TCB */
    ldr r3, =gCurrentTCB
    ldr r2, [r3]  /* r2 = gCurrentTCB*/
    str r0, [r2]  /* ��r0��ֵ��gCurrentTCB->topStack */

    /* �л����������ģ�ע���ջ���棬R3, r14��Ҫ���»ָ�*/
    stmdb sp!, {r3,r14}
    dsb
    isb
    bl TASK_GetCurrentTask
	ldmia sp!, {r3,r14}
    dsb
    isb

    /* ��ȡ������ջ */
	ldr r1, [r3]
	ldr r0, [r1]
	ldmia r0!, {r4-r11}
    dsb
    isb
    msr psp, r0
    isb

	bx r14
    nop
}

static VOID TASK_DelayList(VOID)
{
    volatile TCB_S *tmpTcb = NULL;
    U8 id = 0;

    for (id = 0; id < OS_TASK_MAX; id++)
    {
        tmpTcb = gTaskTcbList[id];
        if (NULL == tmpTcb)
        {
            continue;
        }
        
        if (tmpTcb->delayMax != 0)
        {
            if ((gTaskSysTickCount - tmpTcb->delay) >= tmpTcb->delayMax)
            {
                tmpTcb->delay = 0;
                tmpTcb->delayMax = 0;
                tmpTcb->state = TASK_READY;
                OS_TASK_SWITCH;
            }
            else
            {
                tmpTcb->state = TASK_SUSPENDED;
            }
        }
    }
    
    return;
}

static VOID TASK_WaitForEventList(VOID)
{
    volatile TCB_S *tmpTcb = NULL;
    U8 id = 0;

    for (id = 0; id < OS_TASK_MAX; id++)
    {
        tmpTcb = gTaskTcbList[id];
        if (NULL == tmpTcb)
        {
            continue;
        }
        
        if (NULL == tmpTcb->eventBitMap)
        {
            continue;
        }
        
        if (*tmpTcb->eventBitMap != 0)
        {
            tmpTcb->state = TASK_READY;
            OS_TASK_SWITCH;
        }
        else
        {
            tmpTcb->state = TASK_SUSPENDED;
        }
    }
}

VOID SysTick_Handler(VOID)
{
    TASK_DelayList();
    TASK_WaitForEventList();
    gTaskSysTickCount++;
    if ((gTaskSysTickCount%OS_TASK_SWITCH_INTERVAL) != 0)
    {
        return;
    }

    OS_TASK_SWITCH;
}

/*==================================================================
* Function	: OS_TASK_CreateTask
* Description	: ����������
* Input Para	:     
    IN U8 id,  ����ţ�ͬ�������ȼ���ÿ������Ψһ
    IN TaskFunction_t taskHandle,  ���������������
    IN U16 taskStackDeep,  ���������ջ��ȣ�sizeof(StackSize_t)*taskStackDeep=ʵ��ռ�ڴ��ֽ���
    IN U32 *eventBitMap  �����¼�λͼ����������ͨ�ţ�����Ҫ������NULL
* Output Para	: ��
* Return Value: 
    OS_OK    �����ɹ�
    OS_ERROR ����ʧ��
==================================================================*/
S32 OS_TASK_CreateTask
(
    IN U8 id, 
    IN TaskFunction_t taskHandle, 
    IN U16 taskStackDeep, 
    IN U32 *eventBitMap
)
{
    TCB_S *newTcb = NULL;
    StackSize_t *topStack = NULL;
    
    if (id >= OS_TASK_MAX)
    {
        return OS_ERROR;
    }
    
    newTcb = (TCB_S*)malloc(sizeof(TCB_S));
    if (NULL == newTcb)
    {
        return OS_ERROR;
    }
    
    newTcb->state = TASK_INIT;
    topStack = (StackSize_t *)malloc(sizeof(StackSize_t)*taskStackDeep);
    if (NULL == topStack)
    {
        return OS_ERROR;
    }
    topStack += sizeof(StackSize_t)*taskStackDeep;

    newTcb->topStack = TASK_TaskStackFirstInit(topStack, taskHandle);
    
    newTcb->state = TASK_READY;
    newTcb->delay = 0;
    newTcb->delayMax = 0;
    newTcb->eventBitMap = eventBitMap;
    newTcb->id = id;

    gTaskTcbList[id] = newTcb;
    
    return OS_OK;
}

/*==================================================================
* Function	: OS_TASK_SchedulerTask
* Description	: �����������
* Input Para	: ��
* Output Para	: �� 
* Return Value: ��
==================================================================*/
VOID OS_TASK_SchedulerTask(VOID)
{   
    TASK_StartFirstTask();
    return;
}


/*==================================================================
* Function	: OS_TASK_TaskDelay
* Description	: ������������ȴ���ʱ
* Input Para	: IN U16 ms  ����������
* Output Para	: ��
* Return Value: ��
==================================================================*/
VOID OS_TASK_TaskDelay(IN U16 ms)
{
    if ((0 == gCurrentTCB->delay) && (0 == gCurrentTCB->delayMax))
    {
        gCurrentTCB->delayMax = ms;
        gCurrentTCB->delay = gTaskSysTickCount;
        gCurrentTCB->state = TASK_SUSPENDED;
        OS_TASK_SWITCH;
    }
}

/*==================================================================
* Function	: OS_TASK_WaitForEvent
* Description	: ������������ȴ��¼�
* Input Para	: ��
* Output Para	: ��
* Return Value: ��
==================================================================*/
VOID OS_TASK_WaitForEvent(VOID)
{
    if (NULL == gCurrentTCB->eventBitMap)
    {
        return;
    }
    
    if (0 == *gCurrentTCB->eventBitMap)
    {
        gCurrentTCB->state = TASK_SUSPENDED;
        OS_TASK_SWITCH;
    }
}

