#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "board.h"

using namespace std;

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
    m_conData.reset(new Carrier[T*(T+1)/2]);
    m_semis.reset(new SemiTable());
    m_groups.reset(new Groups(*m_semis));
    m_semis->SetGroups(m_groups.get());

    for (int i = 0; i < T; ++i)
        m_color[i] = SG_EMPTY;
    memset(m_blockIndex.get(), -1, sizeof(cell_t)*T);

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
        b.m_border = ConstBoard::BORDER_WEST;
        b.m_stones.Clear();
        m_state.m_groups->SetGroupDataFromBlock(&b, Const().WEST);
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
        b.m_border = Const().BORDER_EAST;
        b.m_stones.Clear();
        m_state.m_groups->SetGroupDataFromBlock(&b, Const().EAST);
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
        b.m_border = Const().BORDER_SOUTH;
        b.m_stones.Clear();
        m_state.m_groups->SetGroupDataFromBlock(&b, Const().SOUTH);
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
    memcpy(a.m_conData.get(), b.m_conData.get(),
           sizeof(Carrier)*T*(T+1)/2);
    memcpy(a.m_semis.get(), b.m_semis.get(), sizeof(SemiTable));
    memcpy(a.m_groups.get(), b.m_groups.get(), sizeof(Groups));

    a.m_oppBlocks = b.m_oppBlocks;

    a.m_toPlay = b.m_toPlay;
    a.m_history = b.m_history;

    a.m_emptyCells = b.m_emptyCells;

    a.m_winner = b.m_winner;
    a.m_vcWinner = b.m_vcWinner;
    a.m_vcStonePlayed = b.m_vcStonePlayed;
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
        MarkCellDirtyCon(p);
    }
    else {        
        cell->RemoveSemiConnection(b, color);
        cell->AddFull(b, color);
        MarkCellDirtyCon(p);
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
            YTrace() << "MORE THAN 2 LIBERTIES BUT NOT FULL CONNECTED!!\n";
            YTrace() << ToString() << '\n'
                     << " p = " << ToString(p)
                     << " b = " << ToString(b->m_anchor) 
                     << '\n';
            for (int i =0; i < GetConnection(p, b->m_anchor).Size(); ++i) {
                YTrace() << ' ' << ToString(GetConnection(p, b->m_anchor)[i]);
                YTrace() << '\n';
            }
            abort();
        }
        return;
    }
    if (size == 0) {
        cell->RemoveSemiConnection(b, color);
        MarkCellDirtyCon(p);        
    }
    else if(size == 1) {
        cell->RemoveFullConnection(b, color);
	cell->AddSemi(b, color);
        MarkCellDirtyCon(p);
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
        // YTrace() << "p=" << ToString(p) 
        //           << " b1->anchor=" << ToString(b1->m_anchor) 
        //           << " b2->anchor=" << ToString(b2->m_anchor)
        //           << " it=" << ToString(*it) << '\n';
        if (b1 == b2)
            continue;
        else if (b2->m_color == b1->m_color) {
	    AddCellToConnection(b1->m_anchor, b2->m_anchor, p);
	}
        else if (b2->m_color == SG_BORDER && ((b2->m_border&b1->m_border)==0)){
	    AddCellToConnection(b1->m_anchor, b2->m_anchor, p);
	}
    }
}

