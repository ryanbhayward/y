#ifndef BOARDY_H
#define BOARDY_H

#include <string>
#include <vector>
#include <bitset>

#include "SgSystem.h"
#include "SgArrayList.h"
#include "SgHash.h"
#include "SgBoardColor.h"
#include "SgMove.h"
#include "VectorIterator.h"
#include "WeightedRandom.h"

#include <boost/scoped_array.hpp>

//---------------------------------------------------------------------------

template<typename T>
bool Contains(const std::vector<T>& v, const T& val)
{
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i] == val)
            return true;
    return false;
}

template<typename T, int SIZE>
void Append(std::vector<T>& v, const SgArrayList<T,SIZE>& a)
{
    for (int i = 0; i < a.Length(); ++i) {
        if (!Contains(v, a[i]))
            v.push_back(a[i]);
    }
}

template<typename T>
void Include(std::vector<T>& v, const T& val)
{
    if (!Contains(v, val))
	v.push_back(val);
}

//---------------------------------------------------------------------------

//           WEST, EAST, SOUTH
//                 0  *
//                1  * *  
//               2  * * *   
//              3  * * * *   
//             4  * * * * *  
//            5  * * * * * *  
//           6  * * * * * * *   
//          7  * * * * * * * * 
//
// T = S*(S+1)/2 + 3


static const int Y_MAX_CELL = 128;
static const int Y_MAX_SIZE = 15;   // such that TotalCells < Y_MAX_CELL

typedef uint8_t cell_t;

static const int Y_SWAP = -2;  // SG_NULLMOVE == -1

struct MarkedCellsWithList
{
    typedef SgArrayList<cell_t, Y_MAX_CELL> ListType;
    typedef ListType::Iterator Iterator;

    cell_t m_marked[Y_MAX_CELL];
    SgArrayList<cell_t, Y_MAX_CELL> m_list;

    MarkedCellsWithList()
    {
        Clear();
    }

    void Clear()
    {
        memset(m_marked, 0, sizeof(m_marked));
        m_list.Clear();
    }

    bool Mark(cell_t p)
    {
        if (!m_marked[p]) {
            m_marked[p] = true;
            m_list.PushBack(p);
            return true;
        }
        return false;
    }

    void Unmark(cell_t p) 
    {
        if (m_marked[p]) {
            m_marked[p] = false;
            m_list.Exclude(p);
        }
    }
};

 
class ConstBoard 
{
public:

    static const int WEST = 0;
    static const int EAST = 1;
    static const int SOUTH = 2;

    static const int DIR_NW = 0;
    static const int DIR_NE = 1;
    static const int DIR_E  = 2;
    static const int DIR_SE = 3;
    static const int DIR_SW = 4;
    static const int DIR_W  = 5;

    static char ColorToChar(SgBoardColor color);
    static std::string ColorToString(SgBoardColor color);

    static inline int board_row(cell_t p)
    {  
        p -= 3;
        int r = 1;
        while((r)*(r+1)/2 <= p)
            ++r;
        return r - 1;
    }

    static inline int board_col(cell_t p)
    {
        int r = board_row(p);
        return p - r*(r+1)/2 - 3;
    }

    static inline cell_t fatten(int r, int c)
    { return (r)*(r+1)/2 + c + 3; }

    static std::string ToString(cell_t cell);

    static cell_t FromString(const std::string& name);

    int m_size;
    int TotalCells;

    ConstBoard();
    ConstBoard(int size);

    int Size() const { return m_size; }
    bool IsOnBoard(cell_t cell) const
    {
        return std::find(m_cells.begin(), m_cells.end(), cell) != m_cells.end();
    }

    cell_t PointInDir(cell_t cell, int dir) const
    { return m_cell_nbr[cell][dir]; }

private:
    std::vector<cell_t> m_cells;
    std::vector<cell_t> m_cells_edges;
    std::vector<std::vector<cell_t> > m_cell_nbr;

    friend class Board;
    friend class CellIterator;
    friend class CellAndEdgeIterator;
    friend class CellNbrIterator;
};


