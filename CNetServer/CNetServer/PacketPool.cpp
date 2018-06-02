#include "stdafx.h"
#include <windows.h>
#include "PacketPool.h"

CMemoryPool<Packet> *Packet::PacketPool;


Packet::Packet() : Buffer (NULL),DataFieldStart (NULL),DataFieldEnd (NULL),ReadPos (NULL),WritePos (NULL)
{
	//버퍼 사이즈를 입력하지 않는다면, 기본치로 생성.
	Buffer = NULL;
	BufferExpansion = NULL;

	Initial();
	return;
}


Packet::Packet(int iBufferSize) : Buffer (NULL), DataFieldStart (NULL), DataFieldEnd (NULL), ReadPos (NULL), WritePos (NULL)
{
	Buffer = NULL;
	BufferExpansion = NULL;

	Initial(iBufferSize);
	return;
}

Packet::Packet(const Packet &SrcPacket) : Buffer (NULL),_iBufferSize (0), DataFieldStart (NULL), DataFieldEnd (NULL), ReadPos (NULL), WritePos (NULL)
{
	Buffer = NULL;
	BufferExpansion = NULL;
	Initial(SrcPacket._iBufferSize);
	
	/*-----------------------------------------------------------------
	원본 패킷 클래스에서 복사해온다.
	-------------------------------------------------------------------*/
	PutData(SrcPacket.ReadPos, SrcPacket._iDataSize);
	return;
}


Packet::~Packet()
{
	Release();

	return;
}






// 패킷 초기화.
void Packet::Initial(int iBufferSize)
{
	_iBufferSize = iBufferSize;

	if ( NULL == Buffer )
	{
		if ( BUFFER_DEFAULT < _iBufferSize )
		{
			BufferExpansion = new char[_iBufferSize];
			Buffer = BufferExpansion;
		}
		else
		{
			BufferExpansion = NULL;
			Buffer = BufferDefault;
		}
	}

	DataFieldStart = Buffer+HEADERSIZE_DEFAULT;
	DataFieldEnd = Buffer + (_iBufferSize- HEADERSIZE_DEFAULT);

	ReadPos = WritePos = HeaderStartPos = DataFieldStart;

	_iDataSize = 0;
	HeaderSize = 0;
	iRefCnt = 1;
	
	return;
}

//RefCnt를 1 증가시킴. 
void Packet::Add (void)
{
	InterlockedIncrement (( volatile long * )&iRefCnt);
	return;
}

// RefCnt를 하나 차감시키고 REfCnt가 0이 되면 자기자신 delete하고 빠져나옴.
void Packet::Release (void)
{
	if ( NULL != BufferExpansion )
	{
		delete[] BufferExpansion;
	}
	delete this;
	return;
}


// 패킷 초기화
void Packet::Clear(void)
{

	DataFieldStart = Buffer + HEADERSIZE_DEFAULT;
	DataFieldEnd = Buffer + (_iBufferSize - HEADERSIZE_DEFAULT);

	ReadPos = WritePos = HeaderStartPos = DataFieldStart;

	_iDataSize = 0;
}


// Wirtepos 이동. (음수이동은 안됨)
int Packet::MoveWritePos(int iSize)
{
	if ( 0 > iSize ) return 0;

	/*-----------------------------------------------------------------
	 이동할 자리가 부족하다면.
	-------------------------------------------------------------------*/
	if ( WritePos + iSize > DataFieldEnd )
		return 0;

	WritePos += iSize;
	_iDataSize += iSize;

	return iSize;
}

// 버퍼 ReadPos 이동.
int Packet::MoveReadPos(int iSize)
{
	if ( 0 > iSize ) return 0;

	// 이동할만큼 데이타가 없다면.
	if ( iSize > _iDataSize )
		return 0;

	ReadPos += iSize;
	_iDataSize -= iSize;

	return iSize;
}








