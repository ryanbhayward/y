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

std::string ConstBoard::ToString(cell_t cell)
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

cell_t ConstBoard::FromString(const string& name)
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
    , TotalCells(m_size*(m_size+1)/2)
    , TotalGBCells((m_size+(2*GUARDS+1))*(m_size+(2*GUARDS+1)+1)/2 + 3)
    , m_cells()
    , m_cell_nbr()
{
    m_cells.clear();
    for (int r=0; r<Size(); r++)
        for (int c=0; c<=r; c++) {
            m_cells.push_back(fatten(r,c));
        }

    m_cells_edges = m_cells;
    m_cells_edges.push_back(WEST);
    m_cells_edges.push_back(EAST);
    m_cells_edges.push_back(SOUTH);
    
    m_cell_nbr.resize(TotalGBCells);
    for (int r=0; r<Size(); r++) {
        for (int c=0; c<=r; c++) {
            cell_t p = fatten(r,c);
            m_cell_nbr[p].resize(6);

            cell_t rr = r + 2;

            // spin clockwise from top left neighbor
            m_cell_nbr[p][0] = p - rr - 1;
            m_cell_nbr[p][1] = p - rr;
            m_cell_nbr[p][2] = p + 1;
            m_cell_nbr[p][3] = p + (rr + 1) + 1;
            m_cell_nbr[p][4] = p + (rr + 1);
            m_cell_nbr[p][5] = p - 1;
        }
    }
}

//----------------------------------------------------------------------

Board::Board(int size)
{ 
    SetSize(size);
}

