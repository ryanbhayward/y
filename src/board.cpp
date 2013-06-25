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
    char str[16];
    sprintf(str, "%c%1d",board_col(cell)+'a',board_row(cell)+1); 
    return str;
}

int ConstBoard::FromString(const string& name) const
{
    if (name.size() >= 4 && name.substr(0,4) == "swap")
    	return Y_SWAP;
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
    m_color.resize(T);
    std::fill(m_color.begin(), m_color.end(), SG_BORDER);

    m_block.resize(T+3);
    std::fill(m_block.begin(), m_block.end(), (Block*)0);

    m_blockList.resize(T+3);
}

void Board::SetSize(int size)
{ 
    m_constBrd = ConstBoard(size);

    const int N = Size();
    const int T = Const().TotalGBCells;
    m_state.Init(T);
 
    // Create block for left edge
    {
        Block& b = m_state.m_blockList[T+0];
        b.m_anchor = T;  // never used for edge blocks
        b.m_border = BORDER_LEFT;
        b.m_color = SG_BORDER;
        b.m_stones.Clear();
        for (int i = 0; i < N; ++i)
            b.m_liberties.PushBack(Const().fatten(i,0));
    }
    // Create block for right edge
    {
        Block& b = m_state.m_blockList[T+1];
        b.m_anchor = T+1;  // never used for edge blocks
        b.m_border = BORDER_RIGHT;
        b.m_color = SG_BORDER;
        b.m_stones.Clear();
        for (int i = 0; i < N; ++i)
            b.m_liberties.PushBack(Const().fatten(i,i));
    }
    // Create block for bottom edge
    {
        Block& b = m_state.m_blockList[T+2];
        b.m_anchor = T+2;  // never used for edge blocks
        b.m_border = BORDER_BOTTOM;
        b.m_color = SG_BORDER;
        b.m_stones.Clear();
        for (int i = 0; i < N; ++i)
            b.m_liberties.PushBack(Const().fatten(N,i-1));
    }
    // set guards to point to their border block
    for (int i=0; i<N; i++) { 
        m_state.m_block[Const().fatten(i,0)-1] = &m_state.m_blockList[T+0];
        m_state.m_block[Const().fatten(i,i)+1] = &m_state.m_blockList[T+1];
        m_state.m_block[Const().fatten(N,i)] = &m_state.m_blockList[T+2];
    }
    m_state.m_block[Const().fatten(-1,0)-1] = &m_state.m_blockList[T+0];
    m_state.m_block[Const().fatten(-1,-1)+1] = &m_state.m_blockList[T+1];
    m_state.m_block[Const().fatten(N,N)] = &m_state.m_blockList[T+2];
   
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
        if (m_state.m_color[*it] == SG_EMPTY){
            b->m_liberties.PushBack(*it);
	    for (CellNbrIterator it2(Const(), *it); it2; ++it2)
		if (GetColor(*it2) == color && *it2 != p){
		    AddSharedLiberty(b, m_state.m_block[*it2], *it);
		}
	}
    }
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
    //b->UpdateAnchor(p);
    b->m_border |= border;
    b->m_stones.PushBack(p);
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (m_state.m_color[*it] == SG_EMPTY) {
            if (!IsAdjacent(*it, b)) {
                b->m_liberties.PushBack(*it);
                for (CellNbrIterator it2(Const(), *it); it2; ++it2) {
                    if (*it2 != p 
                        && GetColor(*it2) == GetColor(p) 
                        && m_state.m_block[*it2] != b) 
                    {
                        //std::cout << "\n" << Const().ToString(p) << "\n" 
                        //          << Const().ToString(*it2) << "\n";
                        AddSharedLiberty(b, m_state.m_block[*it2], *it);
                    }
                }
            }
        }
    }
    m_state.m_block[p] = b;
}

void Board::MergeBlocks(int p, int border, SgArrayList<Block*, 3>& adjBlocks)
{
    Block* largestBlock = 0;
    int largestBlockStones = 0;
    for (SgArrayList<Block*,3>::Iterator it(adjBlocks); it; ++it)
    {
        Block* adjBlock = *it;
        int numStones = adjBlock->m_stones.Length();
        if (numStones > largestBlockStones)
        {
            largestBlockStones = numStones;
            largestBlock = adjBlock;
        }
    }
    m_state.m_block[p] = largestBlock;
    largestBlock->m_border |= border;
    largestBlock->m_stones.PushBack(p);

    bool seen[Y_MAX_CELL];
    memset(seen, 0, sizeof(seen));
    for (Block::LibertyIterator lib(largestBlock->m_liberties); lib; ++lib)
        seen[*lib] = true;
    for (SgArrayList<Block*,3>::Iterator it(adjBlocks); it; ++it)
    {
        Block* adjBlock = *it;
        if (adjBlock == largestBlock)
            continue;
	MergeSharedLiberty(adjBlock, largestBlock);
        largestBlock->m_border |= adjBlock->m_border;
        for (Block::StoneIterator stn(adjBlock->m_stones); stn; ++stn)
        {
            largestBlock->m_stones.PushBack(*stn);
            m_state.m_block[*stn] = largestBlock;
        }
        for (Block::LibertyIterator lib(adjBlock->m_liberties); lib; ++lib)
            if (!seen[*lib]) {
                seen[*lib] = true;
                largestBlock->m_liberties.PushBack(*lib);
            }
    }
    for (CellNbrIterator it(Const(), p); it; ++it)
        if (m_state.m_color[*it] == SG_EMPTY && !seen[*it]) {
            largestBlock->m_liberties.PushBack(*it);
	    for (CellNbrIterator it2(Const(), *it); it2; ++it2)
		if (*it2 != p 
                    && GetColor(*it2) == GetColor(p) 
                    && m_state.m_block[*it2] != largestBlock) {
		    AddSharedLiberty(largestBlock, m_state.m_block[*it2], *it);
		}
	}
}


