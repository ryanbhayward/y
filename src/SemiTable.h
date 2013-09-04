#pragma once

#include "SgSystem.h"
#include "SgArrayList.h"
#include "SgHash.h"
#include "SgBoardColor.h"
#include "YException.h"
#include "board.h"

#include <string>
#include <vector>

//---------------------------------------------------------------------------

struct SemiConnection
{
    cell_t m_key;
    cell_t m_p1;
    cell_t m_p2;
    SgArrayList<cell_t, 5> m_carrier;
    uint32_t m_hash;

    typedef SgArrayList<cell_t, 5>::Iterator Iterator;

    SemiConnection()
    { }

    bool Contains(cell_t p) const
    { return m_carrier.Contains(p); }

    bool Intersects(const MarkedCells& cells) const 
    {
        for (int i = 0; i < m_carrier.Length(); ++i) {
            if (cells.Marked(m_carrier[i]))
                return true;
        }
        return false;
    }

    bool Intersects(const SemiConnection& other) const
    {
        for (int i = 0; i < m_carrier.Length(); ++i) {
            if (other.Contains(m_carrier[i]))
                return true;
        }
        return false;
    }

    bool operator==(const SemiConnection& other) const
    { return m_hash == other.m_hash; }

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

class SemiTable
{
public:
    static const int NUM_SLOTS = 32;  // needs to be power of 2
    static const int SLOT_MASK = 31;  // needs to be 1 minus above
    static const int MAX_ENTRIES_PER_SLOT = 32;
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

    static uint32_t HashEndpoints(const SemiConnection& s)
    { return Hash(s.m_p1) ^ Hash(s.m_p2); }

    static uint32_t ComputeHash(const SemiConnection& s)
    {
        uint32_t ret = HashEndpoints(s);
        for (int i = 0; i < s.m_carrier.Length(); ++i)
            ret ^= Hash(s.m_carrier[i]);
        return ret;
    }

    SemiTable()
    {
        for (int i = 0; i < MAX_ENTRIES_IN_TABLE; ++i)
            m_freelist.PushBack(i);
    }

    void Add(const SemiConnection& s);

    bool Contains(const SemiConnection& s) const;

    const SemiConnection& Lookup(uint32_t hash) const;

    void RemoveContaining(cell_t p);

    void RemoveAllBetween(cell_t a, cell_t b);

    void TransferEndpoints(cell_t from, cell_t to, const Board& brd);

    class IteratorPair
    {
    public:
        IteratorPair(cell_t a, cell_t b, const SemiTable* st)
            : m_st(st) 
            , m_slot(SlotIndex(Hash(a) ^ Hash(b)))
            , m_index(-1)
            , m_a(a)
            , m_b(b)
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
                if ((semi.m_p1 == m_a && semi.m_p2 == m_b) ||
                    (semi.m_p1 == m_b && semi.m_p2 == m_a))
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

private:

    SgArrayList<int, MAX_ENTRIES_PER_SLOT> m_end_table[NUM_SLOTS];
    SgArrayList<int, MAX_ENTRIES_PER_SLOT> m_hash_table[NUM_SLOTS];
    SgArrayList<SemiConnection, MAX_ENTRIES_IN_TABLE> m_entries;
    SgArrayList<int, MAX_ENTRIES_IN_TABLE> m_freelist;
    SgArrayList<int, MAX_ENTRIES_IN_TABLE> m_usedlist;

    void Remove(int index);
};
