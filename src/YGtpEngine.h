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
    void CmdBoardSize(GtpCommand& cmd);
    void CmdClearBoard(GtpCommand& cmd);
    void CmdSolve(GtpCommand& cmd);
    void CmdUndo(GtpCommand& cmd);
    void CmdGenMove(GtpCommand& cmd);
    void CmdAnalyzeCommands(GtpCommand& cmd);
    void CmdSetPlayer(GtpCommand& cmd);
    void CmdTimeLeft(GtpCommand& cmd);
    void CmdTimeSettings(GtpCommand& cmd);
    void CmdUseGridCoords(GtpCommand& cmd);
    void CmdUctParamSearch(GtpCommand& cmd);
    void CmdUctProvenNodes(GtpCommand& cmd);
    void CmdUctScores(GtpCommand& cmd);
    void CmdRaveScores(GtpCommand& cmd);
    void CmdWinner(GtpCommand& cmd);
    void CmdVersion(GtpCommand& cmd);

    // @}

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

    int ColorArg(const GtpCommand& cmd, std::size_t number) const;
    int SgColorToYColor(SgBlackWhite color) const;
    SgBlackWhite YColorToSgColor(int color) const;
};

//----------------------------------------------------------------------------

#endif // YGTPENGINE_H
