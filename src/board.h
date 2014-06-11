#pragma once

#include <string>
#include <vector>

#include "SgSystem.h"
#include "SgArrayList.h"
#include "SgHash.h"
#include "SgBoardColor.h"
#include "SgMove.h"

#include "YSystem.h"
#include "VectorIterator.h"
#include "WeightedRandom.h"
#include "YUtil.h"
#include "ConstBoard.h"
#include "SemiTable.h"
#include "Groups.h"
#include "YException.h"

#include <boost/scoped_array.hpp>
#include <boost/scoped_ptr.hpp>

class SemiConnection;
class SemiTable;

//----------------------------------------------------------------------

struct LocalMoves
{
    static const float WEIGHT_SAVE_BRIDGE = 1e+5;
    static const float WEIGHT_DEAD_CELL   = 1e-6;
    static const float WEIGHT_WIN_THREAT  = 1e+6;

    std::vector<int> move;
    std::vector<float> gamma;
    float gammaTotal;
    
    void Clear()
    {
        move.clear();
        gamma.clear();
        gammaTotal = 0.0f;
    }

    float Total() const 
    { return gammaTotal; }

    void AddWeight(cell_t p, float w)
    {
        for (size_t i = 0; i < move.size(); ++i) {
            if (move[i] == p) {
                gamma[i] += w;
                gammaTotal += w;
                return;
            }
        }
        move.push_back(p);
        gamma.push_back(w);
        gammaTotal += w;
    }

    SgMove Choose(float random)
    {
        assert(!move.empty());

        gamma.back() += 1.0; // ensure it doesn't go past end

        int i = -1;
        do {
            random -= gamma[++i];
        } while (random > 0.0001f);
        
        gamma.back() -= 1.0;

        return move[i];
    }
};

//---------------------------------------------------------------------------

struct Block
{
    static const int MAX_STONES = Y_MAX_CELL;
    static const int MAX_LIBERTIES = Y_MAX_CELL;

    typedef SgArrayList<cell_t, MAX_LIBERTIES> LibertyList;
    typedef LibertyList::Iterator LibertyIterator;
    typedef SgArrayList<cell_t, MAX_STONES> StoneList;
    typedef StoneList::Iterator StoneIterator;

    cell_t m_anchor;
    int m_border;
    SgBlackWhite m_color;
    LibertyList m_liberties;    
    StoneList m_stones;

    Block()
    { }

    Block(cell_t p, SgBlackWhite color)
        : m_anchor(p)
        , m_color(color)
    { }

    std::string ToString(const ConstBoard& cbrd) const
    {
        std::ostringstream os;
        os << "[color=" <<  m_color
           << " anchor=" << cbrd.ToString(m_anchor)
           << " border=" << m_border
           << "]\n";
        return os.str();
    }
};

struct Board 
{
    struct Statistics
    {
        size_t m_maxSharedLiberties;

        size_t m_numMovesPlayed;
        
        size_t m_numDirtyCellsPerMove;

        Statistics()
        { 
            Clear(); 
        }

        void Clear()
        {
            m_maxSharedLiberties = 0;
            m_numMovesPlayed = 0;
            m_numDirtyCellsPerMove = 0;
        }

        std::string ToString() const
        {
            std::ostringstream os;
            os << '['
               << "max_shared_liberties=" << m_maxSharedLiberties
               << " num_moves_played=" << m_numMovesPlayed
               << " num_dirty_cells_per_move=" << m_numDirtyCellsPerMove
               << ']';
            return os.str();
        }

        static Statistics& Get()
        {
            static Statistics s_stats;
            return s_stats;
        }
    };

    struct EmptyIterator : public MarkedCellsWithList::Iterator
    {
        EmptyIterator(const Board& brd)
            : MarkedCellsWithList::Iterator(brd.m_state.m_emptyCells)
        { };
    };

    struct History
    {
        SgArrayList<SgMove, Y_MAX_CELL> m_move;
        SgArrayList<SgBoardColor, Y_MAX_CELL> m_color;

        int NumMoves() const 
        { 
            return m_move.Length();
        }

        void Clear()
        {
            m_move.Clear();
            m_color.Clear();
        }

        void PushBack(SgBoardColor color, SgMove move)
        {
            m_color.PushBack(color);
            m_move.PushBack(move);
        }

