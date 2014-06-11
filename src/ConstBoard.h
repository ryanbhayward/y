#pragma once

#include <string>
#include <vector>
#include <bitset>

#include "SgSystem.h"
#include "SgArrayList.h"
#include "SgHash.h"
#include "SgBoardColor.h"
#include "SgMove.h"
#include "VectorIterator.h"
#include "YUtil.h"

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

static const int Y_MAX_CELL = 127;
static const int Y_MAX_SIZE = 15;   // such that TotalCells < Y_MAX_CELL

typedef int8_t cell_t;

static const int Y_SWAP = -2;  // SG_NULLMOVE == -1

typedef std::pair<cell_t, cell_t> CellPair;

struct MarkedCells
{
    std::bitset<Y_MAX_CELL> m_marked;

    MarkedCells()
    {
        Clear();
    }

    void Clear()
    {
        m_marked.reset();
    }

    bool Marked(cell_t p) const
    { 
        return m_marked.test(p);
    }

    bool Mark(cell_t p)
    {
        bool ret = !Marked(p);
        m_marked.set(p);
        return ret;
    }

    void Mark(const MarkedCells& other)
    {
        m_marked |= other.m_marked;
    }

    bool Intersects(const MarkedCells& other)
    {
        return (m_marked & other.m_marked).any();
    }

    void Unmark(cell_t p) 
    {
        m_marked.reset(p);
    }

    void Unmark(const MarkedCells& other)
    {
        m_marked &= ~other.m_marked;
    }

    size_t Count() const
    {
        return m_marked.count(); 
    }

    class Iterator
    {
    public:
        
        // FIXME: USE A REAL BITSET ITERATOR!!

        Iterator(const MarkedCells& marked)
            : m_cur(-1)
            , m_marked(marked)
        {
            operator++();
        }

        cell_t operator*() const
        { return m_cur; }

        void operator++()
        { 
            do {
                ++m_cur;
            } while (m_cur < Y_MAX_CELL && !m_marked.Marked(m_cur));
        }

        operator bool() const
        { return m_cur < Y_MAX_CELL; }

    private:
        cell_t m_cur;
        const MarkedCells& m_marked;
    };
};

template<typename T>
void Include(std::vector<T>& v, const MarkedCells& a)
{
    for (MarkedCells::Iterator i(a); i; ++i)
        Include(v, *i);
}

struct MarkedCellsWithList
{
    typedef SgArrayList<cell_t, Y_MAX_CELL> ListType;
    typedef ListType::Iterator ListIterator;

    class Iterator : public ListIterator
    {
    public:
        Iterator(const MarkedCellsWithList& mc)
            : ListIterator(mc.m_list)
        { }
    };

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

    bool Marked(cell_t p) const
    { 
        return m_marked[p];
    }

    int Size() const
    { return m_list.Length(); }

    bool IsEmpty() const
    { return m_list.IsEmpty(); }

    bool Intersects(const MarkedCells& other) const;
    bool Intersects(const SgArrayList<cell_t, 6>& other) const;
    int  IntersectSize(const MarkedCells& other) const;
    int  IntersectSize(const MarkedCellsWithList& other) const;
    int  IntersectSize(const SgArrayList<cell_t, 6>& other) const;
};

class ConstBoard 
{
public:

    static const int FIRST_EDGE = 0;
    static const int WEST = 0;
    static const int EAST = 1;
    static const int SOUTH = 2;
    static const int FIRST_NON_EDGE = 3;
    
    static const int DIR_NW = 0;
    static const int DIR_NE = 1;
    static const int DIR_E  = 2;
    static const int DIR_SE = 3;
    static const int DIR_SW = 4;
    static const int DIR_W  = 5;

    static const int BORDER_NONE  = 0; // 000   border values, used bitwise
    static const int BORDER_WEST  = 1; // 001
    static const int BORDER_EAST  = 2; // 010
    static const int BORDER_SOUTH = 4; // 100
    static const int BORDER_ALL   = 7; // 111

    static bool IsEdge(cell_t cell)
    {
        return (cell == WEST || cell == EAST || cell == SOUTH);
    }

    static int ToBorderValue(cell_t cell)
    {
        if (cell == WEST)
            return BORDER_WEST;
        if (cell == EAST)
            return BORDER_EAST;
        if (cell == SOUTH)
            return BORDER_SOUTH;
        return BORDER_NONE;
    }
    static std::string BorderToString(int border);

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

    static std::string ToString(SgMove cell);

    static SgMove FromString(const std::string& name);

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

class EdgeIterator
{
public:
    EdgeIterator()
        : m_index(ConstBoard::FIRST_EDGE)
    { }
    
    /** Advance the state of the iteration to the next liberty. */
    void operator++()
    { ++m_index; }
    
    /** Return the current liberty. */
    cell_t operator*() const
    { return m_index; }
    
    /** Return true if iteration is valid, otherwise false. */
    operator bool() const
    { return m_index < ConstBoard::FIRST_NON_EDGE; }
    
private:
    cell_t m_index;
};

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
