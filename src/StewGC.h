class TGC
	{
	public:

		// Initialization of GC system --------------------------------------

		TGC(int a_nThreads)							
		{ 
			m_pRootHead = nullptr; 
			m_pActiveHead = nullptr; 
			m_nGCBytes = 0;
			m_nGCPrevBytes = 0;
			m_nGCPrevTimestamp = std::chrono::high_resolution_clock::now();
			m_nThreads = a_nThreads;
			m_pWalkerHeads = new std::atomic<TGCNode*>[a_nThreads];
			m_bCollectMarkingSignal = false;
			m_nCollectReadyThreads = 0;
			m_nCollectDoneThreads = 0;
			m_bRootsAdded = false;
		}

		// (Not Thread-Safe)
		~TGC()
		{
			TGCNode * pNode = m_pActiveHead.load();
			while (pNode != nullptr)
			{
				delete m_pActiveHead.load();
				pNode = pNode->GCGetActiveNext();
			}
			delete[] m_pWalkerHeads;
		}

		// (Not Thread-Safe)
		void AddRootNode(TGCNode * a_pNode)
		{ 
			a_pNode->GCSetWalkerNext(m_pRootHead); 
			m_pRootHead = a_pNode;
			NewObject(a_pNode);
		}

		// (Not Thread-Safe)
		void RemoveRootNode(TGCNode * a_pNode)
		{ 
			if (a_pNode == m_pRootHead)
			{
				m_nGCBytes -= a_pNode->GCGetBytes();
				m_pRootHead = m_pRootHead->GCGetWalkerNext();
			}
			else if (m_pRootHead != nullptr)
			{
				TGCNode* pPrevNode = m_pRootHead;
				TGCNode* pNode = m_pRootHead->GCGetWalkerNext();
				while (pNode != nullptr)
				{
					if (pNode == a_pNode)
					{
						m_nGCBytes -= a_pNode->GCGetBytes();
						pPrevNode->GCSetWalkerNext(pNode->GCGetWalkerNext());
						break;
					}
					pPrevNode = pNode;
					pNode = pNode->GCGetWalkerNext();
				}
			}
		}

		// GC as a utility --------------------------------------

		// Thread-Safe
		void NewObject(TGCNode * a_pNode)
		{ 
			TGCNode* pHead = m_pActiveHead.load();
			while (true)
			{
				a_pNode->GCSetActiveNext(pHead);
				if (m_pActiveHead.compare_exchange_strong(pHead, a_pNode)) break;
				Utils::ThreadSpin();
			}
			m_nGCBytes += a_pNode->GCGetBytes();
		}

		// GC collection coordination --------------------------------------

		// Thread-Safe IsStopTheWorld sync
		bool IsStopTheWorld()
		{
			return m_bCollectMarkingSignal;
		}

		// Thread-Safe GC tick
		void Tick(int a_nThreadIndex)
		{
			std::uint64_t
			if (a_nThreadIndex == 0) // Thread Zero
			{
				auto GoodAfer = m_nGCPrevTimestamp + std::chrono::milliseconds(100);
				if (m_nGCBytes.load() > m_nGCPrevBytes * 2 && GoodAfer < std::chrono::high_resolution_clock::now()) // Is collection time?
				{
					// Stop the world
					m_nCollectReadyThreads = 1;
					m_nCollectDoneThreads = 1;
					m_bRootsAdded = false;
					for (int i = 0; i < m_nThreads; i++)
						m_pWalkerHeads[i].store(nullptr);
					m_bCollectMarkingSignal = true; // This bit starts marking sync stage
					while (m_nCollectReadyThreads < m_nThreads) Utils::ThreadSpin();

					// Mark roots
					MarkRoots();
					m_bRootsAdded = true;

					// Mark pendings
					MarkPendings(a_nThreadIndex);

					// Wait for all threads to report done, the sweep
					while (m_nCollectDoneThreads < m_nThreads) Utils::ThreadSpin();
					m_bCollectMarkingSignal = false; // This bit re-starts running of code
					Sweep();

					// Update
					m_nGCPrevBytes = m_nGCBytes.load();
					m_nGCPrevTimestamp = std::chrono::high_resolution_clock::now();
				}
			}
			else if (m_bCollectMarkingSignal) // Other thread, time to mark?
			{
				// Stop the world, sync
				m_nCollectReadyThreads++;
				while (m_nCollectReadyThreads < m_nThreads) Utils::ThreadSpin();

				// Mark
				MarkPendings(a_nThreadIndex);
				m_nCollectDoneThreads++;

				// Wait for run signal
				while (m_bCollectMarkingSignal) Utils::ThreadSpin();
			}
		}

		// GC collection procedure --------------------------------------

		// Thread-Safe Stop-The-World Mark'ing from root list
		void MarkRoots()
		{
			// Get parents from root list and process them
			for (TGCNode* pNode = m_pRootHead; pNode != nullptr; pNode = pNode->GCGetWalkerNext())
			{
				pNode->GCSetMarked(true);
				pNode->GCMarkAllPushParents(this, 0);
			}
		}

		// Thread-Safe Stop-The-World Mark'ing
		void MarkPendings(int a_nThreadIndex)
		{
			TGCNode* pNode = MarkPop(a_nThreadIndex);
			while (pNode != nullptr)
			{
				pNode->GCMarkAllPushParents(this, a_nThreadIndex);
				pNode = MarkPop(a_nThreadIndex);
			}
		}

		// Thread-Safe Sweep, this implementation is thread-safe but it always skips first item (most recently added, no big deal)
		void Sweep()
		{
			TGCNode *pPrevNode, *pNode = m_pActiveHead.load();
			if (pNode != nullptr)
			{
				pPrevNode = pNode;
				pNode = pNode->GCGetActiveNext();
				while (pNode != nullptr) 
				{
					if (pNode->GCGetMarked())
					{
						pNode->GCSetMarked(false);
						pNode = pNode->GCGetActiveNext();
					}
					else // Delete
					{
						pPrevNode->GCSetActiveNext(pNode->GCGetActiveNext());
						m_nGCBytes -= pNode->GCGetBytes();
						delete pNode;
						pNode = pPrevNode->GCGetActiveNext();
					}
				}
			}
		}

		// GC thread-safe multi-producer stacks --------------------------------------

		void MarkPush(TGCNode* a_pNode, int a_nThreadIndex)
		{
			auto& pWalkerHead = m_pWalkerHeads[a_nThreadIndex];
			TGCNode* pHead = pWalkerHead.load();
			while (true)
			{
				a_pNode->GCSetWalkerNext(pHead);
				if (pWalkerHead.compare_exchange_strong(pHead, a_pNode)) break;
				Utils::ThreadSpin();
			}
		}

		TGCNode* MarkPop(int a_nThreadIndex)
		{
			TGCNode* pNode = MarkTryPopAny(a_nThreadIndex);
			while (pNode == nullptr && !m_bRootsAdded)
			{
				Utils::ThreadSpin();
				pNode = MarkTryPopAny(a_nThreadIndex);
			}
			if (pNode == nullptr)
				pNode = MarkPopAny(a_nThreadIndex);
			return pNode;
		}

		TGCNode* MarkPopAny(int a_nPreferredThreadIndex)
		{
			TGCNode* pNode = MarkPopAt(a_nPreferredThreadIndex);
			if (pNode == nullptr)
			{
				pNode = MarkPopAt(0);
				if (pNode == nullptr)
				{
					for (int i = a_nPreferredThreadIndex + 1; i < m_nThreads; i++)
					{
						pNode = MarkPopAt(i);
						if (pNode != nullptr)
							return pNode;
					}
					for (int i = 1; i < a_nPreferredThreadIndex; i++)
					{
						pNode = MarkPopAt(i);
						if (pNode != nullptr)
							return pNode;
					}
				}
			}
			return pNode;
		}

		TGCNode* MarkTryPopAny(int a_nPreferredThreadIndex)
		{
			TGCNode* pNode = MarkTryPopAt(a_nPreferredThreadIndex);
			if (pNode == nullptr)
			{
				pNode = MarkTryPopAt(0);
				if (pNode == nullptr)
				{
					for (int i = a_nPreferredThreadIndex + 1; i < m_nThreads; i++)
					{
						pNode = MarkTryPopAt(i);
						if (pNode != nullptr)
							return pNode;
					}
					for (int i = 1; i < a_nPreferredThreadIndex; i++)
					{
						pNode = MarkTryPopAt(i);
						if (pNode != nullptr)
							return pNode;
					}
				}
			}
			return pNode;
		}

		TGCNode* MarkPopAt(int a_nThreadIndex)
		{
			auto& pWalkerHead = m_pWalkerHeads[a_nThreadIndex];
			TGCNode* pNode = pWalkerHead.load();
			while (pNode != nullptr && !pWalkerHead.compare_exchange_strong(pNode, pNode->GCGetWalkerNext()))
				Utils::ThreadSpin();
			return pNode;
		}

		TGCNode* MarkTryPopAt(int a_nThreadIndex)
		{
			auto& pWalkerHead = m_pWalkerHeads[a_nThreadIndex];
			TGCNode* pNode = pWalkerHead.load();
			if (pNode != nullptr && pWalkerHead.compare_exchange_strong(pNode, pNode->GCGetWalkerNext()))
				return pNode;
			return nullptr;
		}
	
	private:
		TGCNode*										m_pRootHead;

		std::atomic<TGCNode*>*							m_pWalkerHeads;

		std::atomic<TGCNode*>							m_pActiveHead;

		std::atomic_uint64_t							m_nGCBytes;
		std::atomic_uint64_t							m_nGCPrevBytes;
		std::chrono::high_resolution_clock::time_point	m_nGCPrevTimestamp;

		int												m_nThreads;
		std::atomic_bool								m_bCollectMarkingSignal;
		std::atomic_int									m_nCollectReadyThreads;
		std::atomic_int									m_nCollectDoneThreads;
		std::atomic_bool								m_bRootsAdded;
	};
