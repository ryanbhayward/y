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
    bool first = true;
    for (Group::BlockAndEdgeIterator i(*this); i; ++i) {
        if (!first) os << ", ";
        os << ConstBoard::ToString(*i);
        first = false;
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
       << " left=" << Group::IDToString(m_left)
       << " right=" << Group::IDToString(m_right)
       << " con=" << m_con.ToString();
    for (EdgeIterator e; e; ++e) {
        if (m_econ[*e].IsDefined())
            os << " con" << ConstBoard::ToString(*e) 
               << "=" << m_econ[*e].ToString();
    }
    os << " border=" << ConstBoard::BorderToString(m_border)
       << " blocks=" << BlocksToString()
       << " carrier=" << CarrierToString()
       << "]";
    return os.str();
}

//---------------------------------------------------------------------------

Groups::Groups(SemiTable& semis)
    : m_detaching(false)
    , m_semis(semis)
{
    for (int i = Y_MAX_CELL-1; i >= ConstBoard::FIRST_NON_EDGE; --i)
        m_freelist.PushBack(i);
}

cell_t Groups::ObtainID()
{
    assert(!m_freelist.IsEmpty());
    cell_t id = m_freelist.Last();
    m_freelist.PopBack();
    return id;
}

Group* Groups::GetRootGroup(Group* g) 
{
    while (g->m_parent != SG_NULLMOVE) {
        g = GetGroupById(g->m_parent);
    }
    assert(IsRootGroup(g->m_id));
    return g;
}

Group* Groups::GetRootGroup(cell_t p)
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
    if (g->m_left != (cell_t)SG_NULLMOVE) {
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
    else {
        Group::BlockAndEdgeIterator it(*g);
        os << ConstBoard::ToString(*it);
    }
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
    g->m_con.Clear();
    g->m_border = b->m_border;
    g->m_carrier.Clear();
    g->m_blocks.Clear();
    if (!ConstBoard::IsEdge(b->m_anchor))
        g->m_blocks.PushBack(b->m_anchor);
    return id;
}

void Groups::UnlinkEdgeConnections(Group* g)
{
    for (EdgeIterator e; e; ++e) {
        if (g->m_econ[*e].IsDefined()) {
            UnlinkSemis(g->m_econ[*e]);
            g->m_econ[*e].Clear();
        }
    }
}

void Groups::Free(Group* g)
{ 
    UnlinkSemis(g->m_con);
    UnlinkEdgeConnections(g);
    m_freelist.PushBack(g->m_id);
}

void Groups::UnlinkSemis(const FullConnection& con)
{
    if (con.m_semi1 != -1) {
        m_semis.LookupIndex(con.m_semi1).m_group_id = -1;
    }
    if (con.m_semi2 != -1) {
        m_semis.LookupIndex(con.m_semi2).m_group_id = -1;
    }
}

void Groups::ChangeSemiGID(const FullConnection& con, int new_gid)
{
    if (con.m_semi1 != -1)
        m_semis.LookupIndex(con.m_semi1).m_group_id = new_gid;
    if (con.m_semi2 != -1)
        m_semis.LookupIndex(con.m_semi2).m_group_id = new_gid;
}

void Groups::SetSemis(Group* g, cell_t t, 
                      SemiConnection* s1, SemiConnection* s2)
{
    // This semi cannot be used by anybody else
    // (could already be set to this group if we are reconnecting
    // the group in RestructureAfterMove().
    assert(s1->m_group_id == -1 || s1->m_group_id == g->m_id);
    assert(s2->m_group_id == -1 || s2->m_group_id == g->m_id);

    s1->m_group_id = g->m_id;
    s1->m_con_type = t;

    s2->m_group_id = g->m_id;
    s2->m_con_type = t;

    FullConnection& c = ConstBoard::IsEdge(t) ? g->m_econ[t] : g->m_con;
    c.m_semi1 = m_semis.HashToIndex(s1->m_hash);
    c.m_semi2 = m_semis.HashToIndex(s2->m_hash);
}

bool Groups::ConnectsToBothSemis(Group* g, const FullConnection& con)
{
    return g->ContainsSemiEndpoint(m_semis.LookupIndex(con.m_semi1))
        && g->ContainsSemiEndpoint(m_semis.LookupIndex(con.m_semi2));
}

