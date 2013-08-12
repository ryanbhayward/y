//----------------------------------------------------------------------------
/** @file YUctSearch.h
*/
//----------------------------------------------------------------------------

#ifndef Y_UCTSEARCH_H
#define Y_UCTSEARCH_H

#include "SgUctSearch.h"
#include "SgUctTree.h"
#include "SgRandom.h"
#include "board.h"
#include "WeightedRandom.h"

//----------------------------------------------------------------------------

class YUctSearch;

/** Base class for the thread state. */
class YUctThreadState : public SgUctThreadState
{
public:

    YUctThreadState(const YUctSearch& search, const unsigned int threadId);

    ~YUctThreadState();

    /** @name Pure virtual functions from SgUctSearch. */
    // @{

    SgUctValue Evaluate();

    void Execute(SgMove move);

    void ExecutePlayout(SgMove move);

    bool GenerateAllMoves(SgUctValue count, 
                          std::vector<SgUctMoveInfo>& moves,
                          SgUctProvenType& provenType);

    SgMove GeneratePlayoutMove(bool& skipRaveUpdate);

    void StartSearch();

    void TakeBackInTree(std::size_t nuMoves);

    void TakeBackPlayout(std::size_t nuMoves);

    // @} // name

    /** @name Virtual functions from SgUctSearch */
    // @{

    void GameStart();
    
    void StartPlayouts();

    void StartPlayout();

    void EndPlayout();

    // @} // name

    void StartPlayout(const Board& other);

    /** Fills weights with weight for each move.
        Call after GenerateMove(). */
    void GetWeightsForLastMove(std::vector<float>& weights, 
                               SgBlackWhite toPlay) const;


 private:

    const YUctSearch& m_search;

    Board m_brd;

    SgRandom m_random;

    WeightedRandom* m_weights;

    LocalMoves m_localMoves;

    void ComputeWeight(cell_t p);
    void InitializeWeights();
    SgMove GenerateLocalMove();
    SgMove GenerateGlobalMove();

};

//----------------------------------------------------------------------------

class YUctSearch;

/** Create game specific thread state.
    @see YUctThreadState
*/
class YUctThreadStateFactory : public SgUctThreadStateFactory
{
public:
    virtual ~YUctThreadStateFactory();

    YUctThreadState* Create(const unsigned int threadId, 
                             const SgUctSearch& search);
};

//----------------------------------------------------------------------------

/** Monte Carlo tree search using UCT. */
class YUctSearch : public SgUctSearch
{
public:
    struct PlayoutStatistics
    {
        size_t m_localMoves;
        size_t m_globalMoves;
        size_t m_totalMoves;

        PlayoutStatistics()
        { 
            Clear(); 
        }

        void Clear()
        {
            m_localMoves = 0;
            m_globalMoves = 0;
            m_totalMoves = 0;
        }

        std::string ToString() const
        {
            std::ostringstream os;
            os << '['
               << "local_moves=" << m_localMoves << ' '
               << "global_moves=" << m_globalMoves << ' '
               << "total_moves=" << m_totalMoves
               << ']';
            return os.str();
        }

        static PlayoutStatistics& Get()
        {
            static PlayoutStatistics s_stats;
            return s_stats;
        }
    };

    YUctSearch(YUctThreadStateFactory* threadStateFactory);

    virtual ~YUctSearch();

    /** @name Pure virtual functions from SgUctSearch */
    // @{

    std::string MoveString(SgMove move) const;

    SgUctValue UnknownEval() const;

    // @} // name

    /** @name Virtual functions from SgUctSearch */
    // @{

    void OnSearchIteration(SgUctValue gameNumber, 
                           const unsigned int threadId,
                           const SgUctGameInfo& info);
    
    void OnStartSearch();

    void OnEndSearch();

    void OnThreadStartSearch(YUctThreadState& state);

    void OnThreadEndSearch(YUctThreadState& state);

    // @} // name

    void SetPosition(const Board& brd);

    const Board& GetBoard() const;

    bool UseSaveBridge() const    { return m_useSaveBridge; }
    void SetUseSaveBridge(bool f) { m_useSaveBridge = f; }

    bool LiveGfx() const          { return m_liveGfx; }
    void SetLiveGfx(bool f)       { m_liveGfx = f; }

private:
    Board m_brd;

    bool m_useSaveBridge;

    bool m_liveGfx;

    SgUctValue m_nextLiveGfx;
};

inline void YUctSearch::SetPosition(const Board& brd)
{
    m_brd.SetPosition(brd);
}

inline const Board& YUctSearch::GetBoard() const
{
    return m_brd;
}

//----------------------------------------------------------------------------

#endif // Y_UCTSEARCH_H
