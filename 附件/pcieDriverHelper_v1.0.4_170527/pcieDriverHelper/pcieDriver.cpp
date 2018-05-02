
#include "stdafx.h"
#include "Public.h"
#include <windows.h>
#include <setupapi.h>
#include <stddef.h>

#ifdef CS_HU
#pragma unmanaged
#endif

HANDLE hPcieDev = INVALID_HANDLE_VALUE;
TCHAR lastError[100] = L"�޴���";

TCHAR* __stdcall GetLastDevError()
{
	return lastError;
}

bool __stdcall OpenPcieDev()
{
	bool status = false;
	HDEVINFO hDevInfo;
	SP_DEVICE_INTERFACE_DATA dia;
	dia.cbSize = sizeof(dia);
	SP_DEVINFO_DATA da;
	da.cbSize = sizeof(da);
	PSP_DEVICE_INTERFACE_DETAIL_DATA didd;//һ��ָ��SP_DEVICE_INTERFACE_DETAIL_DATA�ṹ��ָ�룬�ýṹ���ڽ���ָ���ӿڵ���Ϣ��
	DWORD nRequiredSize;
	int ii;
	BOOL isFind;

	hDevInfo = SetupDiGetClassDevs(
		(LPGUID)&GUID_DEVINTERFACE_PcieForDSP,
		NULL,
		NULL,
		DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);

	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		wcscpy_s(lastError, L"δɨ�赽PCIE�豸��");
		return status;
	}

	ii = 0;
	isFind = TRUE;
	while (isFind)
	{
		isFind = SetupDiEnumDeviceInterfaces(
			hDevInfo,
			NULL,
			(LPGUID)&GUID_DEVINTERFACE_PcieForDSP,
			ii,
			&dia);
		if (isFind)
		{
			SetupDiGetDeviceInterfaceDetail(
				hDevInfo,&dia,NULL,0,&nRequiredSize,&da);

			didd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(nRequiredSize);
			didd->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

			if (SetupDiGetDeviceInterfaceDetail(
				hDevInfo,&dia,didd,nRequiredSize,NULL,&da))
			{
				hPcieDev = CreateFile(
					didd->DevicePath,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					0,
					NULL);

				free((PVOID) didd);
				if (hPcieDev == INVALID_HANDLE_VALUE)
				{
					wcscpy_s(lastError, L"��PCIE�豸ʧ�ܣ�");
					return status;
				}

				status = true;
				break;
			}
			else
				wcscpy_s(lastError, L"��ȡPCIE�豸��Ϣʧ�ܣ�");
			free((PVOID) didd);
			ii++;
		}
		else
			wcscpy_s(lastError, L"δ����PCIE�豸��");
	}
	return status;
}

void __stdcall ClosePcieDev()
{
	if (hPcieDev != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hPcieDev);
		hPcieDev = INVALID_HANDLE_VALUE;
	}
}

bool __stdcall DmaToDev(LPDWORD pBufferAddr,DWORD bufferSize)
{
	DWORD outputSize;
	if (hPcieDev == INVALID_HANDLE_VALUE)
	{
		wcscpy_s(lastError, L"PCIE�豸δ�򿪣�");
		return false;
	}

	if (bufferSize > MAX_DMABUFFER_SIZE)
	{
		wcscpy_s(lastError, L"д�����ݳ��ȳ�����󳤶ȣ�");
		return false;
	}

	if (!WriteFile(hPcieDev,pBufferAddr,bufferSize,&outputSize,NULL))
	{
		wcscpy_s(lastError, L"DMAд��PCIE�豸ʧ�ܣ�");
		return false;
	}

	 if (outputSize != bufferSize)
	 {
	 	wcscpy_s(lastError, L"�������ݳ������������ݳ��Ȳ�����");
	 	return false;
	 }

	return true;
}