        SgMove LastMove() const
        {
            if (m_move.IsEmpty())
                return SG_NULLMOVE;
            if (m_move.Length() == 1 && m_move.Last() == Y_SWAP)
                return m_move[0];
            return m_move.Last();
        }
    };

    explicit Board(int size);

    const ConstBoard& Const() const { return m_constBrd; }

    void SetPosition(const Board& other);
    void SetSize(int size);
    int Size() const { return Const().Size(); }

    cell_t PointInDir(cell_t cell, int dir) const 
    { return Const().PointInDir(cell, dir); }
    std::string ToString() const;
    static std::string ToString(cell_t p) { return ConstBoard::ToString(p); }

    SgHashCode Hash() const { return m_state.m_hash; };

    //------------------------------------------------------------

    bool IsGameOver() const { return m_state.m_winner != SG_EMPTY; }
    SgBoardColor GetWinner() const { return m_state.m_winner; }
    bool IsWinner(SgBlackWhite player) const 
    { return player == m_state.m_winner; }

    bool HasWinningVC() const   { return m_state.m_vcWinner != SG_EMPTY; }
    SgBoardColor GetVCWinner() const { return m_state.m_vcWinner; }
    bool IsVCWinner(SgBlackWhite player) const
    { return player == m_state.m_vcWinner; }
    cell_t WinningVCStonePlayed() const { return m_state.m_vcStonePlayed; }

    //------------------------------------------------------------

    bool IsOccupied(cell_t cell) const 
    { return m_state.m_color[cell] != SG_EMPTY; }
    
    bool IsEmpty(cell_t cell) const 
    { return m_state.m_color[cell] == SG_EMPTY; }

    bool IsBorder(cell_t cell) const
    { return m_state.m_color[cell] == SG_BORDER; }

    SgBoardColor GetColor(cell_t cell) const
    { return m_state.m_color[cell]; }

    SgBlackWhite ToPlay() const         { return m_state.m_toPlay; }
    void SetToPlay(SgBlackWhite toPlay) { m_state.m_toPlay = toPlay; }
    void FlipToPlay()          { m_state.m_toPlay = SgOppBW(m_state.m_toPlay); }

    void Undo();

    int NumMoves() const { return m_state.m_history.NumMoves(); }

    //------------------------------------------------------------

    void Swap();
    void Play(SgBlackWhite color, cell_t p);

    //------------------------------------------------------------
    // Blocks

    bool IsInBlock(cell_t p, cell_t anchor) const
    { return GetBlock(p)->m_anchor == anchor; }

    bool IsLibertyOfBlock(cell_t p, cell_t anchor) const
    { return GetBlock(anchor)->m_liberties.Contains(p); }

    cell_t BlockAnchor(cell_t p) const 
    { return GetBlock(p)->m_anchor; }

    bool IsBlockAnchor(cell_t p) const
    { return BlockAnchor(p) == p; } 

    std::string BlockInfo(cell_t p) const
    { return GetBlock(p)->ToString(Const()); }

    //------------------------------------------------------------
    // Groups

    std::string GroupStructure(cell_t p) const;

    int GroupBorder(cell_t p) const
    { return GetGroup(BlockAnchor(p))->m_border; }

    std::string GroupInfo(cell_t p) const
    { return GetGroup(BlockAnchor(p))->ToString(); }

    const Group* BlockToGroup(cell_t block) const
    { return GetGroups().GetGroupById(m_state.m_blockToGroup[block]); }

    //------------------------------------------------------------
    // Gui access functions

    std::vector<cell_t> FullConnectedTo(cell_t p, SgBlackWhite c) const;
    std::vector<cell_t> SemiConnectedTo(cell_t p, SgBlackWhite c) const;
    std::vector<cell_t> GetCarrierBetween(cell_t p1, cell_t p2) const;
    std::vector<cell_t> FullConnectsMultipleBlocks(SgBlackWhite c) const;
    const MarkedCells&  GroupCarrier(cell_t p) const;
    const Group::BlockList& GroupBlocks(cell_t p) const;
    std::vector<SemiConnection> GetSemisBetween(cell_t p1, cell_t p2) const;

    // ------------------------------------------------------------

    std::string CellInfo(cell_t p) const
    { return GetCell(p)->ToString(Const()); }

    bool IsCellMarkedDead(cell_t p) const;
    bool DoDeadCellCheck(cell_t p) const;
    void MarkCellAsDead(cell_t p);

