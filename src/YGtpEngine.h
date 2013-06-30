//----------------------------------------------------------------------------
/** @file YGtpEngine.h
 */
//----------------------------------------------------------------------------

#ifndef YGTPENGINE_H
#define YGTPENGINE_H

#include <boost/scoped_ptr.hpp>

#include "GtpEngine.h"
#include "SgBWArray.h"
#include "board.h"
#include "YSearch.h"
#include "YUctSearch.h"

//----------------------------------------------------------------------------

class YGtpEngine: public GtpEngine
{
public:
    YGtpEngine(int boardSize);

    ~YGtpEngine();

    /** @name  Command Callbacks */
    // @{

    void CmdExec(GtpCommand& cmd);
    void CmdPlay(GtpCommand& cmd);
    void CmdName(GtpCommand& cmd);
    void CmdShowBoard(GtpCommand& cmd);
    void CmdShowBorders(GtpCommand& cmd);
    void CmdShowAnchors(GtpCommand& cmd);
    void CmdBoardSize(GtpCommand& cmd);
    void CmdClearBoard(GtpCommand& cmd);
    void CmdSolve(GtpCommand& cmd);
    void CmdUndo(GtpCommand& cmd);
    void CmdGenMove(GtpCommand& cmd);
#if GTPENGINE_INTERRUPT
    void CmdInterrupt(GtpCommand& cmd);
#endif
    void CmdAnalyzeCommands(GtpCommand& cmd);
    void CmdSetPlayer(GtpCommand& cmd);
    void CmdTimeLeft(GtpCommand& cmd);
    void CmdTimeSettings(GtpCommand& cmd);
    void CmdUseGridCoords(GtpCommand& cmd);
    void CmdParam(GtpCommand& cmd);
    void CmdUctProvenNodes(GtpCommand& cmd);
    void CmdUctScores(GtpCommand& cmd);
    void CmdRaveScores(GtpCommand& cmd);
    void CmdPlayoutMove(GtpCommand& cmd);

    void CmdFinalScore(GtpCommand& cmd);
    void CmdVersion(GtpCommand& cmd);

    void CmdBlockInfo(GtpCommand& cmd);
    void CmdBlockStones(GtpCommand& cmd);
    void CmdBlockLiberties(GtpCommand& cmd);
    void CmdSharedLiberties(GtpCommand& cmd);
    void CmdBlockLibertiesWith(GtpCommand& cmd);

    void CmdActiveBlocks(GtpCommand& cmd);
    void CmdGroup(GtpCommand& cmd);
    void CmdGroupBlocks(GtpCommand& cmd);
    void CmdGroupValue(GtpCommand& cmd);

    // @}

#if GTPENGINE_INTERRUPT
    /** Calls SgSetUserAbort(). */
    void Interrupt();
#endif

protected:
    /* Clears SgAbortFlag() */
    void BeforeHandleCommand();

    /* Does nothing */
    void BeforeWritingResponse();

private:

    Board m_brd;

    std::vector<int> m_history;

    YSearch m_search;

    SgSearchHashTable m_hashTable;

    YUctSearch m_uctSearch;

    std::size_t m_uctMaxGames;

    double m_uctMaxTime;

    std::string m_playerName;

    double m_mainTime;

    SgBWArray<double> m_timeLeft;

    bool m_timeSettingsSpecified;

    bool m_ignoreClock;

    bool m_allowSwap;
   
    SgBlackWhite BlackWhiteArg(const GtpCommand& cmd, 
                               std::size_t number) const;

    void ApplyTimeSettings();

    int GenMove(bool useGameClock, int toPlay);

    int CellArg(const GtpCommand& cmd, std::size_t number) const;
   
    void Play(SgBlackWhite color, int cell);

    void Undo();

    void NewGame();

    void RegisterCmd(const std::string& name,
                     GtpCallback<YGtpEngine>::Method method);

    SgBlackWhite ColorArg(const GtpCommand& cmd, std::size_t number) const;
};

//----------------------------------------------------------------------------

#endif // YGTPENGINE_H
