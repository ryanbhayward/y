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

std::string ConstBoard::ToString(SgMove move)
{
    if (move == Y_SWAP)
        return "swap";
    else if (move == WEST)
        return "west";
    else if (move == EAST)
        return "east";
    else if (move == SOUTH)
        return "south";
    char str[16];
    sprintf(str, "%c%1d",board_col(move)+'a',board_row(move)+1); 
    return str;
}

SgMove ConstBoard::FromString(const string& name)
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
    memset(m_blockIndex.get(), -1, sizeof(cell_t)*T);
    memset(m_groupIndex.get(), -1, sizeof(cell_t)*T);

    m_con.reset(new Carrier*[T]);
    for(int i = 0, j = 0; i < T; ++i) {
        m_con[i] = &m_conData[j];
        j += T - i;
    }
    m_emptyCells.Clear();

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
        g.m_carrier.Clear();
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
        g.m_carrier.Clear();
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
        g.m_carrier.Clear();
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

    for (CellIterator i(Const()); i; ++i) {
	if(IsEmpty(*i))
	    m_state.m_emptyCells.Mark(*i);
	for (CellNbrIterator j(Const(), *i); j; ++j)
	    if(IsEmpty(*j))
		GetCell(*i)->m_NumAdj[SG_EMPTY]++;
    }

    m_state.m_history.Clear();
    m_state.m_winner   = SG_EMPTY;
    m_state.m_vcWinner = SG_EMPTY;
}

void Board::SetPosition(const Board& other) 
{
    SetSize(other.Size());
    for (int i = 0; i < other.m_state.m_history.m_move.Length(); ++i) {
        if (other.m_state.m_history.m_move[i] == Y_SWAP)
            Swap();
        else
            Play(other.m_state.m_history.m_color[i], 
                 other.m_state.m_history.m_move[i]);
    }
    m_state.m_toPlay = other.m_state.m_toPlay;
}

void Board::Undo()
{
    History old(m_state.m_history);
    SetSize(Size());
    for (int i = 0; i < old.m_move.Length() - 1; ++i) {
        if (old.m_move[i] == Y_SWAP)
            Swap();
        else
            Play(old.m_color[i], old.m_move[i]);
    }
    m_state.m_toPlay = old.m_color.Last();
}

void Board::Swap()
{
    for (CellIterator it(Const()); it; ++it) {
	if (GetColor(*it) != SG_EMPTY) {
            m_state.m_color[*it] = SgOppBW(GetColor(*it));
            if (*it == BlockAnchor(*it)) {
                Block* b = GetBlock(*it);
                b->m_color = SgOppBW(b->m_color);
            }
        } else if (GetColor(*it) == SG_EMPTY) {
            Cell* cell = GetCell(*it);
            std::swap(cell->m_NumAdj[SG_BLACK], cell->m_NumAdj[SG_WHITE]);
            std::swap(cell->m_SemiConnects[SG_BLACK], cell->m_SemiConnects[SG_WHITE]);
            std::swap(cell->m_FullConnects[SG_BLACK], cell->m_FullConnects[SG_WHITE]);
        }
    }
    m_state.m_history.PushBack(SG_EMPTY, Y_SWAP);
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
    a.m_history = b.m_history;

    a.m_emptyCells = b.m_emptyCells;

    a.m_winner = b.m_winner;
    a.m_vcWinner = b.m_vcWinner;
    a.m_vcGroupAnchor = b.m_vcGroupAnchor;
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
        MarkCellDirty(p);
    }
    else {        
        cell->RemoveSemiConnection(b, color);
        cell->AddFull(b, color);
        MarkCellDirty(p);
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
        MarkCellDirty(p);        
    }
    else if(size == 1) {
        cell->RemoveFullConnection(b, color);
	cell->AddSemi(b, color);
        MarkCellDirty(p);
    }
}

