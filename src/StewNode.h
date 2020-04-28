#pragma once
#ifndef STEWNODE_H
#define STEWNODE_H

#include "StewGC.h"

namespace StewGC
{
    class StewNode
    {
    public:
		  StewNode()                                { m_bSGCMarked = false; m_pSGCActiveNext = nullptr; m_pSGCWalkerNext = nullptr; }
		  virtual ~StewNode() {}

		  virtual size_t SGCGetByteSize() = 0;

		  bool SGCGetMarked()									      { return m_bSGCMarked.load(); }
		  void SGCSetMarked(bool a_bValue)		      { m_bSGCMarked.store(a_bValue); }

		  StewNode* SGCGetActiveNext()							{ return m_pSGCActiveNext.load(); }
		  void SGCSetActiveNext(StewNode* a_pNode)	{ m_pSGCActiveNext.store(a_pNode); }

		  StewNode* SGCGetWalkerNext()							{ return m_pSGCWalkerNext.load(); }
		  void SGCSetWalkerNext(StewNode* a_pNode)	{ m_pSGCWalkerNext.store(a_pNode); }

		  virtual void SGCMarkAllPushParents(StewGC* a_pGC, int a_nThreadIndex)	{}
	
	  private:
		  std::atomic_bool		    m_bSGCMarked;
		  std::atomic<StewNode*>	m_pSGCActiveNext;
		  std::atomic<StewNode*>	m_pSGCWalkerNext;
    };
}

#endif // STEWNODE_H