//////////////////////////////////////////////////////////////////////////
// 연산자 오퍼레이터 시작
//////////////////////////////////////////////////////////////////////////
Packet &Packet::operator = (Packet &SrcPacket)
{
	ReadPos = Buffer;
	WritePos = Buffer;

	//원본 패킷 클래스에서 복사해온다.
	PutData(SrcPacket.ReadPos, SrcPacket._iDataSize);

	return *this;
}

// 넣기.	각 변수 타입마다 모두 만듬.
Packet &Packet::operator << (BYTE byValue)
{
	PutData(( char * )&byValue, sizeof(BYTE)); //캐스팅 기본(char *)&value
	return *this;
}

Packet &Packet::operator << (char chValue)
{
	PutData(&chValue, sizeof(char));
	return *this;
}

Packet &Packet::operator << (short shValue)
{
	PutData(( char * )&shValue, sizeof(short));
	return *this;
}

Packet &Packet::operator << (WORD wValue)
{
	PutData(( char * )&wValue, sizeof(WORD));
	return *this;
}

Packet &Packet::operator << (int iValue)
{
	PutData(reinterpret_cast<char *>(&iValue), sizeof(int));

	return *this;
}

Packet &Packet::operator << (DWORD dwValue)
{
	PutData(( char * )&dwValue, sizeof(DWORD));
	return *this;
}

Packet &Packet::operator << (float fValue)
{
	PutData(( char * )&fValue, sizeof(float));
	return *this;
}

Packet &Packet::operator << (__int64 iValue)
{
	PutData(( char * )&iValue, sizeof(__int64));
	return *this;
}

Packet &Packet::operator << (double dValue)
{
	PutData((char *)&dValue, sizeof(double));
	return *this;
}

//////////////////////////////////////////////////////////////////////////
// 빼기.	각 변수 타입마다 모두 만듬.
//////////////////////////////////////////////////////////////////////////
Packet &Packet::operator >> (BYTE &byValue)
{
	GetData((char *)&byValue, sizeof(BYTE));
	return *this;
}

Packet &Packet::operator >> (char &chValue)
{
	GetData(&chValue, sizeof(char));
	return *this;
}

Packet &Packet::operator >> (short &shValue)
{
	GetData((char *)&shValue, sizeof(short));
	return *this;
}

Packet &Packet::operator >> (WORD &wValue)
{
	GetData((char *)&wValue, sizeof(WORD));
	return *this;
}

Packet &Packet::operator >> (int &iValue)
{
	GetData((char *)&iValue, sizeof(int));
	return *this;
}

Packet &Packet::operator >> (DWORD &dwValue)
{
	GetData((char *)&dwValue, sizeof(DWORD));
	return *this;
}

Packet &Packet::operator >> (float &fValue)
{
	GetData((char *)&fValue, sizeof(float));
	return *this;
}

Packet &Packet::operator >> (__int64 &iValue)
{
	GetData((char *)&iValue, sizeof(__int64));
	return *this;
}

Packet &Packet::operator >> (double &dValue)
{
	GetData((char *)&dValue, sizeof(double));
	return *this;
}







// 데이타 얻기.
int Packet::GetData(char *chpDest, int iSize)
{
	//얻고자 하는 만큼의 데이타가 없다면.
	if ( iSize > _iDataSize )
		return 0;

	memcpy(chpDest, ReadPos, iSize);
	ReadPos += iSize;

	_iDataSize -= iSize;

	return iSize;

}


// 데이타 삽입.
int Packet::PutData(char *chpSrc, int iSrcSize)
{
	//넣을 자리가 없다면.
	if ( WritePos + iSrcSize > DataFieldEnd )
		return 0;

	memcpy(WritePos, chpSrc, iSrcSize);
	WritePos += iSrcSize;

	_iDataSize += iSrcSize;

	return iSrcSize;
}

// 헤더 삽입.
int	Packet::PutHeader (char *chpSrc, int iSrcSize)
{
	if ( HeaderStartPos - iSrcSize < Buffer )
		return 0;

	char *PrePos = HeaderStartPos - iSrcSize;
	memcpy (PrePos, chpSrc, iSrcSize);
	HeaderStartPos = PrePos;

	_iDataSize += iSrcSize;

	return  iSrcSize;
}