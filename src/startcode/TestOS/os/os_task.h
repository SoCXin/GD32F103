#ifndef __OS_TASK_H__
#define __OS_TASK_H__

typedef void (*TaskFunction_t)(void);

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
extern S32 OS_TASK_CreateTask
(
    IN U8 id, 
    IN TaskFunction_t taskHandle, 
    IN U16 taskStackDeep, 
    IN U32 *eventBitMap
);

/*==================================================================
* Function	: OS_TASK_SchedulerTask
* Description	: �����������
* Input Para	: ��
* Output Para	: �� 
* Return Value: ��
==================================================================*/
extern VOID OS_TASK_SchedulerTask(VOID);

/*==================================================================
* Function	: OS_TASK_TaskDelay
* Description	: ������������ȴ���ʱ
* Input Para	: IN U16 ms  ����������
* Output Para	: ��
* Return Value: ��
==================================================================*/
extern VOID OS_TASK_TaskDelay(IN U16 ms);

/*==================================================================
* Function	: OS_TASK_WaitForEvent
* Description	: ������������ȴ��¼�
* Input Para	: ��
* Output Para	: ��
* Return Value: ��
==================================================================*/
extern VOID OS_TASK_WaitForEvent(VOID);

#endif

