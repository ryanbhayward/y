#pragma once

#include "SgSystem.h"
#include "SgArrayList.h"
#include "SgHash.h"
#include "SgBoardColor.h"
#include "YException.h"
#include "Board.h"

#include <string>

//---------------------------------------------------------------------------

struct SemiConnection
{
    typedef SgArrayList<cell_t, 5> Carrier;
    typedef Carrier::Iterator Iterator;

    cell_t m_p1;
    cell_t m_p2;
    cell_t m_key;
    cell_t m_group_id;
    cell_t m_con_type;   // -1: group carrier; WEST,EAST,SOUTH: to that edge
    Carrier m_carrier;
    uint32_t m_hash;

    SemiConnection();

    SemiConnection(cell_t p1, cell_t p2, cell_t key, const Carrier& carrier);

    void ReplaceEndpoint(cell_t from, cell_t to)
    {
        if (m_p1 == from) {
            m_p1 = std::min(to, m_p2);
            m_p2 = std::max(to, m_p2);
        } else {
            assert(m_p2 == from);
            m_p2 = std::max(to, m_p1);
            m_p1 = std::min(to, m_p1);
        }
    }

    bool Contains(cell_t p) const
    { 
        return m_carrier.Contains(p); 
    }

    bool operator==(const SemiConnection& other) const
    { 
        return m_hash == other.m_hash; 
    }

    bool SameEndpoints(cell_t a, cell_t b) const
    {
        return (m_p1 == std::min(a, b) && 
                m_p2 == std::max(a, b));
    }

    bool SameEndpoints(const SemiConnection& other) const
    { 
        return m_p1 == other.m_p1 && m_p2 == other.m_p2;
    }

    bool IsCarrierSubsetOf(const SemiConnection& other) const
    {
        for (int i = 0; i < m_carrier.Length(); ++i)
            if (!other.m_carrier.Contains(m_carrier[i]))
                return false;
        return true;
    }

    bool Intersects(const MarkedCells& cells) const 
    {
        for (int i = 0; i < m_carrier.Length(); ++i)
            if (cells.Marked(m_carrier[i]))
                return true;
        return false;
    }

    bool Intersects(const SemiConnection& other) const
    {
        for (int i = 0; i < m_carrier.Length(); ++i)
            if (other.Contains(m_carrier[i]))
                return true;
        return false;
    }

    std::string ToString() const 
    {
        std::ostringstream os;
        os << ConstBoard::ToString(m_p1) << ' '
           << ConstBoard::ToString(m_p2) << ' '
           << "semi "
           << "and ";
        os << "[";
        for (int i = 0; i < m_carrier.Length(); ++i)
            os << ' ' << ConstBoard::ToString(m_carrier[i]);
        os << " ] ";
        os << "[ ] ";  // for HexGui
        os << ConstBoard::ToString(m_key);
        return os.str();
    }

};

//---------------------------------------------------------------------------

class Board;
class Groups;

class SemiTable
{
public:
    static const int NUM_SLOTS = 32;  // needs to be power of 2
    static const int SLOT_MASK = 31;  // needs to be 1 minus above
    static const int MAX_ENTRIES_PER_SLOT = 64;
    static const int MAX_ENTRIES_IN_TABLE = 1024;
    static uint32_t s_cell_hash[Y_MAX_CELL];

public:

    static void Init()
    {
        SgRandom::SetSeed(1);
        for (int i = 0; i < Y_MAX_CELL; ++i)
            s_cell_hash[i] = SgRandom::Global().Int();
    }

    static uint32_t SlotIndex(uint32_t hash)
    { return hash & SLOT_MASK; }

    static uint32_t Hash(cell_t a) 
    { return s_cell_hash[a]; }

    template<int SIZE>
    static uint32_t Hash(const SgArrayList<cell_t, SIZE>& c)
    {
        uint32_t ret = 0;
        for (int i = 0; i < c.Length(); ++i)
            ret ^= Hash(c[i]);
        return ret;
    }