void Board::State::Init(int T)
{
    m_color.resize(T);
    std::fill(m_color.begin(), m_color.end(), SG_BORDER);

    m_cell.resize(T);
    std::fill(m_cell.begin(), m_cell.end(), (Cell*)0);
    m_block.resize(T);
    std::fill(m_block.begin(), m_block.end(), (Block*)0);
    m_group.resize(T);
    std::fill(m_group.begin(), m_group.end(), (Group*)0); 

    m_cellList.resize(T);
    std::fill(m_cellList.begin(), m_cellList.end(), Cell());
    m_blockList.resize(T);
    std::fill(m_blockList.begin(), m_blockList.end(), Block());
    m_groupList.resize(T);
    std::fill(m_groupList.begin(), m_groupList.end(), Group());

    m_con.resize(T);
    for(int i = 0; i < T; ++i) {
       m_con[i].resize(T);
       std::fill(m_con[i].begin(), m_con[i].end(), SharedLiberties());
    }

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

    // initialize empty cells
    for (CellIterator it(Const()); it; ++it){
        m_state.m_color[*it] = SG_EMPTY;
	m_state.m_cell[*it] = &m_state.m_cellList[*it];
    }

    // Create block/group for left edge
    {
        Block& b = m_state.m_blockList[Const().WEST];
        b.m_color = SG_BORDER;
        b.m_anchor = Const().WEST;
        b.m_border = BORDER_LEFT;
        b.m_stones.Clear();
        b.m_con.clear();
	Group& g = m_state.m_groupList[Const().WEST];
	g.m_anchor = b.m_anchor;
	g.m_border = b.m_border;
	g.m_blocks.Clear();
	g.m_blocks.PushBack(b.m_anchor);
        for (int i = 0; i < N; ++i) {
            cell_t p = Const().fatten(i,0);
            b.m_liberties.PushBack(p);
            GetCell(p)->AddBorderConnection(&b);
        }
	for (int i = 1; i < N; ++i) {
	    cell_t p = Const().fatten(i, 1);
	    GetCell(p)->AddBorderConnection(&b);
	    AddCellToConnection(p, Const().WEST, p-1);
	    AddCellToConnection(p, Const().WEST, p - Const().board_row(p) - 3);
	}
    }
    // Create block/group for right edge
    {
        Block& b = m_state.m_blockList[Const().EAST];
        b.m_color = SG_BORDER;
        b.m_anchor = Const().EAST;
        b.m_border = BORDER_RIGHT;
        b.m_stones.Clear();
        b.m_con.clear();
	Group& g = m_state.m_groupList[Const().EAST];
	g.m_anchor = b.m_anchor;
	g.m_border = b.m_border;
	g.m_blocks.Clear();
	g.m_blocks.PushBack(b.m_anchor);
        for (int i = 0; i < N; ++i) {
            cell_t p = Const().fatten(i,i);
            b.m_liberties.PushBack(p);
            GetCell(p)->AddBorderConnection(&b);
        }
	for (int i = 1; i < N; ++i) {
	    cell_t p = Const().fatten(i, i-1);
	    GetCell(p)->AddBorderConnection(&b);
	    AddCellToConnection(p, Const().EAST, p+1);
	    AddCellToConnection(p, Const().EAST, p - Const().board_row(p) - 2);
	}
    }
    // Create block/group for bottom edge
    {
        Block& b = m_state.m_blockList[Const().SOUTH];
        b.m_color = SG_BORDER;
        b.m_anchor = Const().SOUTH;
        b.m_border = BORDER_BOTTOM;
        b.m_stones.Clear();
        b.m_con.clear();
	Group& g = m_state.m_groupList[Const().SOUTH];
	g.m_anchor = b.m_anchor;
	g.m_border = b.m_border;
	g.m_blocks.Clear();
	g.m_blocks.PushBack(b.m_anchor);
        for (int i = 0; i < N; ++i) {
            cell_t p = Const().fatten(N-1,i);
            b.m_liberties.PushBack(p);
            GetCell(p)->AddBorderConnection(&b);
        }
	for (int i = 0; i < N-1; ++i) {
	    cell_t p = Const().fatten(N-2, i);
	    GetCell(p)->AddBorderConnection(&b);
	    AddCellToConnection(p, Const().SOUTH, p + Const().board_row(p) + 4);
	    AddCellToConnection(p, Const().SOUTH, p + Const().board_row(p) + 3);
	}
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

    for (CellIterator i(Const()); i; ++i)
	for (CellNbrIterator j(Const(), *i); j; ++j)
	    if(GetColor(*j) == SG_EMPTY)
		GetCell(*i)->m_NumAdj[SG_EMPTY]++;

    m_state.m_winner   = SG_EMPTY;
    m_state.m_vcWinner = SG_EMPTY;
    m_state.m_lastMove = SG_NULLMOVE;
}

void Board::SetPosition(const Board& other) 
{
    SetSize(other.Size());
    for (CellIterator it(Const()); it; ++it) {
        SgBlackWhite c = other.GetColor(*it);
        if (c != SG_EMPTY)
            Play(c, *it);
    }
    m_state.m_toPlay = other.m_state.m_toPlay;
    m_state.m_lastMove = other.m_state.m_lastMove;
}

void Board::AddSharedLibertiesAroundPoint(Block* b1, cell_t p, cell_t skip)
{
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (*it == skip)
            continue;
        if (GetColor(*it) == SgOppBW(b1->m_color))
            continue;
        if (GetColor(*it) == SG_EMPTY){
            if (!IsAdjacent(*it, b1)) {
                AddCellToConnection(b1->m_anchor, *it, p);
                PromoteConnectionType(*it, b1, b1->m_color);
            }
            continue;
	}
        Block* b2 = GetBlock(*it);
        // std::cerr << "p=" << ToString(p) 
        //           << " b1->anchor=" << ToString(b1->m_anchor) 
        //           << " b2->anchor=" << ToString(b2->m_anchor)
        //           << " it=" << ToString(*it) << '\n';
        if (b1 == b2)
            continue;
        else if (b2->m_color == b1->m_color) {
            AddSharedLiberty(b1, b2);
	    AddCellToConnection(b1->m_anchor, b2->m_anchor, p);
	}
        else if (b2->m_color == SG_BORDER && ((b2->m_border&b1->m_border)==0)){
            AddSharedLiberty(b1, b2);
	    AddCellToConnection(b1->m_anchor, b2->m_anchor, p);
	}
    }
}

void Board::CreateSingleStoneBlock(cell_t p, SgBlackWhite color, int border)
{
    Block* b = m_state.m_block[p] = &m_state.m_blockList[p];
    b->m_anchor = p;
    b->m_color = color;
    b->m_border = border;
    b->m_stones.SetTo(p);
    b->m_liberties.Clear();
    b->m_con.clear();
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY) {
            b->m_liberties.PushBack(*it);
            GetCell(*it)->AddFull(b, color);
            AddSharedLibertiesAroundPoint(b, *it, p);
	}
    }
    m_state.m_activeBlocks[color].push_back(b);
    ComputeGroupForBlock(b);
}

bool Board::IsAdjacent(cell_t p, const Block* b)
{
    for (CellNbrIterator it(Const(), p); it; ++it)
        if (GetBlock(*it) == b)
            return true;
    return false;
}

void Board::AddStoneToBlock(cell_t p, int border, Block* b)
{
    b->m_border |= border;
    b->m_stones.PushBack(p);
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY) {
            if (!IsAdjacent(*it, b)) {
                b->m_liberties.PushBack(*it);

                GetCell(*it)->AddFull(b, b->m_color);
                GetCell(*it)->RemoveSemiConnection(b, b->m_color);
                m_state.m_con[*it][b->m_anchor].m_liberties.Clear();
                m_state.m_con[b->m_anchor][*it].m_liberties.Clear();

                AddSharedLibertiesAroundPoint(b, *it, p);
            }
        }
    }
    m_state.m_block[p] = b;
    m_state.m_group[p] = GetGroup(b->m_anchor);
    RemoveEdgeSharedLiberties(b);
    ComputeGroupForBlock(b);
}

