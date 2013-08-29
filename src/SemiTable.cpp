#include "SemiTable.h"

uint32_t SemiTable::s_cell_hash[Y_MAX_CELL];

void SemiTable::Add(const SemiConnection& s)
{
    if (m_freelist.IsEmpty())
        throw YException("SemiTable is full!!!");
    int eslot = SlotIndex(HashEndpoints(s));
    if (m_end_table[eslot].Length() >= MAX_ENTRIES_PER_SLOT)
        throw YException("Endpoint list is full!");
    int hslot = SlotIndex(s.m_hash);
    if (m_hash_table[hslot].Length() >= MAX_ENTRIES_PER_SLOT)
        throw YException("Hash list is full!");
    int index = m_freelist.Last();
    m_freelist.PopBack();
    m_usedlist.PushBack(index);
    m_entries[index] = s;
    m_end_table[eslot].PushBack(index);
    m_hash_table[hslot].PushBack(index);
}

bool SemiTable::Contains(const SemiConnection& s) const
{
    int hslot = SlotIndex(s.m_hash);        
    for (int i = 0; i < m_hash_table[hslot].Length(); ++i)
        if (m_entries[ m_hash_table[hslot][i] ].m_hash == s.m_hash)
            return true;
    return false;
}

void SemiTable::Remove(int index)
{
    const SemiConnection& s = m_entries[index];
    m_end_table[SlotIndex(HashEndpoints(s))].Exclude(index);
    m_hash_table[SlotIndex(s.m_hash)].Exclude(index);
    m_freelist.PushBack(index);
    m_usedlist.Exclude(index);
}

void SemiTable::RemoveContaining(cell_t p)
{
    for (int i = 0; i < m_usedlist.Length(); ++i) {
        const SemiConnection& s = m_entries[m_usedlist[i]];
        if (s.Contains(p)) {
            Remove(m_usedlist[i]);
        }
    }
}

void SemiTable::RemoveAllBetween(cell_t a, cell_t b)
{
    for (IteratorPair it(a,b,this); it; ++it) {
        Remove(it.Index());
    }
}

void SemiTable::TransferEndpoints(cell_t from, cell_t to, const Board& brd)
{
    for (IteratorSingle it(from, &brd, this); it; ++it) {
        int index = it.Index();
        SemiConnection& s = m_entries[index];
        m_end_table[SlotIndex(HashEndpoints(s))].Exclude(index);
        m_hash_table[SlotIndex(s.m_hash)].Exclude(index);
        if (s.m_p1 == from)
            s.m_p1 = to;
        if (s.m_p2 == from)
            s.m_p2 = to;
        s.m_hash = ComputeHash(s);
        m_end_table[SlotIndex(HashEndpoints(s))].Include(index);
        m_hash_table[SlotIndex(s.m_hash)].Include(index);
    }
}


void SemiTable::IteratorSingle::operator++()
{
    if (!m_iter) {
        const int T = m_brd->Const().TotalCells;
        while (m_b < T) {
            ++m_b;
            if (m_brd->GetColor(m_b) == m_brd->GetColor(m_a)
                && m_brd->IsBlockAnchor(m_b))
                break;
        }
        if (m_b == T)
            m_b = Y_MAX_CELL;
        else
            m_iter = IteratorPair(m_a, m_b, m_st);
    }
    else
        ++m_iter;
}