    bool IsCellThreat(cell_t p) const;
    void MarkCellNotThreat(cell_t p);
    void MarkCellAsThreat(cell_t p);
    void MarkAllThreats(const MarkedCellsWithList& cells,
                        MarkedCellsWithList& threatInter,
                        MarkedCellsWithList& threatUnion);


    void MarkCellNotDirty(cell_t p);
    void MarkCellDirtyCon(cell_t p); 
    void MarkCellDirtyWeight(cell_t p); 

    const MarkedCellsWithList& GetAllDirtyConCells() const;
    const MarkedCellsWithList& GetAllDirtyWeightCells() const;
    const MarkedCellsWithList& GetAllEmptyCells() const;

    void GroupExpand(cell_t move);

    // Returns SG_NULLMOVE if no savebridge pattern matches, otherwise
    // a move to reestablish the connection.
    int SaveBridge(cell_t lastMove, const SgBlackWhite toPlay, 
                   SgRandom& random) const;

    // Returns SG_NULLMOVE if no savebridge pattern matches last move played
    void GeneralSaveBridge(LocalMoves& local) const;

    float WeightCell(cell_t p) const;

    // ------------------------------------------------------------

    void SetSavePoint1()      { CopyState(m_savePoint1, m_state); }
    void SetSavePoint2()      { CopyState(m_savePoint2, m_state); }
    void RestoreSavePoint1()  { CopyState(m_state, m_savePoint1); }
    void RestoreSavePoint2()  { CopyState(m_state, m_savePoint2); }

    // ------------------------------------------------------------

    void CheckConsistency();
    void DumpBlocks() ;

    std::string EncodeHistory() const;
    int DecodeHistory(const std::string& history, Board::History& out) const;

    const SemiTable& GetSemis() const
    { return *m_state.m_semis.get(); }

    SemiTable& GetSemis()
    { return *m_state.m_semis.get(); }

    const Group* GetGroup(cell_t p) const
    { return m_state.m_groups->GetRootGroup(p); }

private:
    template<typename T, int SIZE>
    class CarrierList : public SgArrayList<T, SIZE>
    {
    public:
        int Size() const  { return SgArrayList<T,SIZE>::Length(); }
        
        void PushBack(T p)
        {
            // TODO: currently discards p if already full.
            // Should probably always keep p and remove something else.
            if (Size() < SIZE) {
                SgArrayList<T,SIZE>::PushBack(p); 
            }
        }

        void Include(T p)
        {
            if (!SgArrayList<T,SIZE>::Contains(p))
                PushBack(p);
        }
    };

    static const int MAX_CARRIER_SIZE = 8;
    typedef CarrierList<cell_t, MAX_CARRIER_SIZE> Carrier;

    struct Cell
    {
        typedef SgArrayList<cell_t, 6> SemiConnectionList;
        typedef SgArrayList<cell_t, 6> FullConnectionList;

        static const int FLAG_DEAD         = 1;
	static const int FLAG_THREAT       = 2;

	SemiConnectionList m_SemiConnects[2];
	FullConnectionList m_FullConnects[2];
	int m_NumAdj[3];
        int m_flags;

	Cell()
            : m_flags(0)
	{
	    memset(m_NumAdj, 0, sizeof(m_NumAdj));
        }

        void AddBorderConnection(const Block* b)
        {
            m_FullConnects[SG_BLACK].Include(b->m_anchor);
            m_FullConnects[SG_WHITE].Include(b->m_anchor);
        }

        void AddFull(const Block* b, SgBlackWhite color)
        {   
            m_FullConnects[color].Include(b->m_anchor); 
        }

	void AddSemi(const Block* b, SgBlackWhite color)
	{  
            m_SemiConnects[color].Include(b->m_anchor); 
        }

	bool RemoveSemiConnection(const Block* b, SgBlackWhite color) 
        {  
            return m_SemiConnects[color].Exclude(b->m_anchor); 
        }

	bool RemoveFullConnection(const Block* b, SgBlackWhite color) 
	{  
            return m_FullConnects[color].Exclude(b->m_anchor); 
        }

	bool IsSemiConnected(const Block* b, SgBlackWhite color ) const
	{  return m_SemiConnects[color].Contains(b->m_anchor); }

	bool IsFullConnected(const Block* b, SgBlackWhite color) const
	{  return m_FullConnects[color].Contains(b->m_anchor); }

