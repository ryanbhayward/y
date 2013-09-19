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
    for (Group::BlockIterator i(*this); i; ++i) {
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
       << " con=" << m_con.ToString()
       << " border=" << ConstBoard::BorderToString(m_border)
       << " blocks=" << BlocksToString()
       << " carrier=" << CarrierToString()
       << "]";
    return os.str();
}

//---------------------------------------------------------------------------

Groups::Groups(SemiTable& semis)
    : m_semis(semis)
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
        Group::BlockIterator it(*g);
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

void Groups::LinkSemis(const FullConnection& con, cell_t gid)
{
    m_semis.LookupIndex(con.m_semi1).m_group_id = gid;    
    m_semis.LookupIndex(con.m_semi2).m_group_id = gid;
}

void Groups::SetSemis(Group* g, 
                      const SemiConnection& s1,
                      const SemiConnection& s2)
{
    // FIXME: we are passed s1 and s2...  Seems dumb to have to lookup
    // a mutable version. Maybe we need a better interface for
    // creating groups?
    assert(s1.m_group_id == -1);
    assert(s2.m_group_id == -1);
    g->m_con.m_semi1 = m_semis.HashToIndex(s1.m_hash);
    g->m_con.m_semi2 = m_semis.HashToIndex(s2.m_hash);
    LinkSemis(g->m_con, g->m_id);
}

