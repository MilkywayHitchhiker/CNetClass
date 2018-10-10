#pragma once
#include "MemoryPool.h"


/*======================================================================================
//싱글 링크드 리스크 형식 락프리 Queue.
=======================================================================================*/
template<class DATA>
class CQueue_LF
{
#define MaxCnt 50000
private :
	struct NODE
	{
		DATA Data;
		NODE *pNext;
	};
	struct _TOP_NODE
	{
		NODE *pNode;
		INT64 UNIQUEUE;
	};


	_TOP_NODE *_pHead;
	_TOP_NODE *_pTail;

	volatile __int64 _NodeCnt;
	__int64 _UniqueNUM;
public :
	CMemoryPool_LF<NODE> *_pMemPool;
	/*//////////////////////////////////////////////////////////////////////
	//생성자.파괴자.
	//////////////////////////////////////////////////////////////////////*/
	CQueue_LF ()
	{
		_TOP_NODE *HNode = ( _TOP_NODE * )_aligned_malloc (sizeof (_TOP_NODE), 16);
		_TOP_NODE *TNode = ( _TOP_NODE * )_aligned_malloc (sizeof (_TOP_NODE), 16);
		_pMemPool = new CMemoryPool_LF<NODE> (0);
	

		HNode->pNode = _pMemPool->Alloc ();
		HNode->pNode->pNext = NULL;
		HNode->UNIQUEUE = 0;
		
		TNode->pNode = HNode->pNode;
		TNode->UNIQUEUE = 0;
		_NodeCnt = 0;
		
		_pHead = HNode;
		_pTail = TNode;


	}
	~CQueue_LF ()
	{
		DATA p;
		while ( 1 )
		{
			
			if ( Dequeue(&p) == false )
			{
				return;
			}
		}
		_pMemPool->Free (_pHead->pNode);
	}


	/*//////////////////////////////////////////////////////////////////////
	//Enqueue
	//인자 : DATA
	//return : true,false
	//////////////////////////////////////////////////////////////////////*/

	bool Enqueue (DATA Data)
	{
		int roop = 0;
		INT64 NodeCnt = InterlockedIncrement64 (&_NodeCnt);
		//Queue가 꽉찼으므로 return false.
		if ( NodeCnt >= MaxCnt )
		{
			return false;
		}

		_TOP_NODE PreNode;
		NODE *pNext;

		NODE *pNode = _pMemPool->Alloc ();
		pNode->Data = Data;


		while ( 1 )
		{
			PreNode.UNIQUEUE = _pTail->UNIQUEUE;
			PreNode.pNode = _pTail->pNode;

			pNext = PreNode.pNode->pNext;
			//Tail의 next가 NULL이 아닐 경우 이미 누군가가 먼저 노드를 넣었으므로 Tail을 밀어줌.
			if ( pNext != NULL )
			{
				InterlockedCompareExchange128 (( volatile LONG64 * )_pTail, ( LONG64 )PreNode.UNIQUEUE + 1, ( LONG64 )pNext, ( LONG64 * )&PreNode);
				roop++;
				if ( roop > 5 )
				{
					CCrashDump::Crash ();
				}
				continue;
			}

			//Tail의 Next가 NULL일 경우 현재 노드 연결

			pNode->pNext = NULL;
			if ( InterlockedCompareExchangePointer (( volatile PVOID * )&_pTail->pNode->pNext,( PVOID ) pNode, NULL) == NULL )
			{
				InterlockedCompareExchange128 (( volatile LONG64 * )_pTail, ( LONG64 )PreNode.UNIQUEUE + 1, ( LONG64 )pNode, ( LONG64 * )&PreNode);
				return true;
			}


		}
	}


	/*//////////////////////////////////////////////////////////////////////
	//Dequeue
	//인자 : 뽑을 데이터를 담을 버퍼.
	//return : true,false
	//////////////////////////////////////////////////////////////////////*/
	bool Dequeue (DATA *pOut)
	{
		_TOP_NODE PreNode;
		_TOP_NODE TailNode;

		NODE *pNext;
		
		INT64 Unique = InterlockedIncrement64 (( volatile LONG64 * )&_UniqueNUM);

		while ( 1 )
		{
			TailNode.UNIQUEUE = _pTail->UNIQUEUE;
			TailNode.pNode = _pTail->pNode;

			pNext = TailNode.pNode->pNext;
			//Tail의 next가 NULL이 아닐 경우 이미 누군가가 먼저 노드를 넣었으므로 Tail을 밀어줌.
			if ( pNext != NULL )
			{
				InterlockedCompareExchange128 (( volatile LONG64 * )_pTail, ( LONG64 )TailNode.UNIQUEUE + 1, ( LONG64 )TailNode.pNode->pNext, ( LONG64 * )&TailNode);
			}
			

			PreNode.UNIQUEUE = _pHead->UNIQUEUE;
			PreNode.pNode = _pHead->pNode;

			
			pNext = PreNode.pNode->pNext;
			//헤드의 Next노드를 뽑는 것이므로 pNode가 NULL이라면 Dequeue 불가능.
			if ( pNext == NULL )
			{
				pOut = NULL;
				return false;
			}
			
			*pOut = pNext->Data;
			if ( InterlockedCompareExchange128 (( volatile LONG64 * )_pHead,( LONG64 ) Unique, ( LONG64 )pNext, ( LONG64 * )&PreNode) )
			{
				_pMemPool->Free (PreNode.pNode);
				InterlockedDecrement64 (&_NodeCnt);
				return true;
			}
		}
	}