        int Flags() const { return m_flags; }
        void SetFlags(int f) { m_flags |= f; }
        void ClearFlags(int f) { m_flags &= ~f; }
        bool IsDead() const { return m_flags & FLAG_DEAD; }
	bool IsThreat() const { return m_flags & FLAG_THREAT; }

	std::string ToString(const ConstBoard& cbrd) const
        {
            SG_UNUSED(cbrd);
            std::ostringstream os;
            os << "[Empty=" << this->m_NumAdj[SG_EMPTY]
               << " Black=" << this->m_NumAdj[SG_BLACK]
               << " White=" << this->m_NumAdj[SG_WHITE]
               << " Flags=" << this->m_flags
               << ((this->m_flags & FLAG_DEAD) ? "(dead)" : "")
               << ((this->m_flags & FLAG_THREAT) ? "(threat)" : "")
               << "]";
            // FullConnectionList& fulls = m_FullConnects[from->m_color];
            // for (int i = 0; i < fulls.Length(); ++i) {
            //     if (i) os << ',';
            //     os << (int)fulls[i];
            // }
            // os << ']';
            // SemiConnectionList& semis = m_SemiConnects[from->m_color];
            // for (int i = 0; i < fulls.Length(); ++i) {
            //     if (i) os << ',';
            //     os << (int)fulls[i];
            // }
            return os.str();
        }
    };

    ConstBoard m_constBrd;

    struct State 
    {
        boost::scoped_array<SgBoardColor> m_color;
	boost::scoped_array<Cell> m_cellList;
        boost::scoped_array<cell_t> m_blockIndex;
        boost::scoped_array<Block> m_blockList;
        boost::scoped_array<Carrier> m_conData;
        boost::scoped_array<Carrier*> m_con;
        boost::scoped_ptr<SemiTable> m_semis;
        boost::scoped_ptr<Groups> m_groups;
        boost::scoped_array<cell_t> m_blockToGroup;

        SgArrayList<cell_t, 3> m_oppBlocks;

        SgHashCode m_hash;
        SgBlackWhite m_toPlay;
        History m_history;

	MarkedCellsWithList m_emptyCells;

        SgBoardColor m_winner;
        SgBoardColor m_vcWinner;
        cell_t       m_vcStonePlayed;

        State() { };
        State(int T);
        void Init(int T);
        void CopyState(const State& other);
    };

    State m_state;
    State m_savePoint1;
    State m_savePoint2;

    MarkedCellsWithList m_dirtyConCells;
    MarkedCellsWithList m_dirtyWeightCells;
    MarkedCellsWithList m_dirtyBlocks;

    void CreateSingleStoneBlock(cell_t p, SgBlackWhite color, int border);

    void AddLibertyToBlock(Block* block, cell_t c);

    bool IsAdjacent(cell_t p, const Block* b);

    void AddStoneToBlock(cell_t p, int border, Block* b);

    void MergeBlocks(cell_t p, int border, SgArrayList<cell_t, 3>& adjBlocks);

    int GetCarrierIndex(Block* b1, Block* b2) const;

    void AddSharedLibertiesAroundPoint(Block* b1, cell_t p, cell_t skip);

    void MergeSharedLiberty(const Block* b1, Block* b2);

    static int NumUnmarkedSharedLiberties(const Carrier& lib, int* seen, 
                                          int id, Carrier& unmarked);

    void RemoveSharedLiberty(cell_t p, Block* a, Block* b);

    void RemoveSharedLiberty(cell_t p, SgArrayList<cell_t, 3>& adjBlocks);

    void UpdateBlockConnection(Block* a, Block* b); 

    void ConstructSemisWithKey(cell_t key, SgBlackWhite color);

    void RemoveConnectionWith(Block* b, const Block* other);

    void MergeBlockConnections(const Block* b1, Block* b2);

    void RemoveEdgeConnections(Block* b, int new_borders);

    void CopyState(Board::State& a, const Board::State& b);

    SgMove MaintainConnection(cell_t b1, cell_t b2) const;

    cell_t BlockIndex(cell_t p) const
    { return m_state.m_blockIndex[p]; }

    Block* GetBlock(cell_t p) 
    { return &m_state.m_blockList[BlockIndex(p)]; }

    const Block* GetBlock(cell_t p) const
    { return &m_state.m_blockList[BlockIndex(p)]; }

