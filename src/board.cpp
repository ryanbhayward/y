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
    , TotalCells(m_size*(m_size+1)/2 + 3)
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
    
    m_cell_nbr.resize(TotalCells);
    for (int r=0; r<Size(); r++) {
        for (int c=0; c<=r; c++) {
            cell_t p = fatten(r,c);
            m_cell_nbr[p].resize(6);
            // spin clockwise from top left neighbor
            m_cell_nbr[p][ DIR_NW ] = (c == 0) ? WEST : p - r - 1;
            m_cell_nbr[p][ DIR_NE ] = (c == r) ? EAST : p - r;
            m_cell_nbr[p][ DIR_E  ] = (c == r) ? EAST : p + 1;
            m_cell_nbr[p][ DIR_SE ] = (r == Size()-1) ? SOUTH : p + (r + 1) + 1;
            m_cell_nbr[p][ DIR_SW ] = (r == Size()-1) ? SOUTH : p + (r + 1);
            m_cell_nbr[p][ DIR_W  ] = (c == 0) ? WEST : p - 1;
        }
    }
}

//----------------------------------------------------------------------

Board::State::State(int T)
{ 
    Init(T);
}

void Board::State::Init(int T)
{
    m_color.reset(new SgBoardColor[T]);
    m_cellList.reset(new Cell[T]);
    m_blockIndex.reset(new cell_t[T]);
    m_blockList.reset(new Block[T]);
    m_groupIndex.reset(new cell_t[T]);
    m_groupList.reset(new Group[T]);
    m_conData.reset(new Carrier[T*(T+1)/2]);

    for (int i = 0; i < T; ++i)
        m_color[i] = SG_EMPTY;
    memset(m_blockIndex.get(), -1, sizeof(m_blockIndex.get()));
    memset(m_groupIndex.get(), -1, sizeof(m_groupIndex.get()));

    m_con.reset(new Carrier*[T]);
    for(int i = 0, j = 0; i < T; ++i) {
        m_con[i] = &m_conData[j];
        j += T - i;
    }
}

Board::Board(int size)
{ 
    SetSize(size);
}

void Board::SetSize(int size)
{
    m_constBrd = ConstBoard(size);
    m_state.Init(m_constBrd.TotalCells);
    m_savePoint1.Init(m_constBrd.TotalCells);
    m_savePoint2.Init(m_constBrd.TotalCells);
    
    const int N = Size();
    // Create block/group for left edge
    {
        Block& b = m_state.m_blockList[Const().WEST];
        m_state.m_blockIndex[Const().WEST] = Const().WEST;
        m_state.m_color[Const().WEST] = SG_BORDER;
        b.m_color = SG_BORDER;
        b.m_anchor = Const().WEST;
        b.m_border = BORDER_WEST;
        b.m_stones.Clear();
        b.m_con.Clear();
	Group& g = m_state.m_groupList[Const().WEST];
        m_state.m_groupIndex[Const().WEST] = Const().WEST;
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
	    AddCellToConnection(p, Const().WEST, PointInDir(p, Const().DIR_W));
	    AddCellToConnection(p, Const().WEST, PointInDir(p, Const().DIR_NW));
	}
    }
    // Create block/group for right edge
    {
        Block& b = m_state.m_blockList[Const().EAST];
        m_state.m_blockIndex[Const().EAST] = Const().EAST;
        m_state.m_color[Const().EAST] = SG_BORDER;
        b.m_color = SG_BORDER;
        b.m_anchor = Const().EAST;
        b.m_border = BORDER_EAST;
        b.m_stones.Clear();
        b.m_con.Clear();
	Group& g = m_state.m_groupList[Const().EAST];
        m_state.m_groupIndex[Const().EAST] = Const().EAST;
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
	    AddCellToConnection(p, Const().EAST, PointInDir(p, Const().DIR_E));
	    AddCellToConnection(p, Const().EAST, PointInDir(p, Const().DIR_NE));
	}
    }
    // Create block/group for bottom edge
    {
        Block& b = m_state.m_blockList[Const().SOUTH];
        m_state.m_blockIndex[Const().SOUTH] = Const().SOUTH;
        m_state.m_color[Const().SOUTH] = SG_BORDER;
        b.m_color = SG_BORDER;
        b.m_anchor = Const().SOUTH;
        b.m_border = BORDER_SOUTH;
        b.m_stones.Clear();
        b.m_con.Clear();
	Group& g = m_state.m_groupList[Const().SOUTH];
        m_state.m_groupIndex[Const().SOUTH] = Const().SOUTH;
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
	    AddCellToConnection(p, Const().SOUTH, PointInDir(p,Const().DIR_SW));
	    AddCellToConnection(p, Const().SOUTH, PointInDir(p,Const().DIR_SE));
	}
    }

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

