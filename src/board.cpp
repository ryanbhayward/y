#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "board.h"

using namespace std;

char ConstBoard::ColorToChar(SgBoardColor color) 
{
    switch(color) {
    case SG_BLACK: return 'b';
    case SG_WHITE: return 'w';
    case SG_EMPTY: return '.';
    default: return '?';
    }
}

std::string ConstBoard::ColorToString(SgBoardColor color) 
{
    switch(color) {
    case SG_BLACK: return "black";
    case SG_WHITE: return "white";
    case SG_EMPTY: return "empty";
    default: return "?";
    }
}

std::string ConstBoard::ToString(int cell) const
{
    if (cell == WEST)
        return "west";
    else if (cell == EAST)
        return "east";
    else if (cell == SOUTH)
        return "south";
    char str[16];
    sprintf(str, "%c%1d",board_col(cell)+'a',board_row(cell)+1); 
    return str;
}

int ConstBoard::FromString(const string& name) const
{
    if (name.size() >= 4 && name.substr(0,4) == "swap")
    	return Y_SWAP;
    if (name == "west")
        return WEST;
    if (name == "east")
        return EAST;
    if (name == "south")
        return SOUTH;
    int x = name[0] - 'a' + 1;
    std::istringstream sin(name.substr(1));
    int y;
    sin >> y;
    return fatten(y-1, x-1);
}

ConstBoard::ConstBoard()
    : m_size(-1)
{ }

ConstBoard::ConstBoard(int size)
    : m_size(size)
    , Np2G(m_size+2*GUARDS)
    , TotalCells(m_size*(m_size+1)/2)
    , TotalGBCells(Np2G*Np2G)
    , WEST(TotalGBCells+0)
    , EAST(TotalGBCells+1)
    , SOUTH(TotalGBCells+2)
{
    Nbr_offsets[0] =  -Np2G;
    Nbr_offsets[1] = 1-Np2G;
    Nbr_offsets[2] = 1;
    Nbr_offsets[3] = Np2G;
    Nbr_offsets[4] = Np2G-1;
    Nbr_offsets[5] = -1;
    Nbr_offsets[6] = -Np2G;

    Bridge_offsets[0] = -2*Np2G+1;
    Bridge_offsets[1] = 2-Np2G;
    Bridge_offsets[2] = Np2G+1;
    Bridge_offsets[3] = 2*Np2G-1;
    Bridge_offsets[4] = Np2G-2;
    Bridge_offsets[5] = -(Np2G+1);
    m_cells.clear();

    for (int r=0; r<Size(); r++)
        for (int c=0; c<=r; c++)
            m_cells.push_back(fatten(r,c));
}

//----------------------------------------------------------------------

Board::Board(int size)
{ 
    SetSize(size);
}

void Board::State::Init(int T)
{
    m_color.resize(T+3);
    std::fill(m_color.begin(), m_color.end(), SG_BORDER);

    m_block.resize(T+3);
    std::fill(m_block.begin(), m_block.end(), (Block*)0);
    m_group.resize(T+3);
    std::fill(m_group.begin(), m_group.end(), (Group*)0); 

    m_blockList.resize(T+3);
    std::fill(m_blockList.begin(), m_blockList.end(), Block());
    m_groupList.resize(T+3);
    std::fill(m_groupList.begin(), m_groupList.end(), Group());

    m_activeBlocks.resize(2);
    m_activeBlocks[SG_BLACK].resize(0);
    m_activeBlocks[SG_WHITE].resize(0);
}

