/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.
	zhu  ��Ӳ����صĲ������ṹ�塢�����ַ��ƫ�Ƶ�ַ�ȶ�д������

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
	WDFDEVICE               Device;             
	ULONG                   PhysicalAddressRegister;

	PUCHAR                  MemBar0Base;      	//zhu: bar0 Registers base address (must be 32bit align)
	ULONG                   MemBar0Length;    	//zhu  bar0 Registers base length

	PUCHAR                  MemBar1Base;      	//zhu: bar1 Registers base address (must be 32bit align)
	ULONG                   MemBar1Length;    	//zhu��bar1 Registers base length

	PUCHAR                  MemBar2Base;      	//zhu��bar2  Registers base address (must be 32bit align)
	ULONG                   MemBar2Length;    	//zhu��bar2  Registers base length

	PUCHAR                  MemBar3Base;      	//zhu��bar3  Registers base address (must be 32bit align)
	ULONG                   MemBar3Length;    	//zhu��bar3  Registers base length

	WDFDMAENABLER           DmaEnabler;         //
	WDFCOMMONBUFFER         CommonBuffer;       // Ϊ DMA �����һ�� 16M ���ڴ�ռ�
	size_t                  CommonBufferSize;   // �ռ��С 16M
	PUCHAR                  CommonBufferBase;   // ����������Է��ʵĻ������������ַ
	PHYSICAL_ADDRESS        CommonBufferBaseLA; // �豸���Է��ʵĻ��������߼���ַ

	WDFQUEUE                WriteQueue;         //write queue for DMA
	WDFREQUEST				WriteRequest;       //DMA����
	ULONG					WriteDmaLength;     //DMA  ���ݳ���
	WDFTIMER                WriteTimer;         //Ԥ�� DMA ������ʱ�Ķ�ʱ��
	BOOLEAN                 WriteTimeout;       //��ʱ���Ƿ�ʱ
	
	WDFREQUEST				ReadRequest;        // ������
	PULONG					ReadBuffer;         // �������� bufferָ��
	WDFTIMER                ReadTimer;          // Ԥ�� ������ ������ʱ�Ķ�ʱ��
	BOOLEAN                 ReadTimeout;        // ��ʱ���Ƿ�ʱ

	ULONG					CurrentRequestMode; // ��ǰ����ģʽ�� 0x0-- DMA��ʽ��0xf--��ȡģʽ�� 0xff--дģʽ

	WDFINTERRUPT            Interrupt;     	    // Returned by InterruptCreate DSP�ķ����ж�
	PKINTERRUPT             InterruptObject;    // address of interrupt object
	ULONG                   IntStatus;          // �ж�״̬����δʹ�ã�

	WDFQUEUE                IoDispatchQueue;    // д������ queue
	WDFREQUEST				IoWriteRequest;     // д��������
	BOOLEAN                 IoWriteTimeout;     // Ԥ�� д���� ������ʱ�Ķ�ʱ��
	WDFTIMER                IoWriteTimer;       // ��ʱ���Ƿ�ʱ


} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device and its callbacks
//

//NTSTATUS
//PcieForDSPCreateDevice(
//    _Inout_ PWDFDEVICE_INIT DeviceInit
//    );

NTSTATUS
PcieInitializeDeviceContext(
//_In_ PWDF_OBJECT_ATTRIBUTES Attributes,
_In_ PDEVICE_CONTEXT DevExt
);

NTSTATUS
PcieInitializeDMA(
_In_ PDEVICE_CONTEXT DevExt
);

EVT_WDF_DRIVER_DEVICE_ADD PcieForDSPEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP PcieForDSPEvtDriverContextCleanup;

EVT_WDF_DEVICE_D0_ENTRY PcieForDspDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT PcieForDspDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE PcieForDspPreparaHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE PcieForDspReleaseHardware;