void Board::RemoveStone(cell_t p)
{
    boost::scoped_array<SgBoardColor> 
        m_oldColor(new SgBoardColor[Const().TotalCells]);
    memcpy(m_oldColor.get(), m_state.m_color.get(), 
           sizeof(SgBoardColor)*Const().TotalCells);
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
    const int T = Const().TotalCells;
    memcpy(a.m_color.get(), b.m_color.get(), sizeof(SgBoardColor)*T);
    memcpy(a.m_cellList.get(), b.m_cellList.get(), sizeof(Cell)*T);
    memcpy(a.m_blockIndex.get(), b.m_blockIndex.get(), sizeof(cell_t)*T);
    memcpy(a.m_blockList.get(), b.m_blockList.get(), sizeof(Block)*T);
    memcpy(a.m_groupIndex.get(), b.m_groupIndex.get(), sizeof(cell_t)*T);
    memcpy(a.m_groupList.get(), b.m_groupList.get(), sizeof(Group)*T);
    memcpy(a.m_conData.get(), b.m_conData.get(),
           sizeof(Carrier)*T*(T+1)/2);

    a.m_oppBlocks = b.m_oppBlocks;

    a.m_toPlay = b.m_toPlay;
    a.m_winner = b.m_winner;
    a.m_vcWinner = b.m_vcWinner;
    a.m_vcGroupAnchor = b.m_vcGroupAnchor;
    a.m_lastMove = b.m_lastMove;
}

//---------------------------------------------------------------------------

void Board::PromoteConnectionType(cell_t p, const Block* b, SgBlackWhite color)
{
    Cell* cell = GetCell(p);
    if (cell->IsFullConnected(b, color))
        return;
    int size = (int)GetConnection(p, b->m_anchor).Size();
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
    int size = (int)GetConnection(p, b->m_anchor).Size();
    if (size >= 2) {
        if (!cell->IsFullConnected(b, color))
        {
            std::cerr << "MORE THAN 2 LIBERTIES BUT NOT FULL CONNECTED!!\n";
            std::cerr << ToString() << '\n'
                      << " p = " << ToString(p)
                      << " b = " << ToString(b->m_anchor) 
                      << '\n';
            for (int i =0; i < GetConnection(p, b->m_anchor).Size(); ++i) {
                std::cerr << ' ' << ToString(GetConnection(p, b->m_anchor)[i]);
                std::cerr << '\n';
            }
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
    Block* b = &m_state.m_blockList[p];
    m_state.m_blockIndex[p] = p;
    b->m_anchor = p;
    b->m_color = color;
    b->m_border = border;
    b->m_stones.SetTo(p);
    b->m_liberties.Clear();
    b->m_con.Clear();
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY) {
            b->m_liberties.PushBack(*it);
            GetCell(*it)->AddFull(b, color);
            AddSharedLibertiesAroundPoint(b, *it, p);
	} else if (GetColor(*it) == SG_BORDER) {
            AddSharedLiberty(b, GetBlock(*it));
        }
    }
    m_state.m_groupIndex[p] = p;
    m_state.m_groupList[p] = Group(p, border);
    ComputeGroupForBlock(b);
}

bool Board::IsAdjacent(cell_t p, const Block* b)
{
    for (CellNbrIterator it(Const(), p); it; ++it)
        if (BlockIndex(*it) == b->m_anchor)
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
                GetConnection(*it, b->m_anchor).Clear();

                AddSharedLibertiesAroundPoint(b, *it, p);
            }
        }
    }
    m_state.m_blockIndex[p] = b->m_anchor;
    m_state.m_groupIndex[p] = m_state.m_groupIndex[b->m_anchor];
    RemoveEdgeSharedLiberties(b);
    ComputeGroupForBlock(b);
}