void Board::SetSize(int size)
{ 
    m_constBrd = ConstBoard(size);

    const int N = Size();
    const int T = Const().TotalGBCells;
    m_state.Init(T);

    // Create block/group for left edge
    {
        Block& b = m_state.m_blockList[Const().WEST];
        b.m_color = SG_BORDER;
        b.m_anchor = Const().WEST;
        b.m_border = BORDER_LEFT;
        b.m_stones.Clear();
        b.m_shared.clear();
	Group& g = m_state.m_groupList[Const().WEST];
	g.m_anchor = b.m_anchor;
	g.m_border = b.m_border;
	g.m_blocks.clear();
	g.m_blocks.push_back(b.m_anchor);
        for (int i = 0; i < N; ++i)
            b.m_liberties.PushBack(Const().fatten(i,0));
    }
    // Create block/group for right edge
    {
        Block& b = m_state.m_blockList[Const().EAST];
        b.m_color = SG_BORDER;
        b.m_anchor = Const().EAST;
        b.m_border = BORDER_RIGHT;
        b.m_stones.Clear();
        b.m_shared.clear();
	Group& g = m_state.m_groupList[Const().EAST];
	g.m_anchor = b.m_anchor;
	g.m_border = b.m_border;
	g.m_blocks.clear();
	g.m_blocks.push_back(b.m_anchor);
        for (int i = 0; i < N; ++i)
            b.m_liberties.PushBack(Const().fatten(i,i));
    }
    // Create block/group for bottom edge
    {
        Block& b = m_state.m_blockList[Const().SOUTH];
        b.m_color = SG_BORDER;
        b.m_anchor = Const().SOUTH;
        b.m_border = BORDER_BOTTOM;
        b.m_stones.Clear();
        b.m_shared.clear();
	Group& g = m_state.m_groupList[Const().SOUTH];
	g.m_anchor = b.m_anchor;
	g.m_border = b.m_border;
	g.m_blocks.clear();
	g.m_blocks.push_back(b.m_anchor);
        for (int i = 0; i < N; ++i)
            b.m_liberties.PushBack(Const().fatten(N,i-1));
    }
    // set guards to point to their border block
    std::vector<Block*>& bptr = m_state.m_block;
    std::vector<Block>& blst = m_state.m_blockList;
    bptr[Const().WEST] = &blst[Const().WEST];
    bptr[Const().EAST] = &blst[Const().EAST];
    bptr[Const().SOUTH] = &blst[Const().SOUTH];
    for (int i = 0; i < N; ++i) { 
        bptr[Const().fatten(i,0)-1] = &blst[Const().WEST];
        bptr[Const().fatten(i,i)+1] = &blst[Const().EAST];
        bptr[Const().fatten(N,i)] = &blst[Const().SOUTH];
    }
    bptr[Const().fatten(-1,0)-1] = &blst[Const().WEST];
    bptr[Const().fatten(-1,-1)+1] = &blst[Const().EAST];
    bptr[Const().fatten(N,N)] = &blst[Const().SOUTH];
   
    std::vector<Group*>& gptr = m_state.m_group;
    std::vector<Group>& glst = m_state.m_groupList;
    gptr[Const().WEST] = &glst[Const().WEST];
    gptr[Const().EAST] = &glst[Const().EAST];
    gptr[Const().SOUTH] = &glst[Const().SOUTH];
    for (int i = 0; i < N; ++i) { 
        gptr[Const().fatten(i,0)-1] = &glst[Const().WEST];
        gptr[Const().fatten(i,i)+1] = &glst[Const().EAST];
        gptr[Const().fatten(N,i)] = &glst[Const().SOUTH];
    }
    gptr[Const().fatten(-1,0)-1] = &glst[Const().WEST];
    gptr[Const().fatten(-1,-1)+1] = &glst[Const().EAST];
    gptr[Const().fatten(N,N)] = &glst[Const().SOUTH];

    for (BoardIterator it(Const()); it; ++it)
        m_state.m_color[*it] = SG_EMPTY;

    m_state.m_winner  = SG_EMPTY;
    m_state.m_lastMove = SG_NULLMOVE;
}

void Board::SetPosition(const Board& other) 
{
    SetSize(other.Size());
    for (BoardIterator it(Const()); it; ++it) {
        SgBlackWhite c = other.m_state.m_color[*it];
        if (c != SG_EMPTY)
            Play(c, *it);
    }
    m_state.m_toPlay = other.m_state.m_toPlay;
    m_state.m_lastMove = other.m_state.m_lastMove;
}

void Board::AddSharedLibertiesAroundPoint(Block* b1, int p, int skip)
{
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (*it == skip)
            continue;
        if (GetColor(*it) == SG_EMPTY)
            continue;
        if (GetColor(*it) == SgOppBW(b1->m_color))
            continue;
        Block* b2 = GetBlock(*it);
        if (b1 == b2)
            continue;
        if (b2->m_color == b1->m_color)
            AddSharedLiberty(b1, b2, p);
        else if (b2->m_color == SG_BORDER && ((b2->m_border&b1->m_border)==0))
            AddSharedLiberty(b1, b2, p);
    }
}