void Board::AddLibertyToBlock(Block* block, cell_t c)
{
    block->m_liberties.PushBack(c);
    GetCell(c)->AddFull(block, block->m_color);
    GetCell(c)->RemoveSemiConnection(block, block->m_color);
    GetConnection(c, block->m_anchor).Clear();
    MarkCellDirtyCon(c);
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
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY) {
            AddLibertyToBlock(b, *it);
            AddSharedLibertiesAroundPoint(b, *it, p);
	}
    }
    GetGroups().CreateSingleBlockGroup(b);
    GetSemis().ClearNewSemis();
    for (MarkedCellsWithList::Iterator it(m_dirtyConCells); it; ++ it)
        ConstructSemisWithKey(*it, color);
    GetGroups().ProcessNewSemis(GetSemis().GetNewSemis());
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
    b->m_stones.PushBack(p);
    int new_borders = ~b->m_border & border;
    if (new_borders) {
        b->m_border |= border;
        RemoveEdgeConnections(b, new_borders); 
        GetGroups().UpdateBorderFromBlock(b);
    }
    SgArrayList<cell_t, 6> newlib;
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY) {
            if (!IsAdjacent(*it, b))
                newlib.PushBack(*it);
        }
    }
    m_state.m_blockIndex[p] = b->m_anchor;
    for (int i = 0; i < newlib.Length(); ++i) {
        AddLibertyToBlock(b, newlib[i]);
        AddSharedLibertiesAroundPoint(b, newlib[i], p);
    }
   
    GetSemis().ClearNewSemis();
    for (MarkedCellsWithList::Iterator it(m_dirtyConCells); it; ++ it)
        ConstructSemisWithKey(*it, b->m_color);
    GetGroups().ProcessNewSemis(GetSemis().GetNewSemis());
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
                MarkCellDirtyCon(*i);

            // Liberties of new captain don't need to be changed
            if (toLiberties[*i]) {
                // Mark anything connected to 'to' as dirty
                MarkCellDirtyCon(*i);
                continue;
            }
                // Mark anything connected to 'to' as dirty
            if (   cell->IsFullConnected(to, to->m_color) 
                || cell->IsSemiConnected(to, to->m_color))
                MarkCellDirtyCon(*i);
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
    int old_border = largestBlock->m_border;
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
        // YTrace() << "*** MergingBlocks: " 
        //           << ToString(adjBlock->m_anchor) << " into " 
        //           << ToString(largestBlock->m_anchor) << '\n';
        largestBlock->m_border |= adjBlock->m_border;
        for (Block::StoneIterator stn(adjBlock->m_stones); stn; ++stn)
        {
            largestBlock->m_stones.PushBack(*stn);
            m_state.m_blockIndex[*stn] = largestBlock->m_anchor;
        }
        for (Block::LibertyIterator lib(adjBlock->m_liberties); lib; ++lib) {
            if (!seen[*lib]) {
                seen[*lib] = true;
                AddLibertyToBlock(largestBlock, *lib);
            }
        }
	UpdateConnectionsToNewAnchor(adjBlock, largestBlock, seen);
        GetSemis().TransferEndpoints(adjBlock->m_anchor, 
                                     largestBlock->m_anchor);
        GetGroups().HandleBlockMerge(adjBlock->m_anchor, 
                                     largestBlock->m_anchor);
    }
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (GetColor(*it) == SG_EMPTY && !seen[*it]) {
            AddLibertyToBlock(largestBlock, *it);
            AddSharedLibertiesAroundPoint(largestBlock, *it, p);
	}
    }
    int new_borders = ~old_border & largestBlock->m_border;
    RemoveEdgeConnections(largestBlock, new_borders);
    GetGroups().UpdateBorderFromBlock(largestBlock);
    
    GetSemis().ClearNewSemis();
    for (MarkedCellsWithList::Iterator it(m_dirtyConCells); it; ++ it)
        ConstructSemisWithKey(*it, largestBlock->m_color);
    GetGroups().ProcessNewSemis(GetSemis().GetNewSemis());
}

void Board::RemoveEdgeConnections(Block* b, int new_borders)
{
    Group* g = GetGroups().GetRootGroup(b->m_anchor);
    if (new_borders & ConstBoard::BORDER_WEST) {
        GetConnection(Const().WEST, b->m_anchor).Clear();
        GetGroups().RemoveEdgeConnection(g, ConstBoard::WEST);
    }
    if (new_borders & ConstBoard::BORDER_EAST) {
        GetConnection(Const().EAST, b->m_anchor).Clear();
        GetGroups().RemoveEdgeConnection(g, ConstBoard::EAST);
    }
    if (new_borders & ConstBoard::BORDER_SOUTH) {
        GetConnection(Const().SOUTH, b->m_anchor).Clear();
        GetGroups().RemoveEdgeConnection(g, ConstBoard::SOUTH);
    }
}