// Merge b1 into b2
void Board::MergeBlockConnections(const Block* b1, Block* b2)
{
    for (int i = 0; i < b1->m_con.Length(); ++i) {
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
            b2->m_con.PushBack(b1->m_con[i]);

            const int j = otherBlock->GetConnectionIndex(b1);
            otherBlock->m_con[j] = b2->m_anchor;
        }
    }
    // Remove mention of any connections from b2->b1, since b1 is now
    // part of b2. 
    RemoveConnectionWith(b2, b1);
}

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
        if (GetConnection(p1,*i).Size() == 0)
            continue;

        for(int j = 0; j < GetConnection(p1,*i).Size(); ++j) {
            AddCellToConnection(p2, *i, GetConnection(p1, *i)[j]);
        }
        if (IsEmpty(*i)) {
            PromoteConnectionType(*i, to, to->m_color);
        }
    }
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
    m_state.m_blockIndex[p] = largestBlock->m_anchor;
    m_state.m_groupIndex[p] = m_state.m_groupIndex[largestBlock->m_anchor];
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
            m_state.m_blockIndex[*stn] = largestBlock->m_anchor;
	    m_state.m_groupIndex[*stn] 
                = m_state.m_groupIndex[largestBlock->m_anchor];
        }
        for (Block::LibertyIterator lib(adjBlock->m_liberties); lib; ++lib) {
            if (!seen[*lib]) {
                seen[*lib] = true;
                largestBlock->m_liberties.PushBack(*lib);
            }
        }
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

// TODO: switch to the promote/demote method
void Board::UpdateBlockConnection(Block* a, Block* b)
{
    if (a->m_color == SG_BORDER && b->m_color == SG_BORDER)
        return;
    if (GetConnection(a->m_anchor, b->m_anchor).Size() == 0) {
	RemoveConnectionWith(a, b);
	RemoveConnectionWith(b, a);
    }
}


void Board::RemoveEdgeSharedLiberties(Block* b)
{
    if (b->m_border & BORDER_WEST)
        GetConnection(Const().WEST, b->m_anchor).Clear();
    if (b->m_border & BORDER_EAST)
        GetConnection(Const().EAST, b->m_anchor).Clear();
    if (b->m_border & BORDER_SOUTH)
        GetConnection(Const().SOUTH, b->m_anchor).Clear();
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

	int seen[Const().TotalCells+10];
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

const Board::Carrier& 
Board::GetCarrier(const Block* b1, const Block* b2) const
{
    // FIXME:: IS THIS THREADSAFE!?
    static Carrier EMPTY_SHARED_LIBERTIES;
    if (b1 == b2 || b1 == 0 || b2 == 0)
	return EMPTY_SHARED_LIBERTIES;
    return GetConnection(b1->m_anchor,b2->m_anchor);
}

void Board::AddSharedLiberty(Block* b1, Block* b2)
{
    b1->m_con.Include(b2->m_anchor);
    b2->m_con.Include(b1->m_anchor);
}

void Board::RemoveConnectionWith(Block* b, const Block* other)
{
    int i = b->GetConnectionIndex(other);
    if (i != -1) {
        b->m_con[i] = b->m_con.Last();
        b->m_con.PopBack();
    }
}

//---------------------------------------------------------------------------

int Board::NumUnmarkedSharedLiberties(const Carrier& lib, 
                                      int* seen, int id)
{
    int count = 0;
    for (int i = 0; i < lib.Size(); ++i)
        if (seen[lib[i]] != id)
            ++count;
    return count;
}

void Board::MarkLibertiesAsSeen(const Carrier& lib, int* seen, int id)
{
    for (int j = 0; j < lib.Size(); ++j)
        seen[lib[j]] = id;
}

void Board::ResetBlocksInGroup(Block* b)
{
    Group* g = GetGroup(b->m_anchor);
    // Start from 1 since first block in list is g
    for(int i = 1; i < g->m_blocks.Length(); ++i) {
        if (IsBorder(g->m_blocks[i]))
            continue;
        Block* b2 = GetBlock(g->m_blocks[i]);
        m_state.m_groupIndex[b2->m_anchor] = b2->m_anchor;
        Group* g2 = GetGroup(b2->m_anchor);
        g2->m_anchor = b2->m_anchor;
        g2->m_border = b2->m_border;
        g2->m_blocks.Clear();
        g2->m_blocks.PushBack(g2->m_anchor);
    }
    g->m_border = b->m_border;
    g->m_anchor = b->m_anchor;
    g->m_blocks.Clear();
    g->m_blocks.PushBack(g->m_anchor);
}

void Board::ComputeGroupForBlock(Block* b)
{
    SgArrayList<cell_t, Y_MAX_CELL> blocks, seenGroups;
    blocks.PushBack(b->m_anchor);

    for (int i = 0; i < b->m_con.Length(); ++i)
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
    int seen[Const().TotalCells+10];
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
    if ((g->m_border & BORDER_WEST) && !g->m_blocks.Contains(Const().WEST))
        g->m_blocks.PushBack(Const().WEST);
    if ((g->m_border & BORDER_EAST) && !g->m_blocks.Contains(Const().EAST))
        g->m_blocks.PushBack(Const().EAST);
    if ((g->m_border & BORDER_SOUTH) && !g->m_blocks.Contains(Const().SOUTH))
        g->m_blocks.PushBack(Const().SOUTH);
}

void Board::GroupSearch(int* seen, Block* b, int id)
{
    seen[b->m_anchor] = 1;
    //std::cerr << "Entered: " << ToString(b->m_anchor) << '\n';
    for (int i = 0; i < b->m_con.Length(); ++i)
    {
        bool visit = false;
	cell_t other = b->m_con[i];
        Carrier& sl = GetConnection(b->m_anchor, other);

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
                m_state.m_groupIndex[other] = g->m_anchor;
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
	    if(border == BORDER_WEST) w++;
	    else if(border == BORDER_EAST) e++;
	    else if(border == BORDER_SOUTH) s++;
	}
    }
    if(w > 1) g->m_border |= BORDER_WEST;
    if(e > 1) g->m_border |= BORDER_EAST;
    if(s > 1) g->m_border |= BORDER_SOUTH;
}

