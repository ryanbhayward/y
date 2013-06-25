#ifndef BOARDY_H
#define BOARDY_H

#include <string>
#include <vector>

#include "SgSystem.h"
#include "SgArrayList.h"
#include "SgHash.h"
#include "SgBoardColor.h"
#include "SgMove.h"
#include "VectorIterator.h"

static const int Y_INFINITY = 9999;

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

struct Board 
{
    explicit Board(int size);

    const ConstBoard& Const() const { return m_constBrd; }

    void SetSize(int size);
    int Size() const { return Const().Size(); }

    void SetPosition(const Board& other);

    std::string ToString() const;
    std::string BorderToString() const;
    std::string AnchorsToString() const;

    SgHashCode Hash() const { return 1; /* FIXME: IMPLEMENT HASH! */ };

    bool IsGameOver() const { return m_state.m_winner != SG_EMPTY; }
    SgBoardColor GetWinner() const { return m_state.m_winner; }
    bool IsWinner(SgBlackWhite player) const 
    { return player == m_state.m_winner; }

    void Swap();

    bool IsOccupied(int cell) const 
    { return m_state.m_color[cell] != SG_EMPTY; }
    
    bool IsEmpty(int cell) const 
    { return m_state.m_color[cell] == SG_EMPTY; }

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

    // Returns SG_NULLMOVE if no savebridge pattern matches last move played
    int GeneralSaveBridge(SgRandom& random) const;

    int Anchor(int p) const 
    { return m_state.m_block[p]->m_anchor; }

    bool IsInBlock(int p, int anchor) const
    { return m_state.m_block[p]->m_anchor == anchor; }

    bool IsLibertyOfBlock(int p, int anchor) const
    { return m_state.m_block[anchor]->m_liberties.Contains(p); }

    const std::vector<int>& GetSharedLiberties(int p1, int p2) const
    { return GetSharedLiberties(m_state.m_block[p1], m_state.m_block[p2]); }

    std::vector<int> GetLibertiesWith(int p1) const
    {
        std::vector<int> ret;
        for (size_t i = 0; i < m_state.m_block[p1]->m_shared.size(); ++i)
            ret.push_back(m_state.m_block[p1]->m_shared[i].m_other);
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
	int m_other;
        std::vector<int> m_liberties;

	SharedLiberties()
            : m_other(-1)
	{ }

        SharedLiberties(int anchor, int liberty)
	    : m_other(anchor)
            , m_liberties(1, liberty)
	{ }

	SharedLiberties(int anchor, const std::vector<int>& liberties)
	    : m_other(anchor)
            , m_liberties(liberties)
	{ }

        ~SharedLiberties()
        { }

        bool Contains(int p) const
        {
            for (size_t i = 0; i < m_liberties.size(); ++i)
                if (m_liberties[i] == p)
                    return true;
            return false;
        }

        void PushBack(int p)
        { m_liberties.push_back(p); }

        void Include(int p)
        {
            if (!Contains(p))
                PushBack(p);
        }

        void Exclude(int p)
        {
            for (size_t j = 0; j < m_liberties.size(); ++j) {
                if (m_liberties[j] == p) {
                    std::swap(m_liberties[j], m_liberties.back());
                    m_liberties.pop_back();
                    return;
                }
            }
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

        int GetSharedLibertiesIndex(const Block* b2) const
        {
            for(size_t i = 0; i != m_shared.size(); ++i)
                if (m_shared[i].m_other == b2->m_anchor)
                    return i;
            return -1;
        }

        void RemoveSharedLiberties(int i)
        {
            m_shared[i] = m_shared.back();
            m_shared.pop_back();
        }

        std::string SharedLibertiesToString(const ConstBoard& cbrd) const
        {
            std::ostringstream os;
            os << "[";
            for (size_t i = 0; i < m_shared.size(); ++i) {
                if (i) os << ",";
                os << "[" << cbrd.ToString(m_shared[i].m_other) << " [";
                for (size_t j=0; j < m_shared[i].m_liberties.size(); ++j)
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

    ConstBoard m_constBrd;

    struct State 
    {
        std::vector<SgBoardColor> m_color;
        std::vector<Block*> m_block;
        std::vector<Block> m_blockList;
        
        SgArrayList<int, 3> m_adjBlocks;
        SgArrayList<int, 3> m_oppBlocks;
                
        SgBlackWhite m_toPlay;
        SgBoardColor m_winner;
        int          m_lastMove;

        void Init(int T);
        void CopyState(const State& other);
    };

    State m_state;
    State m_savePoint1;
    State m_savePoint2;
        
    void CreateSingleStoneBlock(int p, SgBlackWhite color, int border);

    bool IsAdjacent(int p, const Block* b);

    void AddStoneToBlock(int p, int border, Block* b);

    void MergeBlocks(int p, int border, SgArrayList<int, 3>& adjBlocks);

    int GetSharedLibertiesIndex(Block* b1, Block* b2) const;

    const std::vector<int>& GetSharedLiberties(Block* b1, Block* b2) const;

    void AddSharedLiberty(Block* b1, Block* b2, int p);

    void MergeSharedLiberty(Block* b1, Block* b2);

    void RemoveSharedLiberty(int p, Block* a, Block* b);

    void RemoveSharedLiberty(int p, SgArrayList<int, 3>& adjBlocks);

    void CopyState(Board::State& a, const Board::State& b);

    Block* GetBlock(int p) 
    { return m_state.m_block[p]; }

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
