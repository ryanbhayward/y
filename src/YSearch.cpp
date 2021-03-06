//----------------------------------------------------------------------------
/** @file YSearch.cpp
*/
//----------------------------------------------------------------------------

#include "YSearch.h"

//----------------------------------------------------------------------------

YSearchTracer::YSearchTracer(SgNode* root, int boardHeight)
    : SgSearchTracer(root),
      m_height(boardHeight)
{
}

YSearchTracer::~YSearchTracer()
{
}

void YSearchTracer::InitTracing(const std::string& type)
{
    SG_ASSERT(! m_traceNode);
    m_traceNode = new SgNode();
    std::ostringstream os;
    os << "Y " << type;
    std::string comment = os.str();
    TraceComment(comment.c_str());
}

void YSearchTracer::AddMoveProp(SgNode* node, SgMove move,
                                 SgBlackWhite player)
{
    // FIXME: do conversion properly!!
    // SgMove converted = (move <= 0) ? move 
    //     : HvCellUtil::HvToSgPoint(move, m_height);
    SgMove converted = move;
    node->AddMoveProp(converted, player);
}

//----------------------------------------------------------------------------

YSearch::YSearch()
    : SgSearch(0),
      m_brd(8)
{
}

YSearch::~YSearch()
{
}

bool YSearch::CheckDepthLimitReached() const
{
    return true;
}

std::string YSearch::MoveString(SgMove move) const
{
    return m_brd.Const().ToString(static_cast<int>(move));
}

void YSearch::CreateTracer()
{
    YSearchTracer* tracer = new YSearchTracer(0, m_brd.Size());
    SetTracer(tracer);
}

void YSearch::OnStartSearch()
{
}

void YSearch::StartOfDepth(int depthLimit)
{
    SgSearch::StartOfDepth(depthLimit);
    std::cerr << "Depth=" << depthLimit << std::endl;
}

void YSearch::Generate(SgVector<SgMove>* moves, int depth)
{
    SG_UNUSED(depth);
    moves->Clear();
    for (CellIterator it(m_brd); it; ++it)
        if (m_brd.IsEmpty(*it))
            moves->PushBack(*it);
}

int YSearch::Evaluate(bool* isExact, int depth)
{
    SG_UNUSED(depth);
    SgBoardColor type = SG_EMPTY;
    if (m_brd.HasWinningVC())
        type = m_brd.GetVCWinner();
    else if (m_brd.IsGameOver())
        type = m_brd.GetWinner();
    switch(type)
    {
    case SG_EMPTY:
        *isExact = false;
        return 0;
    case SG_BLACK:
        *isExact = true;
        //std::cerr << "BLACK WINS\n" << m_brd.Write() << '\n';
        return m_toPlay == SG_BLACK ? 10 : -10;
    case SG_WHITE:
        *isExact = true;
        //std::cerr << "WHITE WINS\n" << m_brd.Write() << '\n';
        return m_toPlay == SG_WHITE ? 10 : -10;
    default:
         SG_ASSERT(false);
        return 0;
    }
    // Can't be reached!
    return 0;
}

bool YSearch::Execute(SgMove move, int* delta, int depth)
{
    SG_UNUSED(depth);
    *delta = DEPTH_UNIT;
    int cell = static_cast<int>(move);
    m_brd.Play(m_toPlay, cell);
    m_toPlay = SgOppBW(m_toPlay);
    return true;
}

void YSearch::TakeBack()
{
    m_brd.Undo();
    m_toPlay = SgOppBW(m_toPlay);
}

//----------------------------------------------------------------------------