void Board::CreateSingleStoneBlock(int p, SgBlackWhite color, int border)
{
    Block* b = m_state.m_block[p] = &m_state.m_blockList[p];
    b->m_anchor = p;
    b->m_color = color;
    b->m_border = border;
    b->m_stones.SetTo(p);
    b->m_liberties.Clear();
    b->m_shared.clear();
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY) {
            b->m_liberties.PushBack(*it);
            AddSharedLibertiesAroundPoint(b, *it, p);
	}
    }
    m_state.m_activeBlocks[color].push_back(b);
    ComputeGroupForBlock(b);
}

bool Board::IsAdjacent(int p, const Block* b)
{
    for (CellNbrIterator it(Const(), p); it; ++it)
        if (m_state.m_block[*it] == b)
            return true;
    return false;
}

void Board::AddStoneToBlock(int p, int border, Block* b)
{
    b->m_border |= border;
    b->m_stones.PushBack(p);
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY) {
            if (!IsAdjacent(*it, b)) {
                b->m_liberties.PushBack(*it);
                AddSharedLibertiesAroundPoint(b, *it, p);
            }
        }
    }
    m_state.m_block[p] = b;
    m_state.m_group[p] = m_state.m_group[b->m_anchor];
    RemoveEdgeSharedLiberties(b);
    ComputeGroupForBlock(b);
}

void Board::MergeBlocks(int p, int border, SgArrayList<int, 3>& adjBlocks)
{
    Block* largestBlock = 0;
    int largestBlockStones = 0;
    for (SgArrayList<int,3>::Iterator it(adjBlocks); it; ++it)
    {
        Block* adjBlock = GetBlock(*it);
        int numStones = adjBlock->m_stones.Length();
        if (numStones > largestBlockStones)
        {
            largestBlockStones = numStones;
            largestBlock = adjBlock;
        }
    }
    m_state.m_block[p] = largestBlock;
    m_state.m_group[p] = m_state.m_group[largestBlock->m_anchor];
    largestBlock->m_border |= border;
    largestBlock->m_stones.PushBack(p);

    bool seen[Y_MAX_CELL];
    memset(seen, 0, sizeof(seen));
    for (Block::LibertyIterator lib(largestBlock->m_liberties); lib; ++lib)
        seen[*lib] = true;
    for (SgArrayList<int,3>::Iterator it(adjBlocks); it; ++it)
    {
        Block* adjBlock = GetBlock(*it);
        if (adjBlock == largestBlock)
            continue;
	MergeSharedLiberty(adjBlock, largestBlock);
        largestBlock->m_border |= adjBlock->m_border;
        for (Block::StoneIterator stn(adjBlock->m_stones); stn; ++stn)
        {
            largestBlock->m_stones.PushBack(*stn);
            m_state.m_block[*stn] = largestBlock;
	    m_state.m_group[*stn] = m_state.m_group[largestBlock->m_anchor];
        }
        for (Block::LibertyIterator lib(adjBlock->m_liberties); lib; ++lib)
            if (!seen[*lib]) {
                seen[*lib] = true;
                largestBlock->m_liberties.PushBack(*lib);
            }
	m_state.RemoveActiveBlock(adjBlock);
    }
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (m_state.m_color[*it] == SG_EMPTY && !seen[*it]) {
            largestBlock->m_liberties.PushBack(*it);
            AddSharedLibertiesAroundPoint(largestBlock, *it, p);
	}
    }
    RemoveEdgeSharedLiberties(largestBlock);
    ComputeGroupForBlock(largestBlock);
}

void Board::RemoveSharedLiberty(int p, Block* a, Block* b)
{
    // no shared liberties between edge blocks
    // (if corner cells are marked as shared liberties between pairs of
    //  edges, they will be removed twice because they occur in oppBlocks
    //  and adjBlocks)
    if (a->m_color == SG_BORDER && b->m_color == SG_BORDER)
        return;

    int i = a->GetSharedLibertiesIndex(b);
    if (i == -1)
	return;
    a->m_shared[i].Exclude(p);
    if (a->m_shared[i].Empty()) {
        a->RemoveSharedLiberties(i);
    }

    i = b->GetSharedLibertiesIndex(a);
    b->m_shared[i].Exclude(p);
    if (b->m_shared[i].Empty()) {
        b->RemoveSharedLiberties(i);
    }
}

