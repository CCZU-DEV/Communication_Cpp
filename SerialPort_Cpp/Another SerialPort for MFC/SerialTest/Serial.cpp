// Serial.cpp: implementation of the CSerial class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Serial.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static DWORD WINAPI ReadThreadFunc(LPVOID lparam);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//CSerial��Ĺ��캯��
CSerial::CSerial()
{
	m_hPortOwner = NULL;
	m_hComm = INVALID_HANDLE_VALUE;//���ھ����ʼֵΪ��Ч
	memset(&m_ov, 0, sizeof(OVERLAPPED));

//--------------------------------
	// init events
	m_ov.hEvent = NULL;
	m_hWriteEvent = NULL;
	m_hShutdownEvent = NULL;

	m_hReadThread = NULL;
	m_bSerialAlive = false;
//--------------------------------
}

//CSerial�����������
CSerial::~CSerial()
{
//	ClosePort();
	do
	{
		SetEvent(m_hShutdownEvent);		// ���ùرմ����ź�
	}while (m_bSerialAlive);

	if(m_ov.hEvent)
		CloseHandle(m_ov.hEvent);

	if(m_hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
	}

	if(m_hReadThread)
		CloseHandle(m_hReadThread);

}

/*
*�������ܣ��򿪴���
*��ڲ�����pPortOwner	:ʹ�ô˴�����Ĵ�����
		   portNo		:���ں�
		   baud			:������
		   parity		:��żУ��
		   databits		:����λ
		   stopbits		:ֹͣλ
*���ڲ�����(��)
*����ֵ��TRUE:�ɹ��򿪴���;FALSE:�򿪴���ʧ��
*/
BOOL CSerial::OpenPort(HWND hPortOwner,/*ʹ�ô�����Ĵ�����*/
					   UINT portNum,	/*�˿ں�*/
					   UINT baud,		/*������*/
					   UINT parity,		/*У��λ*/
					   UINT databits,	/*����λ*/
					   UINT stopbits	/*ֹͣλ*/
					   )
{
	
	TCHAR sPort[15];

	if(m_hComm != INVALID_HANDLE_VALUE)
	{
		return TRUE;
	}

	ASSERT(hPortOwner != NULL);
//---------------------------------------------------------
	// create events
	if (m_ov.hEvent != NULL)
		ResetEvent(m_ov.hEvent);
	m_ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hWriteEvent != NULL)
		ResetEvent(m_hWriteEvent);
	m_hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hShutdownEvent != NULL)
		ResetEvent(m_hShutdownEvent);
	m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// initialize the event objects
	m_hEventArray[0] = m_hShutdownEvent;	// highest priority
	m_hEventArray[1] = m_ov.hEvent;
	m_hEventArray[2] = m_hWriteEvent;
//------------------------------------------------------------
	m_portNr = portNum;
	m_hPortOwner = hPortOwner;


	//���ô�����
	wsprintf(sPort,L"COM%d:",portNum);
	//�򿪴���
	m_hComm = CreateFile(
		sPort,							/*�˿ں�*/
		GENERIC_READ | GENERIC_WRITE,	/*�ɶ�д*/
		0,								
		NULL,							
		OPEN_EXISTING,					/*�򿪶����Ǵ���*/
		FILE_FLAG_OVERLAPPED,//0,		/*�첽IO��ʽ��*/						
		NULL							
		);
	
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		//������Ч���
//		TRACE(_T("CreateFile ������Ч���"));
		MessageBox(m_hPortOwner,_T("�򿪴���ʧ��!"),_T("��ʾ"),MB_OK);
		return FALSE;
	}

	//��õ�ǰ���ڵ���������
	if(!GetCommState(m_hComm,&commParamm))
	{
		return FALSE;
	}

	commParamm.fBinary = TRUE;		/*������ģʽ*/
	commParamm.fParity = TRUE;		/*֧����żУ��*/
	commParamm.BaudRate = baud;		/*������*/
	commParamm.ByteSize = databits;	/*����λ*/
	commParamm.Parity = parity;		/*��żУ��*/
	commParamm.StopBits = stopbits;	/*ֹͣλ*/

	commParamm.fOutxCtsFlow = FALSE;				// No CTS output flow control 
	commParamm.fOutxDsrFlow = FALSE;				// No DSR output flow control 
	commParamm.fDtrControl = DTR_CONTROL_ENABLE; 
	// DTR flow control type 
	commParamm.fDsrSensitivity = FALSE;			// DSR sensitivity 
	commParamm.fTXContinueOnXoff = TRUE;			// XOFF continues Tx 
	commParamm.fOutX = FALSE;					// No XON/XOFF out flow control 
	commParamm.fInX = FALSE;						// No XON/XOFF in flow control 
	commParamm.fErrorChar = FALSE;				// Disable error replacement 
	commParamm.fNull = FALSE;					// Disable null stripping 
	commParamm.fRtsControl = RTS_CONTROL_ENABLE; 
	// RTS flow control 
	commParamm.fAbortOnError = FALSE;			// �����ڷ������󣬲�����ֹ���ڶ�д

	if (!SetCommState(m_hComm, &commParamm))
	{
//		TRACE(_T("SetCommState error"));
//		AfxMessageBox(_T(""));
		MessageBox(m_hPortOwner,_T("���ô���ʧ��!"),_T("��ʾ"),MB_OK);
		return FALSE;
	}

    //���ô��ڶ�д��ʱ
	
	GetCommTimeouts (m_hComm, &CommTimeOuts);
	CommTimeOuts.ReadIntervalTimeout = MAXDWORD; 
	CommTimeOuts.ReadTotalTimeoutMultiplier = 0;  
	CommTimeOuts.ReadTotalTimeoutConstant = 0;    
	CommTimeOuts.WriteTotalTimeoutMultiplier = 10;  
	CommTimeOuts.WriteTotalTimeoutConstant = 1000;  
	
	if(!SetCommTimeouts(m_hComm, &CommTimeOuts ))
	{
//		TRACE( _T("SetCommTimeouts ���ش���") );
//		AfxMessageBox(_T(""));

		return FALSE;
	}


	//���ö˿ڼ��ӵ��¼��Ӽ�
	SetCommMask(m_hComm,EV_RXCHAR);

	//�����豸������
	SetupComm(m_hComm,1024,1024);

	//��ʼ���������е���Ϣ
	PurgeComm(m_hComm,PURGE_TXCLEAR | PURGE_RXCLEAR);
	

	//�������߳̾��
	m_hReadThread = CreateThread(NULL,0,ReadThreadFunc,this,0,&m_dwReadThreadID);
	