    static uint32_t HashEndpoints(const SemiConnection& s)
    { return Hash(s.m_p1) ^ Hash(s.m_p2); }

    static uint32_t ComputeHash(const SemiConnection& s)
    {
        return HashEndpoints(s) ^ Hash(s.m_carrier);
    }

    SemiTable();

    void SetGroups(Groups* groups)
    { m_groups = groups; }

    void Include(const SemiConnection& s);

    std::string ToString() const;

    int32_t HashToIndex(uint32_t hash) const;

    const SemiConnection& LookupHash(uint32_t hash) const;

    const SemiConnection& LookupIndex(int32_t index) const
    { return m_entries[index]; }

    SemiConnection& LookupIndex(int32_t index)
    { return m_entries[index]; }

    void RemoveContaining(cell_t p);

    void RemoveAllBetween(cell_t a, cell_t b);

    void TransferEndpoints(cell_t from, cell_t to);

    void ClearNewSemis()
    {
        m_newlist.Clear();
    }

    const SgArrayList<SemiConnection*, 128>& GetNewSemis()
    { 
        return m_newlist;
    }

    class IteratorPair
    {
    public:
        IteratorPair(cell_t a, cell_t b, const SemiTable* st)
            : m_st(st) 
            , m_slot(SlotIndex(Hash(a) ^ Hash(b)))
            , m_index(-1)
            , m_a(std::min(a, b))
            , m_b(std::max(a, b))
        {
            operator++();
        }

        const SemiConnection& operator*() const
        {  return m_st->m_entries[m_st->m_end_table[m_slot][m_index]]; }

        int Index() const
        { return m_st->m_end_table[m_slot][m_index]; }

        void operator++()
        { 
            while (++m_index < m_st->m_end_table[m_slot].Length()) {
                const SemiConnection& semi 
                    = m_st->m_entries[m_st->m_end_table[m_slot][m_index]];
                if (semi.m_p1 == m_a && semi.m_p2 == m_b)
                    break;
            }
        }

        operator bool() const
        { return m_index < m_st->m_end_table[m_slot].Length(); }
        
        const SemiTable* m_st;
        int m_slot;
        int m_index;
        int m_a, m_b;
    };

#if 0
    // NOT NEEDED ANYMORE
    // If you want to use this, you need to fix it so that it
    // iterates over all [a, x] with x in [0..Y_MAX_CELL] instead
    // of x in [a..Y_MAX_CELL].
    class IteratorSingle
    {
    public:
        IteratorSingle(cell_t a, const Board* brd, const SemiTable* st)
            : m_st(st)
            , m_brd(brd)
            , m_iter(a, a, st)
            , m_a(a)
            , m_b(a)
        {
            operator++();
        }

        const SemiConnection& operator*() const
        { return *m_iter; }

        int Index() const
        { return m_iter.Index(); }
        
        void operator++();

        operator bool() const
        { 
            return m_b < Y_MAX_CELL;
        }

        const SemiTable* m_st;
        const Board* m_brd;
        IteratorPair m_iter;
        cell_t m_a;
        cell_t m_b;
    };
#endif

private:

    typedef SgArrayList<int16_t, MAX_ENTRIES_PER_SLOT> SlotSizeList;
    typedef SgArrayList<int16_t, MAX_ENTRIES_IN_TABLE> TableSizeList;

    Groups* m_groups;

    SlotSizeList m_end_table[NUM_SLOTS];
    SlotSizeList m_hash_table[NUM_SLOTS];
    TableSizeList m_freelist;
    TableSizeList m_usedlist;
    TableSizeList m_worklist;
    SemiConnection m_entries[MAX_ENTRIES_IN_TABLE];
    SgArrayList<SemiConnection*, 128> m_newlist;

    bool m_using_worklist;

#define BEGIN_USING_WORKLIST \
    assert(!m_using_worklist); \
    m_using_worklist = true;

#define FINISH_USING_WORKLIST \
    m_using_worklist = false;

    void Remove(int index);
};
