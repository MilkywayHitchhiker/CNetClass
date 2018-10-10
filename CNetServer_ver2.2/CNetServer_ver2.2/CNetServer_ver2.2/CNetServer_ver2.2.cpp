// CNetServer_ver2.2.cpp : �ܼ� ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include "CNetServer.h"
//#include "ServerConfig.h"

CCrashDump Dump;

class ECHO :public CNetServer
{
public:
	ECHO (void)
	{
	}
	~ECHO (void)
	{
	}
	virtual void OnStart ()
	{

	}
	virtual void OnStop ()
	{

	}
	virtual void OnRecv (UINT64 SessionID, Packet *p)
	{
		INT64 Num;
		*p >> Num;

		Packet *Pack = Packet::Alloc ();
		*Pack << Num;

		SendPacket (SessionID, Pack);

		Packet::Free (Pack);
		return;
	}
	virtual void OnSend (UINT64 SessionID, INT SendByte)
	{
		return;
	}

	virtual bool OnClientJoin (UINT64 SessionID, WCHAR *IP, int PORT)
	{
		INT64 Data = 0x7FFFFFFFFFFFFFFF;

		Packet *Pack = Packet::Alloc ();
		*Pack << Data;

		SendPacket (SessionID, Pack);


		Packet::Free (Pack);

		return true;
	}
	virtual void OnClientLeave (UINT64 SessionID)
	{
		return;
	}
};

ECHO Network;

int main ()
{
	LOG_DIRECTORY (L"LOG_FILE");
	LOG_LEVEL (LOG_WARNING, false);
	//	CServerConfig::Initialize ();
	wprintf (L"MainThread Start\n");
	Network.Start (L"0.0.0.0", 6000, 200, 10);


	UINT AcceptTotal = 0;
	UINT AcceptTPS = 0;
	UINT RecvTPS = 0;
	UINT SendTPS = 0;
	UINT ConnectSessionCnt = 0;
	INT64 MemoryPoolCnt = 0;
	INT64 MemoryPoolUse = 0;

	DWORD StartTime = GetTickCount ();
	DWORD EndTime;
	while ( 1 )
	{
		EndTime = GetTickCount ();
		if ( EndTime - StartTime >= 1000 )
		{
			wprintf (L"==========================\n");
			wprintf (L"Accept User Total = %d \n", AcceptTotal);
			wprintf (L"Connect Session Cnt = %d \n", ConnectSessionCnt);
			wprintf (L"AcceptTPS = %d \n", AcceptTPS);
			wprintf (L"Sec RecvTPS = %d \n", RecvTPS);
			wprintf (L"Sec SendTPS = %d \n", SendTPS);
			wprintf (L"MemoryPoolFull Cnt = %lld\n", MemoryPoolCnt);
			wprintf (L"MemoryPoolUse Cnt = %lld \n", MemoryPoolUse);

			wprintf (L"==========================\n");

			AcceptTotal = Network.AcceptTotal ();
			AcceptTPS = Network.AcceptTPS (true);
			RecvTPS = Network.RecvTPS (true);
			SendTPS = Network.SendTPS (true);
			ConnectSessionCnt = Network.Use_SessionCnt ();
			MemoryPoolCnt = Network.Full_MemPoolCnt ();
			MemoryPoolUse = Network.Alloc_MemPoolCnt ();

			StartTime = EndTime;
		}


		if ( GetAsyncKeyState ('E') & 0x8001 )
		{
			Network.Stop ();
			break;
		}


		/*
		else if ( GetAsyncKeyState ('S') & 0x8001 )
		{
		Network.Start (L"127.0.0.1", 6000, 200, 3);
		}
		*/

		Sleep (200);
	}


	return 0;
}

