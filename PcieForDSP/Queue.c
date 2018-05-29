/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.
	zhu: ��д�����Ļص�����
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, PcieForDSPQueueInitialize)
//#pragma alloc_test (PAGE, PcieForDSPEvtIoDeviceControl)
#endif

/*******************************************************************************
*  ����������
*  ��������ʼ����Ҫ�Ķ���
*
*  ������
*  DevExt - ָ�� ������ �ľ��
*
*  ����ֵ��
*  NT status code - failure will result in the device stack being torn down
********************************************************************************/
NTSTATUS
PcieForDSPQueueInitialize(
_In_ PDEVICE_CONTEXT DevExt
//    _In_ WDFDEVICE Device
    )
/*++

Routine Description:


     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
//    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG    queueConfig;

    PAGED_CODE();
    
    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
	//
    /*WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchParallel
        );  */


	// zhu ��ʼ��ȱʡ�������ã�����I/O����ַ�����ʽΪ����
	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

	queueConfig.EvtIoWrite = PcieEvtIoWrite;

	status = WdfIoQueueCreate(DevExt->Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DevExt->WriteQueue);

	if (!NT_SUCCESS(status)) {
//#ifdef DEBUG_ZHU
//		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
//			"WdfIoQueueCreate failed: %!STATUS!", status);
//#endif
		return status;
	}

	//
	// Set the Write Queue forwarding for IRP_MJ_WRITE requests.
	// ����Ӧ WriteFile �� queue
	status = WdfDeviceConfigureRequestDispatching(DevExt->Device,
		DevExt->WriteQueue,
		WdfRequestTypeWrite);

	if (!NT_SUCCESS(status)) {
//#ifdef DEBUG_ZHU
//		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
//			"WdfDeviceConfigureRequestDispatching failed: %!STATUS!", status);
//#endif
		return status;
	}

	//
	// Create a new IO Queue for IRP_MJ_READ requests in sequential mode.
	//
/*	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

	queueConfig.EvtIoRead = PcieEvtIoRead;

	status = WdfIoQueueCreate(DevExt->Device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&DevExt->ReadQueue);

	if (!NT_SUCCESS(status)) {
//#ifdef DEBUG_ZHU
//		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
//			"WdfIoQueueCreate failed: %!STATUS!", status);
//#endif
		return status;
	}*/

	// Create a new IO Dispatch Queue for IRP_MJ_DEVICE_CONTROL  requests in sequential mode.
	//
	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchSequential);

    queueConfig.EvtIoDeviceControl = PcieForDSPEvtIoDeviceControl;
    queueConfig.EvtIoStop = PcieForDSPEvtIoStop;

    status = WdfIoQueueCreate(
                 DevExt->Device,
                 &queueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &DevExt->IoDispatchQueue
                 );

    if( !NT_SUCCESS(status) ) {
       // TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

	// Set the IO Dispatch Queue forwarding for IRP_MJ_DEVICE_CONTROL requests.
	// ����Ӧ DeviceIoControl �� queue
	status = WdfDeviceConfigureRequestDispatching(DevExt->Device,
		DevExt->IoDispatchQueue,
		WdfRequestTypeDeviceControl);

	if (!NT_SUCCESS(status)) {
//#ifdef DEBUG_ZHU
//		TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
//			"WdfDeviceConfigureRequestDispatching failed: %!STATUS!", status);
//#endif
		return status;
	}

    return status;
}

/*******************************************************************************
*  ����������
*  �û�������� DeviceIoControl() ʱ��Ӧ�Ļص�����;
*  ���ݲ�ͬ�� IoControlCode ���в�ͬ�Ĳ�����
*
*  ������
*  Queue - ��ǰ�� ���� ���
*  Request - ��ǰ�� ���� ���
*  OutputBufferLength - Size of the output buffer in bytes
*  InputBufferLength - Size of the input buffer in bytes
*  IoControlCode - I/O control code.
*
*  ����ֵ��
********************************************************************************/
VOID
PcieForDSPEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
    //TraceEvents(TRACE_LEVEL_INFORMATION, 
    //            TRACE_QUEUE, 
    //            "!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d", 
    //            Queue, Request, (int) OutputBufferLength, (int) InputBufferLength, IoControlCode);

    //WdfRequestComplete(Request, STATUS_SUCCESS);

	NTSTATUS  status = STATUS_SUCCESS;
	PDEVICE_CONTEXT devExt = NULL;

	int ret_length = 0;
	PVOID out_buffer;
	size_t out_bufsize;
	PVOID in_buffer;
	size_t in_bufsize;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

