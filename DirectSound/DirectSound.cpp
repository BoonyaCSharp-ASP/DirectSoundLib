
#include "stdafx.h"
#include <stdio.h>
#include <dsound.h>
#include <Windows.h>
#include <WinBase.h>

LPDIRECTSOUNDCAPTURE8 directSoundCapture8;
LPDIRECTSOUNDCAPTUREBUFFER8 directSoundCaptureBuffer8;
LPDIRECTSOUNDCAPTUREBUFFER *ppDSCBuffer;
HANDLE notifyHwnd;
#define CODE_DSCAPTURE_FAILED 1
#define CODE_DSCAPTURE_SUCCESS 2
bool isCapturing;


int CreateCaptureDevice()
{
	/*
	* lpcGUID: ���������豸��ID, NULL��Ĭ���豸, DSDEVID_DefaultCaptureϵͳĬ����Ƶ�����豸����NULL�ƺ�һ����, DSDEVID_DefaultVoiceCaptureĬ�����������豸���������ͷ�ϵ���˷��豸��
	* ppDSC8 : IDirectSoundCapture8�ӿ�ָ��
	*/
	HRESULT result = ::DirectSoundCaptureCreate8(NULL, &directSoundCapture8, NULL);
	if (result != DS_OK)
	{
		/*
		* ʧ��, ����ֵ������:
		* DSERR_ALLOCATED
		* DSERR_INVALIDPARAM
		* DSERR_NOAGGREGATION
		* DSERR_OUTOFMEMORY
		* ���������֧��full duplex, ��ô����DSERR_ALLOCATED
		*/
		return CODE_DSCAPTURE_FAILED;
	}

	// �ɹ�
	return CODE_DSCAPTURE_SUCCESS;
}

/*
��������ָÿ��������ٴ�
����λ����ָÿ�β�����λ��,������8λ����16λ, ����λ���ֳ�����λ��
ͨ��������������Ƶ����������,���ǵ�����,��ôÿ�β���һ��ͨ��,������������,��ôÿ�β���������ͬ����
��������ÿ���������bit
������ = ������ * ����λ�� * ͨ����, ������ֽ��ٳ���8
*/
int CreateCaptureBuffer()
{
	// Ҫ¼�ƵĲ���������ʽ����
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	// ͨ������
	waveFormat.nChannels = 2;
	// ������, ÿ���������
	waveFormat.nSamplesPerSec = 44100;
	/*
	* ����λ��, ÿ��������λ��
	* ���wFormatTag��WAVE_FORMAT_PCM, ��������Ϊ8����16, �����Ĳ�֧��
	* ���wFormatTag��WAVE_FORMAT_EXTENSIBLE, ��������Ϊ8�ı���, һЩѹ�������������ֵ, ���Դ�ֵ����Ϊ0
	*/
	waveFormat.wBitsPerSample = 16;
	// ���ֽ�Ϊ��λ���ÿ���롣�������ָ��С���ݵ�ԭ�Ӵ�С�����wFormatTag = WAVE_FORMAT_PCM, nBlockAlignΪ(nChannels * wBitsPerSample) / 8, ���ڷ�PCM��ʽ����ݳ��̵�˵������
	waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
	// �����������ݵĴ�������, ÿ��ƽ��������ֽ���, ��λbyte/s, ���wFormatTag = WAVE_FORMAT_PCM, nAvgBytesPerSecΪnBlockAlign * nSamplesPerSec, ���ڷ�PCM��ʽ����ݳ��̵�˵������
	waveFormat.nAvgBytesPerSec = waveFormat.nBlockAlign * waveFormat.nSamplesPerSec;
	// ������Ϣ�Ĵ�С�����ֽ�Ϊ��λ��������Ϣ�����WAVEFORMATEX�ṹ�Ľ�β�������Ϣ������Ϊ��PCM��ʽ��wFormatTag�������ԣ����wFormatTag����Ҫ�������Ϣ����ֵ����Ϊ0������PCM��ʽ��ֵ�����ԡ�
	waveFormat.cbSize = 0;

	// ¼��������������
	DSCBUFFERDESC captureBufferDesc;
	captureBufferDesc.dwSize = sizeof(DSCBUFFERDESC);
	/*
	* ָ���豸����, ����Ϊ0,
	* DSCBCAPS_CTRLFX:��֧��Ч����Buffer��
	*     ֻ֧�ִ�DirectSoundCaptureCreate8�����������豸����, ��ҪWindowsXP�汾��Capture effects require Microsoft Windows XP��
	* DSCBCAPS_WAVEMAPPED��The Win32 wave mapper will be used for formats not supported by the device.��
	*/
	captureBufferDesc.dwFlags = 0;
	// ���񻺳�����С, �ֽ�Ϊ��λ
	captureBufferDesc.dwBufferBytes = waveFormat.nAvgBytesPerSec; // ��������С����Ϊ��������, ��ôÿһ���������ʹ洢��һ���ӵ���������
																  // �����ֶ�, ���Ժ�ʹ��
	captureBufferDesc.dwReserved = 0;
	// Ҫ����Ĳ��������ĸ�ʽ��Ϣ
	captureBufferDesc.lpwfxFormat = &waveFormat;
	// һ��Ϊ0, ����dwFlag�ֶ�������DSCBCAPS_CTRLFX��־
	captureBufferDesc.dwFXCount = 0;
	captureBufferDesc.lpDSCFXDesc = NULL;

	/*
	* CreateCaptureBuffer����:
	* pcDSCBufferDesc: DSCBUFFERDESC���������������ĵ�ַ
	* ppDSCBuffer:����IDirectSoundCaptureBuffer�ӿڵĵ�ַ, ʹ��QueryInterface����IDirectSoundCaptureBuffer8
	* pUnkOuter:ΪNULL
	*/
	LPDIRECTSOUNDCAPTUREBUFFER directSoundCaptuerBuffer;
	HRESULT result = directSoundCapture8->CreateCaptureBuffer(&captureBufferDesc, &directSoundCaptuerBuffer, NULL);
	if (result == DS_OK)
	{
		directSoundCaptuerBuffer->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)&directSoundCaptureBuffer8); // ʹ��IDirectSoundCaptureBuffer��QueryInterface����IDirectSoundCaptureBuffer8�ӿ�
		directSoundCaptuerBuffer->Release();
		return CODE_DSCAPTURE_SUCCESS;
	}
	else
	{
		/*
		DSERR_INVALIDPARAM
		DSERR_BADFORMAT
		DSERR_GENERIC
		DSERR_NODRIVER
		DSERR_OUTOFMEMORY
		DSERR_UNINITIALIZED
		*/
		return CODE_DSCAPTURE_FAILED;
	}
}