void Board::MergeBlocks(cell_t p, int border, SgArrayList<cell_t, 3>& adjBlocks)
{
    Block* largestBlock = 0;
    int largestBlockStones = 0;
    for (SgArrayList<cell_t,3>::Iterator it(adjBlocks); it; ++it)
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
    m_state.m_group[p] = GetGroup(largestBlock->m_anchor);
    largestBlock->m_border |= border;
    largestBlock->m_stones.PushBack(p);

    bool seen[Y_MAX_CELL];
    memset(seen, 0, sizeof(seen));
    for (Block::LibertyIterator lib(largestBlock->m_liberties); lib; ++lib)
        seen[*lib] = true;
    for (SgArrayList<cell_t,3>::Iterator it(adjBlocks); it; ++it)
    {
        const Block* adjBlock = GetBlock(*it);
        if (adjBlock == largestBlock)
            continue;
	MergeBlockConnections(adjBlock, largestBlock);
	UpdateConnectionsToNewAnchor(adjBlock, largestBlock);
        largestBlock->m_border |= adjBlock->m_border;
        for (Block::StoneIterator stn(adjBlock->m_stones); stn; ++stn)
        {
            largestBlock->m_stones.PushBack(*stn);
            m_state.m_block[*stn] = largestBlock;
	    m_state.m_group[*stn] = GetGroup(largestBlock->m_anchor);
        }
        for (Block::LibertyIterator lib(adjBlock->m_liberties); lib; ++lib) {
            if (!seen[*lib]) {
                seen[*lib] = true;
                largestBlock->m_liberties.PushBack(*lib);
            }
        }
	m_state.RemoveActiveBlock(adjBlock);
    }
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY && !seen[*it]) {
            largestBlock->m_liberties.PushBack(*it);
            AddSharedLibertiesAroundPoint(largestBlock, *it, p);
	}
    }
    RemoveEdgeSharedLiberties(largestBlock);
    ComputeGroupForBlock(largestBlock);
}

void Board::RemoveSharedLiberty(cell_t p, SgArrayList<cell_t, 3>& adjBlocks)
{
    if (adjBlocks.Length() == 0)
        return;

    switch(adjBlocks.Length())
    {
    case 2:
        RemoveCellFromConnection(adjBlocks[0], adjBlocks[1], p);
	UpdateBlockConnection(GetBlock(adjBlocks[0]), GetBlock(adjBlocks[1]));
        break;
    case 3:
        RemoveCellFromConnection(adjBlocks[0], adjBlocks[1], p);
	UpdateBlockConnection(GetBlock(adjBlocks[0]), GetBlock(adjBlocks[1]));
        RemoveCellFromConnection(adjBlocks[0], adjBlocks[2], p);
	UpdateBlockConnection(GetBlock(adjBlocks[0]), GetBlock(adjBlocks[2]));
        RemoveCellFromConnection(adjBlocks[1], adjBlocks[2], p);
	UpdateBlockConnection(GetBlock(adjBlocks[1]), GetBlock(adjBlocks[2]));
        break;
    }

    // now remove between blocks and empty cells
    for (int i = 0; i < adjBlocks.Length(); ++i) {
        for (CellNbrIterator j(Const(), p); j; ++j) {
            if (IsEmpty(*j)) {
                Block* b = GetBlock(adjBlocks[i]);
                if (RemoveCellFromConnection(adjBlocks[i], *j, p)) {
		    if(!IsBorder(adjBlocks[i]))
			DemoteConnectionType(*j, b, b->m_color);
		    else
                    {
                        // shrink it for opponent
			DemoteConnectionType(*j, b, SgOppBW(GetColor(p)));
                        // nuke it for self
                        GetCell(*j)->RemoveSemiConnection(b, GetColor(p)); 
                        GetCell(*j)->RemoveFullConnection(b, GetColor(p));
                        // NOTE: no connection between p and edge for
                        // p's color, so table is just tracking opp(p) and
                        // edge.
                    }
		}
            }
        }        
    }
}

void Board::UpdateBlockConnection(Block* a, Block* b)
{
    if (a->m_color == SG_BORDER && b->m_color == SG_BORDER)
        return;
    if(m_state.m_con[a->m_anchor][b->m_anchor].Size() == 0) {
	RemoveConnectionWith(a, b);
	RemoveConnectionWith(b, a);
    }
}

