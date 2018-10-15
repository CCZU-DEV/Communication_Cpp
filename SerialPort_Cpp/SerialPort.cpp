#include "SerialPort.h"
//#include "stdafx.h" 
#pragma comment(lib, "user32.lib")

CSerialPort::CSerialPort(
	const porttype & portNum,
	DWORD baudRate /* = 9600 */,
	BYTE byteSize /* = 8 */,
	BYTE stopBit /* = ONESTOPBIT */,
	BYTE parityBit /* = NOPARITY */	
	) : m_portNum(portNum),
	m_dwBaudRate(baudRate),
	m_byteSize(byteSize),
	m_stopBit(stopBit),
	m_parityBit(parityBit),	
	m_bOpen(false)
{
 
}
 
CSerialPort::~CSerialPort()
{
 
}
 
// �򿪴��ڳɹ�,���� true
 
bool CSerialPort::openComm()
{
#ifdef _UNICODE
	m_hComm = CreateFile(m_portNum,
		GENERIC_READ | GENERIC_WRITE, //�������д
		0,		//��ռ��ʽ
		NULL, OPEN_EXISTING,   //�򿪶����Ǵ���
		0,   //ͬ����ʽ
		NULL
		);
#else
	m_hComm = CreateFileA(m_portNum.c_str(),
		GENERIC_READ | GENERIC_WRITE, //�������д
		0,		//��ռ��ʽ
		NULL, OPEN_EXISTING,   //�򿪶����Ǵ���
		0,   //ͬ����ʽ
		NULL
		);
#endif
 
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
#ifdef _UNICODE
		TCHAR szBuf[1024] = { 0 };
		wsprintf(szBuf, L"�� %s ʧ��,����: %d", m_portNum.c_str(), GetLastError());
		MessageBox(NULL, szBuf, L"Warnning", MB_OK);
#else
		char szBuf[1024] = { 0 };
		sprintf_s(szBuf, "�� %s ʧ��,����: %d", m_portNum.c_str(), GetLastError());
		MessageBox(NULL, szBuf, "Warnning", MB_OK);
#endif // _UNICODE
 
		return false;
 
	}
	else
	{
		DCB dcb;
		SetupComm(m_hComm, MAX_BUFFER_SIZE, MAX_BUFFER_SIZE);	// ���ö�д��������С
		GetCommState(m_hComm, &dcb);
		dcb.BaudRate = m_dwBaudRate;
		dcb.ByteSize = m_byteSize;
		dcb.Parity = m_parityBit;
		dcb.StopBits = m_stopBit;
 
		if (!SetCommState(m_hComm, &dcb))
		{
#ifdef _UNICODE
			TCHAR szBuf[1024] = { 0 };
			wsprintf(szBuf, L"��������ʧ��,�������: %d", GetLastError());
			MessageBox(NULL, szBuf, TEXT("ERROR"), MB_OK);
#else
			char szBuf[1024] = { 0 };
			wsprintf(szBuf, "��������ʧ��,�������: %d", GetLastError());
			MessageBox(NULL, szBuf, "ERROR", MB_OK);
#endif
			return false;
		}
 
	}
 
	//�ڶ�д����ǰ���� PurgeComm ������ջ�����
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_TXABORT | PURGE_TXABORT);
 
	m_bOpen = true;
 
	return true;
 
}
 
// �رմ���
 
void CSerialPort::closeComm()
{
	CloseHandle(m_hComm);
}
 
 
// �򴮿ڷ�������
bool CSerialPort::writeToComm(BYTE data[], DWORD dwLength)
{
#ifdef _DEBUG
	assert(m_bOpen == true || dwLength > 0);
	//return false;
#endif // _DEBUG
	DWORD dwError = 0;
	if (ClearCommError(m_hComm, &dwError, NULL) && dwError > 0)
	{
		PurgeComm(m_hComm, PURGE_TXABORT | PURGE_TXCLEAR);
	}
 
	DWORD dwTx = 0;
	BOOL ret = FALSE;
	ret = WriteFile(m_hComm, data, dwLength, &dwTx, NULL);
 
	if (ret == FALSE)
	{
#ifdef _UNICODE
		TCHAR szBuf[1024] = { 0 };
		wsprintf(szBuf, _T("��ȡ����ʧ��,�������: %d"), GetLastError());
		MessageBox(NULL, szBuf, L"ERROR", MB_OK);
#else
		char szBuf[1024] = { 0 };
		sprintf_s(szBuf, "��ȡ����ʧ��, �������: %d", GetLastError());
		MessageBox(NULL, szBuf, "Error", MB_OK);
#endif // _UNICODE
 
		return false;
	}
 
	return true;
 
 
}
 
// �Ӵ����ж�ȡ����
 
bool CSerialPort::readFromComm(char buffer[], DWORD dwLength)
{
#ifdef _DEBUG
	assert(m_bOpen == true || dwLength > 0);
	//return false;
#endif // _DEBUG
 
	COMSTAT comStat;
	DWORD dwError = 0;
	if (ClearCommError(m_hComm, &dwError, &comStat) && dwError > 0)
	{
		PurgeComm(m_hComm, PURGE_RXABORT | PURGE_RXCLEAR);
	}
 
	DWORD dwRx = 0;		// ������ֽ���
	BOOL ret = FALSE;
	BYTE* byReadData = new BYTE[dwLength];
	char szTmp[2] = { 0 };
	int sizeOfBytes = sizeof(char);
	ret = ReadFile(m_hComm, byReadData, dwLength, &dwRx, NULL);	// ��������
 
	if (ret == TRUE)
	{
		/*for (int i = 0; i < dwRx; ++i)
		{
			sprintf_s(szTmp, "%02x", byReadData[i]);
			strcat_s(buffer, sizeOfBytes*dwLength, szTmp);
		}*/
		for (int i = 0; i < dwRx; ++i)
		{
			sprintf_s(szTmp, "%c", byReadData[i]);
			strcat_s(buffer, sizeOfBytes*dwLength+1, szTmp);
		}
 
		// �ͷ��ڴ�
		delete byReadData;
 
		return true;
	}
	else
	{
#ifdef _UNICODE
		TCHAR szBuf[1024] = { 0 };
		wsprintf(szBuf, _T("���ݶ�ȡʧ��,�������: %d"), GetLastError());
		MessageBox(NULL, szBuf, L"Error", MB_OK);
#else
		char szBuf[1024] = { 0 };
		wsprintf(szBuf, "���ݶ�ȡʧ��,�������: %d", GetLastError());
		MessageBox(NULL, szBuf, "Error", MB_OK);
#endif // _UNICODE
 
		return false;
	}
 
	return true;
}