void Board::AddSharedLibertiesAroundPoint(Block* b1, cell_t p, cell_t skip)
{
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (*it == skip)
            continue;
        if (GetColor(*it) == SgOppBW(b1->m_color))
            continue;
        if (GetColor(*it) == SG_EMPTY) {
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
            MarkCellDirty(*it);

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
    SgArrayList<cell_t, 6> newlib;
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY) {
            if (!IsAdjacent(*it, b)) {
                b->m_liberties.PushBack(*it);
                newlib.PushBack(*it);
            }
        }
    }
    m_state.m_blockIndex[p] = b->m_anchor;
    for (int i = 0; i < newlib.Length(); ++i) {
        cell_t c = newlib[i];

        GetCell(c)->AddFull(b, b->m_color);
        GetCell(c)->RemoveSemiConnection(b, b->m_color);
        GetConnection(c, b->m_anchor).Clear();
        MarkCellDirty(c);

        AddSharedLibertiesAroundPoint(b, c, p);
    }

    // NOTE: Need to do this because block has changed, so weights
    // of any empty cells connecting to b possibly need to change.
    // This is expensive and stupid.
    // TODO: have blocks store list of empty cells they are connected to.
    for(EmptyIterator i(*this); i; ++i) 
    {
	Cell* cell = GetCell(*i);
	if (   cell->IsFullConnected(b, b->m_color) 
	       || cell->IsSemiConnected(b, b->m_color))
	    MarkCellDirty(*i);
    }

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

