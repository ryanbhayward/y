//----------------------------------------------------------------------------
/** @file YSgUtil.hpp */
//----------------------------------------------------------------------------

#ifndef YSGUTIL_HPP
#define YSGUTIL_HPP

#include "SgBlackWhite.h"
#include "SgPoint.h"
#include "SgProp.h"
#include "SgNode.h"

#include "Board.h"

//----------------------------------------------------------------------------

/** Utilities to convert from/to Hex and Sg. */
namespace YSgUtil
{
#if 0
    /** Converts a Y point to a SgPoint. */
    SgPoint HexPointToSgPoint(int p, int height);
#endif

    /** Initialize SG_PROP for reading/writing sgfs. */
    void Init();

    /** Converts from from a SgPoint to a Y point. */
    int SgPointToYPoint(SgPoint p, const ConstBoard& cbrd);

    //------------------------------------------------------------------------

    /** Returns true if node constains any of the following
        properties: propAddBlack, propAddWhite, propAddEmpty,
        propPlayer. */
    bool NodeHasSetupInfo(SgNode* node);

    /** Puts the position in the given vectors. */
    void GetSetupPosition(const SgNode* node, 
                          const ConstBoard& cbrd,
                          std::vector<cell_t>& black,
                          std::vector<cell_t>& white,
                          std::vector<cell_t>& empty);

#if 0
    /** Adds the move to the sg node; does proper conversions. */
    void AddMoveToNode(SgNode* node, HexColor color, 
                       HexPoint cell, int height);


    /** Set the position setup properties of this node to encode the 
        given board.*/
    void SetPositionInNode(SgNode* root, 
                           const StoneBoard& brd, 
                           HexColor color);

    //------------------------------------------------------------------------

    /** Write the given tree to a sgf file. Returns true on success,
        false otherwise. */
    bool WriteSgf(SgNode* tree, const char* filename, int boardsize);
#endif
}

//----------------------------------------------------------------------------

#endif // YSGUTIL_HPP