int SetCaptureNotifications()
{
	// ��directSoundCaptureBuffer8�����ﴴ��һ��¼��֪ͨ����
	LPDIRECTSOUNDNOTIFY8 directSoundNotify8;
	HRESULT result = directSoundCaptureBuffer8->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&directSoundNotify8);
	if (result != DS_OK)
	{
		return CODE_DSCAPTURE_FAILED;
	}

	// ��ȡ������������Ϣ
	WAVEFORMATEX waveFormat;
	result = directSoundCaptureBuffer8->GetFormat(&waveFormat, sizeof(WAVEFORMATEX), NULL);
	if (result != DS_OK)
	{
		// ��ȡ¼����ʽ��Ϣʧ��
		return CODE_DSCAPTURE_FAILED;
	}

	// ����¼��֪ͨλ��, ��¼������ָ���Ļ�����λ�õ�ʱ��, �ᴥ��֪ͨ�¼�, ��֪ͨ�¼��ص�����Զ�ȡ�������е���Ƶ����
	notifyHwnd = CreateEvent(NULL, 1, 0, NULL);
	if (notifyHwnd == NULL)
	{
		// ����֪ͨ����ʧ��
		return CODE_DSCAPTURE_FAILED;
	}
	// ����DirectSoundʹ�õ�֪ͨ����
	DSBPOSITIONNOTIFY dsPositionNotify[1];
	// ���û������е�֪ͨλ��
	dsPositionNotify[0].dwOffset = waveFormat.nAvgBytesPerSec / 2; // ����֪ͨλ��Ϊÿ�봫������, ��˼���ǵ�¼��һ���ӵ���Ƶ����֮��ͻᴥ��֪ͨ�¼�, ���ֱ������ΪnAvgBytesPerSec�Ļ�, �ᱨ����
	dsPositionNotify[0].hEventNotify = notifyHwnd;

	/*
	* ����֪ͨ����
	* dwPositionNotifies:DSBPOSITIONNOTIFY�ṹ�������
	* pcPositionNotifies:DSBPOSITIONNOTIFY�����ָ��, ��������СΪDSBNOTIFICATIONS_MAX
	*/
	result = directSoundNotify8->SetNotificationPositions(1, dsPositionNotify);
	directSoundNotify8->Release();
	if (result != DS_OK)
	{
		// ����֪ͨλ��ʧ��
		return CODE_DSCAPTURE_FAILED;
	}

	return CODE_DSCAPTURE_SUCCESS;
}

int StartCapture()
{
	/*
	* ��ʼ¼��
	* dwFlags:ָ��¼���������Ķ���, DSCBSTART_LOOPING��ʾ��¼�����������˵�ʱ��, ���ͷ��ʼ����¼��ֱ������Stop��������¼��
	* ���ʹ�ö��߳�¼��, �ڲ��񻺳�����ʱ��, ����Start�������̱߳������
	*/
	HRESULT result = directSoundCaptureBuffer8->Start(DSCBSTART_LOOPING);
	if (result != DS_OK)
	{
		// DSERR_INVALIDPARAM, DSERR_NODRIVER, DSERR_OUTOFMEMORY
		return CODE_DSCAPTURE_FAILED;
	}

	return CODE_DSCAPTURE_SUCCESS;
}