#ifdef DEBUG_ZHU
	//DbgPrint("zhu:-->PcieEvtIoDeviceControl()<--");
#endif // DEBUG_ZHU
	
	

	devExt = DeviceGetContext(WdfIoQueueGetDevice(Queue));


	status = WdfRequestRetrieveOutputBuffer(Request, 1, &out_buffer, &out_bufsize);
	if (!NT_SUCCESS(status)){
		WdfRequestCompleteWithInformation(Request, status, ret_length);
		return;
	}

	status = WdfRequestRetrieveInputBuffer(Request, 1, &in_buffer, &in_bufsize);
	if (!NT_SUCCESS(status)){
		WdfRequestCompleteWithInformation(Request, status, ret_length);
		#ifdef DEBUG_ZHU
				//TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER,
				//	"WdfRequestRetrieveInputBuffer failed: %!STATUS!", status);
		#endif
		return;
	}

	#ifdef DEBUG_ZHU
		//TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
		//	"PcieEvtIoDeviceControl: in_buffer 0x%x in_bufsize 0x%x",
		//	(unsigned int)in_buffer, in_bufsize);
	
		//TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
		//	"PcieEvtIoDeviceControl: out_buffer 0x%x out_bufsize 0x%x",
		//	(unsigned int)out_buffer, out_bufsize);
	#endif

		
	devExt->CurrentRequestMode = 0xff;
	switch (IoControlCode)
	{
	case PCIeDMA_IOCTL_WRITE_REG:
	{
#ifdef DEBUG_ZHU
		DbgPrint("zhu:-->PCIeDMA_IOCTL_WRITE_REG<--");
#endif // DEBUG_ZHU
		

		ULONG *ptr = (PULONG)in_buffer;
		ULONG size = ptr[0] / sizeof(ULONG);
		PULONG data = &ptr[1];
		ULONG i;

		devExt->IoWriteRequest = Request;
		devExt->CurrentRequestMode = 0xff;

		//�� bar2 ��  0x8 �� 2 ��ʾCPU��ʽ
		PcieDeviceWriteReg(devExt->MemBar2Base, 0x8, 2);
		//�� bar2 ��  0xc �� CPU��ʽ�����ݳ��ȣ���ʵ�����ݳ��ȣ���ַ�������ڣ�
		PcieDeviceWriteReg(devExt->MemBar2Base, 0xc,size);

		//�� bar1 �� 0x100 ��ʼ����ַ�����ݣ���һ����ַһ�����ݵķ�ʽ���η���
		if (devExt->MemBar1Base)
		{
			for (i = 0; i < size; i++)
			{
				//��ַ
				PcieDeviceWriteReg(devExt->MemBar1Base, 0x100 + i*2*sizeof(ULONG), data[i*2]);
				//����
				PcieDeviceWriteReg(devExt->MemBar1Base, 0x104 + i*2*sizeof(ULONG), data[i*2 +1]);
			}
			status = STATUS_SUCCESS;
		}

		//�����жϣ�֪ͨDSP��ʼ��FPGA������
		PcieDeviceEnableInterrupt(devExt->MemBar0Base);
		PcieDeviceWriteReg(devExt->MemBar0Base, 0x180, 0x1);

		
		//������ʱ��
		devExt->IoWriteTimeout = FALSE;
		PcieTimerStart(devExt->IoWriteTimer, 1000);
		//WdfRequestCompleteWithInformation(Request, status, ret_length);
		break;
	}
	case PCIeDMA_IOCTL_READ_REG:
	{
#ifdef DEBUG_ZHU
		DbgPrint("zhu:-->PCIeDMA_IOCTL_READ_REG<--");
#endif
		ULONG *ptr = (PULONG)in_buffer;
		ULONG addr = ptr[0];

		devExt->ReadRequest = Request;
		devExt->CurrentRequestMode = 0xf;
		devExt->ReadBuffer = out_buffer;

		//�� bar2 ��  0x8 �� 4 ��ʾCPU��ʽ
		PcieDeviceWriteReg(devExt->MemBar2Base, 0x8, 0x4);
		//��bar2 ��  0x18  д�����ȡ�ĵ�ַ
		PcieDeviceWriteReg(devExt->MemBar2Base, 0x18, addr);
		//�����жϣ�֪ͨDSP��ʼ��FPGA������
		PcieDeviceEnableInterrupt(devExt->MemBar0Base);
		PcieDeviceWriteReg(devExt->MemBar0Base, 0x180, 0x1);

		//������ʱ��
		devExt->ReadTimeout = FALSE;
		PcieTimerStart(devExt->ReadTimer, 5000);

		
		break;
	}
	case PCIE_IOCTL_DEBUG:
	{
#ifdef DEBUG_ZHU
		DbgPrint("zhu:-->PCIE_IOCTL_DEBUG<--");
#endif
		

		ULONG *ptr = (PULONG)in_buffer;
		ULONG barX = ptr[0];
		ULONG addr = ptr[1];
		ULONG data = ptr[2];

		if (barX == 0)
		{
			PcieDeviceWriteReg(devExt->MemBar0Base, addr, data);
			status = STATUS_SUCCESS;
		}
		else if (barX == 1)
		{
			PcieDeviceWriteReg(devExt->MemBar1Base, addr, data);
			status = STATUS_SUCCESS;
		}
		else if (barX == 2)
		{
			PcieDeviceWriteReg(devExt->MemBar2Base,addr, data);
			status = STATUS_SUCCESS;
		}
		WdfRequestCompleteWithInformation(Request, status, ret_length);
		break;
	}
	case DSP_PRELOADING:
	{
#ifdef DEBUG_ZHU
		DbgPrint("zhu:-->DSP_PRELOADING<--");
#endif
		ULONG totalSize = sizeof(ULONG);
		ULONG num = 0;
		ULONG *ptr = (PULONG)in_buffer;
		ULONG entryPoint = ptr[0];
		ULONG dataSize = ptr[1] / sizeof(ULONG);
		ULONG startAddr = ptr[2];
		PULONG data = &ptr[3];
		PUCHAR barBaseAddr = devExt->MemBar1Base;

		PcieDeviceWriteReg(devExt->MemBar1Base, (MAGIC_ADDR - LL2_START), entryPoint);

		while (totalSize < (in_bufsize - 3*sizeof(ULONG)))
		{
			dataSize = ptr[num + 1]/sizeof(ULONG);
			startAddr = ptr[num + 2];
			data = &ptr[num + 3];

			if (startAddr < LL2_START)
			{
#ifdef DEBUG_ZHU
				DbgPrint("zhu:  startAddr: 0x%x  is error! ", startAddr);
#endif
				WdfRequestCompleteWithInformation(Request, status, ret_length);
				return;
			}
			else if (startAddr < 0x0087FFFF)
			{
				startAddr &= 0x00FFFFFF;
				startAddr = startAddr - LL2_START;
#ifdef DEBUG_ZHU
				DbgPrint("zhu:  In the LL2_START, offset: 0x%x ", startAddr);
#endif
				barBaseAddr = devExt->MemBar1Base;
			}
			else if ((startAddr >= MSMC_START) && (startAddr < MSMC_END))
			{
				startAddr = startAddr - MSMC_START;
#ifdef DEBUG_ZHU
				DbgPrint("zhu:  In the MSMC_START, offset: 0x%x ", startAddr);
#endif
				barBaseAddr = devExt->MemBar2Base;
			}
			else if ((startAddr >= DDR_START) && (startAddr < DDR_END))
			{
				startAddr = startAddr - DDR_START;
#ifdef DEBUG_ZHU
				DbgPrint("zhu:  In the DDR_START, offset: 0x%x ", startAddr);
#endif
				barBaseAddr = devExt->MemBar3Base;
			}
			else{
#ifdef DEBUG_ZHU
				DbgPrint("zhu:  startAddr: 0x%x  is error! ", startAddr);
#endif
				WdfRequestCompleteWithInformation(Request, status, ret_length);
				return;
			}

#ifdef DEBUG_ZHU
			DbgPrint("zhu: dataSize: 0x%x || startAddr: 0x%x  ", dataSize,startAddr);
#endif

			for (ULONG i = 0; i < dataSize; i++)
			{
				ULONG temp = i*sizeof(ULONG) + totalSize;
				if (temp >= in_bufsize){
#ifdef DEBUG_ZHU
					DbgPrint("zhu: dataSize: 0x%x is error! ",dataSize);
#endif
					WdfRequestCompleteWithInformation(Request, status, ret_length);
					return;

				}
#ifdef DEBUG_ZHU
				DbgPrint("zhu: offsetAddr: 0x%x || data: 0x%x  ", startAddr + i * sizeof(ULONG), data[i]);
#endif
				WRITE_REGISTER_ULONG((PULONG)((ULONG_PTR)barBaseAddr + startAddr + i * sizeof(ULONG)), data[i]);
			}
			totalSize += (dataSize + 2)*sizeof(ULONG);
			num += dataSize +2;
		}
		

		WdfRequestCompleteWithInformation(Request, status, ret_length);
		break;
	}
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		WdfRequestCompleteWithInformation(Request, status, ret_length);
		break;
	}

	

    return;
}