void Groups::ComputeConnectionCarrier(Group* g, cell_t type)
{
    if (type == -1)
        ComputeConnectionCarrier(g->m_con);
    else
        ComputeConnectionCarrier(g->m_econ[type]);
}

void Groups::ComputeConnectionCarrier(FullConnection& con)
{
    con.m_carrier.Clear();
    const SemiConnection& s1 = m_semis.LookupIndex(con.m_semi1);
    for (SemiConnection::Iterator it(s1.m_carrier); it; ++it) 
        con.m_carrier.Mark(*it); 
    const SemiConnection& s2 = m_semis.LookupIndex(con.m_semi2);
    for (SemiConnection::Iterator it(s2.m_carrier); it; ++it) 
        con.m_carrier.Mark(*it);
}

void Groups::ComputeCarrier(Group* g, bool root)
{
    g->m_carrier.Clear();
    if (g->m_con.IsDefined()) {
        g->m_carrier = g->m_con.m_carrier;
        const Group* left  = GetGroupById(g->m_left);
        const Group* right = GetGroupById(g->m_right);
        g->m_carrier.Mark(left->m_carrier);
        g->m_carrier.Mark(right->m_carrier);
    }
    if (root) {
        for (EdgeIterator e; e; ++e)
            if (g->m_econ[*e].IsDefined())
                g->m_carrier.Mark(g->m_econ[*e].m_carrier);
    }
}

void Groups::RecomputeFromChildren(Group* g)
{
    assert(!ConstBoard::IsEdge(g->m_id));
    if (!g->IsLeaf()) {
        // Obtain border and block list from children
        const Group* left  = GetGroupById(g->m_left);
        const Group* right = GetGroupById(g->m_right);
        g->m_border = left->m_border | right->m_border;
        if (IsRootGroup(g->m_id)) {
            for (EdgeIterator e; e; ++e)
                if (g->m_econ[*e].IsDefined())
                    g->m_border |= ConstBoard::ToBorderValue(*e);
        }
        g->m_blocks = left->m_blocks;
        g->m_blocks.PushBackList(right->m_blocks);
    }
    ComputeCarrier(g, IsRootGroup(g->m_id));
}

void Groups::RecomputeFromChildrenToTop(Group* g)
{
    RecomputeFromChildren(g);
    if (g->m_parent != SG_NULLMOVE)
        RecomputeFromChildrenToTop(GetGroupById(g->m_parent));
}

void Groups::PrintRootGroups()
{
    for (int i = 0; i < m_rootGroups.Length(); ++i)
        YTrace() << " " << (int)m_rootGroups[i];
    YTrace() << '\n';
}

void Groups::Detach(Group* g, Group* p)
{
    assert(m_detaching);
    assert(!IsRootGroup(g->m_id));
    assert(!ConstBoard::IsEdge(g->m_id));
    //YTrace() << "Detach: g=" << (int)g->m_id << " p=" << (int)p->m_id << '\n';

    assert(g->m_parent == p->m_id);
    m_detached.PushBack(g->m_id);
    g->m_parent = SG_NULLMOVE;

    // g's parent will now have only a single child, so tentatively
    // make sibling a child of grandparent
    cell_t sid = SiblingID(p, g->m_id);
    assert(!ConstBoard::IsEdge(sid));
    Group* sibling = GetGroupById(sid);
    sibling->m_parent = p->m_parent;

    // p must now die... poor p, so noble, so brave...
    m_recomputeEdgeCon.Exclude(p->m_id);
    if (IsRootGroup(p->m_id)) {
        m_rootGroups.Exclude(p->m_id);
        m_rootGroups.PushBack(sid);
        m_recomputeEdgeCon.Include(sid);
        Free(p);
        return;
    }
    Group* gp = GetGroupById(p->m_parent);
    gp->ChangeChild(p->m_id, sid);
    m_recomputeEdgeCon.Include(GetRootGroup(p)->m_id);
    Free(p);

    RecomputeFromChildren(gp);
    // If both semis in gp do not connect to sibling, then sibling
    // cannot really be a child of gp. Fix this by detaching sibling
    // from gp.
    if (!ConnectsToBothSemis(sibling, gp->m_con))
    {
        Detach(sibling, gp);
    } 
    else
    {
        // Sibling is a valid child of gp. Need to check that
        // the removal of g didn't break connections above gp.
        CheckStructure(gp);
    }
}