void Board::RemoveSharedLiberty(int p, Block* a, Block* b)
{
    int i = a->GetSharedLibertiesIndex(b);
    a->m_shared[i].Exclude(p);
    if (a->m_shared[i].m_liberties.empty()) {
        a->RemoveSharedLiberties(i);
    }

    i = b->GetSharedLibertiesIndex(a);
    b->m_shared[i].Exclude(p);
    if (b->m_shared[i].m_liberties.empty()) {
        b->RemoveSharedLiberties(i);
    }
}

void Board::RemoveSharedLiberty(int p, SgArrayList<Block*, 3>& adjBlocks)
{
    switch(adjBlocks.Length())
    {
    case 0:
    case 1:
        return;
    case 2:
        RemoveSharedLiberty(p, adjBlocks[0], adjBlocks[1]);
        return;
    case 3:
        RemoveSharedLiberty(p, adjBlocks[0], adjBlocks[1]);
        RemoveSharedLiberty(p, adjBlocks[0], adjBlocks[2]);
        RemoveSharedLiberty(p, adjBlocks[1], adjBlocks[2]);
        return;
    default:
        return;
    }
}

void Board::Play(SgBlackWhite color, int p) 
{
    m_state.m_lastMove = p;    
    m_state.m_toPlay = color;
    m_state.m_color[p] = color;
    int border = 0;
    SgArrayList<Block*, 3> adjBlocks;
    SgArrayList<Block*, 3> oppBlocks;
    for (CellNbrIterator it(Const(), p); it; ++it) {
        if (m_state.m_color[*it] == SG_BORDER) {
            border |= m_state.m_block[*it]->m_border;
        }
        else if (m_state.m_color[*it] != SG_EMPTY) {
            Block* b = m_state.m_block[*it];
            b->m_liberties.Exclude(p);
            if (b->m_color == color && !adjBlocks.Contains(b))
                adjBlocks.PushBack(b);
            else if (b->m_color == SgOppBW(color) && !oppBlocks.Contains(b))
                oppBlocks.PushBack(b);
        }
    }
    RemoveSharedLiberty(p, adjBlocks);
    RemoveSharedLiberty(p, oppBlocks);
    if (adjBlocks.Length() == 0)
        CreateSingleStoneBlock(p, color, border);
    else if (adjBlocks.Length() == 1)
        AddStoneToBlock(p, border, adjBlocks[0]);
    else
        MergeBlocks(p, border, adjBlocks);
    if (m_state.m_block[p]->m_border == BORDER_ALL) {
        m_state.m_winner = color;
    }
    FlipToPlay();
}

std::vector<int> Board::GetSharedLiberties(Block* b1, Block* b2) const
{
    std::vector<int> ret;
    if(b1 == b2 || b1 == 0 || b2 == 0)
	return ret;
    for(size_t i = 0; i != b1->m_shared.size(); ++i) {
	if(b1->m_shared[i].m_other == b2->m_anchor)
            return b1->m_shared[i].m_liberties;
    }
    return ret;
}

void Board::AddSharedLiberty(Block* b1, Block* b2, int p)
{
    int i = b1->GetSharedLibertiesIndex(b2);
    if(i != -1) {
        int j = b2->GetSharedLibertiesIndex(b1);
        assert(j != -1);
        b1->m_shared[i].Include(p);
        b2->m_shared[j].Include(p);
    }
    else {
	// b1->m_shared.push_back(SharedLiberties(b2->m_anchor, p));
	// b2->m_shared.push_back(SharedLiberties(b1->m_anchor, p));
        SharedLiberties s1(b2->m_anchor, p);
        b1->m_shared.push_back(s1);

        SharedLiberties s2(b1->m_anchor, p);
        b2->m_shared.push_back(s2);
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
            for (size_t j = 0; j < b1->m_shared[i].m_liberties.size(); ++j)
            {
                const int lib = b1->m_shared[i].m_liberties[j];
                if(!(b2->m_shared[index]).Contains(lib)) {
                    b2->m_shared[index].PushBack(lib);
                    otherBlock->m_shared[index2].PushBack(lib);
                }
            }
            // remove mention of b1 from other's list
            const int j = otherBlock->GetSharedLibertiesIndex(b1);
            otherBlock->RemoveSharedLiberties(j);
        }
        else {
            {
                SharedLiberties s1(b1->m_shared[i].m_other,
                                   b1->m_shared[i].m_liberties);
                b2->m_shared.push_back(s1);
            }

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
    for (BoardIterator it(Const()); it; ++it) {
        if (b.m_color[*it] != SG_EMPTY) {
            a.m_block[*it] = &a.m_blockList[b.m_block[*it]->m_anchor];
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
            std::cerr << Const().ToString(*it) << " color = " 
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
            std::cerr << "id=" << Const().ToString(i)  << " " << b->ToString(Const()) << '\n';
        }
    }
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
		os << Const().ToString(Anchor(psn));
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
