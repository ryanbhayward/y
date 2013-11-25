//----------------------------------------------------------------------------
/** @file YUctSearch.cpp
 */
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "YSystem.h"
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
    m_weights = new WeightedRandom[2];
}

YUctThreadState::~YUctThreadState()
{
    delete [] m_weights;
}

void YUctThreadState::StartSearch()
{
    m_brd.SetPosition(m_search.GetBoard());
    m_brd.SetSavePoint1();
}

void YUctThreadState::GameStart()
{
    m_brd.RestoreSavePoint1();
}


//---------------------------------------------------------------------------
// Tree stuff

void YUctThreadState::TakeBackInTree(std::size_t nuMoves)
{
    SG_UNUSED(nuMoves);
}

bool YUctThreadState::GenerateAllMoves(SgUctValue count, 
                                       std::vector<SgUctMoveInfo>& moves,
                                       SgUctProvenType& provenType)
{
    moves.clear();
    if (m_brd.HasWinningVC()) 
    {
        if (m_brd.IsVCWinner(m_brd.ToPlay()))
            provenType = SG_PROVEN_WIN;
        else
            provenType = SG_PROVEN_LOSS;
        return false;
    }
    // TODO: NEED THIS CHECK IN ADDITION TO ABOVE CHECK?
    if (m_brd.IsGameOver())
    {
        if (m_brd.IsWinner(m_brd.ToPlay()))
            provenType = SG_PROVEN_WIN;
        else 
            provenType = SG_PROVEN_LOSS;
        return false;
    }
    SG_UNUSED(count);
    for (Board::EmptyIterator it(m_brd); it; ++it)
    {
	moves.push_back(*it);
    }
    provenType = SG_NOT_PROVEN;
    return false;
}

void YUctThreadState::Execute(SgMove move)
{
    // YTrace() << m_brd.ToString() << '\n'
    //           << "move=" << m_brd.ToString(move) << '\n';
    m_brd.Play(m_brd.ToPlay(), move);
    //m_brd.GroupExpand(move);
}

//---------------------------------------------------------------------------
// Playout stuff

SgUctValue YUctThreadState::Evaluate()
{
    SG_ASSERT(m_brd.IsGameOver());
    return m_brd.IsWinner(m_brd.ToPlay()) ? 1.0 : 0.0;
}

SgMove YUctThreadState::GenerateLocalMove()
{
    SgMove move = SG_NULLMOVE;
    if (m_search.UseSaveBridge()) 
    {
        m_localMoves.Clear();
        
        m_brd.GeneralSaveBridge(m_localMoves);

        float random = m_random.Float(m_weights[m_brd.ToPlay()].Total() + 
                                      m_localMoves.Total());
        if (random < m_localMoves.Total())
        {
            move = m_localMoves.Choose(random);
            YUctSearch::PlayoutStatistics::Get().m_localMoves++;
        }
    } 
    return move;
}

SgMove YUctThreadState::GenerateGlobalMove()
{
    SgMove move = SG_NULLMOVE;

    // m_weights[toPlay].Build();
    // move = m_weights[toPlay].Choose(m_random);
    move = m_weights[m_brd.ToPlay()].ChooseLinear(m_random);
    //YTrace() << "global move = " << m_brd.ToString(move) << '\n';
    YUctSearch::PlayoutStatistics::Get().m_globalMoves++;

    if (!m_brd.IsEmpty(move)) {
        //throw BenzeneException() << "Weighted move not empty!\n";
        YTrace() << "Weighted move not empty!\n";
        abort();
    }
    return move;
}

SgMove YUctThreadState::GeneratePlayoutMove(bool& skipRaveUpdate)
{
    skipRaveUpdate = false;
    //m_brd.CheckConsistency();
    if (m_weights[m_brd.ToPlay()].Total() < 0.0001)
        return SG_NULLMOVE;
    if (m_brd.IsGameOver())
        return SG_NULLMOVE;

    YUctSearch::PlayoutStatistics::Get().m_totalMoves++;
    SgMove move = SG_NULLMOVE;

    move = GenerateLocalMove();
    if (move == SG_NULLMOVE)
    {
        move = GenerateGlobalMove();
    }
    // YTrace() << m_brd.ToString() << '\n'
    //           << "Move: " << m_brd.ToString(move) 
    //           << " Weight: " << m_weights[m_brd.ToPlay()][move] << '\n';
    return move;
}

