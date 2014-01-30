//----------------------------------------------------------------------------
/** @file YSearch.h
 */
//----------------------------------------------------------------------------

#ifndef Y_SEARCH_H
#define Y_SEARCH_H

#include "SgSystem.h"
#include "SgSearch.h"
#include "SgHashTable.h"
#include "board.h"

//----------------------------------------------------------------------------

class YSearchTracer : public SgSearchTracer
{
public: 
    YSearchTracer(SgNode* root, int height);

    virtual ~YSearchTracer();

    /** Creates a new root node for tracing. */
    virtual void InitTracing(const std::string& type);

    /** Adds a move to the node. */
    virtual void AddMoveProp(SgNode* node, SgMove move,
                             SgBlackWhite player);
private:
    int m_height;
};

//----------------------------------------------------------------------------

class YSearch : public SgSearch
{
public:

    YSearch();

    ~YSearch();

    bool CheckDepthLimitReached() const;

    std::string MoveString(SgMove move) const;

    void SetToPlay(SgBlackWhite toPlay);

    SgBlackWhite GetToPlay() const;

    void SetPosition(const Board& brd);

    virtual void CreateTracer();

    virtual void StartOfDepth(int depthLimit);

    virtual void OnStartSearch();

    virtual void Generate(SgVector<SgMove>* moves, int depth);

    virtual int Evaluate(bool* isExact, int depth);

    virtual bool Execute(SgMove move, int* delta, int depth);

    virtual void TakeBack();

    virtual SgHashCode GetHashCode() const;

    virtual bool EndOfGame() const;

private:
    SgBlackWhite m_toPlay;

    Board m_brd;

    std::vector<int> m_history;
};

inline void YSearch::SetToPlay(SgBlackWhite toPlay)
{
    m_toPlay = toPlay;
}

inline SgBlackWhite YSearch::GetToPlay() const
{
    return m_toPlay;
}

inline void YSearch::SetPosition(const Board& brd)
{
    m_brd.SetPosition(brd);
}

inline SgHashCode YSearch::GetHashCode() const
{
    return m_brd.Hash();
}

inline bool YSearch::EndOfGame() const
{
    return m_brd.HasWinningVC() || m_brd.IsGameOver();
}

//----------------------------------------------------------------------------

#endif // Y_SEARCH_H