void Board::RemoveSharedLiberty(cell_t p, SgArrayList<cell_t, 3>& adjBlocks)
{
    if (adjBlocks.Length() == 0)
        return;

    switch(adjBlocks.Length())
    {
    case 2:
        RemoveCellFromConnection(adjBlocks[0], adjBlocks[1], p);
        break;
    case 3:
        RemoveCellFromConnection(adjBlocks[0], adjBlocks[1], p);
        RemoveCellFromConnection(adjBlocks[0], adjBlocks[2], p);
        RemoveCellFromConnection(adjBlocks[1], adjBlocks[2], p);
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
                        MarkCellDirtyCon(*j);
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
    // YTrace() << "=============================\n"
    //          << ToString() << '\n'
    //          << ConstBoard::ColorToString(color) 
    //          << " p=" << ToString(p) << '(' << (int)p << ")\n"
    //          << "vcwin=" << HasWinningVC() << ' '
    //          << (HasWinningVC() ? ToString(m_state.m_vcStonePlayed) : "") 
    //          << '\n';
    Statistics::Get().m_numMovesPlayed++;
    m_dirtyConCells.Clear();
    m_dirtyWeightCells.Clear();
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
    GetSemis().RemoveContaining(p);
    GetGroups().RestructureAfterMove(p, color, *this);
       
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

    // Mark every cell requiring a weight update as dirty.
    m_dirtyWeightCells = m_dirtyConCells;
    for (EmptyIterator i(*this); i; ++i) {
        if (m_dirtyWeightCells.Marked(*i))
            continue;
        Cell* cell = GetCell(*i);
        for(MarkedCellsWithList::Iterator j(m_dirtyBlocks); j; ++j) {
            Block* b = GetBlock(*j);
            if (   cell->IsFullConnected(b, b->m_color) 
                   || cell->IsSemiConnected(b, b->m_color)) {
                MarkCellDirtyWeight(*i);
                break;
            }
        }
    }

    // Break old win if necessary
    if (HasWinningVC() 
        && GroupBorder(m_state.m_vcStonePlayed) != ConstBoard::BORDER_ALL)
    {
        m_state.m_vcWinner = SG_EMPTY;
    }

    // Check for a new vc win
    if (!HasWinningVC() 
        && GroupBorder(p) == ConstBoard::BORDER_ALL)
    {
        m_state.m_vcWinner = color;
        m_state.m_vcStonePlayed = p;
    }

    // Check for a solid win
    if (GetBlock(p)->m_border == ConstBoard::BORDER_ALL) 
    {
        m_state.m_winner = color;
    }

    FlipToPlay();
}

//---------------------------------------------------------------------------

void Board::ConstructSemisWithKey(cell_t key, SgBlackWhite color)
{
    const Cell* cell = GetCell(key);
    if (cell->m_FullConnects[color].Length() < 2)
        return;
    for (int i = 0; i < cell->m_FullConnects[color].Length(); ++i) {
        cell_t b1 = cell->m_FullConnects[color][i];
        SemiConnection::Carrier carrier;

        // TODO: handle more than 2 cells in carrier
        for(int j = 0; j < 2 && j < GetConnection(key, b1).Length() ; ++j)
            carrier.PushBack(GetConnection(key, b1)[j]);
        carrier.PushBack(key);

        for (int j=i+1; j < cell->m_FullConnects[color].Length(); ++j) {
            cell_t b2 = cell->m_FullConnects[color][j];

            // Ensure at least one endpoint is not an edge
            if (ConstBoard::IsEdge(b2) && ConstBoard::IsEdge(b1))
                continue;
            // FIXME: instead of checking for two edges, shouldn't we
            // just ensure one endpoint is the block we just
            // created/modified?

            // Don't build semis between blocks that touch an edge and
            // that edge
            if (ConstBoard::IsEdge(b1) || ConstBoard::IsEdge(b2)) {
                if (GetBlock(b1)->m_border & GetBlock(b2)->m_border)
                    continue;
            }

            SgArrayList<cell_t, 10> potential;
            for (int k = 0; k < GetConnection(key, b2).Length(); ++k) {
                cell_t p = GetConnection(key, b2)[k];
                if (!carrier.Contains(p))
                    potential.PushBack(p);
            }
            if ((potential.Length() < 2) && !GetConnection(key, b2).IsEmpty()) {
                continue;
            }

            // TODO: use the same function as above to select two 
            // cells from 'potential'.
            bool needToPop = false;
            if (!potential.IsEmpty()) {
                needToPop = true;
                carrier.PushBack(potential[0]);
                carrier.PushBack(potential[1]);
            }
            SemiConnection semi(b1, b2, key, carrier);
            GetSemis().Include(semi);

            if (needToPop) {
                carrier.PopBack();
                carrier.PopBack();                
            }                
        }
    }
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
            YTrace() << ToString();
            YTrace() << ToString(*it) << " color = " 
                     << GetColor(*it) << '\n';
            abort();
        }
    }
}

