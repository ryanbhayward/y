//----------------------------------------------------------------------------
/** @file YUctSearchUtil.cpp */
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "SgBWSet.h"
#include "SgPointSet.h"
#include "SgProp.h"
#include "SgUctSearch.h"

#include "YUctSearchUtil.h"

#include <iomanip>
#include <iostream>

//----------------------------------------------------------------------------

namespace {

void GoGuiGfxStatus(const SgUctSearch& search, std::ostream& out)
{
    const SgUctTree& tree = search.Tree();
    const SgUctNode& root = tree.Root();
    const SgUctSearchStat& stat = search.Statistics();
    out << std::fixed
        << "TEXT N=" << static_cast<size_t>(root.MoveCount())
        << " V=" << std::setprecision(2) << root.Mean()
        << " Len=" << static_cast<int>(stat.m_gameLength.Mean())
        << " Tree=" << std::setprecision(1) << stat.m_movesInTree.Mean()
        << "/" << static_cast<int>(stat.m_movesInTree.Max())
#if 0
        // one of our local fuego extensions
        << " Know=" << std::setprecision(1) << stat.m_knowledgeDepth.Mean()
        << "/" << static_cast<int>(stat.m_knowledgeDepth.Max())
#endif
        << " Gm/s=" << static_cast<int>(stat.m_gamesPerSecond) << '\n';
}

}

const char* YUctSearchUtil::CleanCount(std::size_t count)
{
    static char str[16];
    if (count < 1000)
        sprintf(str, "%lu", count);
    else if (count < 1000*1000)
        sprintf(str, "%luk", count / 1000);
    else 
        sprintf(str, "%.2fm", ((float)count / (1000*1000)));
    return str;
}

void YUctSearchUtil::GoGuiGfx(const SgUctSearch& search, SgBlackWhite toPlay,
                              const ConstBoard& cbrd, std::ostream& out)
{
    const SgUctTree& tree = search.Tree();
    const SgUctNode& root = tree.Root();
    out << "VAR";
    std::vector<const SgUctNode*> bestValueChild;
    bestValueChild.push_back(search.FindBestChild(root));
    for (int i = 0; i < 4; i++) 
    {
	if (bestValueChild[i] == 0) 
            break;
	SgPoint move = bestValueChild[i]->Move();
	if (0 == (i % 2))
	    out << ' ' << (toPlay == SG_BLACK ? 'B' : 'W') << ' '
		<< cbrd.ToString(move);
	else
	    out << ' ' << (toPlay == SG_WHITE ? 'B' : 'W') << ' '
		<< cbrd.ToString(move);
	bestValueChild.push_back(search.FindBestChild(*(bestValueChild[i])));
    }
    out << "\n";
    out << "INFLUENCE";
    for (SgUctChildIterator it(tree, root); it; ++it)
    {
        const SgUctNode& child = *it;
        if (child.MoveCount() == 0)
            continue;
        SgUctValue influence = search.InverseEval(child.Mean());
        SgPoint move = child.Move();
        out << ' ' << cbrd.ToString(move) 
            << " ." << FixedValue(influence, 3);
    }
    out << '\n'
        << "LABEL";
    int numChildren = 0;
    for (SgUctChildIterator it(tree, root); it; ++it)
    {
        const SgUctNode& child = *it;
        size_t count = static_cast<size_t>(child.MoveCount());
	numChildren++;
	out << ' ' << cbrd.ToString(child.Move())
	    << ' ' << CleanCount(count);
    }
    out << '\n';
    GoGuiGfxStatus(search, out);
}

int YUctSearchUtil::ComputeMaxNumMoves()
{
    return Y_MAX_CELL;
}

//----------------------------------------------------------------------------

namespace 
{

void SaveNode(std::ostream& out, const SgUctTree& tree, const SgUctNode& node, 
              SgBlackWhite toPlay, const ConstBoard& cbrd,
              int maxDepth, int depth)
{
    out << "C[MoveCount " << node.MoveCount()
        << "\nPosCount " << node.PosCount()
        << "\nMean " << std::fixed << std::setprecision(2) << node.Mean();
    if (!node.HasChildren())
    {
        out << "]\n";
        return;
    }
    out << "\n\nRave:";
    for (SgUctChildIterator it(tree, node); it; ++it)
    {
        const SgUctNode& child = *it;
        int move = static_cast<int>(child.Move());
        if (child.HasRaveValue())
        {
            out << '\n' << cbrd.ToString(move) << ' '
                << std::fixed << std::setprecision(2) << child.RaveValue()
                << " (" << child.RaveCount() << ')';
        }
    }
    out << "]\nLB";
    for (SgUctChildIterator it(tree, node); it; ++it)
    {
        const SgUctNode& child = *it;
        if (! child.HasMean())
            continue;
        int move = static_cast<int>(child.Move());
        out << "[" << cbrd.ToString(move) << ':' 
            << child.MoveCount() << '@' << child.Mean() << ']';
    }
    out << '\n';
    if (maxDepth >= 0 && depth >= maxDepth)
        return;
    for (SgUctChildIterator it(tree, node); it; ++it)
    {
        const SgUctNode& child = *it;
        if (! child.HasMean())
            continue;
        int move = static_cast<int>(child.Move());
        out << "(;" << (toPlay == SG_BLACK ? 'B' : 'W') 
            << '[' << cbrd.ToString(move) << ']';
        SaveNode(out, tree, child, SgOppBW(toPlay), cbrd, maxDepth, depth + 1);
        out << ")\n";
    }
}

}

#if 0
void YUctSearchUtil::SaveTree(const SgUctTree& tree, const StoneBoard& brd, 
                              SgBlackWhite toPlay, std::ostream& out, int maxDepth)
{
    out << "(;FF[4]GM[11]SZ[" << brd.Width() << "]\n";
    out << ";AB";
    for (BitsetIterator it(brd.GetBlack()); it; ++it)
        out << '[' << *it << ']';
    out << '\n';
    out << "AW";
    for (BitsetIterator it(brd.GetWhite()); it; ++it)
        out << '[' << *it << ']';
    out << '\n';
    out << "AE";
    for (BitsetIterator it(brd.GetEmpty()); it; ++it)
        out << '[' << *it << ']';
    out << '\n';
    out << "PL[" << (toPlay == SG_BLACK ? "B" : "W") << "]\n";
    SaveNode(out, tree, tree.Root(), toPlay, maxDepth, 0);
    out << ")\n";
}
#endif

//----------------------------------------------------------------------------


