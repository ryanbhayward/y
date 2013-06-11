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
    return m_brd.ToString(static_cast<int>(move));
}

void YSearch::CreateTracer()
{
    //YSearchTracer* tracer = new YSearchTracer(0, m_brd.Const().Width());
    YSearchTracer* tracer = new YSearchTracer(0, 10);
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
    for (BoardIterator it(m_brd); it; ++it)
        if (m_brd.IsEmpty(*it))
            moves->PushBack(*it);
}

int YSearch::Evaluate(bool* isExact, int depth)
{
    SG_UNUSED(depth);
    YGameOverType type = m_brd.GetWinner();
    switch(type)
    {
    case Y_NO_WINNER:
        *isExact = false;
        return 0;
    case Y_BLACK_WINS:
        *isExact = true;
        //std::cerr << "BLACK WINS\n" << m_brd.Write() << '\n';
        return m_toPlay == SG_BLACK ? 10 : -10;
    case Y_WHITE_WINS:
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
    int bdset;
    m_brd.move(Move(m_toPlay, cell),true, bdset);
    m_history.push_back(cell);
    m_toPlay = SgOppBW(m_toPlay);
    return true;
}

void YSearch::TakeBack()
{
    SG_ASSERT(!m_history.empty());
    // FIXME: implement undo
    //m_brd.RemoveStone(m_history.back());
    m_history.pop_back();
    m_toPlay = SgOppBW(m_toPlay);
}

//----------------------------------------------------------------------------