void Board::RemoveSharedLiberty(int p, SgArrayList<int, 3>& adjBlocks)
{
    switch(adjBlocks.Length())
    {
    case 0:
    case 1:
        return;
    case 2:
        RemoveSharedLiberty(p, GetBlock(adjBlocks[0]), GetBlock(adjBlocks[1]));
        return;
    case 3:
        RemoveSharedLiberty(p, GetBlock(adjBlocks[0]), GetBlock(adjBlocks[1]));
        RemoveSharedLiberty(p, GetBlock(adjBlocks[0]), GetBlock(adjBlocks[2]));
        RemoveSharedLiberty(p, GetBlock(adjBlocks[1]), GetBlock(adjBlocks[2]));
        return;
    default:
        return;
    }
}

void Board::Play(SgBlackWhite color, int p) 
{
    // std::cerr << ToString() << '\n'
    //           << "p=" << ToString(p) << '\n'
    //           << "pval=" << p << '\n';
    m_state.m_lastMove = p;    
    m_state.m_toPlay = color;
    m_state.m_color[p] = color;
    int border = 0;
    SgArrayList<int, 3> adjBlocks;
    SgArrayList<int, 3>& oppBlocks = m_state.m_oppBlocks;
    adjBlocks.Clear();
    oppBlocks.Clear();
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_BORDER) {
            Block* b = GetBlock(*it);
            border |= b->m_border;
            b->m_liberties.Exclude(p);
            if (!oppBlocks.Contains(b->m_anchor))
                oppBlocks.PushBack(b->m_anchor);
            if (!adjBlocks.Contains(b->m_anchor))
                adjBlocks.PushBack(b->m_anchor);
        }
        else if (GetColor(*it) != SG_EMPTY) {
            Block* b = GetBlock(*it);
            b->m_liberties.Exclude(p);
            if (b->m_color == color && !adjBlocks.Contains(b->m_anchor))
                adjBlocks.PushBack(b->m_anchor);
            else if (b->m_color == SgOppBW(color) 
                     && !oppBlocks.Contains(b->m_anchor))
                oppBlocks.PushBack(b->m_anchor);
        }
    }
    RemoveSharedLiberty(p, adjBlocks);
    RemoveSharedLiberty(p, oppBlocks);

    // Create/update block containing p (ignores edge blocks)
    {
        SgArrayList<int, 3> realAdjBlocks;
        for (int i = 0; i < adjBlocks.Length(); ++i)
            if (GetColor(adjBlocks[i]) != SG_BORDER)
                realAdjBlocks.PushBack(adjBlocks[i]);

        if (realAdjBlocks.Length() == 0)
            CreateSingleStoneBlock(p, color, border);
        else if (realAdjBlocks.Length() == 1)
            AddStoneToBlock(p, border, GetBlock(realAdjBlocks[0]));
        else
            MergeBlocks(p, border, realAdjBlocks);
    }

    // Recompute groups for adjacent opponent blocks
    if (oppBlocks.Length() > 1) {
	bool seen[Const().TotalGBCells+10];
	memset(seen, 0, sizeof(seen));
	//for(int i = 0; i < oppBlocks.Length(); ++i) 
        //    GetBlock(oppBlocks[i])->m_group = oppBlocks[i];
	for(int i = 0; i < oppBlocks.Length(); ++i) 
            ComputeGroupForBlock(GetBlock(oppBlocks[i]));
    }

    // Check for a win
    if (   m_state.m_group[p]->m_border == BORDER_ALL 
        || m_state.m_block[p]->m_border == BORDER_ALL) 
    {
        m_state.m_winner = color;
    }

    //std::cerr << m_state.m_group[p]->m_border << '\n';
    FlipToPlay();
    //std::cerr << ToString();
}

const Board::SharedLiberties& 
Board::GetSharedLiberties(const Block* b1, const Block* b2) const
{
    // FIXME:: IS THIS THREADSAFE!?
    static SharedLiberties EMPTY_SHARED_LIBERTIES;

    if(b1 == b2 || b1 == 0 || b2 == 0)
	return EMPTY_SHARED_LIBERTIES;
    for(size_t i = 0; i < b1->m_shared.size(); ++i) {
	if(b1->m_shared[i].m_other == b2->m_anchor)
            return b1->m_shared[i];
    }
    return EMPTY_SHARED_LIBERTIES;
}