bool __stdcall DmaToDevWithMode(LPDWORD pBufferAddr,DWORD bufferSize,DWORD mode)
{
	DWORD outputSize;
	if (hPcieDev == INVALID_HANDLE_VALUE)
	{
		wcscpy_s(lastError, L"PCIE�豸δ�򿪣�");
		return false;
	}

	if (bufferSize > MAX_DMABUFFER_SIZE)
	{
		wcscpy_s(lastError, L"д�����ݳ��ȳ�����󳤶ȣ�");
		return false;
	}

	if (mode == 3)
	{
		DWORD32 pInputBuf[3];
		DWORD32 outputBuf;
		DWORD outputBufSize;
		pInputBuf[0] = 2;
		pInputBuf[1] = 0x8;
		pInputBuf[2] = 0x3;
		bool state = true;

		state = DeviceIoControl(hPcieDev,PCIE_IOCTL_DEBUG,pInputBuf,3*sizeof(DWORD32),&outputBuf,sizeof(DWORD32),&outputBufSize,NULL);
		if(!state)
		{
			wcscpy_s(lastError, L"д��ʧ�ܣ�");
			return false;
		}
	} 
	else
	{
		DWORD32 pInputBuf[3];
		DWORD32 outputBuf;
		DWORD outputBufSize;
		pInputBuf[0] = 2;
		pInputBuf[1] = 0x8;
		pInputBuf[2] = 0x1;
		bool state = true;

		state = DeviceIoControl(hPcieDev,PCIE_IOCTL_DEBUG,pInputBuf,3*sizeof(DWORD32),&outputBuf,sizeof(DWORD32),&outputBufSize,NULL);
		if(!state)
		{
			wcscpy_s(lastError, L"д��ʧ�ܣ�");
			return false;
		}
	}

	if (!WriteFile(hPcieDev,pBufferAddr,bufferSize,&outputSize,NULL))
	{
		wcscpy_s(lastError, L"DMAд��PCIE�豸ʧ�ܣ�");
		return false;
	}

	if (outputSize != bufferSize)
	{
		wcscpy_s(lastError, L"�������ݳ������������ݳ��Ȳ�����");
		return false;
	}

	return true;
}
// bool __stdcall DmaFromDev(LPDWORD pBufferAddr,DWORD bufferSize)
// {
// 	DWORD outputSize;
// 	if (hPcieDev == INVALID_HANDLE_VALUE)
// 	{
// 		wcscpy_s(lastError, L"PCIE�豸δ�򿪣�");
// 		return false;
// 	}

// 	if (bufferSize > MAX_DMABUFFER_SIZE)
// 	{
// 		wcscpy_s(lastError, L"��ȡ���ݳ��ȳ�����󳤶ȣ�");
// 		return false;
// 	}

// 	if (!ReadFile(hPcieDev,pBufferAddr,bufferSize,&outputSize,NULL))
// 	{
// 		wcscpy_s(lastError, L"DMA��ȡPCIE�豸ʧ�ܣ�");
// 		return false;
// 	}

// 	if (outputSize != bufferSize)
// 	{
// 		wcscpy_s(lastError, L"�������ݳ������������ݳ��Ȳ�����");
// 		return false;
// 	}

// 	return true;
// }


bool __stdcall SetByCPUMode(PDWORD32 pRegAddr,DWORD32 dataNum)
{
	PDWORD32 pInputBuf;
	DWORD32 inputBufNum;
	DWORD32 outputBuf;
	DWORD outputBufSize;

	if (hPcieDev == INVALID_HANDLE_VALUE)
	{
		wcscpy_s(lastError, L"PCIE�豸δ�򿪣�");
		return false;
	}

	inputBufNum = dataNum*2 +1;
	pInputBuf = new DWORD32[inputBufNum];

	pInputBuf[0] = dataNum*sizeof(DWORD32);

	
	//for(DWORD i=0;i< dataNum*2;i++)
	//{
	//	pInputBuf[i+1] = pRegAddr[i];
	//}
	//pInputBuf[0] = devRegAddr;
	
	memcpy(&pInputBuf[1],pRegAddr,dataNum*2*sizeof(DWORD32));
	if (!DeviceIoControl(hPcieDev,PCIeDMA_IOCTL_WRITE_REG,
		pInputBuf,inputBufNum * sizeof(DWORD32),&outputBuf,sizeof(DWORD32),&outputBufSize,NULL))
	{
		wcscpy_s(lastError, L"д��PCIE�豸�Ĵ���ʧ�ܣ�");
		delete pInputBuf;
		return false;
	}
	
	delete pInputBuf;
	return true;
}