void Board::Play(SgBlackWhite color, cell_t p) 
{
    // std::cerr << ToString() << '\n'
    //           << "p=" << ToString(p) << '\n'
    //           << "pval=" << (int)p << '\n';
    m_state.m_lastMove = p;    
    m_state.m_toPlay = color;
    m_state.m_color[p] = color;
    int border = 0;
    SgArrayList<cell_t, 3> adjBlocks;
    SgArrayList<cell_t, 3>& oppBlocks = m_state.m_oppBlocks;
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
	else {
	    GetCell(*it)->m_NumAdj[SG_EMPTY]--;
	    GetCell(*it)->m_NumAdj[color]++;
	}
    }
    RemoveSharedLiberty(p, adjBlocks);
    RemoveSharedLiberty(p, oppBlocks);
       
    // Create/update block containing p (ignores edge blocks)
    {
        SgArrayList<cell_t, 3> realAdjBlocks;
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

    // Recompute groups for adjacent opponent blocks.
    if (oppBlocks.Length() > 1) 
    {
        // For each non-edge oppBlock: add all blocks in its group to 
        // list of blocks whose group we need to compute.
        SgArrayList<int, Y_MAX_CELL> blocks, seenGroups;
        for (int i = 0; i < oppBlocks.Length(); ++i)
        {
            if (IsBorder(oppBlocks[i]))
                continue;
            cell_t gAnchor = GroupAnchor(oppBlocks[i]);
            if (!seenGroups.Contains(gAnchor)) {
                seenGroups.PushBack(gAnchor);
                Group* g = GetGroup(gAnchor);
                for (int j = 0; j < g->m_blocks.Length(); ++j) {
                    if (!IsBorder(g->m_blocks[j])) {
                        blocks.Include(BlockAnchor(g->m_blocks[j]));
                        // ^^ Note use of BlockAnchor!!
                        // We are in a transitionary period where the old
                        // groups may use out of date block anchors.
                    }
                }
		ResetBlocksInGroup(GetBlock(gAnchor));
            }
        } 

	int seen[Const().TotalGBCells+10];
	memset(seen, 0, sizeof(seen));
	for(int i = 0; i < blocks.Length(); ++i) {
            Block* b = GetBlock(blocks[i]);
            if (seen[blocks[i]] != 1)
                ComputeGroupForBlock(b, seen, 100 + i);
        }
    }
    
    // Break old win if necessary
    if (HasWinningVC() && GroupBorder(m_state.m_vcGroupAnchor) != BORDER_ALL)
    {
        m_state.m_vcWinner = SG_EMPTY;
    }

    // Check for a new vc win
    if (GroupBorder(p) == BORDER_ALL)
    {
        m_state.m_vcWinner = color;
        m_state.m_vcGroupAnchor = GroupAnchor(p);
    }

    // Check for a solid win
    if (GetBlock(p)->m_border == BORDER_ALL) 
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
    if(m_state.m_con[b1->m_anchor][b2->m_anchor].Size() > 0)
	return m_state.m_con[b1->m_anchor][b2->m_anchor];
    return EMPTY_SHARED_LIBERTIES;
}

void Board::FixOrderOfConnectionsFromBack(Block* b1)
{
    if (!IsBorder(b1->m_con.back())) {
        // ensure edges appear last in list
        size_t i = b1->m_con.size() - 1;
        while (i > 0 && IsBorder(b1->m_con[i - 1])) {
            std::swap(b1->m_con[i - 1], b1->m_con[i]);
            --i;
        }
    }
}

void Board::AddSharedLiberty(Block* b1, Block* b2)
{
    Include(b1->m_con, b2->m_anchor);
    Include(b2->m_con, b1->m_anchor);
    FixOrderOfConnectionsFromBack(b1);
    FixOrderOfConnectionsFromBack(b2);
}

void Board::RemoveConnectionAtIndex(Block* b, size_t i)
{
    b->m_con[i] = b->m_con.back();
    b->m_con.pop_back();
    // ensure edges always appear last in list
    if (IsBorder(b->m_con[i])) {
        while (i + 1 < b->m_con.size() 
               && !IsBorder(b->m_con[i+1])) 
        {
            std::swap(b->m_con[i], b->m_con[i+1]);
            ++i;
        }
    }
}

void Board::RemoveConnectionWith(Block* b, const Block* other)
{
    int i = b->GetConnectionIndex(other);
    if (i != -1)
        RemoveConnectionAtIndex(b, i);
}