void Board::DumpBlocks() 
{
    for(CellAndEdgeIterator i(Const()); i; ++i) {
        if (GetColor(*i) != SG_EMPTY) {
            const Block * b = GetBlock(*i);
            YTrace() << "id=" << ToString(*i)  << ' ' 
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

// TODO: Mark threats taking into account color.
// eg: Black to play, both black and white have a single win threat.
// Black should play his over blocking white's, currently they are
// treated as the same.
void Board::MarkAllThreats(const MarkedCellsWithList& cells,
                           MarkedCellsWithList& threatInter,
                           MarkedCellsWithList& threatUnion)
{
    threatUnion.Clear();
    threatInter = m_state.m_emptyCells;

    for (MarkedCellsWithList::Iterator it(cells); it; ++it)
        GetCell(*it)->ClearFlags(Cell::FLAG_THREAT);
    for (MarkedCellsWithList::Iterator it(cells); it; ++it) {
        const Cell* cell = GetCell(*it);
        if (cell->IsThreat())
            continue;
        for (SgBWIterator c; c; ++c) {
            int border = 0;
            for (int i = 0; i < cell->m_FullConnects[*c].Length(); ++i) {
                const Group* g=GetGroup(BlockAnchor(cell->m_FullConnects[*c][i]));
                // Cell is connected to a winning group; our work here
                // is done.
                if (g->m_border == ConstBoard::BORDER_ALL) {
                    for (MarkedCells::Iterator j(g->m_carrier); j; ++j) {
                        threatUnion.Mark(*j);
                    }
                    threatInter.Clear();
                    return;
                }
                
                border |= g->m_border;
                if (border == ConstBoard::BORDER_ALL) {
                    MarkCellAsThreat(*it);
                    
                    break;
                }
            }
        }
    }
}

float Board::WeightCell(cell_t p) const
{
    float ret = 1.0;
    const Cell* cell = GetCell(p);
    SgArrayList<cell_t, 12> seenGroups;
    int border = 0;
    for (SgBWIterator it; it; ++it) {
	for (int i = 0; i < cell->m_FullConnects[*it].Length(); ++i) {
	    ret += GetBlock(cell->m_FullConnects[*it][i])->m_stones.Length();
            cell_t block = BlockAnchor(cell->m_FullConnects[*it][i]);
	    const Group* g = GetGroup(block);
	    if (GetColor(block) == SG_BORDER)
		continue;
	    if (seenGroups.Contains(g->m_id))
		continue;
	    if (g->m_carrier.Marked(p))
		continue;
	    ret += 2 * g->m_blocks.Length();
	    border |= g->m_border;
	    seenGroups.PushBack(g->m_id);
	}
    }
    ret += border;
    return ret;
}

//---------------------------------------------------------------------------

std::vector<cell_t> Board::FullConnectedTo(cell_t p, SgBlackWhite c) const
{
    std::vector<cell_t> ret;
    if (IsEmpty(p)) 
        Append(ret, GetCell(p)->m_FullConnects[c]);
    else {
        const Block* b = GetBlock(p);
        for (EmptyIterator i(*this); i; ++i) {
            if (GetCell(*i)->IsFullConnected(b, c))
                ret.push_back(*i);
        }
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
    if (IsEmpty(p))
        Append(ret, GetCell(p)->m_SemiConnects[c]);
    else {
        const Block* b = GetBlock(p);
        for (EmptyIterator i(*this); i; ++i) {
            if (GetCell(*i)->IsSemiConnected(b, c))
                ret.push_back(*i);
        }
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

std::vector<SemiConnection> Board::GetSemisBetween(cell_t p1, cell_t p2) const 
{
    std::vector<SemiConnection> ret;
    if (!IsOccupied(p1) || !IsOccupied(p2))
        return ret;
    p1 = GetBlock(p1)->m_anchor;
    p2 = GetBlock(p2)->m_anchor;
    for (SemiTable::IteratorPair it(p1, p2, &GetSemis()); it; ++it)
        ret.push_back(*it);
    return ret;
}

std::string Board::GroupStructure(cell_t p) const
{
    const Group* g = GetGroup(BlockAnchor(p));
    return GetGroups().Encode(g);
}

//---------------------------------------------------------------------------