/*******************************************************************************
*  ����������
*  
*
*  ������
*  Queue - ��ǰ�� ���� ���
*  Request - ��ǰ�� ���� ���
*  ActionFlags -
*  
*  ����ֵ��
********************************************************************************/
VOID
PcieForDSPEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
#ifdef DEBUG_ZHU
	//TraceEvents(TRACE_LEVEL_INFORMATION, 
	//	TRACE_QUEUE,
	//	"!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d",
	//	Queue, Request, ActionFlags);
#endif
	UNREFERENCED_PARAMETER(ActionFlags);
	//UNREFERENCED_PARAMETER(Request);
	UNREFERENCED_PARAMETER(Queue);
    
	NTSTATUS status = STATUS_SUCCESS;
	//
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
    //   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
    //   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
    //   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //
    //   Before it can call these methods safely, the driver must make sure that
    //   its implementation of EvtIoStop has exclusive access to the request.
    //
    //   In order to do that, the driver must synchronize access to the request
    //   to prevent other threads from manipulating the request concurrently.
    //   The synchronization method you choose will depend on your driver's design.
    //
    //   For example, if the request is held in a shared context, the EvtIoStop callback
    //   might acquire an internal driver lock, take the request from the shared context,
    //   and then release the lock. At this point, the EvtIoStop callback owns the request
    //   and can safely complete or requeue the request.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge with
    //   a Requeue value of FALSE.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time.
    //
    // In this case, the framework waits until the specified request is complete
    // before moving the device (or system) to a lower power state or removing the device.
    // Potentially, this inaction can prevent a system from entering its hibernation state
    // or another low system power state. In extreme cases, it can cause the system
    // to crash with bugcheck code 9F.
    //
	DbgPrint("zhu:-->PcieForDSPEvtIoStop<-- ");
	WdfRequestComplete(Request, status);
	return;
}

