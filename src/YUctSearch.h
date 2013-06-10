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

 private:

    const YUctSearch& m_search;

    Board m_brd;

    SgRandom m_random;

    std::vector<SgMove> m_emptyCells;
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
    YUctSearch(YUctThreadStateFactory* threadStateFactory);

    virtual ~YUctSearch();

    /** @name Pure virtual functions from SgUctSearch */
    // @{

    std::string MoveString(SgMove move) const;

    SgUctValue UnknownEval() const;

    // @} // name

    /** @name Virtual functions from SgUctSearch */
    // @{

    void OnSearchIteration(std::size_t gameNumber, const unsigned int threadId,
                           const SgUctGameInfo& info);
    
    void OnStartSearch();

    void OnEndSearch();

    void OnThreadStartSearch(YUctThreadState& state);

    void OnThreadEndSearch(YUctThreadState& state);

    // @} // name

    void SetBoard(const Board& brd);

    const Board& GetBoard() const;

private:
    Board m_brd;
};

inline void YUctSearch::SetBoard(const Board& brd)
{
    m_brd = brd;
}

inline const Board& YUctSearch::GetBoard() const
{
    return m_brd;
}

//----------------------------------------------------------------------------

#endif // Y_UCTSEARCH_H
