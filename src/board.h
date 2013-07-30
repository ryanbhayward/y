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

    inline int fatten(int r, int c) const 
    { return (r+GUARDS+1)*(r+GUARDS+2)/2 + c + GUARDS; }
    
    inline int board_row(int p) const 
    {  
        int r = 2;
        while((r+1)*(r+2)/2 < p)
            ++r;
        return r - 2;
    }

    inline int board_col(int p) const 
    {
        int r = board_row(p) + 2;
        return p - r*(r+1)/2 - GUARDS;
    }

    int Size() const { return m_size; }

    bool IsOnBoard(int cell) const
    {
        return std::find(m_cells.begin(), m_cells.end(), cell) != m_cells.end();
    }

    std::string ToString(int cell) const;
    int FromString(const std::string& name) const;

private:
    std::vector<int> m_cells;
    std::vector<int> m_cells_edges;
    std::vector<std::vector<int> > m_cell_nbr;

    friend class Board;
    friend class CellIterator;
    friend class CellAndEdgeIterator;
    friend class CellNbrIterator;
};


//----------------------------------------------------------------------

class CellNbrIterator
{
public:
    CellNbrIterator(const ConstBoard& cbrd, int p)
        : m_cbrd(cbrd)
        , m_point(p)
        , m_index(0)
    { }
    
    /** Advance the state of the iteration to the next liberty. */
    void operator++()
    { ++m_index; }
    
    /** Return the current liberty. */
    int operator*() const
    { return m_cbrd.m_cell_nbr[m_point][m_index]; }
    
    /** Return true if iteration is valid, otherwise false. */
    operator bool() const
    { return m_index < 6; }
    
private:
    const ConstBoard& m_cbrd;
    int m_point;
    int m_index;
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

    void AddWeight(int p, float w)
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
    std::string ToString(int p) const { return Const().ToString(p); }
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
    int WinningVCGroup() const { return m_state.m_vcGroupAnchor; }