//----------------------------------------------------------------------

class CellNbrIterator
{
public:
    CellNbrIterator(const ConstBoard& cbrd, cell_t p)
        : m_cbrd(cbrd)
        , m_point(p)
        , m_index(0)
    { }
    
    /** Advance the state of the iteration to the next liberty. */
    void operator++()
    { ++m_index; }
    
    /** Return the current liberty. */
    cell_t operator*() const
    { return m_cbrd.m_cell_nbr[m_point][m_index]; }
    
    /** Return true if iteration is valid, otherwise false. */
    operator bool() const
    { return m_index < 6; }
    
private:
    const ConstBoard& m_cbrd;
    cell_t m_point;
    cell_t m_index;
};

//----------------------------------------------------------------------

struct LocalMoves
{
    static const float WEIGHT_SAVE_BRIDGE = 1000000.0;
    static const float WEIGHT_DEAD_CELL   = 1e-6;

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

    explicit Board(int size);

    const ConstBoard& Const() const { return m_constBrd; }

    void SetPosition(const Board& other);
    void SetSize(int size);
    int Size() const { return Const().Size(); }

    cell_t PointInDir(cell_t cell, int dir) const 
    { return Const().PointInDir(cell, dir); }
    std::string ToString();
    static std::string ToString(cell_t p) { return ConstBoard::ToString(p); }

    SgHashCode Hash() const { return 1; /* FIXME: IMPLEMENT HASH! */ };

    //------------------------------------------------------------

    bool IsGameOver() const { return m_state.m_winner != SG_EMPTY; }
    SgBoardColor GetWinner() const { return m_state.m_winner; }
    bool IsWinner(SgBlackWhite player) const 
    { return player == m_state.m_winner; }

    bool HasWinningVC() const   { return m_state.m_vcWinner != SG_EMPTY; }
    SgBoardColor GetVCWinner() const { return m_state.m_vcWinner; }
    bool IsVCWinner(SgBlackWhite player) const
    { return player == m_state.m_vcWinner; }
    cell_t WinningVCGroup() const { return m_state.m_vcGroupAnchor; }

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

    void RemoveStone(cell_t lcn);
    cell_t LastMove() const { return m_state.m_lastMove; }
    void SetLastMove(cell_t lcn) { m_state.m_lastMove = lcn; }

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

    cell_t GroupAnchor(cell_t p) const
    { return GetGroup(BlockAnchor(p))->m_anchor; }

    int GroupBorder(cell_t p) const
    { return GetGroup(BlockAnchor(p))->m_border; }

    //------------------------------------------------------------
    // Gui access functions

    std::vector<cell_t> FullConnectedTo(cell_t p, SgBlackWhite c) const;
    std::vector<cell_t> SemiConnectedTo(cell_t p, SgBlackWhite c) const;
    std::vector<cell_t> GetCarrierBetween(cell_t p1, cell_t p2) const;
    std::vector<cell_t> GetBlocksInGroup(cell_t p) const;
    std::vector<cell_t> GroupCarrier(cell_t p) const;
    std::vector<cell_t> FullConnectsMultipleBlocks(SgBlackWhite c) const;

    // ------------------------------------------------------------

    std::string CellInfo(cell_t p) const
    { return GetCell(p)->ToString(Const()); }

    bool IsCellDead(cell_t p) const;
    void MarkCellAsDead(cell_t p);

    void MarkCellNotDirty(cell_t p);
    void MarkCellDirty(cell_t p); 
    const MarkedCellsWithList& GetAllDirtyCells() const;

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

private:
    static const int BORDER_NONE  = 0; // 000   border values, used bitwise
    static const int BORDER_SOUTH = 1; // 001
    static const int BORDER_WEST  = 2; // 010
    static const int BORDER_EAST  = 4; // 100
    static const int BORDER_ALL   = 7; // 111

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