void Board::AddSharedLiberty(Block* b1, Block* b2, int p)
{
    int i = b1->GetSharedLibertiesIndex(b2);
    if(i != -1) {
        int j = b2->GetSharedLibertiesIndex(b1);
        assert(j != -1);
        b1->m_shared[i].Include(p);
        b2->m_shared[j].Include(p);
        Statistics& stats = Statistics::Get();
        stats.m_maxSharedLiberties 
            = std::max(stats.m_maxSharedLiberties, b1->m_shared[i].Size());
    }
    else {
	b1->m_shared.push_back(SharedLiberties(b2->m_anchor, p));
	b2->m_shared.push_back(SharedLiberties(b1->m_anchor, p));
    }
}

// Merge b1 into b2
void Board::MergeSharedLiberty(Block* b1, Block* b2)
{
    for (size_t i = 0; i < b1->m_shared.size(); ++i) {
        const int otherAnchor = b1->m_shared[i].m_other;
	if (otherAnchor == b2->m_anchor)
            continue;
        Block* otherBlock = m_state.m_block[otherAnchor];
        assert(otherBlock);     
        int index = b2->GetSharedLibertiesIndex(otherBlock);
        if (index != -1) {
            int index2 = otherBlock->GetSharedLibertiesIndex(b2);
            assert(index2 != -1);
            // transfer any new liberties from b1 to b2
            for (size_t j = 0; j < b1->m_shared[i].Size(); ++j)
            {
                const int lib = b1->m_shared[i].m_liberties[j];
                if(!(b2->m_shared[index]).Contains(lib)) {
                    b2->m_shared[index].PushBack(lib);
                    otherBlock->m_shared[index2].PushBack(lib);
                }
            }
            // remove mention of b1 from other's list.
            const int j = otherBlock->GetSharedLibertiesIndex(b1);
            otherBlock->RemoveSharedLiberties(j);
        }
        else {
            b2->m_shared.push_back(b1->m_shared[i]);
            const int j = otherBlock->GetSharedLibertiesIndex(b1);
            otherBlock->m_shared[j].m_other = b2->m_anchor;
        }
    }
    // Remove mention of any connections from b2->b1, since b1 is now
    // part of b2. Index could be -1 if there was only a single shared
    // liberty between them.
    int index = b2->GetSharedLibertiesIndex(b1);
    if (index != -1)
        b2->RemoveSharedLiberties(index);
}

void Board::RemoveEdgeSharedLiberties(Block* b)
{
    if (b->m_border & BORDER_LEFT) {
        b->RemoveSharedLibertiesWith(GetBlock(Const().WEST));
        GetBlock(Const().WEST)->RemoveSharedLibertiesWith(b);
    }
    if (b->m_border & BORDER_RIGHT) {
        b->RemoveSharedLibertiesWith(GetBlock(Const().EAST));
        GetBlock(Const().EAST)->RemoveSharedLibertiesWith(b);
    }
    if (b->m_border & BORDER_BOTTOM) {
        b->RemoveSharedLibertiesWith(GetBlock(Const().SOUTH));
        GetBlock(Const().SOUTH)->RemoveSharedLibertiesWith(b);
    }
}

bool Board::BlocksVirtuallyConnected(const SharedLiberties& lib, bool* seen)
{
    int count = 0;
    for (size_t i = 0; i < lib.Size(); ++i)
        if (!seen[lib.m_liberties[i]])
            ++count;
    return count > 1;
}

void Board::MarkLibertiesAsSeen(const SharedLiberties& lib, bool* seen)
{
    for (size_t j = 0; j < lib.Size(); ++j)
        seen[lib.m_liberties[j]] = true;
}

void Board::ComputeGroupForBlock(Block* b)
{
    bool seen[Const().TotalGBCells+10];
    memset(seen, 0, sizeof(seen));
    Group* g = m_state.m_group[b->m_anchor] = &m_state.m_groupList[b->m_anchor];
    g->m_anchor = b->m_anchor;
    g->m_border = b->m_border;
    g->m_blocks.clear();
    g->m_blocks.push_back(g->m_anchor);
    GroupSearch(seen, b);
}