    std::vector<int> GroupCarrier(int p) const
    {
        std::vector<int> ret;
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

    bool IsOccupied(int cell) const 
    { return m_state.m_color[cell] != SG_EMPTY; }
    
    bool IsEmpty(int cell) const 
    { return m_state.m_color[cell] == SG_EMPTY; }

    bool IsBorder(int cell) const
    { return m_state.m_color[cell] == SG_BORDER; }

    SgBoardColor GetColor(int cell) const
    { return m_state.m_color[cell]; }

    int ToPlay() const         { return m_state.m_toPlay; }
    void SetToPlay(int toPlay) { m_state.m_toPlay = toPlay; }
    void FlipToPlay()          { m_state.m_toPlay = SgOppBW(m_state.m_toPlay); }

    void RemoveStone(int lcn);
    int LastMove() const { return m_state.m_lastMove; }
    void SetLastMove(int lcn) { m_state.m_lastMove = lcn; } // used after undo
    void Play(SgBlackWhite color, int p);

    // Returns SG_NULLMOVE if no savebridge pattern matches, otherwise
    // a move to reestablish the connection.
    int SaveBridge(int lastMove, const SgBlackWhite toPlay, 
                   SgRandom& random) const;

    int MaintainConnection(int b1, int b2) const;
    // Returns SG_NULLMOVE if no savebridge pattern matches last move played
    void GeneralSaveBridge(LocalMoves& local) const;

    bool IsCellDead(int p) const;

    int BlockAnchor(int p) const 
    { return m_state.m_block[p]->m_anchor; }

    int GroupAnchor(int p) const
    { return GetGroup(BlockAnchor(p))->m_anchor; }

    int GroupBorder(int p) const
    { return GetGroup(BlockAnchor(p))->m_border; }

    bool IsInBlock(int p, int anchor) const
    { return m_state.m_block[p]->m_anchor == anchor; }

    bool IsLibertyOfBlock(int p, int anchor) const
    { return m_state.m_block[anchor]->m_liberties.Contains(p); }

    bool IsSharedLiberty(int p1, int p2, int p) const
    { return GetSharedLiberties(p1, p2).Contains(p); }

    std::vector<int> GetLibertiesWith(int p1) const
    {
        std::vector<int> ret;
        for (size_t i = 0; i < m_state.m_block[p1]->m_shared.size(); ++i)
            ret.push_back(m_state.m_block[p1]->m_shared[i].m_other);
        return ret;
    }

    std::vector<int> GetBlocksInGroup(int p) const
    {
	std::vector<int> ret;
        const Group* g = GetGroup(BlockAnchor(p));
        for (int i = 0; i < g->m_blocks.Length(); ++i) {
            ret.push_back(g->m_blocks[i]);
            if (g->m_blocks[i] == g->m_anchor)
                std::swap(ret.back(), ret[0]);
        }
	return ret;
    }

    std::string BlockInfo(int p) const
    { return GetBlock(p)->ToString(Const()); }

    std::string CellInfo(int p) const
    { return GetCell(p)->ToString(Const()); }

    std::vector<int> CellFullConnectedTo(int p, SgBlackWhite c) const
    {
        std::vector<int> ret;
        const Cell::FullConnectionList& fulls = GetCell(p)->m_FullConnects[c];
        for (int i = 0; i < fulls.Length(); ++i) {
            ret.push_back(fulls[i]);
        }
        return ret;
    }

    std::vector<int> CellSemiConnectedTo(int p, SgBlackWhite c) const
    {
        std::vector<int> ret;
        const Cell::SemiConnectionList& semis = GetCell(p)->m_SemiConnects[c];
        for (int i = 0; i < semis.Length(); ++i) {
            ret.push_back(semis[i]);
        }
        return ret;
    }

    std::vector<int> GetConnectedToFromTable(int p) const
    {
	if(IsEmpty(p))
	    return GetConnectionsWith(p);
	else
	    return GetConnectionsWith(GetBlock(p)->m_anchor);
    }

    std::vector<int> GetConnectionCarrier(int p1, int p2) const 
    {
	if(IsOccupied(p1)) {
	    if(IsOccupied(p2))
		return GetCarrierBetween(GetBlock(p1)->m_anchor, 
					 GetBlock(p2)->m_anchor);
	    else
		return GetCarrierBetween(GetBlock(p1)->m_anchor, p2);
	}
	else {
	    if(IsOccupied(p2))
		return GetCarrierBetween(p1, GetBlock(p2)->m_anchor);
	    else
		return GetCarrierBetween(p1, p2);
	}
    }
    

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

	int m_other;
        SgArrayList<int, MAX_LIBERTIES> m_liberties;

	SharedLiberties()
            : m_other(-1)
	{ }

	SharedLiberties(int anchor)
	    : m_other(anchor)
	{ }

        SharedLiberties(int anchor, int liberty)
	    : m_other(anchor)
            , m_liberties(liberty)
	{ }

        bool Empty() const
        { return m_liberties.IsEmpty(); }

        size_t Size() const
        { return m_liberties.Length(); }

        bool Contains(int p) const
        { return m_liberties.Contains(p); }

        void PushBack(int p)
        { 
            if (Size() < MAX_LIBERTIES)
                m_liberties.PushBack(p); 
        }

        void Include(int p)
        {
            if (!Contains(p))
                PushBack(p);
        }

        void Exclude(int p)
        {
            m_liberties.Exclude(p);
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

        int m_anchor;
        int m_border;
        SgBlackWhite m_color;
        LibertyList m_liberties;    
        StoneList m_stones;
	std::vector<SharedLiberties> m_shared;

        Block()
        { }

        Block(int p, SgBlackWhite color)
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

	int m_anchor;
	int m_border;
	SgArrayList<int, MAX_BLOCKS> m_blocks;

	Group()
	{ }

	Group(int anchor, int border)
	    : m_anchor(anchor)
	    , m_border(border)
	    , m_blocks(anchor)
	{ }
    };

    struct VC
    {
        int m_block;
        int m_size;
        int m_carrier[2];

        VC()
        { }

        VC(int block)
            : m_block(block)
            , m_size(0)
        { }
    };

    struct Cell
    {
	int m_NumAdj[3];
        typedef SgArrayList<int, 6> SemiConnectionList;        
        typedef SgArrayList<int, 3> FullConnectionList;        
	SemiConnectionList m_SemiConnects[2];
	FullConnectionList m_FullConnects[2];

	Cell()
	{
	    memset(m_NumAdj, 0, sizeof(m_NumAdj));
        }

        void AddFull(const Block* b)
        {
            m_FullConnects[b->m_color].PushBack(b->m_anchor);
        }

	void AddSemi(const Block* b)
	{
	    m_SemiConnects[b->m_color].PushBack(b->m_anchor);
	}

        void AddBorderConnection(const Block* b)
        {
            m_FullConnects[SG_BLACK].PushBack(b->m_anchor);
            m_FullConnects[SG_WHITE].PushBack(b->m_anchor);
        }

        void UpdateConnectionsToNewAnchor(const Block* from, const Block* to)
        {
            FullConnectionList& fulls = m_FullConnects[from->m_color];
            for (int i = 0; i < fulls.Length(); ++i) {
                if (fulls[i] == from->m_anchor)
                    fulls[i] = to->m_anchor;
            }
            SemiConnectionList& semis = m_SemiConnects[from->m_color];
            for (int i = 0; i < semis.Length(); ++i) {
                if (semis[i] == from->m_anchor)
                    semis[i] = to->m_anchor;
            }
        }

	std::string ToString(const ConstBoard& cbrd) const
        {
            SG_UNUSED(cbrd);
            std::ostringstream os;
            os << "[Empty=" << this->m_NumAdj[SG_EMPTY]
               << " Black=" << this->m_NumAdj[SG_BLACK]
               << " White=" << this->m_NumAdj[SG_WHITE]
               << "]\n";
            return os.str();
        }

	bool IsSemiConnected(Block* b) const
	{
	    for(int i = 0; i < m_SemiConnects[b->m_color].Length(); ++i)
		if(m_SemiConnects[b->m_color][i] == b->m_anchor)
		    return true;
	    return false;
	}

	bool IsFullConnected(Block* b) const
	{
	    for(int i = 0; i < m_FullConnects[b->m_color].Length(); ++i)
		if(m_FullConnects[b->m_color][i] == b->m_anchor)
		    return true;
	    return false;
	}

	void RemoveSemiConnection(Block* b) 
        {
            m_SemiConnects[b->m_color].Exclude(b->m_anchor);
	}
	void RemoveFullConnection(Block* b) 
	{
            m_FullConnects[b->m_color].Exclude(b->m_anchor);
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
	std::vector<std::vector<SharedLiberties> > m_connections;

	std::vector< std::vector<Block*> > m_activeBlocks;
        
        SgArrayList<int, 3> m_oppBlocks;
                
        SgBlackWhite m_toPlay;
        SgBoardColor m_winner;
        SgBoardColor m_vcWinner;
        int          m_vcGroupAnchor;
        int          m_lastMove;

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

    void CreateSingleStoneBlock(int p, SgBlackWhite color, int border);

    bool IsAdjacent(int p, const Block* b);

    void AddStoneToBlock(int p, int border, Block* b);

    void MergeBlocks(int p, int border, SgArrayList<int, 3>& adjBlocks);

    int GetSharedLibertiesIndex(Block* b1, Block* b2) const;

    void AddSharedLiberty(Block* b1, Block* b2, int p);

    void AddSharedLibertiesAroundPoint(Block* b1, int p, int skip);

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

    void RemoveSharedLiberty(int p, Block* a, Block* b);

    void RemoveSharedLiberty(int p, SgArrayList<int, 3>& adjBlocks);

    void RemoveConnectionAtIndex(Block* b, size_t i);

    void RemoveConnectionWith(Block* b, const Block* other);

    void RemoveEdgeSharedLiberties(Block* b);

    void FixOrderOfConnectionsFromBack(Block* b1);

    const SharedLiberties&
    GetSharedLiberties(const Block* b1, const Block* b2) const;

    const SharedLiberties& GetSharedLiberties(int p1, int p2) const
    { return GetSharedLiberties(GetBlock(p1), GetBlock(p2)); }

    void CopyState(Board::State& a, const Board::State& b);

    Block* GetBlock(int p) 
    { return m_state.m_block[p]; }

    const Block* GetBlock(int p) const
    { return m_state.m_block[p]; }

    Group* GetGroup(int p)
    { return m_state.m_group[p]; }

    const Group* GetGroup(int p) const
    { return m_state.m_group[p]; }

    Cell* GetCell(int p)
    { return m_state.m_cell[p]; }

    const Cell* GetCell(int p) const
    { return m_state.m_cell[p]; }

    int NumNeighbours(int p, SgBlackWhite color) const
    { return GetCell(p)->m_NumAdj[color]; }

    void AddConnection(int p1, int p2)
    { 
	m_state.m_connections[p1][p2].m_other = 1;
	m_state.m_connections[p2][p1].m_other = 1;
    }

    void AddCarrierToConnection(int p1, int p2, int carrier)
    {
	m_state.m_connections[p1][p2].Include(carrier);
	m_state.m_connections[p2][p1].Include(carrier);
    }

    void UpdateConnectionsToNewAnchor(const Block* from, const Block* to);
    void AddSharedLibertyConnection(int p1, int p2, int carrier);
    void RemoveConnection(int p);
    void UpdateCellConnection(Block* b, int empty);

    bool IsConnected(int p1, int p2) const
    { return m_state.m_connections[p1][p2].m_other == 1; }

    const std::vector<int> GetConnectionsWith(int p) const
    { 
	std::vector<int> ret;
	for(size_t i = 0; i < m_state.m_connections[p].size(); ++i)
	    if(IsConnected(p, (int)i))
		ret.push_back((int) i);
	return ret;
    }

    const std::vector<int> GetCarrierBetween(int p1, int p2) const
    {
	std::vector<int> ret;
	if(IsConnected(p1, p2))
	   for(size_t i = 0; i < m_state.m_connections[p1][p2].Size(); ++i)
	       ret.push_back(m_state.m_connections[p1][p2].m_liberties[i]);
	return ret;
    }

    Board(const Board& other);          // not implemented
    void operator=(const Board& other); // not implemented
};

//----------------------------------------------------------------------

class CellIterator : public VectorIterator<int>
{
public:
    CellIterator(const Board& brd);

    CellIterator(const ConstBoard& brd);
};

inline CellIterator::CellIterator(const Board& brd)
    : VectorIterator<int>(brd.Const().m_cells)
{
}

inline CellIterator::CellIterator(const ConstBoard& brd)
    : VectorIterator<int>(brd.m_cells)
{
}

class CellAndEdgeIterator : public VectorIterator<int>
{
public:
    CellAndEdgeIterator(const Board& brd);

    CellAndEdgeIterator(const ConstBoard& brd);
};

inline CellAndEdgeIterator::CellAndEdgeIterator(const Board& brd)
    : VectorIterator<int>(brd.Const().m_cells_edges)
{
}

inline CellAndEdgeIterator::CellAndEdgeIterator(const ConstBoard& brd)
    : VectorIterator<int>(brd.m_cells_edges)
{
}
    
//----------------------------------------------------------------------

#endif
