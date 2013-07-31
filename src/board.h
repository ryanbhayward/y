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


//                 0  g
//                1  g g  
//               2  g * g   
//              3  g * * g   
//             4  g * * * g  
//            5  g * * * * g  
//           6  g * * * * * g   
//          7  g g g g g g g g 
//
// T = S*(S+1)/2 + 3
// first guard of row n = n(n+1)/2


static const int Y_MAX_CELL = 192;
static const int Y_MAX_SIZE = 15;   // such that TotalCells < Y_MAX_CELL

typedef uint8_t cell_t;

static const int Y_SWAP = -2;  // SG_NULLMOVE == -1
 
class ConstBoard 
{
public:

    static const int GUARDS = 1; // must be at least one

    int m_size;
    int TotalCells;
    int TotalGBCells;

    int WEST;
    int EAST;
    int SOUTH;

    ConstBoard();
    ConstBoard(int size);

    static char ColorToChar(SgBoardColor color);
    static std::string ColorToString(SgBoardColor color);

    inline cell_t fatten(int r, int c) const 
    { return (r+GUARDS+1)*(r+GUARDS+2)/2 + c + GUARDS; }
    
    inline int board_row(cell_t p) const 
    {  
        int r = 2;
        while((r+1)*(r+2)/2 < p)
            ++r;
        return r - 2;
    }

    inline int board_col(cell_t p) const 
    {
        int r = board_row(p) + 2;
        return p - r*(r+1)/2 - GUARDS;
    }

    int Size() const { return m_size; }

    bool IsOnBoard(cell_t cell) const
    {
        return std::find(m_cells.begin(), m_cells.end(), cell) != m_cells.end();
    }

    std::string ToString(cell_t cell) const;
    cell_t FromString(const std::string& name) const;

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


struct Board 
{
    struct Statistics
    {
        size_t m_maxSharedLiberties;

        Statistics()
        { 
            Clear(); 
        }

        void Clear()
        {
            m_maxSharedLiberties = 0;
        }