/*******************************************************************************
*  ����������
*  �û��������WriteFile() ʱ��Ӧ�Ļص�����;
*  �����ݴ��뻺��������DMA��ʽ���͸�DSP��
*
*  ������
*  Queue - ��ǰ�� ���� ���
*  Request - ��ǰ�� ���� ���
*  Length - 
*  ����ֵ��
********************************************************************************/
VOID
PcieEvtIoWrite(
_In_ WDFQUEUE         Queue,
_In_ WDFREQUEST       Request,
_In_ size_t           Length
)
/*++

Routine Description:

Called by the framework as soon as it receives a write request.
If the device is not ready, fail the request.
Otherwise get scatter-gather list for this request and send the
packet to the hardware for DMA.

Arguments:

Queue - Handle to the framework queue object that is associated
with the I/O request.
Request - Handle to a framework request object.

Length - Length of the IO operation
The default property of the queue is to not dispatch
zero lenght read & write requests to the driver and
complete is with status success. So we will never get
a zero length request.

Return Value:

--*/
{
	//UNREFERENCED_PARAMETER(Queue);
	//UNREFERENCED_PARAMETER(Request);
	//UNREFERENCED_PARAMETER(Length);

	NTSTATUS status = STATUS_SUCCESS;
	PDEVICE_CONTEXT devExt;
	PVOID 	in_buffer;
	size_t 	in_bufsize;


#ifdef DEBUG_ZHU
//	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!: Request %p", Request);
	DbgPrint("zhu:-->PcieEvtIoWrite<-- ");
#endif
	

	//
	// Get the DevExt from the Queue handle
	//
	devExt = DeviceGetContext(WdfIoQueueGetDevice(Queue));

	//
	// Validate the Length parameter.
	//
	if (Length > MAX_DMA_SIZE_COMMONBUFFER)  {
		status = STATUS_INVALID_BUFFER_SIZE;
		WdfRequestComplete(Request, status);
		return;
	}

	status = WdfRequestRetrieveInputBuffer(Request, 1, &in_buffer, &in_bufsize);
	if (!NT_SUCCESS(status)){
#ifdef DEBUG_ZHU
		DbgPrint("in_bufsize : %d", in_bufsize);
#endif
		WdfRequestComplete(Request, status);
		return;
	}

	RtlCopyMemory(devExt->CommonBufferBase, in_buffer, in_bufsize);

	
	devExt->WriteDmaLength = in_bufsize;
	KeMemoryBarrier();

	devExt->WriteRequest = Request;
	devExt->CurrentRequestMode = 0x0;
	devExt->WriteTimeout = FALSE;

	PcieDeviceStartDMA(devExt, devExt->Interrupt);

	PcieTimerStart(devExt->WriteTimer,20000);
	return;
}