void Board::GroupSearch(bool* seen, Block* b)
{
    seen[b->m_anchor] = true;
    //std::cerr << "Entered: " << ToString(b->m_anchor) << '\n';
    for (size_t i = 0; i < b->m_shared.size(); ++i)
    {
        SharedLiberties& sl = b->m_shared[i];
	//std::cerr << "Considering: " << ToString(sl.m_other) << '\n';
	if(!seen[sl.m_other] && BlocksVirtuallyConnected(sl, seen))
	{
	    //std::cerr << "Inside!\n";
	    Group* g = m_state.m_group[b->m_anchor];
	    g->m_border |= GetBlock(sl.m_other)->m_border;
	    if(GetColor(sl.m_other) != SG_BORDER) {
                // Note that we are not marking liberties with an edge
                // block: we claim these cannot conflict with group
                // formation.
                MarkLibertiesAsSeen(sl, seen);
                g->m_blocks.push_back(sl.m_other);
                m_state.m_group[sl.m_other] = g;
		GroupSearch(seen, GetBlock(sl.m_other));
            }
            //std::cerr << "Back to: " << ToString(b->m_anchor) << '\n';
	}
    }
}

void Board::RemoveStone(int p)
{
    std::vector<SgBoardColor> m_oldColor(m_state.m_color);
    m_oldColor[p] = SG_EMPTY;
    SetSize(Size());
    for (BoardIterator it(Const()); it; ++it)
        if (m_oldColor[*it] != SG_EMPTY)
            Play(m_oldColor[*it],*it);
}

void Board::Swap()
{
    for (BoardIterator it(Const()); it; ++it) {
	if(m_state.m_color[*it] != SG_EMPTY) {
            m_state.m_color[*it] = SgOppBW(m_state.m_color[*it]);
            if (*it == Anchor(*it)) {
                Block* b = m_state.m_block[*it];
                b->m_color = SgOppBW(b->m_color);
            }
        }
    }
}

void Board::CopyState(Board::State& a, const Board::State& b)
{
    a = b;
    // Fix pointers since they are now pointing into other
    a.m_block[Const().WEST] = &a.m_blockList[Const().WEST];
    a.m_block[Const().EAST] = &a.m_blockList[Const().EAST];
    a.m_block[Const().SOUTH]= &a.m_blockList[Const().SOUTH];
    a.m_group[Const().WEST] = &a.m_groupList[Const().WEST];
    a.m_group[Const().EAST] = &a.m_groupList[Const().EAST];
    a.m_group[Const().SOUTH]= &a.m_groupList[Const().SOUTH];
    for (BoardIterator it(Const()); it; ++it) {
	SgBlackWhite color = b.m_color[*it];
        if (color != SG_EMPTY) {
            a.m_block[*it] = &a.m_blockList[b.m_block[*it]->m_anchor];
	    a.m_group[*it] = &a.m_groupList[b.m_group[*it]->m_anchor];
	    if (!a.IsActive(a.m_block[*it]))
		a.m_activeBlocks[color].push_back(a.m_block[*it]);
	}
    }
}

void Board::CheckConsistency()
{
    // FIXME: ADD MORE CHECKS HERE!!
    DumpBlocks();
    for (BoardIterator it(Const()); it; ++it) {
        int color = m_state.m_color[*it];
        if (color != SG_BLACK && color != SG_WHITE && color != SG_EMPTY)
        {
            std::cerr << ToString();
            std::cerr << ToString(*it) << " color = " 
                      << m_state.m_color[*it] << '\n';
            abort();
        }
        
        if ((color == SG_BLACK || color == SG_WHITE) 
            && m_state.m_block[*it]->m_anchor == *it) 
        {
            Block* b = m_state.m_block[*it];
            for (size_t i = 0; i < b->m_shared.size(); ++i) {
                if (!Const().IsOnBoard(b->m_shared[i].m_other)
                    || GetColor(b->m_shared[i].m_other) == SG_EMPTY)
                {
                    std::cerr << ToString()
                              << "SOMETHING WRONG WITH SHARED LIBS!\n";
                    std::cerr << "Blocks: \n";
                    DumpBlocks();
                    abort();
                }
                    
            }            
        }
    }
}

void Board::DumpBlocks() 
{
    for(size_t i = 0; i < m_state.m_block.size(); ++i) {
        if(m_state.m_block[i] != NULL) {
            const Block * b = m_state.m_block[i];
            if (b->m_color == 3)
                continue;
            std::cerr << "id=" << ToString(i)  << " " << b->ToString(Const()) << '\n';
        }
    }
}

std::string Board::ActiveBlocksToString(SgBlackWhite color) const
{
    ostringstream os;
    for (size_t i = 0; i < m_state.m_activeBlocks[color].size(); ++i)
	os << ' ' << ToString(m_state.m_activeBlocks[color][i]->m_anchor);
    os << '\n';
    return os.str();
}

