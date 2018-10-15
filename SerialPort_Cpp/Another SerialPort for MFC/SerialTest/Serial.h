// Serial.h: interface for the CSerial class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERIAL_H__B0D1DE90_996A_4034_92E9_6190452ED811__INCLUDED_)
#define AFX_SERIAL_H__B0D1DE90_996A_4034_92E9_6190452ED811__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//#include "struct.h"
#define WM_COMM_RXCHAR				WM_USER+7	// A character was received and placed in the input buffer. 

//���崮�ڽ������ݺ�������
typedef void (CALLBACK* ONSERIALREAD)(HWND hWnd,BYTE* buf,int bufLen);
//typedef void (* ONSERIALREAD)(HWND hWnd,BYTE* buf,int bufLen);

class CSerial  
{
public:
	CSerial();
	virtual ~CSerial();

public:
	//�򿪴���
	BOOL OpenPort(HWND hPortOwner,			/*ʹ�ô����࣬������*/
				  UINT portNo	= 1,		/*���ں�*/
				  UINT baud		= 9600,		/*������*/
				  UINT parity	= NOPARITY, /*��żУ��*/
				  UINT databits	= 8,		/*����λ*/
				  UINT stopbits	= 0			/*ֹͣλ*/
				  );
	//�رմ���
	void ClosePort();
	//�򴮿�д������
	BOOL WritePort(const BYTE *buf,DWORD bufLen);

private:
	//�رն��߳�
	void CloseReadThread();

private:

	
	//���߳̾��
	HANDLE m_hReadThread;
	//��д�߳�ID��ʶ
	DWORD m_dwReadThreadID;
		

public:
//	void ProcessErrorMessage(char* ErrorText);

public:
	
	//�Ѵ򿪵Ĵ��ھ��
	HWND m_hPortOwner;

	bool m_bSerialAlive;//���ڴ򿪱�ʶ
	
	UINT m_portNr;//���ں�
	
	CWnd* m_pPortOwner;//�򿪴��ڵĴ���ָ��

	HANDLE	m_hComm;				//static

	ONSERIALREAD m_OnSeriesRead; //���ڶ�ȡ�ص�����

	//���ô��ڳ�ʱ�¼�
	COMMTIMEOUTS CommTimeOuts;

	//�첽I/O �ṹ��
	OVERLAPPED			m_ov;

	//DCB
	DCB commParamm;

	HANDLE				m_hEventArray[3];
	HANDLE				m_hWriteEvent; //����д�����¼�
	HANDLE				m_hShutdownEvent;//���ڹر��¼�

	unsigned char*		m_szWriteBuffer;	// д������
	DWORD				m_WriteLength;

};

#endif // !defined(AFX_SERIAL_H__B0D1DE90_996A_4034_92E9_6190452ED811__INCLUDED_)
