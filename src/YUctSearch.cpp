//----------------------------------------------------------------------------
/** @file YUctSearch.cpp
 */
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "YUctSearch.h"

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
      m_brd(search.GetBoard())
{
}

YUctThreadState::~YUctThreadState()
{
}

SgUctValue YUctThreadState::Evaluate()
{
    SG_ASSERT(m_brd.IsGameOver());
    // std::cerr << "GAME OVER" << m_brd.ToString() << '\n';
    // if (m_brd.IsWinner(BLK))
    //     std::cerr << "BLACK WINS!\n";
    // if (m_brd.IsWinner(WHT))
    //     std::cerr << "WHITE WINS!\n";
    return m_brd.IsWinner(m_brd.ToPlay()) ? 1.0 : 0.0;
}

void YUctThreadState::Execute(SgMove move)
{
    //m_brd.PlayStone(m_brd.ToPlay(), move);
    int bdset;
    m_brd.move(Move(m_brd.ToPlay(), move), false, bdset);
}

void YUctThreadState::ExecutePlayout(SgMove move)
{
    //m_brd.PlayStone(m_brd.ToPlay(), move);
    int bdset;
    m_brd.move(Move(m_brd.ToPlay(), move), false, bdset);
    //std::cerr << m_brd.ToString() << '\n';
    //std::cerr << "played " << m_brd.Const().ToString(move) << '\n';
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
    if (m_emptyCells.empty())
        return SG_NULLMOVE;
    if (m_brd.IsGameOver())
        return SG_NULLMOVE;
    SgMove move = m_emptyCells.back();
    m_emptyCells.pop_back();
    return move;
}

void YUctThreadState::StartSearch()
{
    m_brd = m_search.GetBoard();
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
    m_brd = m_search.GetBoard();
}

void YUctThreadState::StartPlayouts()
{
}

void YUctThreadState::StartPlayout()
{
    m_emptyCells.clear();
    for (BoardIterator it(m_brd); it; ++it)
        if (SG_EMPTY == m_brd.board[*it])
            m_emptyCells.push_back(*it);
    ShuffleVector(m_emptyCells, m_random);
}

void YUctThreadState::EndPlayout()
{
}

//----------------------------------------------------------------------------

YUctSearch::YUctSearch(YUctThreadStateFactory* threadStateFactory)
    : SgUctSearch(threadStateFactory, Y_MAX_CELL+1),
      m_brd(4)
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
    SetMaxNodes(11500000);  // 1GB of memory
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

void YUctSearch::OnSearchIteration(std::size_t gameNumber, 
                                    const unsigned int threadId,
                                    const SgUctGameInfo& info)
{
    SG_UNUSED(gameNumber);
    SG_UNUSED(threadId);
    SG_UNUSED(info);
}

void YUctSearch::OnStartSearch()
{
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