	/*//////////////////////////////////////////////////////////////////////
	//GetUseSize
	//인자 : 없음
	//return : 현재 Node갯수.
	//////////////////////////////////////////////////////////////////////*/
	INT64 GetUseSize (void)
	{
		return _NodeCnt;
	}
};






/*======================================================================================
//싱글 링크드 리스크 형식 Thread Safe SRW Lock버전 Queue
=======================================================================================*/
template<class DATA>
class CQueue_LOCK
{
private:
	struct NODE
	{
		DATA Data;
		NODE *pNext;
	};
	struct _TOP_NODE
	{
		NODE *pNode;
	};


	_TOP_NODE *_pHead;
	_TOP_NODE *_pTail;

	CMemoryPool<NODE> *_pMemPool;

	volatile __int64 _NodeCnt;
	volatile __int64 _MaxCnt;

	SRWLOCK _CS;
public:

	/*//////////////////////////////////////////////////////////////////////
	//생성자.파괴자.
	//////////////////////////////////////////////////////////////////////*/
	CQueue_LOCK ()
	{
		InitializeSRWLock (&_CS);

		_TOP_NODE *HNode = ( _TOP_NODE * )_aligned_malloc (sizeof (_TOP_NODE), 16);
		_TOP_NODE *TNode = ( _TOP_NODE * )_aligned_malloc (sizeof (_TOP_NODE), 16);
		HNode->pNode = ( NODE * )malloc (sizeof (NODE));
		HNode->pNode->pNext = NULL;

		TNode->pNode = HNode->pNode;
		_MaxCnt = MaxCnt;
		_NodeCnt = 0;


		_pHead = HNode;
		_pTail = TNode;

		_pMemPool = new CMemoryPool<NODE> (0);
	}
	~CQueue_LOCK ()
	{
		DATA p;
		while ( 1 )
		{
			
			if ( Dequeue (&p) == false )
			{
				return;
			}
		}
	}


	/*//////////////////////////////////////////////////////////////////////
	//Enqueue
	//인자 : DATA
	//return : true,false
	//////////////////////////////////////////////////////////////////////*/

	bool Enqueue (DATA Data)
	{

		//Queue가 꽉찼으므로 return false.
		if ( _NodeCnt >= _MaxCnt )
		{
			return false;
		}

		_TOP_NODE PreNode;


		NODE *pNode = _pMemPool->Alloc ();
		pNode->Data = Data;
		pNode->pNext = NULL;

		LOCK ();

		PreNode.pNode = _pTail->pNode;

		_pTail->pNode->pNext = pNode;
		_pTail->pNode = pNode;
		
		InterlockedIncrement64 (&_NodeCnt);

		Free ();
		return true;
	}


	/*//////////////////////////////////////////////////////////////////////
	//Dequeue
	//인자 : 뽑을 데이터를 담을 버퍼.
	//return : true,false
	//////////////////////////////////////////////////////////////////////*/
	bool Dequeue (DATA *pOut)
	{
		_TOP_NODE PreNode;
		NODE *pNode;
		LOCK ();

		PreNode.pNode = _pHead->pNode;

		pNode = PreNode.pNode->pNext;

		//헤드의 Next노드를 뽑는 것이므로 pNode가 NULL이라면 Dequeue 불가능.
		if ( pNode == NULL )
		{
			pOut = NULL;
			Free ();
			return false;
		}

		_pHead->pNode = pNode;

		*pOut = pNode->Data;
		_pMemPool->Free (PreNode.pNode);
		InterlockedDecrement64 (&_NodeCnt);

		Free ();

		return true;
	}



	/*//////////////////////////////////////////////////////////////////////
	//GetUseSize
	//인자 : 없음
	//return : 현재 Node갯수.
	//////////////////////////////////////////////////////////////////////*/
	INT64 GetUseSize (void)
	{
		return _NodeCnt;
	}


	void LOCK (void)
	{
		AcquireSRWLockExclusive (&_CS);
	}
	void Free (void)
	{
		ReleaseSRWLockExclusive (&_CS);
	}
};