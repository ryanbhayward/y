#include "Groups.h"
#include "board.h"

Group::Group()
{ 
}

bool Group::ContainsSemiEndpoint(const SemiConnection& s)
{
    return ContainsBlock(s.m_p1) || ContainsBlock(s.m_p2);
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
    os << "[id=" << Group::IDToString(m_id)
       << " parent=" << Group::IDToString(m_parent)
       << " block=" << ConstBoard::ToString(m_block)
       << " left=" << Group::IDToString(m_left)
       << " right=" << Group::IDToString(m_right)
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

std::string Groups::Encode(const Group* g) const
{
    std::ostringstream os;
    os << '(';
    if (g->m_block == (cell_t)SG_NULLMOVE) {
        os << Encode(GetGroupById(g->m_left));
        os << '[';
        bool first = true;
        for (MarkedCells::Iterator it(g->m_carrier); it; ++it) {
            os << (first ? "" : " ") << ConstBoard::ToString(*it);
            first = false;
        }
        os << ']';
        os << Encode(GetGroupById(g->m_right));
    }
    else 
        os << ConstBoard::ToString(g->m_block);
    os << ')';
    return os.str();
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
    g->m_parent = g->m_left = g->m_right = SG_NULLMOVE;
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
    std::cerr << "gid=" << (int)g->m_id << '\n';
    const Group* left  = GetGroupById(g->m_left);
    const Group* right = GetGroupById(g->m_right);
    g->m_border = left->m_border | right->m_border;
    g->m_blocks = left->m_blocks;
    g->m_blocks.PushBackList(right->m_blocks);
    g->m_carrier = g->m_con_carrier;
    g->m_carrier.Mark(left->m_carrier);
    g->m_carrier.Mark(right->m_carrier);
}

void Groups::PrintRootGroups()
{
    for (int i = 0; i < m_rootGroups.Length(); ++i)
        std::cerr << " " << (int)m_rootGroups[i];
    std::cerr << '\n';
}

void Groups::Detach(Group* g, Group* p)
{
    if (!ConstBoard::IsEdge(g->m_id))
        m_detached.PushBack(g->m_id);

    if (IsRootGroup(g->m_id)) {
        PrintRootGroups();
        m_rootGroups.Exclude(g->m_id);
        std::cerr << "Excluded " << (int)g->m_id << '\n';
        PrintRootGroups();
        return;
    }
    // g's parent will now have only a single child, so move
    // g's sibling up into parent's position
    cell_t sid = SiblingID(p, g->m_id);
    Group* sibling = GetGroupById(sid);
    m_freelist.PushBack(p->m_id);  // parent always dies
    if (!ConstBoard::IsEdge(sid))
        sibling->m_parent = p->m_parent;
    if (IsRootGroup(p->m_id)) {
        m_rootGroups.Exclude(p->m_id);
        if (!ConstBoard::IsEdge(sid))
            m_rootGroups.PushBack(sid);
        return;
    }
    Group* gp = GetGroupById(p->m_parent);
    // Tentatively make sibling a child of gp
      gp->ChangeChild(p->m_id, sid);
    // If both semis in gp do not connect to sibling, then sibling
    // cannot really be a child of gp. Fix this by detaching sibling
    // from gp.
    if (   !sibling->ContainsSemiEndpoint(m_semis.Lookup(gp->m_semi1))
        || !sibling->ContainsSemiEndpoint(m_semis.Lookup(gp->m_semi2)))
    {
        Detach(sibling, gp);
    } 
    else
    {
        CheckStructure(gp);
    }
}

void Groups::CheckStructure(Group* p)
{
    if (IsRootGroup(p->m_id)) {
        RecomputeFromChildren(p);
        return;
    }
    Group* gp = GetGroupById(p->m_parent);
    if (   p->ContainsSemiEndpoint(m_semis.Lookup(gp->m_semi1))
        && p->ContainsSemiEndpoint(m_semis.Lookup(gp->m_semi2)))
    {
        RecomputeFromChildren(p);
        CheckStructure(gp);
    }
    else
        Detach(p, gp);
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

    if (IsRootGroup(g1->m_id)) {
        m_rootGroups.Exclude(g1->m_id);
        m_rootGroups.PushBack(id);
    } else {
        GetGroupById(g1->m_parent)->ChangeChild(g1->m_id, id);
    }

    g->m_parent = g1->m_parent;
    g1->m_parent = id;
    if (!ConstBoard::IsEdge(g2->m_id))
        g2->m_parent = id;

    g->m_left = g1->m_id;
    g->m_right = g2->m_id;
    g->m_semi1 = s1.m_hash;
    g->m_semi2 = s2.m_hash;

    std::cerr << "m.s1: " << s1.ToString() << ' ' 
              << YUtil::HashString(s1.m_hash)<< '\n';
    std::cerr << "m.s2: " << s2.ToString() << ' ' 
              << YUtil::HashString(s2.m_hash)<< '\n';
   
    g->m_block = (cell_t)SG_NULLMOVE;
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
        const SemiConnection* x = s[i];
        std::cerr << x->ToString() << '\n';
        assert(x->m_p1 == bx || x->m_p2 == bx);
        const cell_t ox = (x->m_p1 == bx ? x->m_p2 : x->m_p1);
        if (ga->ContainsBlock(ox))
            continue;
        Group* gb = GetGroup(ox);
        if (CanMergeOnSemi(ga, gb, *x, &y)) {
            m_rootGroups.Exclude(gb->m_id);
            std::cerr << "MERGE: " << y->ToString() << '\n';
            Merge(ga, gb, *x, *y);
            ga = GetGroup(bx);  // Get the new group!!
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

// NOTE: g must be a detached group
bool Groups::Merge(Group* g, const GroupList& list, const Board& brd)
{
    assert(!ConstBoard::IsEdge(g->m_id));
    const SemiConnection *x, *y;
    const SgBlackWhite color = brd.GetColor(g->m_id);
    for (int i = 0; i < list.Length(); ++i) {
        Group* other = GetGroupById(m_rootGroups[i]);
        if (brd.GetColor(other->m_id)==color && CanMerge(other, g, &x, &y)) {
            Merge(other, g, *x, *y);
            return true;
        }
    }
    return false;
}

void Groups::RestructureAfterMove(Group* g, cell_t p, const Board& brd)
{
    Group* left = GetGroupById(g->m_left);
    Group* right = GetGroupById(g->m_right);
    if (left->m_carrier.Marked(p)) {
        RestructureAfterMove(left, p, brd);
        return;
    }
    else if (right->m_carrier.Marked(p)) {
        RestructureAfterMove(right, p, brd);
        return;
    }

    assert(g->m_con_carrier.Marked(p));
    const SemiConnection *x, *y;
    if (CanMerge(left, right, &x, &y)) {
        g->m_semi1 = x->m_hash;
        g->m_semi2 = y->m_hash;
        ComputeConnectionCarrier(g);
        RecomputeFromChildren(g);
        return;
    } 

    m_detached.Clear();
    if (IsRootGroup(g->m_id)) {
        Detach(right, g);
    } 
    else {
        // if both semis attach to left then
        // we can move left into g's position.
        Group* p = GetGroupById(g->m_parent);
        const SemiConnection& s1 = m_semis.Lookup(p->m_semi1);
        const SemiConnection& s2 = m_semis.Lookup(p->m_semi2);
        if (   left->ContainsSemiEndpoint(s1)
               && left->ContainsSemiEndpoint(s2)) 
        {
            Detach(right, g);
        } else {
            Detach(left, g);
        }
    }
}

void Groups::RestructureAfterMove(cell_t p, SgBlackWhite color, 
                                  const Board& brd)
{
    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        Group* g = GetGroupById(m_rootGroups[i]);
        if (g->m_carrier.Marked(p)) {
            RestructureAfterMove(g, p, brd);
            for (int i = 0; i < m_detached.Length(); ++i) {
                Group* g2 = GetGroupById(m_detached[i]);
                // Try merging detached groups only for the opponent,
                // we will rebuild the groups for color using new semi
                // connections later.
                if (brd.GetColor(g->m_blocks[0]) == color 
                    || !Merge(g2, m_rootGroups, brd)) 
                {
                    m_rootGroups.PushBack(g2->m_id);
                    g2->m_parent = SG_NULLMOVE;
                }
            }
        }
    }
}

//---------------------------------------------------------------------------