void Groups::CheckStructure(Group* p)
{
    assert(!ConstBoard::IsEdge(p->m_id));
    if (!IsRootGroup(p->m_id)) 
    {
        Group* gp = GetGroupById(p->m_parent);
        if (ConnectsToBothSemis(p, gp->m_con))
        {
            RecomputeFromChildren(gp);
            CheckStructure(gp);
        }
        else
            Detach(p, gp);
    }
}

void Groups::ConnectGroupToEdge(Group* g, cell_t edge, 
                                SemiConnection* s1, SemiConnection* s2)
{
    assert(IsRootGroup(g->m_id));
    assert(ConstBoard::IsEdge(edge));
    // These asserts make things safer, but cause problems in
    // RestructureAfterMove() (when we were already connected, etc).
    // assert((g->m_border & ConstBoard::ToBorderValue(edge)) == 0);
    // assert(!g->m_econ[edge].IsDefined());
    g->m_border |= ConstBoard::ToBorderValue(edge);
    SetSemis(g, edge, s1, s2);
    ComputeConnectionCarrier(g->m_econ[edge]);
    ComputeCarrier(g, true);
}                                 

bool Groups::CanConnectToEdgeOnSemi(Group* ga, cell_t edge,
                                    const SemiConnection& x, 
                                    SemiConnection** outy) const
{
    assert(!ConstBoard::IsEdge(ga->m_id));
    assert(ConstBoard::IsEdge(edge));

    if (x.Intersects(ga->m_carrier))
        return false;

    for (Group::BlockIterator ja(*ga); ja; ++ja) {
        cell_t ya = *ja;
        if (ConstBoard::IsEdge(ya))
            continue;

        for (SemiTable::IteratorPair yit(ya,edge,&m_semis); yit; ++yit) 
        {
            const SemiConnection& y = *yit;
            if (x == y)
                continue;
            if (    !y.Intersects(ga->m_carrier)
                 && !y.Intersects(x))
            {
                *outy = &const_cast<SemiConnection&>(y);
                return true;
            }
        }
    }
    return false;
}

// NOTE: g1 must be a valid root group
// NOTE: g2 needs to be a free group
// NOTE: merges g1 and g2 at the root of both trees
Group* Groups::Merge(Group* g1, Group* g2,
                     SemiConnection* s1, SemiConnection* s2)
{
    assert(!ConstBoard::IsEdge(g1->m_id));
    assert(g1->m_parent == SG_NULLMOVE);
    assert(g2->m_parent == SG_NULLMOVE);
    assert(!IsRootGroup(g2->m_id));
    assert(IsRootGroup(g1->m_id));

    RemoveEdgeConnections(g1);
    RemoveEdgeConnections(g2);
    m_rootGroups.Exclude(g1->m_id);
    
    cell_t id = ObtainID();
    Group* g = GetGroupById(id);
    m_rootGroups.PushBack(id);

    g->m_id = id;
    g->m_parent = SG_NULLMOVE;
    g->m_left = g1->m_id;
    g->m_right = g2->m_id;
    SetSemis(g, -1, s1, s2);
    ComputeConnectionCarrier(g->m_con);
    g1->m_parent = id;
    g2->m_parent = id;
    
    RecomputeFromChildren(g);
    ComputeEdgeConnections(g);
    return g;
}

