#ifndef __DRV_FMC_H__
#define __DRV_FMC_H__

extern VOID DRV_FMC_Erase(IN U32 addrMax, IN U32 addrStart);
/* len���Ȳ��ܳ���buf�ռ䳤�� */
extern VOID DRV_FMC_ReadBuffer(IN U32 addr, IN U32 len, OUT U8 *buf);
/* buf�ռ������4�ı�����len���Ȳ��ܳ���buf�ռ䳤�� */
extern VOID DRV_FMC_WriteBuffer(IN U32 addr, IN U8 *buf, IN U32 len);

#endif

