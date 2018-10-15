#ifndef _SERIALPORT_H
#define _SERIALPORT_H
 
#include <string>
#include <Windows.h>
#include <cstddef>
#include <cstdlib>
#include <cassert>
 
// ���崮������
#ifdef _UNICODE
typedef CString porttype;
#else
typedef std::string porttype;
#endif // _UNICODE
 
typedef unsigned long ulong;
typedef unsigned char uchar;
 
 
class CSerialPort
{
public:
	CSerialPort(
		const porttype & portNum,		// ���ں�
		DWORD baudRate = 9600,			// ������
		BYTE  byteSize = 8,				// ����λ
		BYTE  stopBit = ONESTOPBIT,		// ֹͣλ
		BYTE  parityBit = NOPARITY		// ����λ		
		);
 
	~CSerialPort();
 
public:
 
	bool openComm();										// �򿪴���
	void closeComm();										// �رմ���
	bool writeToComm(BYTE data[], DWORD dwLegnth);			// ��������
	bool readFromComm(char buffer[], DWORD dwLength);		// ��ȡ����
 
private:
 
	HANDLE m_hComm;		// ͨ���豸
	porttype m_portNum; // ���ں�
	DWORD m_dwBaudRate; // ������
	BYTE  m_byteSize;	// ����λ
	BYTE  m_stopBit;	// ֹͣλ
	BYTE  m_parityBit;  // У��λ	
	bool  m_bOpen;		// ���ڿ��ر�־
private:
	
	enum BufferSize
	{
		MIN_BUFFER_SIZE = 256,
		BUFFER_SIZE = 512,
		MAX_BUFFER_SIZE = 1024
	};
 
	// ���ô��ں�
	void setPortNum(const porttype &portNum)
	{
		this->m_portNum = portNum;
	}
	// ���ò�����
	void setBaudRate(const ulong baudRate)
	{
		this->m_dwBaudRate = baudRate;
	}
	// ��������λ
	void setByteSize(const uchar byteSize)
	{
		this->m_byteSize = byteSize;
	}
	// ���ü���λ
	void setParityBit(const uchar parityBit)
	{
		this->m_parityBit = parityBit;
	}
	// ����ֹͣλ
	void setStopBit(const uchar stopBit)
	{
		this->m_stopBit = stopBit;
	}
 
	// ��ȡ���ں�
	porttype getPortNum() { return m_portNum; }
	// ��ȡ������
	ulong getBaudRate() { return m_dwBaudRate; }
	// ��ȡ����λ
	uchar getByteSize() { return m_byteSize; }
	// ��ȡ����λ
	uchar getParityBit() { return m_parityBit; }
	// ��ȡֹͣλ
	uchar getStopBit() { return m_stopBit; }
};
 
 
#endif		// _SERIALPORT_H