    struct Block
    {
        static const int MAX_STONES = Y_MAX_CELL;
        static const int MAX_LIBERTIES = Y_MAX_CELL;
        static const int MAX_CONNECTIONS = Y_MAX_CELL / 2;

        typedef SgArrayList<cell_t, MAX_LIBERTIES> LibertyList;
        typedef LibertyList::Iterator LibertyIterator;
        typedef SgArrayList<cell_t, MAX_STONES> StoneList;
        typedef StoneList::Iterator StoneIterator;
        typedef SgArrayList<cell_t, MAX_CONNECTIONS> ConnectionList;
        typedef ConnectionList::Iterator ConnectionIterator;

        cell_t m_anchor;
        int m_border;
        SgBlackWhite m_color;
        LibertyList m_liberties;    
        StoneList m_stones;
        ConnectionList m_con;

        Block()
        { }

        Block(cell_t p, SgBlackWhite color)
            : m_anchor(p)
            , m_color(color)
        { }

	Block* Smaller(Block* b1, Block* b2)
	{ return b1->m_anchor < b2->m_anchor ? b1 : b2; }

        int GetConnectionIndex(const Block* b2) const
        {
            for(int i = 0; i != m_con.Length(); ++i)
                if (m_con[i] == b2->m_anchor)
                    return i;
            return -1;
        }

        std::string BlockConnectionsToString(const ConstBoard& cbrd) const
        {
            std::ostringstream os;
            os << "[";
            for (int i = 0; i < m_con.Length(); ++i) {
                if (i) os << ",";
                os << "[" << cbrd.ToString(m_con[i]) << "]";
            }
            os << "]";
            return os.str();
        }
        std::string ToString(const ConstBoard& cbrd) const
        {
            std::ostringstream os;
            os << "[color=" <<  m_color
               << " anchor=" << cbrd.ToString(m_anchor)
               << " border=" << m_border
               << " connected=" << BlockConnectionsToString(cbrd)
               << "]\n";
            return os.str();
        }
    };

    struct Group
    {
	static const size_t MAX_BLOCKS = Y_MAX_CELL/2;

	cell_t m_anchor;
	int m_border;
	SgArrayList<cell_t, MAX_BLOCKS> m_blocks;

	Group()
	{ }

	Group(cell_t anchor, cell_t border)
	    : m_anchor(anchor)
	    , m_border(border)
	    , m_blocks(anchor)
	{ }
    };

    struct Cell
    {
        typedef SgArrayList<cell_t, 6> SemiConnectionList;
        typedef SgArrayList<cell_t, 6> FullConnectionList;

        static const int FLAG_DIRTY = 1;
        static const int FLAG_DEAD  = 2;

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
        bool IsDirty() const { return m_flags & FLAG_DIRTY; } 
        bool IsDead() const { return m_flags & FLAG_DEAD; }

	std::string ToString(const ConstBoard& cbrd) const
        {
            SG_UNUSED(cbrd);
            std::ostringstream os;
            os << "[Empty=" << this->m_NumAdj[SG_EMPTY]
               << " Black=" << this->m_NumAdj[SG_BLACK]
               << " White=" << this->m_NumAdj[SG_WHITE]
               << " Flags=" << this->m_flags
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
	boost::scoped_array<cell_t> m_groupIndex;
	boost::scoped_array<Group> m_groupList;
        boost::scoped_array<Carrier> m_conData;
        boost::scoped_array<Carrier*> m_con;

        SgArrayList<cell_t, 3> m_oppBlocks;

        SgBlackWhite m_toPlay;
        SgBoardColor m_winner;
        SgBoardColor m_vcWinner;
        cell_t       m_vcGroupAnchor;
        cell_t       m_lastMove;

        State() { };
        State(int T);
        void Init(int T);
        void CopyState(const State& other);
    };

    State m_state;
    State m_savePoint1;
    State m_savePoint2;

    MarkedCellsWithList m_dirtyCells;

    void CreateSingleStoneBlock(cell_t p, SgBlackWhite color, int border);

    bool IsAdjacent(cell_t p, const Block* b);

    void AddStoneToBlock(cell_t p, int border, Block* b);