//----------------------------------------------------------------------

std::string Board::ToString() const
{
    ostringstream os;
    const int N = Size();
    os << "\n";
    for (int j = 0; j < N; j++) {
        for (int k = 0; k < N-j; k++) 
            os << ' ';
        os << (j+1<10?" ":"") << j+1 << "  "; 
        for (int k = 0; k <= j ; k++) 
            os << ConstBoard::ColorToChar(m_state.m_color[Const().fatten(j,k)]) << "  ";
        os << "\n";
    }
    os << "   ";
    for (char ch='a'; ch < 'a'+N; ch++) 
        os << ' ' << ch << ' '; 

    return os.str();
}

std::string Board::BorderToString() const
{
    ostringstream os;
    const int Np2G = Const().Np2G;
    os << "\n" << "Border Values:\n";
    int psn = 0;
    for (int j = 0; j < Np2G; j++) {
	for (int k = 0; k < j; k++)
	    os << ' ';
	for (int k = 0; k < Np2G; k++) {
            char c = '*';
	    if (m_state.m_block[psn] != 0) {
                if (m_state.m_block[psn]->m_border != BORDER_NONE)
                    c = '0' + m_state.m_block[psn]->m_border;
            }
            os << "  " << c;

            psn++;
	}
	os << "\n";
    }
    os << "\n"; 
    return os.str();
}

std::string Board::AnchorsToString() const
{
    ostringstream os;
    const int N = Size();
    os << "\n" << "Block Anchors for cells:\n\n";
    for (int j = 0; j < N; j++) {
	int psn = Const().fatten(j,0);
	for (int k = 0; k < N-j; k++) 
	    os << ' ';
	for (int k = 0; k <= j; k++) {
	    if(m_state.m_block[psn] != 0) {
		if(Const().board_row(psn)+1 < 10)
		    os << ' ';
		os << ToString(Anchor(psn));
	    }
	    else
		os << " * ";
	    psn++;
	}
	os << "\n";
    }
    return os.str();
}

//////////////////////////////////////////////////////////////////////

int Board::GeneralSaveBridge(SgRandom& random) const
{
    int ret = SG_NULLMOVE;
    int num = 0;
    
    const SgArrayList<int, 3>& opp = m_state.m_oppBlocks;
    switch(opp.Length())
    {
    case 0:
    case 1:
        return SG_NULLMOVE;
    case 2:
        {
            const SharedLiberties& libs = GetSharedLiberties(opp[0], opp[1]);
            if (libs.Size() == 1)
                return libs.m_liberties[0];
            return SG_NULLMOVE;
        }
    case 3:
        for (int i = 0; i < 2; ++i)
        {
            for (int j = i + 1; j < 3; ++j) {
                const SharedLiberties& libs=GetSharedLiberties(opp[i], opp[j]);
                if (libs.Size() == 1) {
                    ++num;
                    if (num==1 || random.Int(num)==0)
                        ret = libs.m_liberties[0];
                }
            }    
        }
        return ret;
    default:
        return SG_NULLMOVE;
    }
    return SG_NULLMOVE;
}

int Board::SaveBridge(int lastMove, const SgBlackWhite toPlay, 
                      SgRandom& random) const
{
    //if (m_oppNbs < 2 || m_emptyNbs == 0 || m_emptyNbs > 4)
    //    return INVALID_POINT;
    // State machine: s is number of cells matched.
    // In clockwise order, need to match CEC, where C is the color to
    // play and E is an empty cell. We start at a random direction and
    // stop at first match, which handles the case of multiple bridge
    // patterns occuring at once.
    // TODO: speed this up?
    int s = 0;
    const int start = random.Int(6);
    int ret = SG_NULLMOVE;
    for (int j = 0; j < 8; ++j)
    {
        const int i = (j + start) % 6;
        const int p = lastMove + Const().Nbr_offsets[i];
        const bool mine = m_state.m_color[p] == toPlay;
        if (s == 0)
        {
            if (mine) s = 1;
        }
        else if (s == 1)
        {
            if (mine) s = 1;
            else if (m_state.m_color[p] == !toPlay) s = 0;
            else
            {
                s = 2;
                ret = p;
            }
        }
        else if (s == 2)
        {
            if (mine) return ret; // matched!!
            else s = 0;
        }
    }
    return SG_NULLMOVE;
}
