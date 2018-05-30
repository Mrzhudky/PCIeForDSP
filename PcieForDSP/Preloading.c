#include "driver.h"
#include "Preloading.tmh"

//#pragma  alloc_text (PAGE,DspInBound)
//#pragma  alloc_text (PAGE,WriteDspProject)
/*******************************************************************************
*  ����������
*  ����DSP��Outbound���ò���.
*
*  ������
*
*  ����ֵ��
********************************************************************************/
VOID
DspInBound(
_In_ PDEVICE_CONTEXT devExt
)
{
#ifdef DEBUG_ZHU
	DbgPrint("zhu: --> DspInBound Start!<--");
#endif
	//Configure IB_BAR0;
	//for (int i = 0; i < 4; i++)
	//{
	//	m_MemoryRange0.outd(IB_BAR(i), i);//Configure IB_BAR0;
	//	m_MemoryRange0.outd(IB_START_LO(i), m_basePhysAddressBar[i]);	//???��λ��BAR��ַ	
	//	m_MemoryRange0.outd(IB_START_HI(i), 0);
	//}
	//ULONG addr = 0;
		
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_BAR(0), 0);
		//addr = PcieDeviceReadReg(devExt->MemBar0Base, 0x1010);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_LO(0), 0xF7B00000);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_HI(0), 0);

		PcieDeviceWriteReg(devExt->MemBar0Base, IB_BAR(1), 1);
		//PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_LO(1), (ULONG_PTR)devExt->MemBar1Base);
		//addr = PcieDeviceReadReg(devExt->MemBar0Base, 0x1014);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_LO(1), 0xE6000000);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_HI(1), 0);

		PcieDeviceWriteReg(devExt->MemBar0Base, IB_BAR(2), 2);
		//PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_LO(2), (ULONG_PTR)devExt->MemBar2Base);
		//addr = PcieDeviceReadReg(devExt->MemBar0Base, 0x1018);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_LO(2), 0xE4000000);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_HI(2), 0);

		PcieDeviceWriteReg(devExt->MemBar0Base, IB_BAR(3), 3);
		//PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_LO(3), (ULONG_PTR)devExt->MemBar3Base);
		//addr = PcieDeviceReadReg(devExt->MemBar0Base, 0x101c);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_LO(3), 0xE0000000);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_START_HI(3), 0);


		//	m_MemoryRange0.outd(IB_OFFSET(0), PCIE_BASE_ADDRESS);
		//m_MemoryRange0.outd(IB_OFFSET(1), LL2_START(0));
		//m_MemoryRange0.outd(IB_OFFSET(2), MSMC_START);
		//m_MemoryRange0.outd(IB_OFFSET(3), DDR_START);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_OFFSET(0), PCIE_BASE_ADDRESS);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_OFFSET(1), LL2_START_0);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_OFFSET(2), MSMC_START);
		PcieDeviceWriteReg(devExt->MemBar0Base, IB_OFFSET(3), DDR_START);

		//enable outboud register
		//m_MemoryRange0.outd(CMD_STATUS, 0x7);
		PcieDeviceWriteReg(devExt->MemBar0Base, CMD_STATUS, 0x7);
	
	

#ifdef DEBUG_ZHU
	DbgPrint("zhu: --> DspInBound End!<--");
#endif
	return;
}