    void MergeBlocks(cell_t p, int border, SgArrayList<cell_t, 3>& adjBlocks);

    int GetCarrierIndex(Block* b1, Block* b2) const;

    void AddSharedLiberty(Block* b1, Block* b2);

    void AddSharedLibertiesAroundPoint(Block* b1, cell_t p, cell_t skip);

    void MergeSharedLiberty(const Block* b1, Block* b2);

    static int
        NumUnmarkedSharedLiberties(const Carrier& lib, 
                                   int* seen, int id);

    static void MarkLibertiesAsSeen(const Carrier& lib, 
                                    int* seen, int id);

    void ResetBlocksInGroup(Block* b);

    void ComputeGroupForBlock(Block* b);

    void ComputeGroupForBlock(Block* b, int* seen, int id);

    void GroupSearch(int* seen, Block* b, int id);

    void AddNonGroupEdges(int* seen, Group* g, int id);

    void RemoveSharedLiberty(cell_t p, Block* a, Block* b);

    void RemoveSharedLiberty(cell_t p, SgArrayList<cell_t, 3>& adjBlocks);

    void UpdateBlockConnection(Block* a, Block* b); 

    void RemoveConnectionWith(Block* b, const Block* other);

    void MergeBlockConnections(const Block* b1, Block* b2);

    void RemoveEdgeSharedLiberties(Block* b);

    void CopyState(Board::State& a, const Board::State& b);

    SgMove MaintainConnection(cell_t b1, cell_t b2) const;

    cell_t BlockIndex(cell_t p) const
    { return m_state.m_blockIndex[p]; }

    Block* GetBlock(cell_t p) 
    { return &m_state.m_blockList[BlockIndex(p)]; }

    const Block* GetBlock(cell_t p) const
    { return &m_state.m_blockList[BlockIndex(p)]; }

    Group* GetGroup(cell_t p)
    { return &m_state.m_groupList[m_state.m_groupIndex[p]]; }

    const Group* GetGroup(cell_t p) const
    { return &m_state.m_groupList[m_state.m_groupIndex[p]]; }

    Cell* GetCell(cell_t p)
    { return &m_state.m_cellList[p]; }

    const Cell* GetCell(cell_t p) const
    { return &m_state.m_cellList[p]; }

    int NumNeighbours(cell_t p, SgBlackWhite color) const
    { return GetCell(p)->m_NumAdj[color]; }

    Carrier& GetConnection(cell_t p1, cell_t p2);

    const Carrier& GetConnection(cell_t p1, cell_t p2) const;

    void AddCellToConnection(cell_t p1, cell_t p2, cell_t cell)
    {  GetConnection(p1, p2).Include(cell);  }

    bool RemoveCellFromConnection(cell_t p1, cell_t p2, cell_t cell)
    {  return GetConnection(p1, p2).Exclude(cell);  }

    void UpdateConnectionsToNewAnchor(const Block* from, const Block* to,
                                      const bool* toLiberties);
    void PromoteConnectionType(cell_t p, const Block* b, SgBlackWhite color);
    void DemoteConnectionType(cell_t p, Block* b, SgBlackWhite color);

    Board(const Board& other);          // not implemented
    void operator=(const Board& other); // not implemented
};

inline void Board::MarkCellAsDead(cell_t p) 
{ 
    GetCell(p)->SetFlags(Cell::FLAG_DEAD);
}

inline void Board::MarkCellNotDirty(cell_t p)
{
    GetCell(p)->ClearFlags(Cell::FLAG_DIRTY);
    //m_dirtyCells.Unmark(p);
    // NOTE: no need to call unmark since Board::Play() clears it 
}

inline void Board::MarkCellDirty(cell_t p) 
{
    if (m_dirtyCells.Mark(p)) {
        GetCell(p)->SetFlags(Cell::FLAG_DIRTY);
    }
}

inline const MarkedCellsWithList& Board::GetAllDirtyCells() const
{
    return m_dirtyCells;
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

#endif