//	TRACE(_T("���ڴ򿪳ɹ�"));
	
	return TRUE; 
}

/*
*�������ܣ��رմ���
*��ڲ�����(��)
*���ڲ�����(��)
*����ֵ��  (��)
*/
void CSerial::ClosePort()
{
	//���ڻ�û��
	if(m_hComm == INVALID_HANDLE_VALUE)
	{
		return ;
	}
	//�ر��߳�
	CloseReadThread();

	//�رմ���
	CloseHandle(m_hComm);//lx
	m_hComm = INVALID_HANDLE_VALUE;
}



/*
*�������ܣ��򴮿ڷ�������
*��ڲ�����buf		: ��Ҫ������д������ݵĻ�����
		   bufLen	: ��Ҫ������д������ݵĻ���������
*���ڲ�����(��)
*����ֵ��TRUE:��ʾ�ɹ��ؽ�Ҫ���͵����ݴ��ݵ�д�߳���Ϣ���С�
		 FALSE:��ʾ��Ҫ���͵����ݴ��ݵ�д�߳���Ϣ����ʧ�ܡ�
		 ע�ӣ��˴���TRUE,��ֱ�Ӵ�������һ���ɹ�д�뵽�����ˡ�
*/
BOOL CSerial::WritePort(const BYTE* buf,DWORD bufLen)
{
//	DWORD dwWriteLen;		//��ȡ�ֽ���
	DWORD dwHaveWriteLen = 0;	//�Ѷ�ȡ�ֽ���

	if(m_hComm == INVALID_HANDLE_VALUE)
		return false;
	m_WriteLength = bufLen;//���һ�η���150�ֽ����ң����������(�д��Ľ�)
//	strcpy((char *)m_szWriteBuffer,(const char *)buf);
/*
	for(int i=0;i<bufLen;i++)
	{
		m_szWriteBuffer[i] = (unsigned char)buf[i];
	}
*/
	m_szWriteBuffer = (unsigned char *)buf;
	// set event for write
	SetEvent(m_hWriteEvent);

/*
	do{
		BOOL bl = WriteFile(m_hComm,buf+dwHaveWriteLen,bufLen-dwHaveWriteLen,&dwWriteLen,&m_ov);
		if(bl)
		{
			dwHaveWriteLen = dwHaveWriteLen + dwWriteLen;
			if(dwHaveWriteLen >= bufLen)
			{
				break ;
			}
		}
		else
		{
			return FALSE;
		}
	}while(TRUE);
*/
	return TRUE;

}