//---------------------------------------------------------------------------


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
            for (int i = 0; i < b->m_con.Length(); ++i) {
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
    for(CellAndEdgeIterator i(Const()); i; ++i) {
        if (GetColor(*i) != SG_EMPTY) {
            const Block * b = GetBlock(*i);
            std::cerr << "id=" << ToString(*i)  << ' ' 
                      << b->ToString(Const()) << '\n';
        }
    }
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
            os << ConstBoard::ColorToChar(GetColor(Const().fatten(j,k)))<< "  ";
        os << "\n";
    }
    os << "   ";
    for (char ch='a'; ch < 'a'+N; ch++) 
        os << ' ' << ch << ' '; 

    return os.str();
}

//---------------------------------------------------------------------------

SgMove Board::MaintainConnection(cell_t b1, cell_t b2) const
{
    const Carrier& libs = GetCarrier(b1, b2);
    if (libs.Size() != 1)
        return SG_NULLMOVE;
    const Group* g1 = GetGroup(b1);
    const Group* g2 = GetGroup(b2);
    if (GetColor(b1) == SG_BORDER || GetColor(b2) == SG_BORDER) {
        if ((g1->m_border & g2->m_border) == 0)
            return libs[0];
    } else if (g1 != g2)
        return libs[0];
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

//---------------------------------------------------------------------------

std::vector<cell_t> Board::FullConnectedTo(cell_t p, SgBlackWhite c) const
{
    std::vector<cell_t> ret;
    if (IsEmpty(p)) 
    {
        Append(ret, GetCell(p)->m_FullConnects[c]);
        return ret;
    }
    const Block* b = GetBlock(p);
    if (IsBorder(p))
    {
        // If edge: need to look on for given color
        for (int i = 0; i < b->m_con.Length(); ++i) {
            if (GetBlock(b->m_con[i])->m_color == c
                && GetConnection(b->m_anchor, b->m_con[i]).Size() != 1)
                ret.push_back(b->m_con[i]);
        }
    }
    else if (c == b->m_color)
    {
        // look from block's perspective
        for (int i = 0; i < b->m_con.Length(); ++i) {
            if (GetConnection(b->m_anchor, b->m_con[i]).Size() != 1)
                ret.push_back(b->m_con[i]);
        }
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
    if (IsBorder(p)) 
    {
        for (int i = 0; i < b->m_con.Length(); ++i) {
            if (GetBlock(b->m_con[i])->m_color == c
                && GetConnection(b->m_anchor, b->m_con[i]).Size() == 1)
                ret.push_back(b->m_con[i]);
        }
    }
    else if (c == b->m_color)
    {
        for (int i = 0; i < b->m_con.Length(); ++i) {
            if (GetConnection(b->m_anchor, b->m_con[i]).Size() == 1)
                ret.push_back(b->m_con[i]);
        }
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
    Append(ret, GetConnection(p1, p2));
    return ret;
}