    Group* GetGroup(cell_t p)
    { return m_state.m_groups->GetRootGroup(p); }

    Cell* GetCell(cell_t p)
    { return &m_state.m_cellList[p]; }

    const Cell* GetCell(cell_t p) const
    { return &m_state.m_cellList[p]; }

    int NumNeighbours(cell_t p, SgBlackWhite color) const
    { return GetCell(p)->m_NumAdj[color]; }

    Carrier& GetConnection(cell_t p1, cell_t p2);

    const Carrier& GetConnection(cell_t p1, cell_t p2) const;

    const Groups& GetGroups() const
    { return *m_state.m_groups.get(); }

    Groups& GetGroups()
    { return *m_state.m_groups.get(); }

    void AddCellToConnection(cell_t p1, cell_t p2, cell_t cell)
    {  GetConnection(p1, p2).Include(cell);  }

    bool RemoveCellFromConnection(cell_t p1, cell_t p2, cell_t cell)
    {  return GetConnection(p1, p2).Exclude(cell);  }

    void UpdateConnectionsToNewAnchor(const Block* from, const Block* to,
                                      const bool* toLiberties);
    void PromoteConnectionType(cell_t p, const Block* b, SgBlackWhite color);
    void DemoteConnectionType(cell_t p, Block* b, SgBlackWhite color);

    //------------------------------------------------------------
    
    SgHashCode HashForBoardsize(int size) const
    {
        return SgHashZobrist<64>::GetTable().
            Get(SgHashZobrist<64>::MAX_HASH_INDEX - 1 - size);
    }

    SgHashCode HashForCell(cell_t cell, SgBlackWhite color) const
    {
        return SgHashZobrist<64>::GetTable().
            Get((color == SG_WHITE ? 0 : 500) + static_cast<int>(cell));
    }

    //------------------------------------------------------------

    Board(const Board& other);          // not implemented
    void operator=(const Board& other); // not implemented
};

inline void Board::MarkCellAsDead(cell_t p) 
{ 
    GetCell(p)->SetFlags(Cell::FLAG_DEAD);
}

inline bool Board::IsCellThreat(cell_t p) const
{
    return GetCell(p)->IsThreat();
}

inline void Board::MarkCellNotThreat(cell_t p)
{
    GetCell(p)->ClearFlags(Cell::FLAG_THREAT);
}

inline void Board::MarkCellAsThreat(cell_t p)
{
    GetCell(p)->SetFlags(Cell::FLAG_THREAT);
}

inline void Board::MarkCellDirtyCon(cell_t p) 
{
    m_dirtyConCells.Mark(p);
}

inline void Board::MarkCellDirtyWeight(cell_t p) 
{
    m_dirtyWeightCells.Mark(p);
}

inline const MarkedCellsWithList& Board::GetAllDirtyConCells() const
{
    return m_dirtyConCells;
}

inline const MarkedCellsWithList& Board::GetAllDirtyWeightCells() const
{
    return m_dirtyWeightCells;
}

inline const MarkedCellsWithList& Board::GetAllEmptyCells() const
{
    return m_state.m_emptyCells;
}

inline Board::Carrier& Board::GetConnection(cell_t p1, cell_t p2)
{
    if (p1 > p2) std::swap(p1,p2);
    return m_state.m_con[p1][p2-p1];
}

inline const Board::Carrier& Board::GetConnection(cell_t p1, cell_t p2) const
{
    if (p1 > p2) std::swap(p1,p2);
    return m_state.m_con[p1][p2-p1];
}

inline const MarkedCells& Board::GroupCarrier(cell_t p) const
{
    return GetGroup(BlockAnchor(p))->m_carrier;
}

//----------------------------------------------------------------------

class CellIterator : public VectorIterator<cell_t>
{
public:
    CellIterator(const Board& brd)
        : VectorIterator<cell_t>(brd.Const().m_cells)
    { }

    CellIterator(const ConstBoard& brd)
        : VectorIterator<cell_t>(brd.m_cells)
    { }
};

class CellAndEdgeIterator : public VectorIterator<cell_t>
{
public:
    CellAndEdgeIterator(const Board& brd)
        : VectorIterator<cell_t>(brd.Const().m_cells_edges)
    { }

    CellAndEdgeIterator(const ConstBoard& brd)
        : VectorIterator<cell_t>(brd.m_cells_edges)
    { }
};

//----------------------------------------------------------------------