// Merge b1 into b2
void Board::MergeBlockConnections(const Block* b1, Block* b2)
{
    for (size_t i = 0; i < b1->m_con.size(); ++i) {
        const cell_t otherAnchor = b1->m_con[i];
	if (otherAnchor == b2->m_anchor)
            continue;
        Block* otherBlock = GetBlock(otherAnchor);
        assert(otherBlock);     
        int index = b2->GetConnectionIndex(otherBlock);
        if (index != -1) {
            RemoveConnectionWith(otherBlock, b1);
        }
        else {
            b2->m_con.push_back(b1->m_con[i]);
            FixOrderOfConnectionsFromBack(b2);

            const int j = otherBlock->GetConnectionIndex(b1);
            otherBlock->m_con[j] = b2->m_anchor;
            // No need to worry about order here, since b1 and b2
            // are both not border blocks and otherBlock's list
            // is properly ordered.
        }
    }
    // Remove mention of any connections from b2->b1, since b1 is now
    // part of b2. 
    RemoveConnectionWith(b2, b1);
}

void Board::RemoveEdgeSharedLiberties(Block* b)
{
    if (b->m_border & BORDER_LEFT) {
        RemoveConnectionWith(b, GetBlock(Const().WEST));
        RemoveConnectionWith(GetBlock(Const().WEST), b);
    }
    if (b->m_border & BORDER_RIGHT) {
        RemoveConnectionWith(b, GetBlock(Const().EAST));
        RemoveConnectionWith(GetBlock(Const().EAST), b);
    }
    if (b->m_border & BORDER_BOTTOM) {
        RemoveConnectionWith(b, GetBlock(Const().SOUTH));
        RemoveConnectionWith(GetBlock(Const().SOUTH), b);
    }
}

int Board::NumUnmarkedSharedLiberties(const SharedLiberties& lib, 
                                      int* seen, int id)
{
    int count = 0;
    for (size_t i = 0; i < lib.Size(); ++i)
        if (seen[lib.m_liberties[i]] != id)
            ++count;
    return count;
}

void Board::MarkLibertiesAsSeen(const SharedLiberties& lib, int* seen, int id)
{
    for (size_t j = 0; j < lib.Size(); ++j)
        seen[lib.m_liberties[j]] = id;
}

void Board::ResetBlocksInGroup(Block* b)
{
    Group* g = GetGroup(b->m_anchor);
    if( g != NULL) {
	Group* g2;
	Block* b2;
	// Start from 1 since first block in list is g
	for( int i = 1; i < g->m_blocks.Length(); ++i) {
	    if(IsBorder(g->m_blocks[i]))
		continue;
	    b2 = GetBlock(g->m_blocks[i]);
	    g2 = m_state.m_group[b2->m_anchor] = &m_state.m_groupList[b2->m_anchor];
	    g2->m_anchor = b2->m_anchor;
	    g2->m_border = b2->m_border;
	    g2->m_blocks.Clear();
	    g2->m_blocks.PushBack(g2->m_anchor);
	}
    }
    g = m_state.m_group[b->m_anchor] = &m_state.m_groupList[b->m_anchor];
    g->m_anchor = b->m_anchor;
    g->m_border = b->m_border;
    g->m_blocks.Clear();
    g->m_blocks.PushBack(g->m_anchor);
}

void Board::ComputeGroupForBlock(Block* b)
{
    SgArrayList<int, Y_MAX_CELL> blocks, seenGroups;
    blocks.PushBack(b->m_anchor);

    for (size_t i = 0; i < b->m_con.size(); ++i)
    {
	cell_t other = b->m_con[i];
        if (IsBorder(other))
            continue;
        cell_t gAnchor = GroupAnchor(other);
        if (!seenGroups.Contains(gAnchor)) {
            seenGroups.PushBack(gAnchor);
            Group* g = GetGroup(gAnchor);
            for (int j = 0; j < g->m_blocks.Length(); ++j) {
                if (!IsBorder(g->m_blocks[j])) {
                    blocks.Include(BlockAnchor(g->m_blocks[j]));
                    // ^^ Note use of BlockAnchor!!
                    // We are in a transitionary period where the old
                    // groups may use out of date block anchors.
                }
            }
	    ResetBlocksInGroup(GetBlock(gAnchor));
        }
    }    

    ResetBlocksInGroup(b);
    int seen[Const().TotalGBCells+10];
    memset(seen, 0, sizeof(seen));
    for(int i = 0; i < blocks.Length(); ++i) {
        if (seen[blocks[i]] != 1)
            ComputeGroupForBlock(GetBlock(blocks[i]), seen, 100 + i);
    }
}

void Board::ComputeGroupForBlock(Block* b, int* seen, int id)
{
    //std::cerr << "ComputeGroupForBlock: " << ToString(b->m_anchor) << '\n';

    GroupSearch(seen, b, id);

    Group* g = GetGroup(b->m_anchor);

    //Currently breaks proven wins at root.
    //AddNonGroupEdges(seen, g, id);

    // Add edge to group if we are touching
    if ((g->m_border & BORDER_LEFT) && !g->m_blocks.Contains(Const().WEST))
        g->m_blocks.PushBack(Const().WEST);
    if ((g->m_border & BORDER_RIGHT) && !g->m_blocks.Contains(Const().EAST))
        g->m_blocks.PushBack(Const().EAST);
    if ((g->m_border & BORDER_BOTTOM) && !g->m_blocks.Contains(Const().SOUTH))
        g->m_blocks.PushBack(Const().SOUTH);
}

