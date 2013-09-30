#include "Groups.h"
#include "board.h"

Group::Group()
{ 
}

bool Group::ContainsSemiEndpoint(const SemiConnection& s)
{
    std::cerr << "ContainsSemiEndpoint: p1=" 
              << ConstBoard::ToString(s.m_p1)
              << " p2=" << ConstBoard::ToString(s.m_p2) << '\n';
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

void Groups::Free(Group* g)
{ 
    UnlinkSemis(g->m_con);
    m_freelist.PushBack(g->m_id);
}

void Groups::UnlinkSemis(const FullConnection& con)
{
    if (con.m_semi1 != -1)
        m_semis.LookupIndex(con.m_semi1).m_group_id = -1;
    if (con.m_semi2 != -1)
        m_semis.LookupIndex(con.m_semi2).m_group_id = -1;
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
    assert(g->m_left != SG_NULLMOVE);
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
    ComputeCarrier(g, IsRootGroup(g->m_id));
}

void Groups::RecomputeFromChildrenToTop(Group* g)
{
    std::cerr << "RecomputeToTop: id=" << (int)g->m_id << '\n';
    RecomputeFromChildren(g);
    if (g->m_parent != SG_NULLMOVE)
        RecomputeFromChildrenToTop(GetGroupById(g->m_parent));
}

void Groups::PrintRootGroups()
{
    for (int i = 0; i < m_rootGroups.Length(); ++i)
        std::cerr << " " << (int)m_rootGroups[i];
    std::cerr << '\n';
}

void Groups::Detach(Group* g, Group* p)
{
    assert(m_detaching);
    assert(!IsRootGroup(g->m_id));
    std::cerr << "Detach: g=" << (int)g->m_id << " p=" << (int)p->m_id << '\n';
    if (!ConstBoard::IsEdge(g->m_id)) {
        assert(g->m_parent == p->m_id);
        m_detached.PushBack(g->m_id);
        g->m_parent = SG_NULLMOVE;
    }

    // g's parent will now have only a single child, so tentatively
    // make sibling a child of grandparent
    cell_t sid = SiblingID(p, g->m_id);
    Group* sibling = GetGroupById(sid);
    Free(p);  // parent always dies
    if (!ConstBoard::IsEdge(sid))
        sibling->m_parent = p->m_parent;
    if (IsRootGroup(p->m_id)) {
        m_rootGroups.Exclude(p->m_id);
        if (!ConstBoard::IsEdge(sid))
            m_rootGroups.PushBack(sid);
        return;
    }
    Group* gp = GetGroupById(p->m_parent);
    gp->ChangeChild(p->m_id, sid);
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
    std::cerr << "CheckStructure: p=" << (int)p->m_id << '\n';
    assert(!ConstBoard::IsEdge(p->m_id));
    if (!IsRootGroup(p->m_id)) 
    {
        Group* gp = GetGroupById(p->m_parent);
        if (ConnectsToBothSemis(p, gp->m_con))
        {
            std::cerr << "passed!\n";
            std::cerr << p->ToString() << '\n';
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
    std::cerr << "CanConnectToEdgeOnSemi:\n";
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
// NOTE: g2 needs to be a free group or an edge group.
// NOTE: merges g1 and g2 at the root of both trees
Group* Groups::Merge(Group* g1, Group* g2,
                     SemiConnection* s1, SemiConnection* s2)
{
    assert(!ConstBoard::IsEdge(g1->m_id));
    assert(g1->m_parent == SG_NULLMOVE);
    assert(g2->m_parent == SG_NULLMOVE);
    assert(!IsRootGroup(g2->m_id));
    assert(IsRootGroup(g1->m_id));

    m_rootGroups.Exclude(g1->m_id);
    
    cell_t id = ObtainID();
    Group* g = GetGroupById(id);
    m_rootGroups.PushBack(id);

    g->m_id = id;
    g->m_parent = SG_NULLMOVE;
    g->m_left = g1->m_id;
    g->m_right = g2->m_id;
    g->m_border = g1->m_border | g2->m_border;
    SetSemis(g, -1, s1, s2);
    ComputeConnectionCarrier(g->m_con);
    std::cerr << "m.s1: " << s1->ToString() << ' ' 
              << YUtil::HashString(s1->m_hash)<< '\n';
    std::cerr << "m.s2: " << s2->ToString() << ' ' 
              << YUtil::HashString(s2->m_hash)<< '\n';

    g1->m_parent = id;
    g2->m_parent = id;
    
    // promote edge connections in g1 and g2 to g
    for (EdgeIterator e; e; ++e) {
        if ((g->m_border & ConstBoard::ToBorderValue(*e)) == 0)
            continue;

        if (g1->m_econ[*e].IsDefined()) {

            g->m_econ[*e] = g1->m_econ[*e];
            ChangeSemiGID(g->m_econ[*e], g->m_id);
            g1->m_econ[*e].Clear();

            if (g2->m_econ[*e].IsDefined()) {
                UnlinkSemis(g2->m_econ[*e]);
                g2->m_econ[*e].Clear();
            }

        } else if (g2->m_econ[*e].IsDefined()) {

            g->m_econ[*e] = g2->m_econ[*e];
            ChangeSemiGID(g->m_econ[*e], g->m_id);
            g2->m_econ[*e].Clear();

        } else {
            // g1 or g2 (or both) must touch e
        }
    }
    ComputeCarrier(g1, false);
    ComputeCarrier(g2, false);
    RecomputeFromChildren(g);
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

void Groups::ProcessNewSemis(const Block* block,
                             const std::vector<SemiConnection*>& s)
{
    std::cerr << "ProcessNewSemis:\n";
    cell_t bx = block->m_anchor;
    Group* ga = GetRootGroup(bx);
    for (size_t i = 0; i < s.size(); ++i) {
        SemiConnection* y;
        SemiConnection* x = s[i];
        std::cerr << x->ToString() << '\n';
        assert(x->m_p1 == bx || x->m_p2 == bx);
        const cell_t ox = (x->m_p1 == bx ? x->m_p2 : x->m_p1);
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
                std::cerr << "MERGE: " << y->ToString() << '\n';
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
    std::cerr << "CanMerge::\n";
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
    std::cerr << "CanConnectToEdge::\n";
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

// NOTE: g must be a detached group
bool Groups::TryMerge(Group* g, const GroupList& list, const Board& brd)
{
    assert(!ConstBoard::IsEdge(g->m_id));
    SemiConnection *x, *y;
    const SgBlackWhite color = brd.GetColor(g->m_id);
    for (int i = 0; i < list.Length(); ++i) {
        Group* other = GetGroupById(m_rootGroups[i]);
        if (brd.GetColor(other->m_id) == color 
            && CanMerge(other, g, &x, &y, other->m_carrier)) 
        {
            Merge(other, g, x, y);
            return true;
        }
    }
    return false;
}

void Groups::RestructureAfterMove(Group* g, cell_t p)
{
    Group* root = g;
    MarkedCells avoid = root->m_carrier;
    if (IsRootGroup(g->m_id)) {
        for (EdgeIterator e; e; ++e) {
            std::cerr << ConstBoard::ToString(*e) << '\n';
            if (   g->m_econ[*e].IsDefined()) {
                std::cerr << "DEFINED!!\n";
                for (MarkedCells::Iterator i(g->m_econ[*e].m_carrier); i; ++i)
                    std::cerr << ConstBoard::ToString(*i) << ' ';
                std::cerr << "\n";
            }
            
            if (   g->m_econ[*e].IsDefined() 
                && g->m_econ[*e].m_carrier.Marked(p)) 
            {
                assert(g->m_econ[*e].IsBroken());
                std::cerr << "Broke connection with " 
                          << ConstBoard::ToString(*e) << '\n';

                avoid.Unmark(g->m_econ[*e].m_carrier);
                SemiConnection *x, *y;
                if (CanConnectToEdge(g, *e, &x, &y, avoid)) {
                    std::cerr << "Reconnected to edge!\n";
                    ConnectGroupToEdge(g, *e, x, y);
                    std::cerr << g->ToString() << '\n';
                    std::cerr << "x:" << x->ToString() << '\n';
                    std::cerr << "y:" << y->ToString() << '\n';
                } else {
                    std::cerr << "Removing connection to edge!\n";
                    RemoveEdgeConnectionFromGroup(g, *e);
                    std::cerr << g->ToString() << '\n';
                }
                return;
            }
        }
    }
    assert(!g->IsLeaf());
    Group* left;
    Group* right;
    while (true) {
        std::cerr << g->ToString() <<'\n';
        left = GetGroupById(g->m_left);
        right = GetGroupById(g->m_right);
        if (left->m_carrier.Marked(p))
            g = left;
        else if (right->m_carrier.Marked(p))
            g = right;
        else
            break;
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
        std::cerr << "RE-MERGED!!\n";
        std::cerr << g->ToString() << '\n';
        std::cerr << "x: " << x->ToString() << '\n';
        std::cerr << "y: " << y->ToString() << '\n';
    } 
    else {
        if (IsRootGroup(g->m_id))
            Detach(right, g);
        else {
            // If both semis attach to left then move left into g's
            // position by detaching right. Otherwise, at least one
            // semi connects to right, so just detach left.
            Group* p = GetGroupById(g->m_parent);
            std::cerr << "Restructure: p->id=" << (int)p->m_id << '\n';
            if (ConnectsToBothSemis(left, p->m_con))
            {
                Detach(right, g);
            } else {
                Detach(left, g);
            }
        }
    }
}

void Groups::RestructureAfterMove(cell_t p, SgBlackWhite color, 
                                  const Board& brd)
{
    // FIXME: THIS IS PROBABLY VERY WRONG>....

    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        Group* g = GetGroupById(m_rootGroups[i]);
        if (g->m_carrier.Marked(p)) {
            std::cerr << "RestructureAfterMove:\n";
            BeginDetaching();
            RestructureAfterMove(g, p);
            for (int j = 0; j < m_detached.Length(); ++j) {
                Group* g2 = GetGroupById(m_detached[j]);
                // Try merging detached groups only for the opponent,
                // we will rebuild the groups for color using new semi
                // connections later.
                if (brd.GetColor(g->m_blocks[0]) == color 
                    || !TryMerge(g2, m_rootGroups, brd)) 
                {
                    // TODO: attempt to connect g2 to each edge!!
                    m_rootGroups.PushBack(g2->m_id);
                }
            }
            FinishedDetaching();
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

void Groups::HandleBlockMerge(Group* g, cell_t from, cell_t to)
{
    g->m_blocks.Exclude(from);
    g->m_blocks.Include(to);
    if (!g->IsLeaf())
        HandleBlockMerge(ChildContaining(g, from), from, to);
}

void Groups::FindParentOfBlock(Group* g, cell_t a, Group** p, Group** c)
{
    std::cerr << "FindParentOfBlock: a=" << ConstBoard::ToString(a)<<'\n';
    std::cerr << "g: " << g->ToString() << '\n';
        
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

void Groups::RemoveEdgeConnectionFromGroup(Group* g, cell_t edge)
{
    assert(ConstBoard::IsEdge(edge));
    assert(IsRootGroup(g->m_id));
    g->m_border ^= ConstBoard::ToBorderValue(edge);
    UnlinkSemis(g->m_econ[edge]);
    g->m_econ[edge].Clear();
    ComputeCarrier(g, true);
}

void Groups::ReplaceLeafWithGroup(Group* g, cell_t a, Group* z)
{
    assert(!g->IsLeaf());
    if (z->IsLeaf())
        return;
    Group *p, *c;
    FindParentOfBlock(g, a, &p, &c);
    p->ChangeChild(c->m_id, z->m_id);
    z->m_parent = p->m_id;
    std::cerr << "ReplaceLeafWithGroup():\n"
              << "p: " << p->ToString() << '\n'
              << "c: " << c->ToString() << '\n'
              << "z: " << z->ToString() << '\n';
    Free(c);
    RecomputeFromChildren(p);
}

void Groups::HandleBlockMerge(cell_t from, cell_t to)
{
    assert(from != to);
    assert(!ConstBoard::IsEdge(from));
    assert(!ConstBoard::IsEdge(to));
    Group* f = GetRootGroup(from);
    Group* t = GetRootGroup(to);
    if (f == t) {
        // f cannot be a leaf since it contains two disjoint blocks
        Group* a = CommonAncestor(f, to, from);
        Group* z = ChildContaining(a, from);
        Group* x = ChildContaining(a, to);
        HandleBlockMerge(z, from, to);
        std::cerr << "HandleBlockMerge\n";
        BeginDetaching();
        if (f == a) {
            // try to detach a leaf so the non-leaf child becomes
            // the root group. If both are leafs, then it doesn't
            // matter which we detach. 
            if (x->IsLeaf())
                std::swap(x, z);
            Detach(z, a);
            // x is now a root group
            if (x->IsLeaf()) {
                assert(z->IsLeaf());
                Free(z);
            }
            else {
                ReplaceLeafWithGroup(x, to, z);
            }
        } else {
            Detach(z, a);
            ReplaceLeafWithGroup(f, to, z);
        }
        // Detaching z should not cause a chain of detaches
        FinishedDetaching();
        assert(m_detached.Length() == 1);

    } else { // (f != t)

        HandleBlockMerge(f, from, to);
        if (f->IsLeaf())
            std::swap(f, t);
        if (f->IsLeaf()) {
            m_rootGroups.Exclude(t->m_id);
            Free(t);            
        } else {
            std::cerr << "YES1\n";
            m_rootGroups.Exclude(t->m_id);
            ReplaceLeafWithGroup(f, to, t);
        }
    }
}

//---------------------------------------------------------------------------

void Groups::BeginDetaching()
{
    assert(!m_detaching);
    m_detaching = true;
    m_detached.Clear();
}

void Groups::FinishedDetaching()
{
    m_detaching = false;
}

//---------------------------------------------------------------------------