bool __stdcall SetDevRegister(DWORD32 devRegAddr,PDWORD32 pRegAddr,DWORD32 regSize)
{
	PDWORD32 pInputBuf;
	DWORD32 inputBufNum;
	DWORD32 outputBuf;
	DWORD outputBufSize;

	if (hPcieDev == INVALID_HANDLE_VALUE)
	{
		wcscpy_s(lastError, L"PCIE�豸δ�򿪣�");
		return false;
	}

	inputBufNum = regSize / sizeof(DWORD32)*2 +1;
	pInputBuf = new DWORD32[inputBufNum];

	pInputBuf[0] = regSize;//--��2017/06/27���⣺׼����regSizeֱ��ΪĿ��ֵ�������漰�������͵����⣡��--
	for(DWORD i=0;i< (inputBufNum-1)/2;i++)
	{
		pInputBuf[i*2+1] = devRegAddr + i*4;
		pInputBuf[i*2+2] = pRegAddr[i];
	}
	//pInputBuf[0] = devRegAddr;
	
	//memcpy(&pInputBuf[2],pRegAddr,regSize);
	if (!DeviceIoControl(hPcieDev,PCIeDMA_IOCTL_WRITE_REG,
		pInputBuf,inputBufNum * sizeof(DWORD32),&outputBuf,sizeof(DWORD32),&outputBufSize,NULL))
	{
		wcscpy_s(lastError, L"д��PCIE�豸�Ĵ���ʧ�ܣ�");
		delete pInputBuf;
		return false;
	}
	
	delete pInputBuf;
	return true;
}

bool __stdcall SetDevRegister(DWORD32 devRegAddr,DWORD32 regData)
{
	DWORD32 pInputBuf[3];
	DWORD32 outputBuf;
	DWORD outputBufSize;

	if (hPcieDev == INVALID_HANDLE_VALUE)
	{
		wcscpy_s(lastError, L"PCIE�豸δ�򿪣�");
		return false;
	}

	
	pInputBuf[0] = sizeof(DWORD32);
	pInputBuf[1] = devRegAddr;
	pInputBuf[2] = regData;
	if (!DeviceIoControl(hPcieDev,PCIeDMA_IOCTL_WRITE_REG,
		pInputBuf,3*sizeof(DWORD32),&outputBuf,sizeof(DWORD32),&outputBufSize,NULL))
	{
		wcscpy_s(lastError, L"д��PCIE�豸�Ĵ���ʧ�ܣ�");
		return false;
	}

	return true;
}



bool __stdcall DebugRegister(DWORD32 barX,DWORD32 OfferAddr,DWORD32 data)
{
	DWORD32 pInputBuf[3];
	DWORD32 outputBuf;
	DWORD outputBufSize;
	bool state;

	if (hPcieDev == INVALID_HANDLE_VALUE)
	{
		wcscpy_s(lastError, L"PCIE�豸δ�򿪣�");
		return false;
	}

	pInputBuf[0] = barX;
	pInputBuf[1] = OfferAddr;
	pInputBuf[2] = data;

	state = DeviceIoControl(hPcieDev,PCIE_IOCTL_DEBUG,pInputBuf,3*sizeof(DWORD32),&outputBuf,sizeof(DWORD32),&outputBufSize,NULL);
	if(!state)
	{
		wcscpy_s(lastError, L"д��ʧ�ܣ�");
		return false;
	}

	return true;

}
#ifdef CS_HU
#pragma managed
#endif