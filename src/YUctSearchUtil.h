//----------------------------------------------------------------------------
/** @file YUctSarchUtil.hpp */
//----------------------------------------------------------------------------

#ifndef YUCTSEARCHUTIL_HPP
#define YUCTSEARCHUTIL_HPP

#include "SgBlackWhite.h"
#include "SgPoint.h"
#include "SgUctSearch.h"
#include "board.h"
//----------------------------------------------------------------------------

/** General utility functions used in GoUct.
    These functions are used in GoUct, but should not depend on other classes
    in GoUct to avoid cyclic dependencies. */
namespace YUctSearchUtil
{
    /** Print information about search as Gfx commands for GoGui.
        Can be used for GoGui live graphics during the search or GoGui
        analyze command type "gfx" after the search (see http://gogui.sf.net).
        The following information is output:
        - Move values as influence
        - Move counts as labels
        - Move with best value marked with circle
        - Best response marked with triangle
        - Move with highest count marked with square (if different from move
          with best value)
        - Status line text:
          - N = Number games
          - V = Value of root node
          - Len = Average simulation sequence length
          - Tree = Average/maximum moves of simulation sequence in tree
          - Abrt = Percentage of games aborted (due to maximum game length)
          - Gm/s = Simulations per second
        @param search The search containing the tree and statistics
        @param toPlay The color toPlay at the root node of the tree
        @param cbrd Used to print the moves
        @param out The stream to write the gfx commands to
    */
    void GoGuiGfx(const SgUctSearch& search, SgBlackWhite toPlay,
                  const ConstBoard& cbrd, std::ostream& out);
    
    /** RAVE is more efficient if we know the max number of moves we
	can have. Simply returns Y_MAX_CELL. */
    int ComputeMaxNumMoves();
    
    /** Returns top digits of a value in [0,1]. */
    int FixedValue(SgUctValue value, int precision);

    /** Returns a human readable count. */
    const char* CleanCount(std::size_t count);
    
#if 0
    /** Saves the uct tree to an sgf. */
    void SaveTree(const SgUctTree& tree, const StoneBoard& brd, 
                  HexColor toPlay, std::ostream& out, int maxDepth);
#endif
}

inline int YUctSearchUtil::FixedValue(double value, int precision)
{
    return (int) (value * pow(10.0f, (double)precision) + 0.5f);
}

//----------------------------------------------------------------------------

#endif // HEXUCTUTIL_HPP