int StopCapture()
{
	isCapturing = false;
	directSoundCaptureBuffer8->Stop();

	return CODE_DSCAPTURE_SUCCESS;
}

int Dispose()
{
	if (isCapturing)
	{
		::StopCapture();
	}
	directSoundCaptureBuffer8->Release();
	directSoundCapture8->Release();
	CloseHandle(notifyHwnd);

	return CODE_DSCAPTURE_SUCCESS;
}


int DSCaptureInitialize()
{
	int resultCode = CODE_DSCAPTURE_SUCCESS;

	resultCode = CreateCaptureDevice();
	if (resultCode != CODE_DSCAPTURE_SUCCESS)
	{
		return resultCode;
	}

	resultCode = CreateCaptureBuffer();
	if (resultCode != CODE_DSCAPTURE_SUCCESS)
	{
		return resultCode;
	}

	resultCode = SetCaptureNotifications();
	if (resultCode != CODE_DSCAPTURE_SUCCESS)
	{
		return resultCode;
	}

	resultCode = StartCapture();
	if (resultCode != CODE_DSCAPTURE_SUCCESS)
	{
		return resultCode;
	}

	return CODE_DSCAPTURE_SUCCESS;
}

void DSCaptureStart()
{
	HRESULT result = DS_OK;

	isCapturing = true;

	int lockOffset = 0;

	while (isCapturing)
	{
		// �ȴ�DirectSound��¼��֪ͨ�¼�
		WaitForSingleObject(notifyHwnd, 3000);

		/*
		* ��ȡ��ǰ����λ�úͶ�ȡλ��, ȷ�������ȡ����ʹ�õĻ�����λ��, �������Ҫ��һ������, ��������ΪNULL
		* pdwCapturePosition:��Ӳ�����Ƴ��������ݽ���λ��
		* pdwReadPosition:�Ѿ���ȫ���񵽻���������Ƶ���ݽ���λ��
		*/
		DWORD capturePosition, readPosition;
		result = directSoundCaptureBuffer8->GetCurrentPosition(&capturePosition, &readPosition);
		if (result != DS_OK)
		{
			// DSERR_INVALIDPARAM, DSERR_NODRIVER, DSERR_OUTOFMEMORY  
			break;
		}

		if (readPosition == 0)
		{
			continue;
		}

		// ��������������ȡ��Ƶ����
		LPVOID audioPtr1, audioPtr2;
		DWORD  captureLength1 = 0, captureLength2 = 0;

		result = directSoundCaptureBuffer8->Lock(lockOffset, readPosition, &audioPtr1, &captureLength1, &audioPtr2, &captureLength2, NULL);
		if (result != DS_OK)
		{
			// DSERR_INVALIDPARAM, DSERR_INVALIDCALL
			break;
		}

		lockOffset = readPosition;

		//printf("dataLength1 = %i, dataLength2 = %i\n", captureLength1, captureLength2);

		// �����ȡ���Ĵ�СС��Ԥ����Ļ����С, ��ô˵���������Ѿ�������, ��ʱaudioPtr2��洢audioPtr1δ��ȡ�������
		if (captureLength1 < readPosition)
		{
		}

		result = directSoundCaptureBuffer8->Unlock(audioPtr1, captureLength1, audioPtr2, captureLength2);
		if (result != DS_OK)
		{
			//DSERR_INVALIDPARAM, DSERR_INVALIDCALL  
			break;
		}

		// �����ȴ�֪ͨ�¼�
		ResetEvent(notifyHwnd);
	}
}

void DSCaptureStop()
{
	::StopCapture();
}




#ifdef __cplusplus
extern "C"
#endif
{
	__declspec(dllexport) int __stdcall TestInvoke(LPCDSCBUFFERDESC pcDSCBufferDesc, LPDIRECTSOUNDCAPTUREBUFFER *ppDSCBuffer, LPUNKNOWN pUnkOuter)
	{
		//HRESULT result = DirectSoundCaptureCreate8(NULL, &directSoundCapture8, NULL);

		//result = directSoundCapture8->CreateCaptureBuffer(pcDSCBufferDesc, ppDSCBuffer, pUnkOuter);

		return 0;
	}

	__declspec(dllexport) int __stdcall TestInvoke2(LPDIRECTSOUNDCAPTURE8 pdsc8, LPCDSCBUFFERDESC pcDSCBufferDesc, LPDIRECTSOUNDCAPTUREBUFFER *ppDSCBuffer, LPUNKNOWN pUnkOuter)
	{
		HRESULT re = pdsc8->CreateCaptureBuffer(pcDSCBufferDesc, ppDSCBuffer, pUnkOuter);

		return 0;
	}

	__declspec(dllexport) int __stdcall TestInvoke3(LPDIRECTSOUNDCAPTUREBUFFER8 buffer)
	{
		//::DSCaptureInitialize();

		//::DSCaptureStart();

		DWORD cursorPos, readPos;
		HRESULT result = buffer->GetCurrentPosition(&cursorPos, &readPos);

		return 0;
	}
}