/*******************************************************************************
*  ����������
*  Ԥ����DSP����.���ļ�����������������
*
*  ������
*  Queue - ��ǰ�� ���� ���
*  Request - ��ǰ�� ���� ���
*  Length -
*  ����ֵ��
********************************************************************************/
NTSTATUS
WriteDspProject(
_In_ PDEVICE_CONTEXT devExt,
_In_ ULONG step
)
{
	HANDLE fileHandle = NULL;      //Դ�ļ����
	NTSTATUS Status = STATUS_SUCCESS;    //����״̬

	OBJECT_ATTRIBUTES ObjectAttributes;  //OBJECT_ATTRIBUTES�ṹ
	UNICODE_STRING fileName;  //Դ�ļ�
	IO_STATUS_BLOCK IoStatusBlock;         //���ؽ��״̬�ṹ��
	LARGE_INTEGER Offset = { 0 };            //���ļ���ƫ��

	ULONG dataSize = 0;
	ULONG targetAddr = 0;                     //Ҫ��д�ĳ���
	PULONG buffer = NULL;                   //bufferָ��

	RtlInitUnicodeString(&fileName, L"\\DosDevices\\D:\\dsp.bin");

	//��ʼ��OBJECT_ATTRIBUTES�ṹ��
	InitializeObjectAttributes(
		&ObjectAttributes,
		&fileName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL,
		NULL);
	if (step >0)
	{
		//��Ŀ���ļ�
		Status = ZwCreateFile(
			&fileHandle,
			GENERIC_READ,
			&ObjectAttributes,
			&IoStatusBlock,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OVERWRITE_IF,
			FILE_NON_DIRECTORY_FILE | FILE_RANDOM_ACCESS | FILE_SYNCHRONOUS_IO_NONALERT,
			NULL, 0);
		if (!NT_SUCCESS(Status))
		{
#ifdef DEBUG_ZHU
			DbgPrint("zhu: Ŀ���ļ���ʧ��!! - %#X", Status);
#endif
			ZwClose(fileHandle);
			//ExFreePool(buffer);
			return Status;
		}
#ifdef DEBUG_ZHU
		DbgPrint("zhu: Ŀ���ļ��򿪳ɹ�!!");
#endif
		if (step>1)
		{
			//��ʼ���ļ�ָ�벢ѭ�������ļ�,ÿ�θ���һ���ֽ�
			Offset.HighPart = 0;
			Offset.LowPart = 0;
			//byteOffset.LowPart = byteOffset.HighPart = 0;
			//Offset.QuadPart = 0;
			while (NT_SUCCESS(Status))
			{
				//�����ݳ���size
				Status = ZwReadFile(
					fileHandle,
					NULL,
					NULL,
					NULL,
					&IoStatusBlock,
					&dataSize,
					sizeof(ULONG),
					&Offset,
					NULL);
				if (!NT_SUCCESS(Status))
				{
#ifdef DEBUG_ZHU
					DbgPrint("zhu: ��ȡʧ��");
#endif
					if (IoStatusBlock.Status == STATUS_END_OF_FILE)
						//			if (STATUS_END_OF_FILE == NULL)
					{
						Status = STATUS_SUCCESS;
#ifdef DEBUG_ZHU
						DbgPrint("zhu: �Ѷ����ļ�ĩβ!");
#endif
					}
					break;
				}
#ifdef DEBUG_ZHU
				DbgPrint("zhu: ��ȡ dataSize�� 0x%x", dataSize);
#endif
				if ((dataSize / 4) * 4 != dataSize)
				{
					dataSize = ((dataSize / 4) + 1) * 4;
				}

				//����Ŀ���ַ
				Status = ZwReadFile(
					fileHandle,
					NULL,
					NULL,
					NULL,
					&IoStatusBlock,
					&targetAddr,
					sizeof(ULONG),
					&Offset,
					NULL);
				if (!NT_SUCCESS(Status))
				{
#ifdef DEBUG_ZHU
					DbgPrint("zhu: ��ȡʧ��");
#endif
					//			if (STATUS_END_OF_FILE == NULL)
					//			{
					//				Status = STATUS_SUCCESS;
					//#ifdef DEBUG_ZHU
					//				DbgPrint("�Ѷ����ļ�ĩβ!");
					//#endif
					//			}
					break;
				}
#ifdef DEBUG_ZHU
				DbgPrint("zhu: ��ȡĿ���ַ�� 0x%x", targetAddr);
#endif
				ULONG offsetAddr = targetAddr - LL2_START;

				if (step >2)
				{
					//��������
					Status = ZwReadFile(
						fileHandle,
						NULL,
						NULL,
						NULL,
						&IoStatusBlock,
						buffer,
						dataSize,
						&Offset,
						NULL);
					if (!NT_SUCCESS(Status))
					{
#ifdef DEBUG_ZHU
						DbgPrint("zhu: ��ȡʧ��");
#endif
						//			if (STATUS_END_OF_FILE == NULL)
						//			{
						//				Status = STATUS_SUCCESS;
						//#ifdef DEBUG_ZHU
						//				DbgPrint("�Ѷ����ļ�ĩβ!");
						//#endif
						//			}
						break;
					}
					ULONG dataNum = dataSize / sizeof(ULONG);
					PULONG dataUlong = (PULONG)buffer;
					for (ULONG i = 0; i < dataNum; i++)
					{
						WRITE_REGISTER_ULONG((PULONG)((ULONG_PTR)devExt->MemBar1Base + offsetAddr + i*sizeof(ULONG)), dataUlong[i]);
					}
				}
				if (step < 4)
				{
					Status = STATUS_END_OF_FILE;
				}
			}
		}
		
	}
	

	//�ͷ�ָ��,�ͷ�������ڴ�
	ZwClose(fileHandle);
	//ExFreePool(buffer);
	return Status;
}