void Board::GroupSearch(int* seen, Block* b, int id)
{
    seen[b->m_anchor] = 1;
    //std::cerr << "Entered: " << ToString(b->m_anchor) << '\n';
    for (size_t i = 0; i < b->m_con.size(); ++i)
    {
        bool visit = false;
	cell_t other = b->m_con[i];
        SharedLiberties& sl = m_state.m_con[b->m_anchor][other];

	//std::cerr << "Considering: " << ToString(other) << '\n';
        int count = NumUnmarkedSharedLiberties(sl, seen, id);
        // Never seen this block before:
        // visit it if we have a vc and mark it as potential if we have a semi
	if (seen[other] == 0) 
        {
            if (count > 1)
                visit = true;
            else if (count == 1) {
                //std::cerr << "Marking as potential!\n";
                seen[other] = id;
                MarkLibertiesAsSeen(sl, seen, id);
            }
        }
        // block is marked as potential:
        // visit it if we have a vc or a semi (since the other semi
        // we found earlier combined with this semi gives a vc)
        else if (seen[other] == id && count >= 1)
        {
            visit = true;
        }

        if (visit)
	{
	    //std::cerr << "Inside!\n";
	    Group* g = GetGroup(b->m_anchor);
	    g->m_border |= GetBlock(other)->m_border;
            g->m_blocks.PushBack(other);
	    if(GetColor(other) != SG_BORDER) {
                // Note that we are not marking liberties with an edge
                // block: we claim these cannot conflict with group
                // formation.
                MarkLibertiesAsSeen(sl, seen, id);
                m_state.m_group[other] = g;
		GroupSearch(seen, GetBlock(other), id);
            }
            //std::cerr << "Back to: " << ToString(b->m_anchor) << '\n';
	}
    }
}

void Board::AddNonGroupEdges(int* seen, Group* g, int id)
{
    int w, e, s;
    w = e = s = 0;
    for(CellIterator it(Const()); it; ++it) {
	if(seen[*it] == id && GetColor(*it) == ToPlay()) {
	    int border = GetBlock(*it)->m_border;
	    if(border == BORDER_LEFT) w++;
	    else if(border == BORDER_RIGHT) e++;
	    else if(border == BORDER_BOTTOM) s++;
	}
    }
    if(w > 1) g->m_border |= BORDER_LEFT;
    if(e > 1) g->m_border |= BORDER_RIGHT;
    if(s > 1) g->m_border |= BORDER_BOTTOM;
}

void Board::RemoveStone(cell_t p)
{
    std::vector<SgBoardColor> m_oldColor(m_state.m_color);
    m_oldColor[p] = SG_EMPTY;
    SetSize(Size());
    for (CellIterator it(Const()); it; ++it)
        if (m_oldColor[*it] != SG_EMPTY)
            Play(m_oldColor[*it],*it);
}