bool Groups::ConnectsToBothSemis(Group* g, const FullConnection& con)
{
    return g->ContainsSemiEndpoint(m_semis.LookupIndex(con.m_semi1))
        && g->ContainsSemiEndpoint(m_semis.LookupIndex(con.m_semi2));
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

void Groups::RecomputeFromChildren(Group* g)
{
    assert(!ConstBoard::IsEdge(g->m_id));
    assert(g->m_left != SG_NULLMOVE);
    const Group* left  = GetGroupById(g->m_left);
    const Group* right = GetGroupById(g->m_right);
    g->m_border = left->m_border | right->m_border;
    g->m_blocks = left->m_blocks;
    g->m_blocks.PushBackList(right->m_blocks);
    g->m_carrier = g->m_con.m_carrier;
    g->m_carrier.Mark(left->m_carrier);
    g->m_carrier.Mark(right->m_carrier);
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
    std::cerr << "Detach: g=" << (int)g->m_id << " p=" << (int)p->m_id << '\n';
    if (!ConstBoard::IsEdge(g->m_id))
        m_detached.PushBack(g->m_id);

    if (IsRootGroup(g->m_id)) {
        PrintRootGroups();
        m_rootGroups.Exclude(g->m_id);
        std::cerr << "Excluded " << (int)g->m_id << '\n';
        PrintRootGroups();
        return;
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

// Create merged group in g1's location g1 down and creating
// a new internal node with g1 and g2 as children.
// NOTE: g1 must be a valid onboard group (never be an edge group!)
// NOTE: g2 needs to be a free group or an edge group.
// NOTE: we are always merging g1 and g2 at the root of both trees.
// We could examine the two endpoints of s1 and s2 that occur in g1
// and descend down g1 until they stradle g1->left and g1->right.
// This attaches g2 'locally' in g1... not sure of the advantages of
// this, if there are any.
cell_t Groups::Merge(Group* g1, Group* g2,
                     const SemiConnection& s1, const SemiConnection& s2)
{
    assert(!ConstBoard::IsEdge(g1->m_id));

    cell_t id = ObtainID();
    Group* g = GetGroupById(id);
    g->m_id = id;

    if (IsRootGroup(g1->m_id)) {
        m_rootGroups.Exclude(g1->m_id);
        m_rootGroups.PushBack(id);
    } else {
        assert(g1->m_parent != SG_NULLMOVE);
        GetGroupById(g1->m_parent)->ChangeChild(g1->m_id, id);
    }

    g->m_parent = g1->m_parent;
    g1->m_parent = id;
    if (!ConstBoard::IsEdge(g2->m_id))
        g2->m_parent = id;

    g->m_left = g1->m_id;
    g->m_right = g2->m_id;

    SetSemis(g, s1, s2);

    std::cerr << "m.s1: " << s1.ToString() << ' ' 
              << YUtil::HashString(s1.m_hash)<< '\n';
    std::cerr << "m.s2: " << s2.ToString() << ' ' 
              << YUtil::HashString(s2.m_hash)<< '\n';
   
    ComputeConnectionCarrier(g->m_con);
    RecomputeFromChildren(g);
    return id;
}

bool Groups::CanMergeOnSemi(const Group* ga, const Group* gb,
                            const SemiConnection& x, 
                            const SemiConnection** outy) const
{
    if (x.Intersects(ga->m_carrier) || x.Intersects(gb->m_carrier))
        return false;
    
    assert(!ConstBoard::IsEdge(ga->m_id));
    bool bIsEdge = ConstBoard::IsEdge(gb->m_id);

    for (Group::BlockIterator ja(*ga); ja; ++ja) {
        cell_t ya = *ja;
        // since ga is not the edge group, we can never connect to some
        // other group over an edge block.
        if (ConstBoard::IsEdge(ya))
            continue;

        for (Group::BlockIterator jb(*gb); jb; ++jb) {
            cell_t yb = *jb;
            if (ConstBoard::IsEdge(yb) && (!bIsEdge || ga->ContainsEdge(yb)))
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
            ga = GetGroup(bx);  // Get the new group for block!!
        }
    }
}

bool Groups::CanMerge(const Group* ga, const Group* gb, 
                      const SemiConnection** outx, 
                      const SemiConnection** outy,
                      const MarkedCells& avoid) const
{
    bool aIsEdge = ConstBoard::IsEdge(ga->m_id);
    bool bIsEdge = ConstBoard::IsEdge(gb->m_id);
    assert(!aIsEdge || !bIsEdge);

    std::cerr << "CanMerge::\n";
    for (Group::BlockIterator ia(*ga); ia; ++ia) {
        cell_t xa = *ia;
        // Must use non-edge blocks to connect to other groups.  
        // If xa is an edge block then fall through here only if ga is
        // the actual edge and gb does not already contain it.
        if (ConstBoard::IsEdge(xa) && (!aIsEdge || gb->ContainsEdge(xa)))
            continue;
            
        for (Group::BlockIterator ib(*gb); ib; ++ib) {
            cell_t xb = *ib;
            if (ConstBoard::IsEdge(xb) && (!bIsEdge || ga->ContainsEdge(xb)))
                continue;

            for (SemiTable::IteratorPair xit(xa,xb,&m_semis); xit; ++xit) {
                const SemiConnection& x = *xit;
                if (   x.Intersects(avoid)
                    || x.Intersects(ga->m_carrier) 
                    || x.Intersects(gb->m_carrier))
                    continue;
 
                for (Group::BlockIterator ja(*ga, ia.Index()); ja; ++ja) {
                    cell_t ya = *ja;
                    if (   ConstBoard::IsEdge(ya) 
                        && (!aIsEdge || gb->ContainsEdge(ya)))
                        continue;
                    
                    for (Group::BlockIterator jb(*gb, ib.Index()); jb; ++jb) {
                        cell_t yb = *jb;
                        if (   ConstBoard::IsEdge(yb)
                            && (!bIsEdge || ga->ContainsEdge(yb)))
                            continue;

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
        if (brd.GetColor(other->m_id)==color 
            && CanMerge(other, g, &x, &y, other->m_carrier)) {
            Merge(other, g, *x, *y);
            return true;
        }
    }
    return false;
}

void Groups::RestructureAfterMove(Group* g, cell_t p)
{
    Group* root = g;
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
    assert(g->m_con.m_carrier.Marked(p));
    assert(g->m_con.IsBroken());
    MarkedCells avoid = root->m_carrier;
    avoid.Unmark(g->m_con.m_carrier);

    // Try to immediately reform the group
    // FIXME: we know which semi was killed, we could try 
    // CanMergeOnSemi() with the remaining one first.
    const SemiConnection *x, *y;
    if (CanMerge(left, right, &x, &y, avoid)) {
        UnlinkSemis(g->m_con);
        SetSemis(g, *x, *y);
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
    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        Group* g = GetGroupById(m_rootGroups[i]);
        if (g->m_carrier.Marked(p)) {
            std::cerr << "HERE\n";
            m_detached.Clear();
            RestructureAfterMove(g, p);
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

Group* Groups::RootGroupContaining(cell_t from)
{
    assert(!ConstBoard::IsEdge(from));
    Group* ret = NULL;
    for (int i = 0; i < m_rootGroups.Length(); ++i) {
        Group* g = GetGroupById(m_rootGroups[i]);
        if (g->ContainsBlock(from)) {
            assert(ret == NULL);
            ret = g;
        }
    }
    return ret;
}

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
    assert(!g->IsLeaf());
    while (true) {
        Group* child = ChildContaining(g, a);
        if (child->IsLeaf()) {
            assert(child->m_blocks[0] == a);
            *p = g;
            *c = child;
            std::cerr << "BLAH BLAH BALH\n";
            break;
        }
        g = child;
    }
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
    Group* f = RootGroupContaining(from);
    Group* t = RootGroupContaining(to);
    if (f == t) {
        // f cannot be a leaf since it contains two disjoint blocks
        Group* a = CommonAncestor(f, to, from);
        Group* z = ChildContaining(a, from);
        Group* x = ChildContaining(a, to);
        HandleBlockMerge(z, from, to);
        m_detached.Clear();
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