        std::string ToString() const
        {
            std::ostringstream os;
            os << '['
               << "max_shared_liberties=" << m_maxSharedLiberties
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

    void SetSize(int size);
    int Size() const { return Const().Size(); }

    void SetPosition(const Board& other);

    std::string ToString() const;
    std::string ToString(cell_t p) const { return Const().ToString(p); }
    std::string BorderToString() const;
    std::string AnchorsToString() const;
    std::string ActiveBlocksToString(SgBlackWhite color) const;

    SgHashCode Hash() const { return 1; /* FIXME: IMPLEMENT HASH! */ };

    bool IsGameOver() const { return m_state.m_winner != SG_EMPTY; }
    SgBoardColor GetWinner() const { return m_state.m_winner; }
    bool IsWinner(SgBlackWhite player) const 
    { return player == m_state.m_winner; }

    bool HasWinningVC() const   { return m_state.m_vcWinner != SG_EMPTY; }
    SgBoardColor GetVCWinner() const { return m_state.m_vcWinner; }
    bool IsVCWinner(SgBlackWhite player) const
    { return player == m_state.m_vcWinner; }
    cell_t WinningVCGroup() const { return m_state.m_vcGroupAnchor; }

    std::vector<cell_t> GroupCarrier(cell_t p) const
    {
        std::vector<cell_t> ret;
        const Group* g = GetGroup(GroupAnchor(p));
        for (int i = 0; i < g->m_blocks.Length(); ++i) {
            for (int j = i + 1; j < g->m_blocks.Length(); ++j) {
                const SharedLiberties& sl 
                    = GetSharedLiberties(g->m_blocks[i], g->m_blocks[j]);
                for (size_t k = 0; k < sl.Size(); ++k) {
                    if (!Contains(ret, sl.m_liberties[k]))
                        ret.push_back(sl.m_liberties[k]);
                }
            }
        }
        return ret;
    }

    void Swap();

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
    void SetLastMove(cell_t lcn) { m_state.m_lastMove = lcn; } // used after undo
    void Play(SgBlackWhite color, cell_t p);

    // Returns SG_NULLMOVE if no savebridge pattern matches, otherwise
    // a move to reestablish the connection.
    int SaveBridge(cell_t lastMove, const SgBlackWhite toPlay, 
                   SgRandom& random) const;

    SgMove MaintainConnection(cell_t b1, cell_t b2) const;
    // Returns SG_NULLMOVE if no savebridge pattern matches last move played
    void GeneralSaveBridge(LocalMoves& local) const;

    bool IsCellDead(cell_t p) const;

    cell_t BlockAnchor(cell_t p) const 
    { return m_state.m_block[p]->m_anchor; }

    bool IsBlockAnchor(cell_t p) const
    { return BlockAnchor(p) == p; } 

    cell_t GroupAnchor(cell_t p) const
    { return GetGroup(BlockAnchor(p))->m_anchor; }

    int GroupBorder(cell_t p) const
    { return GetGroup(BlockAnchor(p))->m_border; }

    bool IsInBlock(cell_t p, cell_t anchor) const
    { return m_state.m_block[p]->m_anchor == anchor; }

    bool IsLibertyOfBlock(cell_t p, cell_t anchor) const
    { return m_state.m_block[anchor]->m_liberties.Contains(p); }

    bool IsSharedLiberty(cell_t p1, cell_t p2, cell_t p) const
    { return GetSharedLiberties(p1, p2).Contains(p); }

    std::vector<cell_t> GetLibertiesWith(cell_t p1) const
    {
        std::vector<cell_t> ret;
        for (size_t i = 0; i < m_state.m_block[p1]->m_shared.size(); ++i)
            ret.push_back(m_state.m_block[p1]->m_shared[i].m_other);
        return ret;
    }

    std::vector<cell_t> GetBlocksInGroup(cell_t p) const
    {
	std::vector<cell_t> ret;
        const Group* g = GetGroup(BlockAnchor(p));
        for (int i = 0; i < g->m_blocks.Length(); ++i) {
            ret.push_back(g->m_blocks[i]);
            if (g->m_blocks[i] == g->m_anchor)
                std::swap(ret.back(), ret[0]);
        }
	return ret;
    }

    std::string BlockInfo(cell_t p) const
    { return GetBlock(p)->ToString(Const()); }

    std::string CellInfo(cell_t p) const
    { return GetCell(p)->ToString(Const()); }

    std::vector<cell_t> FullConnectedTo(cell_t p, SgBlackWhite c) const;
    std::vector<cell_t> SemiConnectedTo(cell_t p, SgBlackWhite c) const;
    std::vector<cell_t> GetCarrierBetween(cell_t p1, cell_t p2) const;
    
    void SetSavePoint1()      { CopyState(m_savePoint1, m_state); }
    void SetSavePoint2()      { CopyState(m_savePoint2, m_state); }
    void RestoreSavePoint1()  { CopyState(m_state, m_savePoint1); }
    void RestoreSavePoint2()  { CopyState(m_state, m_savePoint2); }

    void CheckConsistency();
    void DumpBlocks() ;
private:

    static const int BORDER_NONE  = 0; // 000   border values, used bitwise
    static const int BORDER_BOTTOM= 1; // 001
    static const int BORDER_LEFT  = 2; // 010
    static const int BORDER_RIGHT = 4; // 100
    static const int BORDER_ALL   = 7; // 111

    struct SharedLiberties
    {
        static const size_t MAX_LIBERTIES = 8;

	cell_t m_other;
        SgArrayList<cell_t, MAX_LIBERTIES> m_liberties;

	SharedLiberties()
            : m_other(0)
	{ }

	SharedLiberties(cell_t anchor)
	    : m_other(anchor)
	{ }

        SharedLiberties(cell_t anchor, cell_t liberty)
	    : m_other(anchor)
            , m_liberties(liberty)
	{ }

        bool Empty() const
        { return m_liberties.IsEmpty(); }

        size_t Size() const
        { return m_liberties.Length(); }

        bool Contains(cell_t p) const
        { return m_liberties.Contains(p); }

        void PushBack(cell_t p)
        { 
            if (Size() < MAX_LIBERTIES)
                m_liberties.PushBack(p); 
        }

        void Include(cell_t p)
        {
            if (!Contains(p))
                PushBack(p);
        }

        bool Exclude(cell_t p)
        {
            return m_liberties.Exclude(p);
        }

    };

    struct Block
    {
        static const int MAX_STONES = Y_MAX_CELL;
        static const int MAX_LIBERTIES = Y_MAX_CELL;

        typedef SgArrayList<int, MAX_LIBERTIES> LibertyList;
        typedef LibertyList::Iterator LibertyIterator;
        typedef SgArrayList<int, MAX_STONES> StoneList;
        typedef StoneList::Iterator StoneIterator;

        cell_t m_anchor;
        int m_border;
        SgBlackWhite m_color;
        LibertyList m_liberties;    
        StoneList m_stones;
	std::vector<SharedLiberties> m_shared;

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
            for(size_t i = 0; i != m_shared.size(); ++i)
                if (m_shared[i].m_other == b2->m_anchor)
                    return i;
            return -1;
        }

        std::string SharedLibertiesToString(const ConstBoard& cbrd) const
        {
            std::ostringstream os;
            os << "[";
            for (size_t i = 0; i < m_shared.size(); ++i) {
                if (i) os << ",";
                os << "[" << cbrd.ToString(m_shared[i].m_other) << " [";
                for (size_t j = 0; j < m_shared[i].Size(); ++j)
                {
                    if (j) os << ",";
                    os << cbrd.ToString(m_shared[i].m_liberties[j]);
                }
                os << "]]";
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
               << " shared=" << SharedLibertiesToString(cbrd)
               << "]\n";
            return os.str();
        }
    };

    struct Group
    {
	static const size_t MAX_BLOCKS = Y_MAX_CELL/4;

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
	int m_NumAdj[3];
        typedef SgArrayList<cell_t, 6> SemiConnectionList;        
        typedef SgArrayList<cell_t, 6> FullConnectionList;        
	SemiConnectionList m_SemiConnects[2];
	FullConnectionList m_FullConnects[2];

	Cell()
	{
	    memset(m_NumAdj, 0, sizeof(m_NumAdj));
        }

        void AddBorderConnection(const Block* b)
        {
            m_FullConnects[SG_BLACK].Include(b->m_anchor);
            m_FullConnects[SG_WHITE].Include(b->m_anchor);
        }

        void AddFull(const Block* b, SgBlackWhite color)
        {  m_FullConnects[color].Include(b->m_anchor); }

	void AddSemi(const Block* b, SgBlackWhite color)
	{  m_SemiConnects[color].Include(b->m_anchor); }

	void RemoveSemiConnection(const Block* b, SgBlackWhite color) 
        {  m_SemiConnects[color].Exclude(b->m_anchor); }

	void RemoveFullConnection(const Block* b, SgBlackWhite color) 
	{  m_FullConnects[color].Exclude(b->m_anchor); }

	bool IsSemiConnected(const Block* b, SgBlackWhite color ) const
	{  return m_SemiConnects[color].Contains(b->m_anchor); }

	bool IsFullConnected(const Block* b, SgBlackWhite color) const
	{  return m_FullConnects[color].Contains(b->m_anchor); }

	std::string ToString(const ConstBoard& cbrd) const
        {
            SG_UNUSED(cbrd);
            std::ostringstream os;
            os << "[Empty=" << this->m_NumAdj[SG_EMPTY]
               << " Black=" << this->m_NumAdj[SG_BLACK]
               << " White=" << this->m_NumAdj[SG_WHITE]
               << " Fulls=[ ";
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
        std::vector<SgBoardColor> m_color;
	std::vector<Cell*> m_cell;
	std::vector<Cell> m_cellList;
        std::vector<Block*> m_block;
        std::vector<Block> m_blockList;
	std::vector<Group*> m_group;
	std::vector<Group> m_groupList;
        std::vector<std::vector<SharedLiberties> > m_con;
        //SharedLiberties m_con[Y_MAX_CELL][Y_MAX_CELL];

	std::vector< std::vector<Block*> > m_activeBlocks;
        
        SgArrayList<cell_t, 3> m_oppBlocks;
                
        SgBlackWhite m_toPlay;
        SgBoardColor m_winner;
        SgBoardColor m_vcWinner;
        cell_t       m_vcGroupAnchor;
        cell_t       m_lastMove;

        void Init(int T);
        void CopyState(const State& other);

	bool IsActive(const Block* b) const
	{
	    SgBlackWhite color = b->m_color;
	    for (size_t i = 0; i < m_activeBlocks[color].size(); ++i)
		if (m_activeBlocks[color][i]->m_anchor == b->m_anchor)
		    return true;
	    return false;
	}
	void RemoveActiveBlock(const Block* b)
	{
	    SgBlackWhite color = b->m_color;
	    for (size_t i = 0; i < m_activeBlocks[color].size(); ++i)
		if(m_activeBlocks[color][i] == b) {
		    std::swap(m_activeBlocks[color][i], 
			      m_activeBlocks[color].back());
		    m_activeBlocks[color].pop_back();
		    return;
		}
	}
	int GetActiveIndex(const Block* b) const
	{
	    for(size_t i = 0; i != m_activeBlocks[b->m_color].size(); ++i)
		if (m_activeBlocks[b->m_color][i]->m_anchor == b->m_anchor)
		    return i;
	    return -1;
	}
    };

    State m_state;
    State m_savePoint1;
    State m_savePoint2;

    void CreateSingleStoneBlock(cell_t p, SgBlackWhite color, int border);

    bool IsAdjacent(cell_t p, const Block* b);

    void AddStoneToBlock(cell_t p, int border, Block* b);

    void MergeBlocks(cell_t p, int border, SgArrayList<cell_t, 3>& adjBlocks);

    int GetSharedLibertiesIndex(Block* b1, Block* b2) const;

    void AddSharedLiberty(Block* b1, Block* b2, cell_t p);

    void AddSharedLibertiesAroundPoint(Block* b1, cell_t p, cell_t skip);

    void MergeSharedLiberty(const Block* b1, Block* b2);

    static int
        NumUnmarkedSharedLiberties(const SharedLiberties& lib, 
                                   int* seen, int id);

    static void MarkLibertiesAsSeen(const SharedLiberties& lib, 
                                    int* seen, int id);

    void ResetBlocksInGroup(Block* b);

    void ComputeGroupForBlock(Block* b);

    void ComputeGroupForBlock(Block* b, int* seen, int id);

    void GroupSearch(int* seen, Block* b, int id);

    void AddNonGroupEdges(int* seen, Group* g, int id);

    void RemoveSharedLiberty(cell_t p, Block* a, Block* b);

    void RemoveSharedLiberty(cell_t p, SgArrayList<cell_t, 3>& adjBlocks);

    void RemoveConnectionAtIndex(Block* b, size_t i);

    void RemoveConnectionWith(Block* b, const Block* other);

    void RemoveEdgeSharedLiberties(Block* b);

    void FixOrderOfConnectionsFromBack(Block* b1);

    const SharedLiberties&
    GetSharedLiberties(const Block* b1, const Block* b2) const;

    const SharedLiberties& GetSharedLiberties(cell_t p1, cell_t p2) const
    { return GetSharedLiberties(GetBlock(p1), GetBlock(p2)); }

    void CopyState(Board::State& a, const Board::State& b);

    Block* GetBlock(cell_t p) 
    { return m_state.m_block[p]; }

    const Block* GetBlock(cell_t p) const
    { return m_state.m_block[p]; }

    Group* GetGroup(cell_t p)
    { return m_state.m_group[p]; }

    const Group* GetGroup(cell_t p) const
    { return m_state.m_group[p]; }

    Cell* GetCell(cell_t p)
    { return m_state.m_cell[p]; }

    const Cell* GetCell(cell_t p) const
    { return m_state.m_cell[p]; }

    int NumNeighbours(cell_t p, SgBlackWhite color) const
    { return GetCell(p)->m_NumAdj[color]; }

    void AddCellToConnection(cell_t p1, cell_t p2, cell_t cell)
    {
	m_state.m_con[p1][p2].Include(cell);
	m_state.m_con[p2][p1].Include(cell);
    }

    bool RemoveCellFromConnection(cell_t p1, cell_t p2, cell_t cell)
    {
	m_state.m_con[p1][p2].Exclude(cell);
	return m_state.m_con[p2][p1].Exclude(cell);
    }

    void UpdateConnectionsToNewAnchor(const Block* from, const Block* to);
    void PromoteConnectionType(cell_t p, const Block* b, SgBlackWhite color);
    void DemoteConnectionType(cell_t p, Block* b, SgBlackWhite color);

    Board(const Board& other);          // not implemented
    void operator=(const Board& other); // not implemented
};

//----------------------------------------------------------------------

class CellIterator : public VectorIterator<cell_t>
{
public:
    CellIterator(const Board& brd);

    CellIterator(const ConstBoard& brd);
};

inline CellIterator::CellIterator(const Board& brd)
    : VectorIterator<cell_t>(brd.Const().m_cells)
{
}

inline CellIterator::CellIterator(const ConstBoard& brd)
    : VectorIterator<cell_t>(brd.m_cells)
{
}

class CellAndEdgeIterator : public VectorIterator<cell_t>
{
public:
    CellAndEdgeIterator(const Board& brd);

    CellAndEdgeIterator(const ConstBoard& brd);
};

inline CellAndEdgeIterator::CellAndEdgeIterator(const Board& brd)
    : VectorIterator<cell_t>(brd.Const().m_cells_edges)
{
}

inline CellAndEdgeIterator::CellAndEdgeIterator(const ConstBoard& brd)
    : VectorIterator<cell_t>(brd.m_cells_edges)
{
}
    
//----------------------------------------------------------------------

#endif
