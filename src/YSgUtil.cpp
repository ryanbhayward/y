//----------------------------------------------------------------------------
/** @file YSgUtil.cpp */
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "SgNode.h"
#include "SgProp.h"
#include "SgGameWriter.h"

#include "YSgUtil.h"

//----------------------------------------------------------------------------

void YSgUtil::Init()
{
    SgProp* moveProp = new SgPropMove(0);
    SG_PROP_MOVE_BLACK
        = SgProp::Register(moveProp, "B",
                           SG_PROPCLASS_MOVE + SG_PROPCLASS_BLACK);
    SG_PROP_MOVE_WHITE
        = SgProp::Register(moveProp, "W",
                           SG_PROPCLASS_MOVE + SG_PROPCLASS_WHITE);
}

int YSgUtil::SgPointToYPoint(SgPoint p, const ConstBoard& cbrd)
{
    int c = SgPointUtil::Col(p);
    int r = SgPointUtil::Row(p);
    return cbrd.fatten(cbrd.Size() - r, c - 1);
    //HexPointUtil::coordsToPoint(c - 1, height - r);
}

bool YSgUtil::NodeHasSetupInfo(SgNode* node)
{
    if (   node->HasProp(SG_PROP_ADD_BLACK)
        || node->HasProp(SG_PROP_ADD_WHITE)
        || node->HasProp(SG_PROP_ADD_EMPTY)
        || node->HasProp(SG_PROP_PLAYER))
        return true;
    return false;
}

void YSgUtil::GetSetupPosition(const SgNode* node, 
                               const ConstBoard& cbrd,
                               std::vector<cell_t>& black,
                               std::vector<cell_t>& white,
                               std::vector<cell_t>& empty)
{
    black.clear();
    white.clear();
    empty.clear();
    if (node->HasProp(SG_PROP_ADD_BLACK)) 
    {
        SgPropPointList* prop = (SgPropPointList*)node->Get(SG_PROP_ADD_BLACK);
        const SgVector<SgPoint>& vec = prop->Value();
        for (int i = 0; i < vec.Length(); ++i)
            black.push_back(YSgUtil::SgPointToYPoint(vec[i], cbrd));
    }
    if (node->HasProp(SG_PROP_ADD_WHITE)) 
    {
        SgPropPointList* prop = (SgPropPointList*)node->Get(SG_PROP_ADD_WHITE);
        const SgVector<SgPoint>& vec = prop->Value();
        for (int i = 0; i < vec.Length(); ++i)
            white.push_back(YSgUtil::SgPointToYPoint(vec[i], cbrd));
    }
    if (node->HasProp(SG_PROP_ADD_EMPTY)) 
    {
        SgPropPointList* prop = (SgPropPointList*)node->Get(SG_PROP_ADD_EMPTY);
        const SgVector<SgPoint>& vec = prop->Value();
        for (int i = 0; i < vec.Length(); ++i)
            empty.push_back(YSgUtil::SgPointToYPoint(vec[i], cbrd));
    }
}

#if 0
SgPoint YSgUtil::YPointToSgPoint(HexPoint p, int height)
{
    int c, r;
    HexPointUtil::pointToCoords(p, c, r);
    return SgPointUtil::Pt(1 + c, height - r);
}

void YSgUtil::AddMoveToNode(SgNode* node, HexColor color, 
                              HexPoint cell, int height)
{
    SgPoint sgcell = YSgUtil::HexPointToSgPoint(cell, height); 
    SgBlackWhite sgcolor = YSgUtil::HexColorToSgColor(color);
    HexProp::AddMoveProp(node, sgcell, sgcolor);
}


void YSgUtil::SetPositionInNode(SgNode* node, const StoneBoard& brd, 
                                  HexColor color)
{
    int height = brd.Height();
    SgVector<SgPoint> blist = YSgUtil::BitsetToSgVector(brd.GetBlack() 
                                       & brd.Const().GetCells(), height);
    SgVector<SgPoint> wlist = YSgUtil::BitsetToSgVector(brd.GetWhite()
                                       & brd.Const().GetCells(), height);
    SgVector<SgPoint> elist = YSgUtil::BitsetToSgVector(brd.GetEmpty()
                                       & brd.Const().GetCells(), height);
    SgPropPlayer* pprop = new SgPropPlayer(SG_PROP_PLAYER);
    SgPropAddStone* bprop = new SgPropAddStone(SG_PROP_ADD_BLACK);
    SgPropAddStone* wprop = new SgPropAddStone(SG_PROP_ADD_WHITE);
    SgPropAddStone* eprop = new SgPropAddStone(SG_PROP_ADD_EMPTY);
    pprop->SetValue(YSgUtil::HexColorToSgColor(color));    
    bprop->SetValue(blist);
    wprop->SetValue(wlist);
    eprop->SetValue(elist);
    node->Add(pprop);
    node->Add(bprop);
    node->Add(wprop);
    node->Add(eprop);
}

bool YSgUtil::WriteSgf(SgNode* tree, const char* filename, int boardsize)
{
    // Set the boardsize property
    tree->Add(new SgPropInt(SG_PROP_SIZE, boardsize));
    std::ofstream f(filename);
    if (f) 
    {
        SgGameWriter sgwriter(f);
        // NOTE: 11 is the sgf gamenumber for Hex
        sgwriter.WriteGame(*tree, true, 0, 11, boardsize);
        f.close();
    } 
    else 
    {
        LogWarning() << "Could not open '" << filename << "' "
                     << "for writing!\n";
        return false;
    }
    return true;
}
#endif

//----------------------------------------------------------------------------
