#include "Groups.h"
#include "board.h"

Group::Group()
{ 
}

std::string Group::BlocksToString() const
{
    std::ostringstream os;
    os << "[";
    for (int i = 0; i < m_blocks.Length(); ++i) {
        if (i) os << ", ";
        os << ConstBoard::ToString(m_blocks[i]);
    }
    os << "]";
    return os.str();
}

std::string Group::CarrierToString() const
{
    std::ostringstream os;
    os << "[";
    int cnt = 0;
    for (MarkedCells::Iterator i(m_carrier); i; ++i) {
        if (cnt) os << ", ";
        os << ConstBoard::ToString(*i);
        ++cnt;
    }
    os << "]";
    return os.str();
}

std::string Group::ToString() const
{
    std::ostringstream os;
    os << "[id=" << m_id
       << " parent=" << m_parent
       << " block=" << m_block
       << " left=" << m_left
       << " right=" << m_right
       << " semi1=" << YUtil::HashString(m_semi1)
       << " semi2=" << YUtil::HashString(m_semi2)
       << " border=" << ConstBoard::BorderToString(m_border)
       << " blocks=" << BlocksToString()
       << " carrier=" << CarrierToString()
       << "]";
    return os.str();
}

//---------------------------------------------------------------------------

Groups::Groups(const SemiTable& semis)
    : m_semis(semis)
{
    for (int i = Y_MAX_CELL-1; i >= ConstBoard::FIRST_NON_EDGE; --i)
        m_freelist.PushBack(i);
}

cell_t Groups::ObtainID()
{
    assert(!m_freelist.IsEmpty());
    assert(m_freelist.Length() + m_groups.Length() == MAX_GROUPS);
    cell_t id = m_freelist.Last();
    m_freelist.PopBack();
    m_rootGroups.PushBack(id);
    return id;
}


Group* Groups::GetGroup(cell_t p)
{
    if (ConstBoard::IsEdge(p))
        return &m_groupData[p];
    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        Group* g = &m_groupData[m_rootGroups[i]];
        if (g->m_blocks.Contains(p))
            return g;
    }
    return NULL;
}

cell_t Groups::CreateSingleBlockGroup(const Block* b)
{
    cell_t id = SetGroupDataFromBlock(b, ObtainID());
    m_rootGroups.PushBack(id);
    return id;
}

cell_t Groups::SetGroupDataFromBlock(const Block* b, int id)
{
    Group* g = &m_groupData[id];
    g->m_id = id;
    g->m_parent = g->m_left = g->m_right = Group::NULL_GROUP;
    g->m_semi1 = g->m_semi2 = 0;
    g->m_border = b->m_border;
    g->m_block = b->m_anchor;
    g->m_carrier.Clear();
    g->m_blocks.Clear();
    g->m_blocks.PushBack(b->m_anchor);
    return id;
}

void Groups::RecomputeFromChildren(Group* g)
{
    const Group* left  = GetGroupById(g->m_left);
    const Group* right = GetGroupById(g->m_right);
    g->m_border = left->m_border | right->m_border;
    g->m_blocks = left->m_blocks;
    g->m_blocks.PushBackList(right->m_blocks);
    g->m_carrier = g->m_con_carrier;
    g->m_carrier.Mark(left->m_carrier);
    g->m_carrier.Mark(right->m_carrier);
    if (g->m_parent != Group::NULL_GROUP)
        RecomputeFromChildren(GetGroupById(g->m_parent));
}

void Groups::Detach(Group* g)
{
    if (g->IsTopLevel()) {
        m_rootGroups.Exclude(g->m_id);
        return;
    }
    // g's parent will now have only a single child, so move
    // g's sibling up into parent's position
    Group* p = GetGroupById(g->m_parent);
    cell_t sibling = SiblingID(p, g->m_id);
    m_freelist.PushBack(g->m_parent);  // parent always dies
    GetGroupById(sibling)->m_parent = p->m_parent;
    if (p->IsTopLevel()) {
        m_rootGroups.Exclude(g->m_parent);
        m_rootGroups.PushBack(sibling);
        return;
    }
    Group* gp = GetGroupById(p->m_parent);
    gp->ChangeChild(g->m_parent, sibling);
    RecomputeFromChildren(gp);
}