bool Groups::CanMergeOnSemi(const Group* ga, const Group* gb,
                            const SemiConnection& x, 
                            SemiConnection** outy)
{
    assert(!ConstBoard::IsEdge(ga->m_id));
    assert(!ConstBoard::IsEdge(gb->m_id));

    if (x.Intersects(ga->m_carrier) || x.Intersects(gb->m_carrier))
        return false;

    for (Group::BlockIterator ja(*ga); ja; ++ja) {
        cell_t ya = *ja;
        for (Group::BlockIterator jb(*gb); jb; ++jb) {
            cell_t yb = *jb;
            for (SemiTable::IteratorPair yit(ya,yb,&m_semis); yit; ++yit) 
            {
                const SemiConnection& y = *yit;
                if (x == y)
                    continue;
                if (      !y.Intersects(ga->m_carrier)
                       && !y.Intersects(gb->m_carrier)
                       && !y.Intersects(x))
                {
                    *outy = &const_cast<SemiConnection&>(y);
                    return true;
                }
            }
        }
    }
    return false;
}

void Groups::ProcessNewSemis(const SgArrayList<SemiConnection*, 128>& s)
{
    for (int i = 0; i < s.Length(); ++i) {
        SemiConnection* y;
        SemiConnection* x = s[i];
        cell_t bx = x->m_p1;
        cell_t ox = x->m_p2;
        if (ConstBoard::IsEdge(bx))
            std::swap(bx, ox);
        assert(!ConstBoard::IsEdge(bx));
        Group* ga = GetRootGroup(bx);
        if (ga->ContainsBlock(ox))
            continue;
        if (ConstBoard::IsEdge(ox)) {
            if (CanConnectToEdgeOnSemi(ga, ox, *x, &y)) {
                ConnectGroupToEdge(ga, ox, x, y);
            }
        } else {
            Group* gb = GetRootGroup(ox);
            if (CanMergeOnSemi(ga, gb, *x, &y)) {
                m_rootGroups.Exclude(gb->m_id);
                Merge(ga, gb, x, y);
                ga = GetRootGroup(bx);  // Get the new group for block!!
            }
        }
    }
}

