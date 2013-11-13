#include "SemiTable.h"
#include "Groups.h"

uint32_t SemiTable::s_cell_hash[Y_MAX_CELL];

//---------------------------------------------------------------------------

SemiConnection::SemiConnection()
    : m_group_id(-1)
{ }

SemiConnection::SemiConnection(cell_t p1, cell_t p2, cell_t key, 
                               const Carrier& carrier)
    : m_p1(std::min(p1, p2))
    , m_p2(std::max(p1, p2))
    , m_key(key)
    , m_group_id(-1)
    , m_carrier(carrier)
    , m_hash(  SemiTable::Hash(p1) 
               ^ SemiTable::Hash(p2) 
               ^ SemiTable::Hash(carrier))
{ }

//---------------------------------------------------------------------------

SemiTable::SemiTable()
    : m_using_worklist(false)
{
    for (int i = MAX_ENTRIES_IN_TABLE-1; i >= 0; --i)
        m_freelist.PushBack(i);
}

void SemiTable::Include(const SemiConnection& s)
{
    // Abort include if s is a superset of an existing connection.
    // Make a list of all existing connections that are supsersets of s.
    BEGIN_USING_WORKLIST;
    m_worklist.Clear();
    int eslot = SlotIndex(HashEndpoints(s));
    for (int i = 0; i < m_end_table[eslot].Length(); ++i) {
        const int index = m_end_table[eslot][i];
        const SemiConnection& other = m_entries[ index ];
        if (other.SameEndpoints(s)) {
            if (other.IsCarrierSubsetOf(s)) {
                // std::cerr << "############### SKIPPING SUPERSET ####\n";
                // std::cerr << "s: " << s.ToString() << '\n';
                // std::cerr << "o: " << other.ToString() << '\n';
                FINISH_USING_WORKLIST;
                return;
            }
            else if (s.IsCarrierSubsetOf(other)) {
                m_worklist.PushBack(index);
            }
        }
    }

    // If an existing superset of s is used in a group we can replace
    // it with s; otherwise, just free all supersets.
    int replace_index = -1;
    for (int i = 0; i < m_worklist.Length(); ++i) {
        const int index = m_worklist[i];
        SemiConnection& other = m_entries[ index ];
        if (other.m_group_id != -1) {
            // Only be possible to do this once: If a semi from
            // this list is used in a group connection, then the
            // endpoints of this list are in the same group, hence,
            // there are exactly two disjoint semis from this list
            // used in the group connection. Hence if s is a subset of
            // semi1 it cannot be a subset of semi2 (and vice versa).
            assert(replace_index == -1);
            replace_index = index;
        } else {
            Remove(index);
            Exclude(m_newlist, &other);
        }
    }
    FINISH_USING_WORKLIST;

    // Replace an existing semi
    if (replace_index != -1) 
    {
        SemiConnection& other = m_entries[ replace_index ];

        // Move it into its new hash slot
        m_hash_table[SlotIndex(other.m_hash)].Exclude(replace_index);
        int hslot = SlotIndex(s.m_hash);
        if (m_hash_table[hslot].Length() >= MAX_ENTRIES_PER_SLOT)
            throw YException("Hash list is full!");
        m_hash_table[hslot].PushBack(replace_index);

        int group_id = other.m_group_id;
        cell_t type = other.m_con_type;
        
        std::cerr << "Replacing SemiConnection!\n"
                  << "gid: " << group_id << " type: " << type << "s: "
                  << s.ToString() << '\n';
        
        m_entries[replace_index] = s;
        m_entries[replace_index].m_group_id = group_id;
        m_entries[replace_index].m_con_type = type;

        // FIXME: add it to the new list of semis??!

        // Shrink all groups above g
        // FIXME: Is this okay? Should I really allow SemiTable
        // access to private stuff in Groups?
        Group* g = m_groups->GetGroupById(group_id);
        m_groups->ComputeConnectionCarrier(g, type);
        m_groups->RecomputeFromChildrenToTop(g);

        std::cerr << "index=" << replace_index << '\n';
        std::cerr << "REPLACED: " << s.ToString() << ' ' 
                  << YUtil::HashString(s.m_hash) << '\n';

    }
    // Find room for s
    else 
    {
        if (m_freelist.IsEmpty())
            throw YException("SemiTable is full!!!");
        if (m_end_table[eslot].Length() >= MAX_ENTRIES_PER_SLOT)
            throw YException("Endpoint list is full!");
        int hslot = SlotIndex(s.m_hash);
        if (m_hash_table[hslot].Length() >= MAX_ENTRIES_PER_SLOT)
            throw YException("Hash list is full!");
        
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
    if (s.m_group_id != -1) {
        std::cerr << "Removing " << index << '\n';
        std::cerr << "GroupId = " << (int)s.m_group_id << '\n';
        m_groups->GetGroupById(s.m_group_id)
            ->BreakConnection(index, s.m_con_type);
    }
    m_end_table[SlotIndex(HashEndpoints(s))].Exclude(index);
    m_hash_table[SlotIndex(s.m_hash)].Exclude(index);
    m_freelist.PushBack(index);
    m_usedlist.Exclude(index);
}

void SemiTable::RemoveContaining(cell_t p)
{
    BEGIN_USING_WORKLIST;
    m_worklist = m_usedlist;
    for (int i = 0; i < m_worklist.Length(); ++i) {
        const SemiConnection& s = m_entries[m_worklist[i]];
        if (s.Contains(p)) {
            Remove(m_worklist[i]);
        }
    }
    FINISH_USING_WORKLIST;
}

void SemiTable::RemoveAllBetween(cell_t a, cell_t b)
{
    int eslot = SlotIndex(Hash(a) ^ Hash(b));
    SlotSizeList m_worklist(m_end_table[eslot]);
    for (int i = 0; i < m_worklist.Length(); ++i) {
        const int index = m_worklist[i];
        const SemiConnection& s = m_entries[ index ];
        if (s.SameEndpoints(a, b)) {
            Remove(index);
        }
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
            s.ReplaceEndpoint(from, to);
            m_end_table[SlotIndex(HashEndpoints(s))].Include(index);
            m_hash_table[SlotIndex(s.m_hash)].Include(index);
        }
    }
}

std::string SemiTable::ToString() const
{
    std::ostringstream os;
    for (int i = 0; i < m_usedlist.Length(); ++i) {
        const int index = m_usedlist[i];
        const SemiConnection& s = m_entries[index];
        os << std::setw(3) << index 
           << ' ' << YUtil::HashString(s.m_hash) 
           << ' ' << std::setw(3) << (int)s.m_group_id 
           << ' ' << (int)s.m_con_type
           << ' ' << s.ToString() << '\n';
    }
    return os.str();
}

#if 0
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
#endif