void YUctThreadState::ExecutePlayout(SgMove move)
{
    // YTrace() << m_brd.ToString() << '\n'
    //           << "move=" << m_brd.ToString(move) << '\n';
    m_brd.Play(m_brd.ToPlay(), move);
    m_weights[SG_BLACK].SetWeight(move, 0.0f);
    m_weights[SG_WHITE].SetWeight(move, 0.0f);

    const MarkedCellsWithList& dirty = m_brd.GetAllDirtyWeightCells();
    Board::Statistics::Get().m_numDirtyCellsPerMove += dirty.m_list.Length();

    // MarkedCellsWithList threatInter, threatUnion;
    // m_brd.MarkAllThreats(dirty, threatInter, threatUnion);
    // if (threatInter.IsEmpty()) {
    //     // store the win carrier; from now on all moves are played
    //     // into this carrier
    //     m_winCarrier = threatUnion;
    // }
    for (MarkedCellsWithList::Iterator i(dirty); i; ++i) {
        cell_t p = *i;
        ComputeWeight(p);
    }
}

void YUctThreadState::InitializeWeights()
{
    m_weights[SG_BLACK].Clear();
    m_weights[SG_WHITE].Clear();
    //MarkedCellsWithList threatInter, threatUnion;
    // m_brd.MarkAllThreats(m_brd.GetAllEmptyCells(),
    //                      threatsInter, threatsUnion);
    for (Board::EmptyIterator it(m_brd); it; ++it) {
	ComputeWeight(*it);
    }
    m_weights[SG_BLACK].Build();
    m_weights[SG_WHITE].Build();
}

void YUctThreadState::TakeBackPlayout(std::size_t nuMoves)
{
    SG_UNUSED(nuMoves);
}

//---------------------------------------------------------------------------


void YUctThreadState::StartPlayouts()
{
    InitializeWeights();

    if (m_search.NumberPlayouts() > 1) {
        m_brd.SetSavePoint2();
        // TODO: copy the weights somewhere
        YTrace() << "NumberPlayouts() > 1 NOT SUPPORTED!\n";
        abort();
    }
}

void YUctThreadState::StartPlayout()
{
    if (m_search.NumberPlayouts() > 1) {
        m_brd.RestoreSavePoint2();
        // TODO: restore weights here
    }
}

void YUctThreadState::StartPlayout(const Board& other)
{
    m_brd.SetPosition(other);
    InitializeWeights();
}

void YUctThreadState::EndPlayout()
{
}

//----------------------------------------------------------------------------

void YUctThreadState::ComputeWeight(cell_t p)
{
    if (m_brd.IsCellDead(p)) {
        m_weights[SG_BLACK].SetWeight(p, LocalMoves::WEIGHT_DEAD_CELL);
        m_weights[SG_WHITE].SetWeight(p, LocalMoves::WEIGHT_DEAD_CELL);
        m_brd.MarkCellAsDead(p);
    } 
    else if (m_brd.IsCellThreat(p)) {
    	m_weights[SG_BLACK].SetWeight(p, LocalMoves::WEIGHT_WIN_THREAT);
    	m_weights[SG_WHITE].SetWeight(p, LocalMoves::WEIGHT_WIN_THREAT);
    	m_brd.MarkCellAsThreat(p);
    }
    else {
        m_brd.MarkCellNotThreat(p);
        float w = m_brd.WeightCell(p);
        m_weights[SG_BLACK].SetWeight(p, w);
        m_weights[SG_WHITE].SetWeight(p, w);
    }
}

void YUctThreadState::GetWeightsForLastMove
(std::vector<float>& weights, SgBlackWhite toPlay) const
{
    weights.assign(Y_MAX_CELL, 0.0f);
    for (CellIterator i(m_brd.Const()); i; ++i)
        weights[*i] = m_weights[toPlay][*i];
    for (size_t i = 0; i < m_localMoves.move.size(); ++i)
        weights[ m_localMoves.move[i] ] += m_localMoves.gamma[i];
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
    return m_brd.ToString(move);
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
