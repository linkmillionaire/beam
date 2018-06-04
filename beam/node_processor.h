#pragma once

#include <boost/intrusive/set.hpp>
#include "../core/common.h"
#include "../core/storage.h"
#include "node_db.h"

namespace beam {

class NodeProcessor
{
	NodeDB m_DB;
	UtxoTree m_Utxos;
	RadixHashOnlyTree m_Kernels;

	struct DbType {
		static const uint8_t Utxo	= 0;
		static const uint8_t Kernel	= 1;
	};

	void TryGoUp();

	bool GoForward(uint64_t);
	void Rollback();
	void PruneOld();
	void DereferenceFossilBlock(uint64_t);

	struct RollbackData;

	bool HandleBlock(const NodeDB::StateID&, bool bFwd);
	bool HandleValidatedTx(const TxBase&, Height, bool bFwd, RollbackData&);

	bool HandleBlockElement(const Input&, bool bFwd, Height, RollbackData&);
	bool HandleBlockElement(const Output&, Height, bool bFwd);
	bool HandleBlockElement(const TxKernel&, bool bFwd, bool bIsInput);

	void InitCursor();
	void OnCorrupted();
	void get_Definition(Merkle::Hash&, const Merkle::Hash& hvHist);

	bool IsRelevantHeight(Height);
	uint8_t get_NextDifficulty();
	Timestamp get_MovingMedian();

	struct UtxoSig;
	struct UnspentWalker;

public:

	typedef NodeDB::PeerID PeerID;

	void Initialize(const char* szPath);

	struct Horizon {

		Height m_Branching; // branches behind this are pruned
		Height m_Schwarzschild; // original blocks begind this are erased

		Horizon(); // by default both are disabled.

	} m_Horizon;

	struct Cursor
	{
		// frequently used data
		NodeDB::StateID m_Sid;
		Block::SystemState::ID m_ID;
		Block::SystemState::Full m_Full;
		Merkle::Hash m_History;
		Merkle::Hash m_HistoryNext;

	} m_Cursor;

	void get_CurrentLive(Merkle::Hash&);

	void ExportMacroBlock(Block::Body&); // can be time-consuming
	bool ImportMacroBlock(const Block::SystemState::ID&, const Block::Body&);

	struct DataStatus {
		enum Enum {
			Accepted,
			Rejected, // duplicated or irrelevant
			Invalid
		};
	};

	DataStatus::Enum OnState(const Block::SystemState::Full&, bool bIgnorePoW, const PeerID&);
	DataStatus::Enum OnBlock(const Block::SystemState::ID&, const NodeDB::Blob& block, const PeerID&);

	// use only for data retrieval for peers
	NodeDB& get_DB() { return m_DB; }
	UtxoTree& get_Utxos() { return m_Utxos; }
	RadixHashOnlyTree& get_Kernels() { return m_Kernels; }

	void EnumCongestions();

	virtual void RequestData(const Block::SystemState::ID&, bool bBlock, const PeerID* pPreferredPeer) {}
	virtual void OnPeerInsane(const PeerID&) {}
	virtual void OnNewState() {}
	virtual bool VerifyBlock(const Block::Body&, Height h0, Height h1);

	bool IsStateNeeded(const Block::SystemState::ID&);

	ECC::Kdf m_Kdf;

	struct TxPool
	{
		struct Element
		{
			struct Tx
				:public boost::intrusive::set_base_hook<>
			{
				Transaction::Ptr m_pValue;

				bool operator < (const Tx& t) const;
				IMPLEMENT_GET_PARENT_OBJ(Element, m_Tx)
			} m_Tx;

			struct Profit
				:public boost::intrusive::set_base_hook<>
			{
				Amount m_Fee;
				size_t m_nSize;

				bool operator < (const Profit& t) const;

				IMPLEMENT_GET_PARENT_OBJ(Element, m_Profit)
			} m_Profit;

			struct Threshold
				:public boost::intrusive::set_base_hook<>
			{
				Height m_Value;

				bool operator < (const Threshold& t) const { return m_Value < t.m_Value; }

				IMPLEMENT_GET_PARENT_OBJ(Element, m_Threshold)
			} m_Threshold;
		};

		typedef boost::intrusive::multiset<Element::Tx> TxSet;
		typedef boost::intrusive::multiset<Element::Profit> ProfitSet;
		typedef boost::intrusive::multiset<Element::Threshold> ThresholdSet;

		TxSet m_setTxs;
		ProfitSet m_setProfit;
		ThresholdSet m_setThreshold;

		bool AddTx(Transaction::Ptr&&, Height); // return false if transaction doesn't pass context-free validation
		void Delete(Element&);
		void Clear();

		void DeleteOutOfBound(Height);
		void ShrinkUpTo(uint32_t nCount);

		~TxPool() { Clear(); }

	};

	bool GenerateNewBlock(TxPool&, Block::SystemState::Full&, ByteBuffer&, Amount& fees, Block::Body& blockInOut);
	bool GenerateNewBlock(TxPool&, Block::SystemState::Full&, ByteBuffer&, Amount& fees);

private:
	bool GenerateNewBlock(TxPool&, Block::SystemState::Full&, Block::Body& block, Amount& fees, Height, RollbackData&);
	bool GenerateNewBlock(TxPool&, Block::SystemState::Full&, ByteBuffer&, Amount& fees, Block::Body&, bool bInitiallyEmpty);
};



} // namespace beam