void Board::UpdateConnectionsToNewAnchor(const Block* from, const Block* to,
                                         const bool* toLiberties)
{
    cell_t p1 = from->m_anchor;
    cell_t p2 = to->m_anchor;
    for(CellAndEdgeIterator i(Const()); i; ++i) 
    {
        if (GetColor(*i) == SG_EMPTY) 
        {
            Cell* cell = GetCell(*i);
            bool removed = false;
            removed |= cell->RemoveSemiConnection(from, from->m_color);
            removed |= cell->RemoveFullConnection(from, from->m_color);
            if (removed)
                MarkCellDirty(*i);

            // Liberties of new captain don't need to be changed
            if (toLiberties[*i]) {
                // Mark anything connected to 'to' as dirty
                MarkCellDirty(*i);
                continue;
            }
                // Mark anything connected to 'to' as dirty
            if (   cell->IsFullConnected(to, to->m_color) 
                || cell->IsSemiConnected(to, to->m_color))
                MarkCellDirty(*i);
        } 
        else  
        {
            // Skips opponents blocks; note that edges are not
            // caught by this check and so will always transfer to 'to'
            if (GetColor(*i) == SgOppBW(to->m_color))
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
                // give new liberty an empty full connection
                GetCell(*lib)->RemoveSemiConnection(largestBlock, 
                                                    largestBlock->m_color);
                GetCell(*lib)->AddFull(largestBlock, largestBlock->m_color);
                GetConnection(*lib, largestBlock->m_anchor).Clear();
                MarkCellDirty(*lib);                
            }
        }
	MergeBlockConnections(adjBlock, largestBlock);
	UpdateConnectionsToNewAnchor(adjBlock, largestBlock, seen);
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
    for (CellNbrIterator j(Const(), p); j; ++j) {
        if (IsEmpty(*j)) {
            for (int i = 0; i < adjBlocks.Length(); ++i) {
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
                        MarkCellDirty(*j);
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
    // std::cerr << "=============================\n"
    //           << ToString() << '\n'
    //           << "p=" << ToString(p) << '\n'
    //           << "pval=" << (int)p << '\n';
    Statistics::Get().m_numMovesPlayed++;
    m_dirtyCells.Clear();
    m_dirtyBlocks.Clear();
    m_state.m_history.PushBack(color, p);
    m_state.m_toPlay = color;
    m_state.m_color[p] = color;
    m_state.m_emptyCells.Unmark(p);
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
       
    // Create/update block containing p (ignores edge blocks).
    // Compute group for this block as well.
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

    // Find all opponent groups whose carrier contains p
    SgArrayList<int, Y_MAX_CELL> blocks, seenGroups;
    for (CellIterator g(*this); g; ++g)
    {
        if (GetColor(*g) != SgOppBW(color))
            continue;
        const cell_t gAnchor = GroupAnchor(*g);
        if (gAnchor != *g)
            continue;
        if (!seenGroups.Contains(gAnchor)) {
            Group* g = GetGroup(gAnchor);
            if (g->m_carrier.Marked(p)) {
                seenGroups.PushBack(gAnchor);
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
    } 

    // Recompute all opponent groups whose carrier contains p 
    int seen[Const().TotalCells+10];
    memset(seen, 0, sizeof(seen));
    for(int i = 0; i < blocks.Length(); ++i) {
        Block* b = GetBlock(blocks[i]);
        if (seen[blocks[i]] != 1)
            ComputeGroupForBlock(b, seen, 100 + i);
    }
    
    // Mark every cell touching a dirty block (a block changed by the group
    // search, for both colors).
    for (EmptyIterator i(*this); i; ++i) {
	if(!m_dirtyCells.Marked(*i)) {
	    Cell* cell = GetCell(*i);
	    for(MarkedCellsWithList::Iterator j(m_dirtyBlocks); j; ++j) {
		Block* b = GetBlock(*j);
		if (   cell->IsFullConnected(b, b->m_color) 
		       || cell->IsSemiConnected(b, b->m_color)) {
		    MarkCellDirty(*i);
		    break;
		}
	    }
	}
    }

    // Group expand opponent groups
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < blocks.Length(); ++i) {
        cell_t gAnchor = GroupAnchor(blocks[i]);
        if (!seen[gAnchor]) {
            seen[gAnchor] = true;
            GroupExpand(gAnchor);
        }
    }
    // Group expand p's group
    // TODO: need to also group expand other newly created groups
    // for this played
    GroupExpand(p);

    
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
    //std::cerr << ToString();


    FlipToPlay();
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
        g2->m_carrier.Clear();
        g2->m_blocks.Clear();
        g2->m_blocks.PushBack(g2->m_anchor);
    }
    g->m_border = b->m_border;
    g->m_anchor = b->m_anchor;
    g->m_carrier.Clear();
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
    // std::cerr << "ComputeGroupForBlock: " 
    //           << ToString(b->m_anchor) << ' ' << "id=" << id << '\n';

    m_groupSearchPotStack.clear();
    GroupSearch(seen, b, id, m_groupSearchPotStack);

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

    seen[Const().WEST] = 0;
    seen[Const().EAST] = 0;
    seen[Const().SOUTH] = 0;
}

int Board::NumUnmarkedSharedLiberties(const Carrier& lib, int* seen, int id,
                                      Carrier& ret)
{
    int count = 0;
    for (int i = 0; i < lib.Size(); ++i) {
        if (seen[lib[i]] != id) {
            ++count;
            ret.PushBack(lib[i]);
        }
    }
    return count;
}

void Board::GroupSearch(int* seen, Block* b, int id, 
                        std::vector<CellPair>& potStack)
{
    seen[b->m_anchor] = 1;
    if(b->m_color != SG_BORDER)
	m_dirtyBlocks.Mark(b->m_anchor);
    //std::cerr << "Entered: " << ToString(b->m_anchor) << '\n';
    for (int i = 0; i < b->m_con.Length(); ++i)
    {
        bool visit = false;
	cell_t other = b->m_con[i];
        Carrier unmarked;
        Carrier& sl = GetConnection(b->m_anchor, other);

	//std::cerr << "Considering: " << ToString(other) << '\n';
        int count = NumUnmarkedSharedLiberties(sl, seen, id, unmarked);
        // Never seen this block before:
        // visit it if we have a vc and mark it as potential if we have a semi
	if (seen[other] != 1 && seen[other] != id) 
        {
            if (count > 1)
                visit = true;
            else if (count == 1) {
                //std::cerr << "Marking as potential!\n";
                seen[other] = id;
                seen[unmarked[0]] = id;
                potStack.push_back(std::make_pair(other, unmarked[0]));
            }
        }
        // block is marked as potential:
        // visit it if we have a vc or a semi (since the other semi
        // we found earlier combined with this semi gives a vc)
        else if (seen[other] == id && count >= 1)
        {
            visit = true;
            for (size_t i = 0; i < potStack.size(); ++i) {
                if (potStack[i].first == other) {
                    unmarked.Include(potStack[i].second);
                    potStack[i] = potStack.back();
                    potStack.pop_back();
                    break;
                }
            }
        }

        if (visit)
	{
	    //std::cerr << "Inside!\n";
	    Group* g = GetGroup(b->m_anchor);
	    g->m_border |= GetBlock(other)->m_border;
            g->m_blocks.PushBack(other);
            for (int i = 0; i < unmarked.Size(); ++i)
                g->m_carrier.Mark(unmarked[i]);

	    if(GetColor(other) != SG_BORDER) {
                // Note that we are not marking liberties with an edge
                // block: we claim these cannot conflict with group
                // formation.
                for (int i = 0; i < unmarked.Size(); ++i)
                    seen[unmarked[i]] = id;
                m_state.m_groupIndex[other] = g->m_anchor;
		GroupSearch(seen, GetBlock(other), id, potStack);
            }
            else {
                // Block other blocks from revisiting this edge.  This
                // will be reset to 0 at the end of
                // ComputeGroupForBlock() so that group searches from
                // other dirty blocks can go to the edge
                seen[other] = 1;
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

// TODO: Improve this search.
void Board::GroupExpand(cell_t move)
{
    SgBlackWhite color = GetColor(move);
    std::vector<PotentialCarrier>& gStack = m_groupExpandPotStack;
    gStack.clear();

    Group* g1 = GetGroup(BlockAnchor(move));
    cell_t g1Anchor = g1->m_anchor;

    for (MarkedCellsWithList::Iterator it(m_dirtyCells); it; ++ it) {
	Cell* cell = GetCell(*it);
        if (cell->m_FullConnects[color].Length() < 2)
            continue;
        // CHECK: cell not in g1->carrier
	if (g1->m_carrier.Marked(*it))
	    continue;

	// cerr << "Considering cell: " << ToString(*it) << '\n';
        // try all possible connections to g1
	for (int i1 = 0; i1 < cell->m_FullConnects[color].Length(); ++i1) {
            if (GetGroup(cell->m_FullConnects[color][i1]) != g1)
                continue;
            // construct cell->g1 carrier
	    cell_t g1Block = cell->m_FullConnects[color][i1];
	    PotentialCarrier g1cellg2(*it, g1Anchor);
	    for(int j = 0; j < GetConnection(*it, g1Block).Length(); ++j) {
		g1cellg2.m_fullCarrier.Mark(GetConnection(*it, g1Block)[j]);
		g1cellg2.m_carrier1.Include(GetConnection(*it, g1Block)[j]);
	    }

            // try all connections to other groups
            for (int i2 = 0; i2 < cell->m_FullConnects[color].Length(); ++i2) {
		// cerr << "Empty connects to: " << ToString(cell->m_FullConnects[color][i2]) << '\n';;
                if (i2 == i1)
                    continue;
                Group* g2 = GetGroup(cell->m_FullConnects[color][i2]);
                if (g2 == g1)
                    continue;
                if (GetColor(g2->m_anchor) == SG_BORDER && g1->m_blocks.Contains(g2->m_anchor))
                    continue;

                // CHECK: cell not in g2->carrier
		if (g2->m_carrier.Marked(*it))
		    continue;

                cell_t g2Anchor = g2->m_anchor;
		g1cellg2.m_endpoint2 = g2Anchor;
		for(int j = 0; j < g1cellg2.m_carrier2.Length(); ++j)
		    g1cellg2.m_fullCarrier.Unmark(g1cellg2.m_carrier2[j]);
		g1cellg2.m_carrier2.Clear();
                // construct cell->g2 carrier
		cell_t g2Block = cell->m_FullConnects[color][i2];

		// cerr << "Cell connects " << ToString(g1Block) 
                //      << " to " << ToString(g2Block) << '\n';
		bool intersect = false;
		// CHECK: cell->g1 does not intersect cell->g2
                // construct g1->cell->g2 carrier
		for(int j = 0; j < GetConnection(*it, g2Block).Length(); ++j) {
		    if(!g1cellg2.m_fullCarrier.Mark(GetConnection(*it, g2Block)[j])) {
			intersect = true;
			break;
		    }
		    else
			g1cellg2.m_carrier2.Include(GetConnection(*it, g2Block)[j]);
		}
		if(intersect)
		    continue;

                // CHECK: cell->g1 does not intersect g2
                // CHECK: cell->g2 does not intersect g1
		if(g1cellg2.m_fullCarrier.Intersects(g1->m_carrier))
		    continue;
		if(g1cellg2.m_fullCarrier.Intersects(g2->m_carrier))
		    continue;

		// cerr << "Passed intersect tests!\n";
                bool merged = false;
                bool addToStack = true;
                for (size_t j = 0; !merged  && j < gStack.size(); ++j) 
                {
		    // cerr << "gStack key: " << ToString(gStack[j].m_key) << '\n';
		    if (gStack[j].m_endpoint1 != g1cellg2.m_endpoint1)
			continue;
                    if (gStack[j].m_endpoint2 != g1cellg2.m_endpoint2)
			continue;
                    if (g1cellg2.m_fullCarrier.Marked(gStack[j].m_key))
                        continue;
		    if (gStack[j].m_fullCarrier.Marked(*it))
			continue;
		    // cerr << "Passed initial tests! \n";
		    
		    int size = g1cellg2.m_fullCarrier.IntersectSize(gStack[j].m_fullCarrier);
		    if (addToStack == true &&
			size == gStack[j].m_fullCarrier.Size() && 
			size == g1cellg2.m_fullCarrier.Size()) {
			// cerr << "Found same carrier: " 
			//      << ToString(gStack[j].m_key) << '\n';
			addToStack = false;
		    }

		    if (g1cellg2.m_carrier1.Length() > 0)
			if ((g1cellg2.m_carrier1.Length() - 
			     gStack[j].m_fullCarrier.IntersectSize(g1cellg2.m_carrier1)) < 2) {
			    continue;
			}
		    if (g1cellg2.m_carrier2.Length() > 0)
			if ((g1cellg2.m_carrier2.Length() - 
			     gStack[j].m_fullCarrier.IntersectSize(g1cellg2.m_carrier2) < 2)) {
			    continue;
			}
		    if (gStack[j].m_carrier1.Length() > 0)
			if ((gStack[j].m_carrier1.Length() - 
			     g1cellg2.m_fullCarrier.IntersectSize(gStack[j].m_carrier1) < 2)) {
			    continue;
			}
		    if (gStack[j].m_carrier2.Length() > 0)
			if ((gStack[j].m_carrier2.Length() - 
			     g1cellg2.m_fullCarrier.IntersectSize(gStack[j].m_carrier2) < 2)) {
			    continue;
			}

		    merged = true;
		    // cerr << "Found a compatible carrier: " 
		    //      << ToString(gStack[j].m_key) << '\n';
		    MergeGroups(g1Anchor, g2Anchor, gStack[j].m_fullCarrier, 
				g1cellg2.m_fullCarrier);
                }
                if (merged) {
                    // REMOVE ALL MENTION OF G2 AND KEYS ON STACK
		    size_t size = gStack.size();
		    for (size_t j = 0; j < size; ++j) {
			 // cerr << "Key: " << ToString(gStack[j].m_key) 
			 //      << " Endpoint2: " << ToString(gStack[j].m_endpoint2) 
			 //      << '\n';
			if(   gStack[j].m_endpoint2 == g2Anchor 
                              || gStack[j].m_fullCarrier.Intersects(g1->m_carrier))
                        {
			    // cerr << "Popping: " << ToString(gStack[j].m_key) << '\n';
			    gStack[j] = gStack.back();
			    gStack.pop_back();
			}
		    }
                }
                else if (addToStack) {
                    // PUSH NEW ENTRY ON STACK
		    // (cell,g2Anchor, g1cellg2)
		    gStack.push_back(g1cellg2);
		    // cerr << "Pushing: " << ToString(*it) << '\n';
                }
            }
	}
    }
}

void Board::MergeGroups(cell_t group1, cell_t group2, MarkedCellsWithList carrier1, MarkedCellsWithList carrier2)
{
    Group* g1 = GetGroup(group1);
    Group* g2 = GetGroup(group2);
    g1->m_border |= g2->m_border;
    for(int i = 0; i < g2->m_blocks.Length(); ++i){
	g1->m_blocks.Include(g2->m_blocks[i]);
	if(GetColor(g2->m_blocks[i]) != SG_BORDER)
	    m_state.m_groupIndex[g2->m_blocks[i]] = group1;
    }
    for(MarkedCells::Iterator i(g2->m_carrier); i; ++i)
	g1->m_carrier.Mark(*i);
    for(MarkedCellsWithList::Iterator i(carrier1); i; ++i)
	g1->m_carrier.Mark(*i);
    for(MarkedCellsWithList::Iterator i(carrier2); i; ++i)
	g1->m_carrier.Mark(*i);
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
    const Carrier& car = GetConnection(b1, b2);
    if (car.Size() != 1)
        return SG_NULLMOVE;
    const Group* g1 = GetGroup(b1);
    const Group* g2 = GetGroup(b2);
    if (GetColor(b1) == SG_BORDER || GetColor(b2) == SG_BORDER) {
        if ((g1->m_border & g2->m_border) == 0)
            return car[0];
    } else if (g1 != g2)
        return car[0];
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
    if (GetCell(p)->IsDead())
        return true;
    if (NumNeighbours(p, SG_EMPTY) > 2)
        return false;
    if (NumNeighbours(p, SG_BLACK) >= 5)
        return true;
    if (NumNeighbours(p, SG_WHITE) >= 5)
        return true;

    SgBlackWhite lastColor = -1;
    int s = 0;
    for (int j = 0; j < 12; ++j)
    {
	const int i = (j > 6) ? j - 6 : j;
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

bool Board::IsCellThreat(cell_t p) const
{
    const Cell* cell = GetCell(p);
    if (cell->IsThreat())
	return true;
    int border = 0;
    for (int i = 0; i < cell->m_FullConnects[SG_BLACK].Length(); ++i) {
	border |= GetGroup(BlockAnchor(
			       cell->m_FullConnects[SG_BLACK][i]))->m_border;
	if ( border == BORDER_ALL)
	    return true;
    }
    border = 0;
    for (int i = 0; i < cell->m_FullConnects[SG_WHITE].Length(); ++i) {
	border |= GetGroup(BlockAnchor(
			       cell->m_FullConnects[SG_WHITE][i]))->m_border;
	if ( border == BORDER_ALL)
	    return true;
    }
    return false;
}

float Board::WeightCell(cell_t p) const
{
    float ret = 1.0;
    const Cell* cell = GetCell(p);
    ret += cell->m_FullConnects[SG_BLACK].Length();
    ret += cell->m_FullConnects[SG_WHITE].Length();
    ret += (0.5) * cell->m_SemiConnects[SG_BLACK].Length();
    ret += (0.5) * cell->m_SemiConnects[SG_WHITE].Length();
    return ret;
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
    for (EmptyIterator i(*this); i; ++i) {
        if (GetCell(*i)->IsFullConnected(b, c))
            ret.push_back(*i);
    }
    return ret;
}

std::vector<cell_t> Board::FullConnectsMultipleBlocks(SgBlackWhite c) const
{
    std::vector<cell_t> ret;
    for (EmptyIterator i(*this); i; ++i) {
        if (GetCell(*i)->m_FullConnects[c].Length() > 1)
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
    for (EmptyIterator i(*this); i; ++i) {
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

std::vector<cell_t> Board::GetBlocksInGroup(cell_t p) const
{
    std::vector<cell_t> ret;
    const Group* g = GetGroup(BlockAnchor(p));
    for (int i = 0; i < g->m_blocks.Length(); ++i) {
        ret.push_back(g->m_blocks[i]);
        if (g->m_blocks[i] == g->m_anchor)
            std::swap(ret.back(), ret[0]);
    }
    return ret;
}

//---------------------------------------------------------------------------

bool MarkedCellsWithList::Intersects(const MarkedCells& other) const
{
    for(Iterator i(*this); i; ++i)
	if(other.Marked(*i)) 
	    return true;
    return false;
}

bool MarkedCellsWithList::Intersects(const SgArrayList<cell_t, 6>& other) const
{
    for(int i = 0; i < other.Length(); ++i)
	if(this->Marked(other[i]))
	    return true;
    return false;
}

int MarkedCellsWithList::IntersectSize(const MarkedCellsWithList& other) const
{
    int count = 0;
    if (this->Size() < other.Size()) {
	for(Iterator i(*this); i; ++i)
	    if (other.Marked(*i)) 
		count++;
    }
    else {
	for(Iterator i(other); i; ++i)
	    if (this->Marked(*i)) 
		count++;
    }
    return count;
}

int MarkedCellsWithList::IntersectSize(const MarkedCells& other) const
{
    int count = 0;
    for(Iterator i(*this); i; ++i)
	if (other.Marked(*i)) 
	    count++;
    return count;
}

int  MarkedCellsWithList::IntersectSize(const SgArrayList<cell_t, 6>& other) const
{
    int count = 0;
    for(int i = 0; i < other.Length(); ++i)
	if(this->Marked(other[i]))
	    count++;
    return count;
}