void CSerial::CloseReadThread()
{
	SetEvent(m_hShutdownEvent);	

	//������н�Ҫ��������
	PurgeComm(m_hComm, PURGE_TXCLEAR|PURGE_RXCLEAR);
    //�ȴ�1�룬������߳�û���˳�����ǿ���˳�
    if (WaitForSingleObject(m_hReadThread,50) == WAIT_TIMEOUT)
	{
		TerminateThread(m_hReadThread,0);
	}

	if(m_hReadThread)
		CloseHandle(m_hReadThread);//lx
	m_hReadThread = NULL;
	
}
//���ڶ��̺߳���
static DWORD WINAPI ReadThreadFunc(LPVOID lparam)
{
	CSerial *pSerial = (CSerial*) lparam;

	BYTE* readBuf = NULL;	//��Ŷ�ȡ������
	DWORD willReadLen = 0;		//Ҫ��ȡ���ֽ���
	DWORD actualReadLen = 0;	//ʵ�ʶ�ȡ���ֽ���

	DWORD evtMask;			//�����¼�����
	DWORD dwReadErrors;		//ͨ�Ŵ�������
	COMSTAT comState;		//COMSTAT�ͱ���

	DWORD dw_Evt = 0;
	DWORD dwError = 0;


	//��鴮���Ƿ��
	ASSERT(pSerial->m_hComm != INVALID_HANDLE_VALUE);
	//��մ���(������롢�������)
	PurgeComm(pSerial->m_hComm,PURGE_TXCLEAR | PURGE_TXCLEAR);
	//���ô��ڵ��¼��Ӽ�
//	SetCommMask(pSerial->m_hComm,EV_RXCHAR);
	pSerial->m_bSerialAlive = true;

	while(TRUE)
	{
		if(WaitCommEvent(pSerial->m_hComm,&evtMask,&pSerial->m_ov))
		{
			ClearCommError(pSerial->m_hComm,&dwReadErrors,&comState);
			if(comState.cbInQue < 0)
			{
				continue ;
			}
			
		}
		else
		{
			switch (dwError = GetLastError()) 
			{ 
			case ERROR_IO_PENDING: 	
				{ 
					// This is a normal return value if there are no bytes
					// to read at the port.
					// Do nothing and continue
					break;
				}
			case 87:
				{
					// Under Windows NT, this value is returned for some reason.
					// I have not investigated why, but it is also a valid reply
					// Also do nothing and continue.
					break;
				}
			default:
				{
					// All other error codes indicate a serious error has
					// occured.  Process this error.
//					pSerial->ProcessErrorMessage(_T("WaitCommEvent()"));
					break;
				}
			}
		}

		//---------------------------------------------------
		dw_Evt = WaitForMultipleObjects(3, pSerial->m_hEventArray, FALSE, INFINITE);

		switch(dw_Evt)
		{
		case 0://�رմ����¼�
			pSerial->m_bSerialAlive = false;

			AfxEndThread(100);
			break;
		case 1://�����յ������¼�
			GetCommMask(pSerial->m_hComm, &evtMask);  // ��ȡ�¼�����
			if(evtMask & EV_RXCHAR)
			{

				willReadLen = 4096;
				

				readBuf = new BYTE[willReadLen];
				ReadFile(pSerial->m_hComm,readBuf,willReadLen,&actualReadLen,&pSerial->m_ov);

				if(actualReadLen > 0 && actualReadLen < 0x0000FFFF)
				{
					//�����ں�Ҳͨ����Ϣ���͵������ڣ����ںŷŵ�actualReadLen�ĸ���λ
					actualReadLen |= (pSerial->m_portNr<<24);
					//�����ص�����
					//pSerial->m_OnSeriesRead(pSerial->m_hPortOwner,readBuf,actualReadLen);

					::SendMessage(pSerial->m_hPortOwner, WM_COMM_RXCHAR, actualReadLen, (LPARAM)readBuf);
					//����������
					//SerialDataProc(readBuf,actualReadLen);

					//pSerial->WritePort(readBuf,actualReadLen);//test

				}

				delete []readBuf;
			}
			break;
		case 2://���������¼�
			DWORD dwWriteLen;		//��ȡ�ֽ���
//			DWORD dwHaveWriteLen = 0;	//�Ѷ�ȡ�ֽ���\

			ResetEvent(pSerial->m_hWriteEvent);

			// Initailize variables
			pSerial->m_ov.Offset = 0;
			pSerial->m_ov.OffsetHigh = 0;

			// Clear buffer
			PurgeComm(pSerial->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
			if(!WriteFile(pSerial->m_hComm,pSerial->m_szWriteBuffer,pSerial->m_WriteLength,&dwWriteLen,&pSerial->m_ov))
			{
				dwWriteLen =dwWriteLen;
				break;//�첽IO����ʧ�ܿ����ǻ�û�з�����
			}

			break;
		}
		//---------------------------------------------------

	}
	return 0;
}
/*
void CSerial::ProcessErrorMessage(CString str)
{
	char *Temp = new char[200];

	LPVOID lpMsgBuf;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);

	sprintf(Temp, "WARNING:  %s Failed with the following error: \n%s\nPort: %d\n", (char*)ErrorText, lpMsgBuf, m_portNr); 
	MessageBox(NULL, (LPCTSTR)Temp, (LPCTSTR)"Application Error", MB_ICONSTOP);

	LocalFree(lpMsgBuf);
	delete[] Temp;
}
*/