bool Groups::CanMerge(const Group* ga, const Group* gb, 
                      SemiConnection** outx, 
                      SemiConnection** outy,
                      const MarkedCells& avoid) 
{
    assert(!ConstBoard::IsEdge(ga->m_id));
    assert(!ConstBoard::IsEdge(gb->m_id));
    for (Group::BlockIterator ia(*ga); ia; ++ia) {
        cell_t xa = *ia;
        for (Group::BlockIterator ib(*gb); ib; ++ib) {
            cell_t xb = *ib;
            for (SemiTable::IteratorPair xit(xa,xb,&m_semis); xit; ++xit) {
                const SemiConnection& x = *xit;
                if (   x.Intersects(avoid)
                    || x.Intersects(ga->m_carrier) 
                    || x.Intersects(gb->m_carrier))
                    continue;
                
                for (Group::BlockIterator ja(*ga, ia.Index()); ja; ++ja) {
                    cell_t ya = *ja;
                    for (Group::BlockIterator jb(*gb, ib.Index()); jb; ++jb) {
                        cell_t yb = *jb;
                        for (SemiTable::IteratorPair yit(ya,yb,&m_semis); 
                             yit; ++yit) 
                        {
                            const SemiConnection& y = *yit;
                            if (x == y)
                                continue;
                            if (   !y.Intersects(avoid)
                                && !y.Intersects(ga->m_carrier)
                                && !y.Intersects(gb->m_carrier)
                                && !y.Intersects(x))
                            {
                                *outx = &const_cast<SemiConnection&>(x);
                                *outy = &const_cast<SemiConnection&>(y);
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

bool Groups::CanConnectToEdge(const Group* ga, cell_t edge, 
                              SemiConnection** outx, 
                              SemiConnection** outy,
                              const MarkedCells& avoid)
{
    assert(!ConstBoard::IsEdge(ga->m_id));
    assert(ConstBoard::IsEdge(edge));
    for (Group::BlockIterator ia(*ga); ia; ++ia) {
        cell_t xa = *ia;
        for (SemiTable::IteratorPair xit(xa,edge,&m_semis); xit; ++xit) {
            const SemiConnection& x = *xit;
            if (x.Intersects(avoid))
                continue;
            
            for (Group::BlockIterator ja(*ga, ia.Index()); ja; ++ja) {
                cell_t ya = *ja;
                for (SemiTable::IteratorPair yit(ya,edge,&m_semis); 
                     yit; ++yit) 
                {
                    const SemiConnection& y = *yit;
                    if (x == y)
                        continue;
                    if (!y.Intersects(avoid) && !y.Intersects(x))
                    {
                        *outx = &const_cast<SemiConnection&>(x);
                        *outy = &const_cast<SemiConnection&>(y);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void Groups::ComputeEdgeConnections(Group* g)
{
    assert(IsRootGroup(g->m_id));
    MarkedCells avoid(g->m_carrier);
    for (EdgeIterator e; e; ++e) {
        if (!g->TouchesEdge(*e) && !g->m_econ[*e].IsDefined()) {
            SemiConnection *x, *y;
            if (CanConnectToEdge(g, *e, &x, &y, avoid)) {
                ConnectGroupToEdge(g, *e, x, y);
                avoid = g->m_carrier;
            } 
        }
    }
}

void Groups::RestructureAfterMove(Group* g, cell_t p, const Board& brd)
{
    Group* root = g;
    MarkedCells avoid = root->m_carrier;
    if (IsRootGroup(g->m_id)) {
        for (EdgeIterator e; e; ++e) {
            if (   g->m_econ[*e].IsDefined() 
                && g->m_econ[*e].m_carrier.Marked(p)) 
            {
                assert(g->m_econ[*e].IsBroken());
                avoid.Unmark(g->m_econ[*e].m_carrier);
                SemiConnection *x, *y;
                if (CanConnectToEdge(g, *e, &x, &y, avoid)) {
                    // unlink the old remaining semi
                    UnlinkSemis(g->m_econ[*e]);
                    // make connection with new semis
                    ConnectGroupToEdge(g, *e, x, y);
                } else {
                    RemoveEdgeConnection(g, *e);
                }
                return;
            }
        }
    }
    assert(!g->IsLeaf());
    Group* left;
    Group* right;
    while (true) {
        left = GetGroupById(g->m_left);
        right = GetGroupById(g->m_right);
        if (left->m_carrier.Marked(p))
            g = left;
        else if (right->m_carrier.Marked(p))
            g = right;
        else
            break;
    }
    if (!g->m_con.IsBroken()) {
        std::cerr << brd.ToString() << '\n' << "p=" 
                  << ConstBoard::ToString(p) << '\n'
                  << brd.EncodeHistory() << '\n';

    }
    assert(g->m_con.IsBroken());
    assert(g->m_con.m_carrier.Marked(p));
    avoid.Unmark(g->m_con.m_carrier);

    // Try to immediately reform the group
    // FIXME: we know which semi was killed, we could try 
    // CanMergeOnSemi() with the remaining one first.
    SemiConnection *x, *y;
    if (CanMerge(left, right, &x, &y, avoid)) {
        UnlinkSemis(g->m_con);
        SetSemis(g, -1, x, y);
        ComputeConnectionCarrier(g->m_con);
        RecomputeFromChildrenToTop(g);
    } 
    else {
        if (IsRootGroup(g->m_id))
            Detach(right, g);
        else {
            // If both semis attach to left then move left into g's
            // position by detaching right. Otherwise, at least one
            // semi connects to right, so just detach left.
            Group* p = GetGroupById(g->m_parent);
            if (ConnectsToBothSemis(left, p->m_con))
            {
                Detach(right, g);
            } else {
                Detach(left, g);
            }
        }
    }
}

void Groups::RestructureAfterMove(cell_t p, const Board& brd)
{
    GroupList rootGroups(m_rootGroups);
    for (int i = 0; i < rootGroups.Length(); ++i) {
        Group* g = GetGroupById(rootGroups[i]);
        std::cerr << "group: " << (int)g->m_id << '\n';
        if (g->m_carrier.Marked(p)) {
            std::cerr << "in carrier!\n";
            BeginDetaching();
            RestructureAfterMove(g, p, brd);
            FinishedDetaching();
            for (int j = 0; j < m_recomputeEdgeCon.Length(); ++j) {
                Group* g2 = GetGroupById(m_recomputeEdgeCon[j]);
                // TODO: check edge connections for ones that no longer
                // exist and remove those.
                RemoveEdgeConnections(g2);
                ComputeEdgeConnections(g2);
            }
            for (int j = 0; j < m_detached.Length(); ++j) {
                Group* g2 = GetGroupById(m_detached[j]);
                std::cerr << "detached: " << (int)g2->m_id << '\n';
                m_rootGroups.PushBack(g2->m_id);
                ComputeEdgeConnections(g2);
            }
        }
    }
}

void Groups::UpdateBorderFromBlock(const Block* b)
{
    Group* g = GetRootGroup(b->m_anchor);
    while (true) {
        g->m_border |= b->m_border;
        if (g->m_left == SG_NULLMOVE)
            break;
        g = ChildContaining(g, b->m_anchor);
    }
}

//---------------------------------------------------------------------------

// PRE: g contains both a and b
Group* Groups::CommonAncestor(Group* g, cell_t a, cell_t b)
{
    Group* left = GetGroupById(g->m_left);
    if (left->ContainsBlock(a) && left->ContainsBlock(b))
        return CommonAncestor(left, a, b);
    Group* right = GetGroupById(g->m_right);
    if (right->ContainsBlock(a) && right->ContainsBlock(b))
        return CommonAncestor(right, a, b);
    return g;
}

Group* Groups::ChildContaining(Group* g, cell_t a)
{
    Group* left = GetGroupById(g->m_left);
    if (left->ContainsBlock(a))
        return left;
    return GetGroupById(g->m_right);
}

void Groups::RecursiveRelabel(Group* g, cell_t from, cell_t to)
{
    assert(!g->m_blocks.Contains(to));
    g->m_blocks.Exclude(from);
    g->m_blocks.PushBack(to);
    if (!g->IsLeaf())
        RecursiveRelabel(ChildContaining(g, from), from, to);
}

void Groups::FindParentOfBlock(Group* g, cell_t a, Group** p, Group** c)
{
    assert(g->ContainsBlock(a));
    assert(!g->IsLeaf());
    while (true) {
        Group* child = ChildContaining(g, a);
        if (child->IsLeaf()) {
            assert(child->m_blocks[0] == a);
            *p = g;
            *c = child;
            break;
        }
        g = child;
    }
}

void Groups::RemoveEdgeConnection(Group* g, cell_t edge)
{
    assert(ConstBoard::IsEdge(edge));
    if (g->m_econ[edge].IsDefined()) {
        g->m_border &= ~ConstBoard::ToBorderValue(edge);
        UnlinkSemis(g->m_econ[edge]);
        g->m_econ[edge].Clear();
        ComputeCarrier(g, IsRootGroup(g->m_id));
    }
}

void Groups::RemoveEdgeConnections(Group* g)
{
    for (EdgeIterator e; e; ++e)
        if (g->m_econ[*e].IsDefined())
            RemoveEdgeConnection(g, *e);
}

void Groups::ReplaceLeafWithGroup(Group* g, cell_t a, Group* z)
{
    assert(!g->IsLeaf());
    Group *p, *c;
    FindParentOfBlock(g, a, &p, &c);
    p->ChangeChild(c->m_id, z->m_id);
    z->m_parent = p->m_id;
    Free(c);
    RecomputeFromChildrenToTop(p);
}

void Groups::HandleBlockMerge(cell_t from, cell_t to)
{
    assert(from != to);
    assert(!ConstBoard::IsEdge(from));
    assert(!ConstBoard::IsEdge(to));
    Group* f = GetRootGroup(from);
    Group* t = GetRootGroup(to);

    if (f == t) {
        // Merging two blocks from the same group.  
        // Note: f cannot be a leaf since f contains at least two 
        // disjoint blocks.

        Group* a = CommonAncestor(f, to, from);
        Group* z = ChildContaining(a, from);
        Group* x = ChildContaining(a, to);
        RecursiveRelabel(z, from, to);
        if (f == a) {

            // Common ancestor is root-level group.
            // Detaching will result in a new root group.
            // Try to detach a leaf so the non-leaf child becomes
            // the root group. If both are leafs, then it doesn't
            // matter which we detach. 
            if (x->IsLeaf())
                std::swap(x, z);

            BeginDetaching();
            Detach(z, a);
            FinishedDetaching();

            // Only z should be in detached list
            assert(m_detached.Length() == 1);
            assert(m_detached[0] == z->m_id);
            // Only x should be on this list
            assert(m_recomputeEdgeCon.Length() == 1);
            assert(m_recomputeEdgeCon[0] == x->m_id);

            // X is now a root group.
            if (x->IsLeaf()) {
                assert(z->IsLeaf());
                Free(z);
            }
            else {
                ReplaceLeafWithGroup(x, to, z);
            }
            
            ComputeEdgeConnections(GetGroupById(m_recomputeEdgeCon[0]));

        } else { // (f != a)
            // Common ancestor is below root level.  
            // Since we are only reorganizing the tree edge
            // connections do not neet to be performed.

            if (x->IsLeaf())
                std::swap(x, z);
            
            // Move x into a's position and kill a
            assert(a->m_parent != -1);
            Group* p = GetGroupById(a->m_parent);
            p->ChangeChild(a->m_id, x->m_id);
            x->m_parent = p->m_id;
            Free(a);

            // z is now floating free. 
            if (x->IsLeaf()) {
                // Both are leafs, just get rid of z and we're done
                // (since they are leafs of the same block).
                assert(z->IsLeaf());
                Free(z);
                RecomputeFromChildrenToTop(p);
            } else {
                ReplaceLeafWithGroup(x, to, z);
                // ^^ Calls RecomputeToTop() to fix up carriers, etc.
            }
        }
    } else { // (f != t)
        // Merging blocks from two different groups.
        // If group carriers do not intersect, add one group under the other.
        // If they do intersect, remove the merged block from one group.
        RecursiveRelabel(f, from, to);
        if (f->IsLeaf())
            std::swap(f, t);
        if (f->IsLeaf()) {
            m_rootGroups.Exclude(t->m_id);
            Free(t);            
        } else {
            RemoveEdgeConnections(t);
            RemoveEdgeConnections(f);
            if (f->m_carrier.Intersects(t->m_carrier)) {
                // Detach 'to' from t, add all detached groups (except
                // the singleton group containing 'to') as new root
                // groups.  
                Group* p, *c;
                FindParentOfBlock(t, to, &p, &c);
                BeginDetaching();
                Detach(c, p);
                FinishedDetaching();
                for (int j = 0; j < m_recomputeEdgeCon.Length(); ++j) {
                    Group* g2 = GetGroupById(m_recomputeEdgeCon[j]);
                    RemoveEdgeConnections(g2);
                    ComputeEdgeConnections(g2);
                }
                for (int j = 0; j < m_detached.Length(); ++j) {
                    Group* g2 = GetGroupById(m_detached[j]);
                    if (g2->ContainsBlock(to)) {
                        // This is the group containing 'to' from t
                        // that we just detached. It must die because
                        // 'to' is actually inside f now, so it cannot
                        // exist on its own as well.
                        Free(g2);
                    } else {
                        // A remnant of t after 'to' was removed.
                        // Make it a root group.
                        m_rootGroups.PushBack(g2->m_id);
                        ComputeEdgeConnections(g2);
                    }
                }
            } else {
                // Groups are completely disjoint.
                // Replace the leaf 'to' in f with t.
                m_rootGroups.Exclude(t->m_id);
                ReplaceLeafWithGroup(f, to, t);
            }
        }
        ComputeEdgeConnections(f);
    }
}

//---------------------------------------------------------------------------

void Groups::ComputeBlockToGroupIndex(cell_t* bg, const Board& brd) const
{
    SG_UNUSED(brd);
    // for (CellIterator it(brd); it; ++it)
    //     bg[*it] = -1;
    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        const Group* g = GetGroupById(m_rootGroups[i]);
        for (Group::BlockList::Iterator it(g->m_blocks); it; ++it) {
            assert(!ConstBoard::IsEdge(*it));
            bg[*it] = g->m_id;
        }
    }
}

//---------------------------------------------------------------------------

void Groups::BeginDetaching()
{
    assert(!m_detaching);
    m_detaching = true;
    m_detached.Clear();
    m_recomputeEdgeCon.Clear();
}

void Groups::FinishedDetaching()
{
    m_detaching = false;
}

//---------------------------------------------------------------------------
