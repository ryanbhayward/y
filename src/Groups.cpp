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
    g->m_con_carrier.Clear();
    g->m_blocks.Clear();
    if (!ConstBoard::IsEdge(b->m_anchor))
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
    if (ConstBoard::IsEdge(g->m_id))
        return;
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

// Create merged group in g1's location g1 down and creating
// a new internal node with g1 and g2 as children.
// NOTE: g1 must be a valid group (never be an edge group!)
// NOTE: g2 needs to be a free group or an edge group.
// NOTE: we are always merging g1 and g2 at the root of both trees.
// We could examine the two endpoints of s1 and s2 that occur in g1
// and descend down g1 until they stradle g1->left and g1->right.
// This attaches g2 'locally' in g1... not sure of the advantages of
// this, if there are any.
cell_t Groups::Merge(Group* g1, Group* g2,
                     const SemiConnection& s1, const SemiConnection& s2)
{
    cell_t id = ObtainID();
    Group* g = GetGroupById(id);
    g->m_id = id;

    if (g1->IsTopLevel()) {
        m_rootGroups.Exclude(g->m_id);
        m_rootGroups.PushBack(id);
    } else {
        GetGroupById(g1->m_parent)->ChangeChild(g1->m_id, id);
    }

    g1->m_parent = id;
    if (!ConstBoard::IsEdge(g2->m_id))
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

bool Groups::CanMergeOnSemi(const Group* ga, const Group* gb,
                            const SemiConnection& x, 
                            const SemiConnection** outy) const
{
    if (x.Intersects(ga->m_carrier) || x.Intersects(gb->m_carrier))
        return false;
    for (Group::BlockIterator ja(*ga); ja; ++ja) {
        cell_t ya = *ja;
        for (Group::BlockIterator jb(*gb); jb; ++jb) {
            cell_t yb = *jb;
            if (ConstBoard::IsEdge(yb) && ga->ContainsBorder(yb))
                continue;
            if (ConstBoard::IsEdge(ya) && gb->ContainsBorder(ya))
                continue;
            for (SemiTable::IteratorPair yit(ya,yb,&m_semis); yit; ++yit) 
            {
                const SemiConnection& y = *yit;
                if (x == y)
                    continue;
                if (      !y.Intersects(ga->m_carrier)
                       && !y.Intersects(gb->m_carrier)
                       && !y.Intersects(x))
                {
                    *outy = &y;
                    return true;
                }
            }
        }
    }
    return false;
}

void Groups::ProcessNewSemis(const Block* block,
                             std::vector<const SemiConnection*> s)
{
    cell_t bx = block->m_anchor;
    Group* ga = GetGroup(bx);
    for (size_t i = 0; i < s.size(); ++i) {
        const SemiConnection* y;
        const SemiConnection* x = s[0];
        assert(x->m_p1 == bx || x->m_p2 == bx);
        const cell_t ox = (x->m_p1 == bx ? x->m_p2 : x->m_p1);
        if (ga->ContainsBlock(ox))
            continue;
        Group* gb = GetGroup(ox);
        if (CanMergeOnSemi(ga, gb, *x, &y)) {
            Detach(gb);
            Merge(ga, gb, *x, *y);
        }
    }
}

bool Groups::CanMerge(const Group* ga, const Group* gb, 
                      const SemiConnection** outx, 
                      const SemiConnection** outy) const
{
    for (Group::BlockIterator ia(*ga); ia; ++ia) {
        cell_t xa = *ia;
        for (Group::BlockIterator ib(*gb); ib; ++ib) {
            cell_t xb = *ib;
            for (SemiTable::IteratorPair xit(xa,xb,&m_semis); xit; ++xit) {
                const SemiConnection& x = *xit;
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
                            const SemiConnection& y = *yit;
                            if (x == y)
                                continue;
                            if (   !y.Intersects(ga->m_carrier)
                                && !y.Intersects(gb->m_carrier)
                                && !y.Intersects(x))
                            {
                                *outx = &x;
                                *outy = &y;
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
        const SemiConnection *x, *y;
        if (CanMerge(left, right, &x, &y)) {
            g->m_semi1 = x->m_hash;
            g->m_semi2 = y->m_hash;
            ComputeConnectionCarrier(g);
            RecomputeFromChildren(g);
        } else {
            Detach(g);
            m_freelist.PushBack(g->m_id);

            // Note: since g is detached, left and right are also
            // detached as well.
            FindGroupToMergeWith(left, brd);
            FindGroupToMergeWith(right, brd);
        }
    }
}

// NOTE: g must be a detached group
void Groups::FindGroupToMergeWith(Group* g, const Board& brd)
{
    // Edges can appear as leafs as placeholders (possibly in several
    // root groups), but they are not real groups so we do no work
    // here.
    if (ConstBoard::IsEdge(g->m_id))
        return;
    const SemiConnection *x, *y;
    const SgBlackWhite color = brd.GetColor(g->m_id);
    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        Group* other = GetGroupById(m_rootGroups[i]);
        if (brd.GetColor(other->m_id)==color && CanMerge(other, g, &x, &y)) {
            Merge(other, g, *x, *y);
            return;
        }
    }
    // Could find no root level group to merge with, so create a new one
    m_rootGroups.PushBack(g->m_id);
    g->m_parent = Group::NULL_GROUP;
}

void Groups::RestructureAfterMove(cell_t p, SgBlackWhite color, 
                                  const Board& brd)
{
    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        Group* g = GetGroupById(m_rootGroups[i]);
        if (brd.GetColor(g->m_blocks[0]) == color && g->m_carrier.Marked(p))
            RestructureAfterMove(g, p, brd);
    }
}

//---------------------------------------------------------------------------