/*******************************************************************************
*  ����������
*  ����DSP��Outbound���ò���
*
*  ������
*  Bar0Base - Bar0�ռ����ַ
*  hostAddress - PcieInitializeDMA() ������� 16M ���������߼���ַ
*
*  ����ֵ��
********************************************************************************/
VOID
PcieDeviceSetupDMA(
_In_ PUCHAR Bar0Base,
_In_ PHYSICAL_ADDRESS hostAddress
)
// zhu ����DSP �� DMA �Ĵ���
{
	//DbgPrint("-->zhu:PcieDeviceSetupDMA<--");
	ULONG srcAddr;
	ULONG pageBase;

#ifdef DEBUG_ZHU
	DbgPrint("zhu:-->Outbound Start<-- ");
#endif 
	
	srcAddr = hostAddress.LowPart;

	// zhu ����Outbound����
	PcieDeviceWriteReg(Bar0Base, CMD_STATUS, 0x7);
	PcieDeviceWriteReg(Bar0Base, OB_SIZE, 0x3);
	for (ULONG i = 0;i<3; i++)
	{
		pageBase = (srcAddr + (PCIE_ADLEN_8MB * i)) & PCIE_8MB_BITMASK;				
		DbgPrint("zhu:(srcAddr + (PCIE_ADLEN_8MB * %d)) & PCIE_8MB_BITMASK : 0x%x",i, pageBase);
		PcieDeviceWriteReg(Bar0Base, OB_OFFSET_INDEX(i), (pageBase | 0x1));
		PcieDeviceWriteReg(Bar0Base, OB_OFFSET_HI(i), 0x00);
	}
#ifdef DEBUG_ZHU
	DbgPrint("zhu:-->Outbound End<-- ");
#endif // DEBUG_ZHU
	

	
}


