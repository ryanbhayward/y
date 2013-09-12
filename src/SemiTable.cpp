#include "SemiTable.h"

uint32_t SemiTable::s_cell_hash[Y_MAX_CELL];

SemiTable::SemiTable()
{
    for (int i = MAX_ENTRIES_IN_TABLE-1; i >= 0; --i)
        m_freelist.PushBack(i);
}

void SemiTable::Include(const SemiConnection& s)
{
    int hslot = SlotIndex(s.m_hash);
    if (m_hash_table[hslot].Length() >= MAX_ENTRIES_PER_SLOT)
        throw YException("Hash list is full!");
    for (int i = 0; i < m_hash_table[hslot].Length(); ++i)
        if (m_entries[ m_hash_table[hslot][i] ].m_hash == s.m_hash)
            return;     // Duplicate!
    int eslot = SlotIndex(HashEndpoints(s));
    if (m_end_table[eslot].Length() >= MAX_ENTRIES_PER_SLOT)
        throw YException("Endpoint list is full!");
    if (m_freelist.IsEmpty())
        throw YException("SemiTable is full!!!");
    int index = m_freelist.Last();
    m_freelist.PopBack();
    m_usedlist.PushBack(index);
    m_end_table[eslot].PushBack(index);
    m_hash_table[hslot].PushBack(index);
    std::cerr << "index=" << index << '\n';
    m_entries[index] = s;
    m_newlist.push_back(&m_entries[index]);
    std::cerr << "ADDED: " << s.ToString() << ' ' 
              << YUtil::HashString(s.m_hash) << '\n';
}

int32_t SemiTable::HashToIndex(uint32_t hash) const
{
    int hslot = SlotIndex(hash);        
    for (int i = 0; i < m_hash_table[hslot].Length(); ++i)
        if (m_entries[ m_hash_table[hslot][i] ].m_hash == hash)
            return m_hash_table[hslot][i];
    assert(false);
    return -1;
}

const SemiConnection& SemiTable::LookupHash(uint32_t hash) const
{
    int hslot = SlotIndex(hash);        
    for (int i = 0; i < m_hash_table[hslot].Length(); ++i) {
        const SemiConnection& s = m_entries[ m_hash_table[hslot][i] ];
        if (s.m_hash == hash)
            return s;
    }
    throw YException() << "Lookup failed on hash!! (" << hash << ")\n";
    return m_entries[0];  // to avoid compilation error
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
    m_worklist = m_usedlist;
    for (int i = 0; i < m_worklist.Length(); ++i) {
        const SemiConnection& s = m_entries[m_worklist[i]];
        if (s.Contains(p)) {
            Remove(m_worklist[i]);
        }
    }
}

void SemiTable::RemoveAllBetween(cell_t a, cell_t b)
{
    for (IteratorPair it(a,b,this); it; ++it) {
        Remove(it.Index());
    }
}

void SemiTable::TransferEndpoints(cell_t from, cell_t to)
{
    for (int i = 0; i < m_usedlist.Length(); ++i) {
        const int index = m_usedlist[i];
        SemiConnection& s = m_entries[index];
        if (s.m_p1 == from || s.m_p2 == from) {
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
