//----------------------------------------------------------------------------
/** @file YGtpEngine.h
 */
//----------------------------------------------------------------------------

#ifndef YGTPENGINE_H
#define YGTPENGINE_H

#include <boost/scoped_ptr.hpp>

#include "GtpEngine.h"
#include "SgBWArray.h"
#include "Board.h"
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
    void CmdBoardSize(GtpCommand& cmd);
    void CmdClearBoard(GtpCommand& cmd);
    void CmdSolve(GtpCommand& cmd);
    void CmdUndo(GtpCommand& cmd);
    void CmdGenMove(GtpCommand& cmd);
    void CmdLoadSgf(GtpCommand& cmd);

    void CmdEncodeHistory(GtpCommand& cmd);
    void CmdDecodeHistory(GtpCommand& cmd);

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
    void CmdPlayoutWeights(GtpCommand& cmd);
    void CmdPlayoutStatistics(GtpCommand& cmd);

    void CmdFinalScore(GtpCommand& cmd);
    void CmdVersion(GtpCommand& cmd);

    void CmdTracingOn(GtpCommand& cmd);
    void CmdTracingOff(GtpCommand& cmd);

    void CmdBoardStatistics(GtpCommand& cmd);

    void CmdCellInfo(GtpCommand& cmd);
    void CmdFullConnectedWith(GtpCommand& cmd);
    void CmdSemiConnectedWith(GtpCommand& cmd);
    void CmdCarrierBetween(GtpCommand& cmd);
    void CmdSemisBetween(GtpCommand& cmd);
    void CmdNewSemis(GtpCommand& cmd);
    void CmdPrintSemiTable(GtpCommand& cmd);
    void CmdFullConnectsMultipleBlocks(GtpCommand& cmd);
    void CmdDirtyConCells(GtpCommand& cmd);
    void CmdDirtyWeightCells(GtpCommand& cmd);
    
    void CmdBlockInfo(GtpCommand& cmd);
    void CmdBlockStones(GtpCommand& cmd);
    void CmdBlockLiberties(GtpCommand& cmd);

    void CmdGroupInfo(GtpCommand& cmd);
    void CmdGroupBlocks(GtpCommand& cmd);
    void CmdGroupValue(GtpCommand& cmd);
    void CmdGroupCarrier(GtpCommand& cmd);

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

    void NewGame(int size);

    void SetPosition(const SgNode* node);

    void RegisterCmd(const std::string& name,
                     GtpCallback<YGtpEngine>::Method method);

    SgBlackWhite ColorArg(const GtpCommand& cmd, std::size_t number) const;
};

//----------------------------------------------------------------------------

#endif // YGTPENGINE_H
