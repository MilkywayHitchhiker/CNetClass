#pragma once
#include "MemoryPool.h"

template <class DATA>
class CStack_LF
{
public:

	struct st_NODE
	{
		DATA	Data;
		st_NODE *pNext;
	};

	struct st_TOP_NODE
	{
		st_NODE *pTopNode;
		__int64 iUniqueNum;
	};

public:

	/////////////////////////////////////////////////////////////////////////
	// 생성자
	//
	// Parameters: 없음.
	// Return: 없음.
	/////////////////////////////////////////////////////////////////////////
	CStack_LF ()
	{
		_lUseSize = 0;
		_iUniqueNum = 0;

		_pTop = (st_TOP_NODE *)_aligned_malloc(sizeof(st_TOP_NODE), 16);
		_pTop->pTopNode = NULL;
		_pTop->iUniqueNum = 0;

		_pMemoryPool = new CMemoryPool_LF<st_NODE>(0);
	}

	/////////////////////////////////////////////////////////////////////////
	// 파괴자
	//
	// Parameters: 없음.
	// Return: 없음.
	/////////////////////////////////////////////////////////////////////////
	virtual		~CStack_LF ()
	{
		st_NODE *pNode;
		while (_pTop->pTopNode != NULL)
		{
			pNode = _pTop->pTopNode;
			_pTop->pTopNode = _pTop->pTopNode->pNext;
			_pMemoryPool->Free(pNode);
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// 현재 사용중인 용량 얻기.
	//
	// Parameters: 없음.
	// Return: (int)사용중인 용량.
	/////////////////////////////////////////////////////////////////////////
	__int64			GetUseSize(void){ return _lUseSize; }

	/////////////////////////////////////////////////////////////////////////
	// 총 노드 갯수 얻기
	//
	// Parameters: 없음.
	// Return: (int)락프리 스택의 메모리풀 노드 FullCount.
	/////////////////////////////////////////////////////////////////////////
	__int64			GetFullNode (void)
	{
		return _pMemoryPool->GetFullCount();
	}
	/////////////////////////////////////////////////////////////////////////
	// 데이터가 비었는가 ?
	//
	// Parameters: 없음.
	// Return: (bool)true, false
	/////////////////////////////////////////////////////////////////////////
	bool			isEmpty(void)
	{
		if ( _pTop->pTopNode == NULL )
		{
			return true;
		}

		return false;
	}


	/////////////////////////////////////////////////////////////////////////
	// CPacket 포인터 데이타 넣음.
	//
	// Parameters: (DATA)데이타.
	// Return: (bool) true, false
	/////////////////////////////////////////////////////////////////////////
	bool			Push(DATA Data)
	{
		st_TOP_NODE pPreTopNode;

		__int64 iUniqueNum = InterlockedIncrement64(&_iUniqueNum);
		st_NODE *pNode = _pMemoryPool->Alloc ();
		pNode->Data = Data;

		do {
			pPreTopNode.iUniqueNum = _pTop->iUniqueNum;
			pPreTopNode.pTopNode = _pTop->pTopNode;


			pNode->pNext = pPreTopNode.pTopNode;
		} while (!InterlockedCompareExchange128((volatile LONG64 *)_pTop, iUniqueNum, (LONG64)pNode, (LONG64 *)&pPreTopNode));
		InterlockedIncrement64 ((volatile LONG64 * )&_lUseSize);
		return true;
	}

	/////////////////////////////////////////////////////////////////////////
	// 데이타 빼서 가져옴.
	//
	// Parameters: (DATA *) 뽑은 데이터 넣어줄 포인터
	// Return: (bool) true, false
	/////////////////////////////////////////////////////////////////////////
	bool			Pop(DATA *pOutData)
	{
		st_TOP_NODE pPreTopNode;

		InterlockedDecrement64 (( volatile LONG64 * )&_lUseSize);

		__int64 iUniqueNum = InterlockedIncrement64(&_iUniqueNum);

		do
		{
			pPreTopNode.iUniqueNum = _pTop->iUniqueNum;
			pPreTopNode.pTopNode = _pTop->pTopNode;

			if ( pPreTopNode.pTopNode == NULL )
			{
				*pOutData = NULL;
				return false;
			}

		} while (!InterlockedCompareExchange128((volatile LONG64 *)_pTop, iUniqueNum, (LONG64)pPreTopNode.pTopNode->pNext, (LONG64 *)&pPreTopNode));
		*pOutData = pPreTopNode.pTopNode->Data;
		_pMemoryPool->Free(pPreTopNode.pTopNode);

		return true;
	}

private:
	CMemoryPool_LF<st_NODE>	*_pMemoryPool;
	st_TOP_NODE				*_pTop;

	__int64					_iUniqueNum;

	INT64						_lUseSize;
};