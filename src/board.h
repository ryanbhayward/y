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


//  Y-board       N cells per side    N(N+1)/2  total cells
//  fat board, aka. guarded board with GUARDS extra rows/cols,
//  allows fixed offsets for nbrs @ dist 1 or 2, 
//  e.g. below with GUARDS = 2
//          . . g g g g g g g 
//           . g g g g g g g g 
//            g g * * * * * g g 
//             g g * * * * g g . 
//              g g * * * g g . . 
//               g g * * g g . . . 
//                g g * g g . . . . 
//                 g g g g . . . . . 
//                  g g g . . . . . . 


static const int Y_MAX_CELL = 512;
static const int Y_MAX_SIZE = 19;   // such that TotalCells < Y_MAX_CELL

static const int TMP = 4;

static const int Y_SWAP = -2;  // SG_NULLMOVE == -1
 
static const int NumNbrs = 6;              // num nbrs of each cell

class ConstBoard 
{
public:

    static const int GUARDS = 2; // must be at least one

    int m_size;
    int Np2G;
    int TotalCells;
    int TotalGBCells;

    int WEST;
    int EAST;
    int SOUTH;

    int Nbr_offsets[NumNbrs+1];
    int Bridge_offsets[NumNbrs];

    ConstBoard();
    ConstBoard(int size);

    static char ColorToChar(SgBoardColor color);
    static std::string ColorToString(SgBoardColor color);

    inline int fatten(int r,int c) const 
    {  return Np2G*(r+GUARDS) + (GUARDS+m_size-r-1+c); }

    inline int  board_row(int lcn) const 
    {  return (lcn/Np2G)-GUARDS;}

    inline int  board_col(int lcn) const 
    {
        int r = board_row(lcn);
        return (lcn%Np2G)-GUARDS-m_size+r+1;
    }

    inline bool near_edge(int lcn) const
    { 
        //static inline bool near_edge(int lcn,int d) { 
        // near_edge(lcn,0) true if lcn on edge
        // near_edge(lcn,1) true if lcn 1-away from edge
        int r = board_row(lcn);
        int c = board_col(lcn);
        //return ((d==r)||(d==c)||(N==r+c+d+1));
        return ((0==r)||(0==c)||(m_size==r+c+1));
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

    friend class BoardIterator;

    friend class Board;
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
    { return m_point + m_cbrd.Nbr_offsets[m_index]; }
    
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
    { return m_state.m_block[p]->ToString(Const()); }

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


    ConstBoard m_constBrd;

    struct State 
    {
        std::vector<SgBoardColor> m_color;
        std::vector<Block*> m_block;
        std::vector<Block> m_blockList;
	std::vector< std::vector<Block*> > m_activeBlocks;
	std::vector<Group*> m_group;
	std::vector<Group> m_groupList;
        
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
	void RemoveActiveBlock(Block* b)
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

    void MergeSharedLiberty(Block* b1, Block* b2);

    static int
        NumUnmarkedSharedLiberties(const SharedLiberties& lib, 
                                   int* seen, int id);

    static void MarkLibertiesAsSeen(const SharedLiberties& lib, 
                                    int* seen, int id);

    void ResetBlocksInGroup(Block* b);

    void ComputeGroupForBlock(Block* b);

    void ComputeGroupForBlock(Block* b, int* seen, int id);

    void GroupSearch(int* seen, Block* b, int id);

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

    Board(const Board& other);          // not implemented
    void operator=(const Board& other); // not implemented
};

//----------------------------------------------------------------------

class BoardIterator : public VectorIterator<int>
{
public:
    BoardIterator(const Board& brd);

    BoardIterator(const ConstBoard& brd);
};

inline BoardIterator::BoardIterator(const Board& brd)
    : VectorIterator<int>(brd.Const().m_cells)
{
}

inline BoardIterator::BoardIterator(const ConstBoard& brd)
    : VectorIterator<int>(brd.m_cells)
{
}
    
//----------------------------------------------------------------------

extern const int Nbr_offsets[NumNbrs+1] ;  // last = 1st to avoid using %mod
extern const int Bridge_offsets[NumNbrs] ;

static const int RHOMBUS = 0;
static const int TRI   = 1;

void shapeAs(int shape, int X[]);
void showYcore(int X[]) ;  // simple version of Board.show
void display_nearedges();

#endif