/*******************************************************************************
*  ����������
*  ����DSP�Ĵ�������ʼDMA
*
*  ������
*  devExt - ָ�� ������ �ľ��
*  interrupt - ָ���жϵľ������Ϊ��ʼ��ʱ�ǲ�ʹ���жϵģ���������δ�õ���
*
*  ����ֵ��
********************************************************************************/
VOID
PcieDeviceStartDMA(
_In_ PDEVICE_CONTEXT devExt,
//_In_ ULONG DataLength,
_In_ WDFINTERRUPT interrupt
)
// hu ��ʼDMA����
{
	ULONG srcAddr;

	DbgPrint("zhu:-->Start EDMA<--");
	UNREFERENCED_PARAMETER(interrupt);
	srcAddr = PCIE_DATA + (devExt->CommonBufferBaseLA.LowPart & ~PCIE_8MB_BITMASK);
	PcieDeviceWriteReg(devExt->MemBar2Base, 0x0, srcAddr);                //����DMA���ݵ�ַ
	PcieDeviceWriteReg(devExt->MemBar2Base, 0x4, devExt->WriteDmaLength); //����DMA���ݳ���
	//PcieDeviceWriteReg(devExt->MemBar2Base, 0x8, 0x1);                    //0x1 -- DMAģʽ ��0x2 -- �Ĵ���ģʽ ,0x3 -- ��̬����ģʽ
	//WdfInterruptAcquireLock(interrupt);

	// Check if DMA busy or not?
	//DmaCtl.ulong = READ_REGISTER_ULONG((PULONG)&dmaRegs->DmaCtl);

	//����DSP��PC�����ж�
	PcieDeviceEnableInterrupt(devExt->MemBar0Base);
	PcieDeviceWriteReg(devExt->MemBar0Base, 0x180, 0x1);//��DSP�����ж�
	

}

/*******************************************************************************
*  ����������
*  �û��������ReadFile() ʱ��Ӧ�Ļص�����
*
*  ������
*
*  ����ֵ��
********************************************************************************/
//VOID
//PcieEvtIoRead(
//_In_ WDFQUEUE         Queue,
//_In_ WDFREQUEST       Request,
//_In_ size_t           Length
//)
/*++

Routine Description:

Called by the framework as soon as it receives a read request.
If the device is not ready, fail the request.
Otherwise get scatter-gather list for this request and send the
packet to the hardware for DMA.

Arguments:

Queue   	- Default queue handle
Request  	- Handle to the write request
Lenght 		- Length of the data buffer associated with the request.
The default property of the queue is to not dispatch
zero lenght read & write requests to the driver and
complete is with status success. So we will never get
a zero length request.

Return Value:

--*/
//{
//	UNREFERENCED_PARAMETER(Queue);
//	UNREFERENCED_PARAMETER(Request);
//	UNREFERENCED_PARAMETER(Length);
//
//	DbgPrint("zhu:-->PcieEvtIoRead<-- ");
//
//	return;
//}
/*

ULONG
PcieDeviceReadReg(
_In_ PUCHAR BarXBase,
_In_ ULONG Address
)
// hu ��ȡPCIE�豸�Ĵ���ֵ
{
	ULONG ret = 0;

	//#ifdef DEBUG_ZHU
	//	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");
	//#endif

	ret = READ_REGISTER_ULONG((PULONG)((ULONG_PTR)BarXBase + Address));

	DbgPrint("BaseAddr:0x%x  offset:0x%x   data:0x%x",(ULONG_PTR)BarXBase,Address,ret);
	//#ifdef DEBUG_ZHU
	//	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER,
	//		"address 0x%x data 0x%x", (ULONG_PTR)BarXBase + Address, ret);
	//
	//	TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC!");
	//#endif
	return ret;
}

VOID
PcieDeviceWriteReg(
_In_ PUCHAR BarXBase,
_In_ ULONG Address,
_In_ ULONG Data
)
// hu д��PCIE�豸�Ĵ���ֵ
{
	WRITE_REGISTER_ULONG((PULONG)((ULONG_PTR)BarXBase + Address), Data);
	DbgPrint("zhu:BaseAddr:0x%X  OffAdd:0x%x  data:0x%x", (ULONG_PTR)BarXBase, Address, Data);

}
*/