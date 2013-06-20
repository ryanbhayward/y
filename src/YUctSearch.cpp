//----------------------------------------------------------------------------
/** @file YUctSearch.cpp
 */
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "YUctSearch.h"
#include "YUctSearchUtil.h"

//----------------------------------------------------------------------------

namespace {

template<typename T>
void ShuffleVector(std::vector<T>& v, SgRandom& random)
{
    for (int i= v.size() - 1; i > 0; --i) 
    {
        int j = random.Int(i+1);
        std::swap(v[i], v[j]);
    }
}

}

//----------------------------------------------------------------------------

YUctThreadStateFactory::~YUctThreadStateFactory()
{
}

YUctThreadState* YUctThreadStateFactory::Create(const unsigned int threadId, 
                                                  const SgUctSearch& search)
{
    const YUctSearch& hvSearch = dynamic_cast<const YUctSearch&>(search);
    YUctThreadState* state = new YUctThreadState(hvSearch, threadId);
    return state;
}

//----------------------------------------------------------------------------

YUctThreadState::YUctThreadState(const YUctSearch& search, 
                                   const unsigned int threadId)
    : SgUctThreadState(threadId, Y_MAX_CELL + 1),
      m_search(search),
      m_brd(search.GetBoard().Size())
{
}

YUctThreadState::~YUctThreadState()
{
}

SgUctValue YUctThreadState::Evaluate()
{
    SG_ASSERT(m_brd.IsGameOver());
    // std::cerr << "GAME OVER" << m_brd.ToString() << '\n';
    // if (m_brd.IsWinner(SG_BLACK))
    //     std::cerr << "BLACK WINS!\n";
    // if (m_brd.IsWinner(SG_WHITE))
    //     std::cerr << "WHITE WINS!\n";
    return m_brd.IsWinner(m_brd.ToPlay()) ? 1.0 : 0.0;
}

void YUctThreadState::Execute(SgMove move)
{
    m_brd.Play(m_brd.ToPlay(), move);
}

void YUctThreadState::ExecutePlayout(SgMove move)
{
    m_brd.Play(m_brd.ToPlay(), move);
}

bool YUctThreadState::GenerateAllMoves(SgUctValue count, 
                                       std::vector<SgUctMoveInfo>& moves,
                                       SgUctProvenType& provenType)
{
    moves.clear();
    if (m_brd.IsGameOver())
    {
        if (m_brd.IsWinner(m_brd.ToPlay()))
            provenType = SG_PROVEN_WIN;
        else 
            provenType = SG_PROVEN_LOSS;
        return false;
    }
    SG_UNUSED(count);
    for (BoardIterator it(m_brd); it; ++it)
    {
        if (m_brd.IsEmpty(*it))
            moves.push_back(*it);
    }
    provenType = SG_NOT_PROVEN;
    return false;
}

SgMove YUctThreadState::GeneratePlayoutMove(bool& skipRaveUpdate)
{
    skipRaveUpdate = false;
    //m_brd.CheckConsistency();
    if (m_emptyCells.empty())
        return SG_NULLMOVE;
    if (m_brd.IsGameOver())
        return SG_NULLMOVE;
    SgMove move = SG_NULLMOVE;
    if (m_search.UseSaveBridge()) 
    {
        move = m_brd.SaveBridge(m_brd.LastMove(), m_brd.ToPlay(), m_random);
    } 
    if (move == SG_NULLMOVE)
    {
        do {
            move = m_emptyCells.back();
            m_emptyCells.pop_back();
        } while (m_brd.GetColor(move) != SG_EMPTY);
    }
    return move;
}

void YUctThreadState::StartSearch()
{
    m_brd.SetPosition(m_search.GetBoard());
    m_brd.SetSavePoint1();
}

void YUctThreadState::TakeBackInTree(std::size_t nuMoves)
{
    SG_UNUSED(nuMoves);
}

void YUctThreadState::TakeBackPlayout(std::size_t nuMoves)
{
    SG_UNUSED(nuMoves);
}

void YUctThreadState::GameStart()
{
    m_brd.RestoreSavePoint1();
}

void YUctThreadState::StartPlayouts()
{
    if (m_search.NumberPlayouts() > 1)
        m_brd.SetSavePoint2();
}

void YUctThreadState::StartPlayout()
{
    if (m_search.NumberPlayouts() > 1)
        m_brd.RestoreSavePoint2();

    m_emptyCells.clear();
    for (BoardIterator it(m_brd); it; ++it)
        if (SG_EMPTY == m_brd.GetColor(*it))
            m_emptyCells.push_back(*it);
    ShuffleVector(m_emptyCells, m_random);
}

void YUctThreadState::EndPlayout()
{
}

//----------------------------------------------------------------------------

YUctSearch::YUctSearch(YUctThreadStateFactory* threadStateFactory)
    : SgUctSearch(threadStateFactory, Y_MAX_CELL+1)
    , m_brd(13)
    , m_useSaveBridge(true)
    , m_liveGfx(false)
{
    SetMoveSelect(SG_UCTMOVESELECT_COUNT);
    SetNumberThreads(1);    
    SetBiasTermConstant(0.0);
    SetRave(true);
    SetLockFree(true);
    SetWeightRaveUpdates(false);
    SetRandomizeRaveFrequency(0);
    SetRaveWeightInitial(1.0);
    SetRaveWeightFinal(20000.0);
    SetMaxNodes(2000000000 / sizeof(SgUctNode) / 2); // 2GB of memory
}
 
YUctSearch::~YUctSearch()
{
}

std::string YUctSearch::MoveString(SgMove move) const
{
    return m_brd.Const().ToString(move);
}

SgUctValue YUctSearch::UnknownEval() const
{
    return 0.5;
}

void YUctSearch::OnSearchIteration(SgUctValue gameNumber, 
                                   const unsigned int threadId,
                                   const SgUctGameInfo& info)
{
    SgUctSearch::OnSearchIteration(gameNumber, threadId, info);
    if (m_liveGfx && threadId == 0 && gameNumber > m_nextLiveGfx)
    {
        m_nextLiveGfx = gameNumber + Statistics().m_gamesPerSecond;
        std::ostringstream os;
        os << "gogui-gfx:\n";
        os << "uct\n";
        SgBlackWhite toPlay = m_brd.ToPlay();
        YUctSearchUtil::GoGuiGfx(*this, toPlay, m_brd.Const(), os);
        os << '\n';
        std::cout << os.str();
        std::cout.flush();
    }
}

void YUctSearch::OnStartSearch()
{
    m_nextLiveGfx = 1000;
}

void YUctSearch::OnEndSearch()
{
}

void YUctSearch::OnThreadStartSearch(YUctThreadState& state)
{
    SG_UNUSED(state);
}

void YUctSearch::OnThreadEndSearch(YUctThreadState& state)
{
    SG_UNUSED(state);
}

//----------------------------------------------------------------------------

