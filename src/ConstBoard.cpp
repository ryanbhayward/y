#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

#include "YUtil.h"
#include "ConstBoard.h"

using namespace std;

//---------------------------------------------------------------------------

std::string ConstBoard::BorderToString(int border)
{
    std::ostringstream os;
    os << "(";
    if (border & BORDER_WEST)
        os << "(west)";
    if (border & BORDER_EAST)
        os << "(east)";
    if (border & BORDER_SOUTH)
        os << "(south)";
    os << ")";            
    return os.str();
}

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
    else if (move == SG_NULLMOVE)
        return "null";
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

//---------------------------------------------------------------------------