void Board::Swap()
{
    for (CellIterator it(Const()); it; ++it) {
	if(GetColor(*it) != SG_EMPTY) {
            m_state.m_color[*it] = SgOppBW(GetColor(*it));
            if (*it == BlockAnchor(*it)) {
                Block* b = GetBlock(*it);
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
    for (CellIterator it(Const()); it; ++it) {
	SgBlackWhite color = b.m_color[*it];
        if (color != SG_EMPTY) {
            a.m_block[*it] = &a.m_blockList[b.m_block[*it]->m_anchor];
	    a.m_group[*it] = &a.m_groupList[b.m_group[*it]->m_anchor];
	    if (!a.IsActive(a.m_block[*it]))
		a.m_activeBlocks[color].push_back(a.m_block[*it]);
	}
	else {
	    a.m_cell[*it] = &a.m_cellList[*it];
	}
    }
}

void Board::CheckConsistency()
{
    // FIXME: ADD MORE CHECKS HERE!!
    DumpBlocks();
    for (CellIterator it(Const()); it; ++it) {
        int color = GetColor(*it);
        if (color != SG_BLACK && color != SG_WHITE && color != SG_EMPTY)
        {
            std::cerr << ToString();
            std::cerr << ToString(*it) << " color = " 
                      << GetColor(*it) << '\n';
            abort();
        }
        
        if ((color == SG_BLACK || color == SG_WHITE) 
            && GetBlock(*it)->m_anchor == *it) 
        {
            Block* b = GetBlock(*it);
            for (size_t i = 0; i < b->m_con.size(); ++i) {
                if (!Const().IsOnBoard(b->m_con[i])
                    || GetColor(b->m_con[i]) == SG_EMPTY)
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
        if(GetBlock(i) != NULL) {
            const Block * b = GetBlock(i);
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

std::string Board::ToString()
{
    ostringstream os;
    const int N = Size();
    os << "\n";
    for (int j = 0; j < N; j++) {
        for (int k = 0; k < N-j; k++) 
            os << ' ';
        os << (j+1<10?" ":"") << j+1 << "  "; 
        for (int k = 0; k <= j ; k++) 
            os << ConstBoard::ColorToChar(GetColor(Const().fatten(j,k))) << "  ";
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
    const int N = Size() + 2*ConstBoard::GUARDS;

    os << "\n";
    for (int j = 0; j < N; j++) {
        for (int k = 0; k < N-j; k++) 
            os << ' ';
        os << (j+1<10?" ":"") << j+1 << "  "; 
        for (int k = 0; k <= j+1 ; k++) 
        {
            char c = '*';
            cell_t p =  Const().fatten(j-1,k-1);
	    if (GetBlock(p) != NULL) {
                if (GetBlock(p)->m_border != BORDER_NONE)
                    c = '0' + GetBlock(p)->m_border;
            }
            os << "  " << c;
            
        }
        os << "\n";
    }
    os << "   ";
    for (char ch='a'; ch < 'a'+N; ch++) 
        os << ' ' << ch << ' '; 
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
	    if(GetBlock(psn) != NULL) {
		if(Const().board_row(psn)+1 < 10)
		    os << ' ';
		os << ToString(BlockAnchor(psn));
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

void Board::UpdateConnectionsToNewAnchor(const Block* from, const Block* to)
{
    cell_t p1 = from->m_anchor;
    cell_t p2 = to->m_anchor;
    for(CellAndEdgeIterator i(Const()); i; ++i) 
    {
        if (GetColor(*i) == SG_EMPTY) 
        {
            GetCell(*i)->RemoveSemiConnection(from, from->m_color);
            GetCell(*i)->RemoveFullConnection(from, from->m_color);

            // Liberties of new captain don't need to be changed
            if (to->m_liberties.Contains(*i))
                continue;
            
            // Liberties of from now are adjacent to to
            if (from->m_liberties.Contains(*i)) {
                GetCell(*i)->RemoveSemiConnection(to, to->m_color);
                GetCell(*i)->AddFull(to, to->m_color);
                continue;
            }
        } 
        else 
        {
            if (GetColor(*i) != to->m_color)
                continue;
            if (!IsBlockAnchor(*i))
                continue;
            // TODO: remove mention to connection to from in this block
        }
        
        // No liberties to transfer
        if (m_state.m_con[p1][*i].Size() == 0)
            continue;

        for(size_t j = 0; j < m_state.m_con[p1][*i].Size(); ++j) {
            AddCellToConnection(p2, *i, m_state.m_con[p1][*i].m_liberties[j]);
        }
        if (IsEmpty(*i)) {
            PromoteConnectionType(*i, to, to->m_color);
        }
    }
}

void Board::PromoteConnectionType(cell_t p, const Block* b, SgBlackWhite color)
{
    Cell* cell = GetCell(p);
    if (cell->IsFullConnected(b, color))
        return;
    int size = (int)m_state.m_con[p][b->m_anchor].Size();
    if (size == 1) {
	cell->AddSemi(b, color);
    }
    else {        
        cell->RemoveSemiConnection(b, color);
        cell->AddFull(b, color);
    }
}

void Board::DemoteConnectionType(cell_t p, Block* b, SgBlackWhite color)
{
    // Just removed a cell from this connection
    Cell* cell = GetCell(p);
    int size = (int)m_state.m_con[p][b->m_anchor].Size();
    if (size >= 2) {
        if (!cell->IsFullConnected(b, color))
        {
            std::cerr << "MORE THAN 2 LIBERTIES BUT NOT FULL CONNECTED!!\n";
            std::cerr << ToString() << '\n'
                      << " p = " << ToString(p)
                      << " b = " << ToString(b->m_anchor) 
                      << '\n';
            abort();
        }
        return;
    }
    if (size == 0) {
        cell->RemoveSemiConnection(b, color);
        
    }
    else if(size == 1) {
        cell->RemoveFullConnection(b, color);
	cell->AddSemi(b, color);
    }
}

//////////////////////////////////////////////////////////////////////

std::vector<cell_t> Board::FullConnectedTo(cell_t p, SgBlackWhite c) const
{
    std::vector<cell_t> ret;
    if (IsEmpty(p)) {
        Append(ret, GetCell(p)->m_FullConnects[c]);
        return ret;
    }
    const Block* b = GetBlock(p);
    for (size_t i = 0; i < b->m_con.size(); ++i) {
        if (GetBlock(b->m_con[i])->m_color == c
            && m_state.m_con[b->m_anchor][b->m_con[i]].Size() > 1)
            ret.push_back(b->m_con[i]);
    }
    for (CellIterator i(Const()); i; ++i) {
        if (IsOccupied(*i))
            continue;
        if (GetCell(*i)->IsFullConnected(b, c))
            ret.push_back(*i);
    }
    return ret;
}

std::vector<cell_t> Board::SemiConnectedTo(cell_t p, SgBlackWhite c) const
{
    std::vector<cell_t> ret;
    if (IsEmpty(p)) {
        Append(ret, GetCell(p)->m_SemiConnects[c]);
        return ret;
    }
    const Block* b = GetBlock(p);
    for (size_t i = 0; i < b->m_con.size(); ++i) {
        if (GetBlock(b->m_con[i])->m_color == c
            && m_state.m_con[b->m_anchor][b->m_con[i]].Size() == 1)
            ret.push_back(b->m_con[i]);
    }
    for (CellIterator i(Const()); i; ++i) {
        if (IsOccupied(*i))
            continue;
        if (GetCell(*i)->IsSemiConnected(b, c))
            ret.push_back(*i);
    }
    return ret;
}

std::vector<cell_t> Board::GetCarrierBetween(cell_t p1, cell_t p2) const 
{
    if (IsOccupied(p1))
        p1 = GetBlock(p1)->m_anchor;
    if (IsOccupied(p2))
        p2 = GetBlock(p2)->m_anchor;
    std::vector<cell_t> ret;
    Append(ret, m_state.m_con[p1][p2].m_liberties);
    return ret;
}

//////////////////////////////////////////////////////////////////////

SgMove Board::MaintainConnection(cell_t b1, cell_t b2) const
{
    const SharedLiberties& libs = GetSharedLiberties(b1, b2);
    if (libs.Size() != 1)
        return SG_NULLMOVE;
    const Group* g1 = GetGroup(b1);
    const Group* g2 = GetGroup(b2);
    if (GetColor(b1) == SG_BORDER || GetColor(b2) == SG_BORDER) {
        if ((g1->m_border & g2->m_border) == 0)
            return libs.m_liberties[0];
    } else if (g1 != g2)
        return libs.m_liberties[0];
    return SG_NULLMOVE;
}

void Board::GeneralSaveBridge(LocalMoves& local) const
{
    const SgArrayList<cell_t, 3>& opp = m_state.m_oppBlocks;
    for (int i = 0; i < opp.Length() - 1; ++i) {
        for (int j = i + 1; j < opp.Length(); ++j) {
            SgMove move = MaintainConnection(opp[i], opp[j]);
            if (move != SG_NULLMOVE) {
                local.AddWeight(move, LocalMoves::WEIGHT_SAVE_BRIDGE);
            }
        }    
    }
}

int Board::SaveBridge(cell_t lastMove, const SgBlackWhite toPlay, 
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
        const cell_t p = Const().m_cell_nbr[lastMove][i];
        const bool mine = GetColor(p) == toPlay;
        if (s == 0)
        {
            if (mine) s = 1;
        }
        else if (s == 1)
        {
            if (mine) s = 1;
            else if (GetColor(p) == !toPlay) s = 0;
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

bool Board::IsCellDead(cell_t p) const
{
    if (NumNeighbours(p, SG_EMPTY) > 2)
        return false;
    if (NumNeighbours(p, SG_BLACK) >= 4)
        return true;
    if (NumNeighbours(p, SG_WHITE) >= 4)
        return true;

    SgBlackWhite lastColor = -1;
    int s = 0;
    for (int j = 0; j < 12; ++j)
    {
	const int i = j % 6;
	const SgBlackWhite color = GetColor(Const().m_cell_nbr[p][i]);
	if (s == 0)
	{
	    if (color == SG_BLACK || color == SG_WHITE) s = 1;
	}
	else if (s == 1)
	{
	    if (color == lastColor) s = 2;
	    else if (color == SgOppBW(lastColor)) s = 1;
	    else s = 0;
	}
	else if (s == 2)
	{
	    if (color == lastColor) s = 3;
	    else if(GetColor(Const().m_cell_nbr[p][(i+1)%6]) 
		    == SgOppBW(lastColor) && 
		    GetColor(Const().m_cell_nbr[p][(i+2)%6])
		    == SgOppBW(lastColor))
		return true;
	    else s = 0;
	}
	else if (s == 3)
	{
	    if (color == lastColor) return true;
	    else if(GetColor(Const().m_cell_nbr[p][(i+1)%6])
		    == SgOppBW(lastColor))
		return true;
	    else s = 0;
	}
	lastColor = color;
    }
    return false;
}