void Groups::ComputeConnectionCarrier(Group* g)
{
    g->m_con_carrier.Clear();
    for (SemiConnection::Iterator it(m_semis.Lookup(g->m_semi1).m_carrier);
         it; ++it) { g->m_con_carrier.Mark(*it); }
    for (SemiConnection::Iterator it(m_semis.Lookup(g->m_semi2).m_carrier);
         it; ++it) { g->m_con_carrier.Mark(*it); }
}

// Create merged group in g1's location after remove g2 from the tree
cell_t Groups::Merge(Group* g1, Group* g2,
                     const SemiConnection& s1, const SemiConnection& s2)
{
    cell_t id = ObtainID();
    Group* g = GetGroupById(id);
    g->m_id = id;

    Detach(g2);

    GetGroupById(g1->m_parent)->ChangeChild(g1->m_id, id);
    g1->m_parent = id;
    g2->m_parent = id;
    g->m_parent = g1->m_parent;
    g->m_left = g1->m_id;
    g->m_right = g2->m_id;
    g->m_semi1 = s1.m_hash;
    g->m_semi2 = s2.m_hash;
    ComputeConnectionCarrier(g);
    RecomputeFromChildren(g);
    return id;
}

bool Groups::CanMerge(const Group* ga, const Group* gb, 
                      SemiConnection& x, SemiConnection& y)
{
    for (Group::BlockIterator ia(*ga); ia; ++ia) {
        cell_t xa = *ia;
        for (Group::BlockIterator ib(*gb); ib; ++ib) {
            cell_t xb = *ib;
            for (SemiTable::IteratorPair xit(xa,xb,&m_semis); xit; ++xit) {
                x = *xit;
                if (x.Intersects(ga->m_carrier) || x.Intersects(gb->m_carrier))
                    continue;
 
                for (Group::BlockIterator ja(*ga, ia.Index()); ja; ++ja) {
                    cell_t ya = *ja;
                    for (Group::BlockIterator jb(*gb, ib.Index()); jb; ++jb) {
                        cell_t yb = *jb;
                        // if yb is an edge: check if it is already in ga
                        if (   ConstBoard::IsEdge(yb) 
                            && ga->ContainsBorder(yb))
                            continue;
                        for (SemiTable::IteratorPair yit(ya,yb,&m_semis); 
                             yit; ++yit) 
                        {
                            y = *yit;
                            if (x == y)
                                continue;
                            if (   !y.Intersects(ga->m_carrier)
                                && !y.Intersects(gb->m_carrier)
                                && !y.Intersects(x))
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

void Groups::RestructureAfterMove(Group* g, cell_t p, const Board& brd)
{
    Group* left = GetGroupById(g->m_left);
    Group* right = GetGroupById(g->m_right);
    if (left->m_carrier.Marked(p))
        RestructureAfterMove(left, p, brd);
    else if (right->m_carrier.Marked(p))
        RestructureAfterMove(right, p, brd);
    else {
        assert(g->m_con_carrier.Marked(p));
        SemiConnection x, y;
        if (CanMerge(left, right, x, y)) {
            g->m_semi1 = x.m_hash;
            g->m_semi2 = y.m_hash;
            ComputeConnectionCarrier(g);
            RecomputeFromChildren(g);
        } else {
            Detach(g);
            m_freelist.PushBack(g->m_id);

            FindGroupToMergeWith(left, brd);
            FindGroupToMergeWith(right, brd);
        }
    }
}

void Groups::FindGroupToMergeWith(Group* g, const Board& brd)
{
    SemiConnection x, y;
    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        // FIXME: make sure they are the same color!
        Group* other = GetGroupById(m_rootGroups[i]);
        if (CanMerge(other, g, x, y)) {
            Merge(other, g, x, y);
            return;
        }
    }
    m_rootGroups.PushBack(g->m_id);
    g->m_parent = Group::NULL_GROUP;
}

void Groups::RestructureAfterMove(cell_t p, const Board& brd)
{
    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        Group* g = GetGroupById(m_rootGroups[i]);
        if (g->m_carrier.Marked(p)) {
            RestructureAfterMove(g, p, brd);
        }
    }
}

//---------------------------------------------------------------------------
