//----------------------------------------------------------------------------
/** @file YGtpEngine.cpp
 */
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "SgRandom.h"
#include "SgSearchControl.h"
#include "SgSearch.h"
#include "SgTimer.h"
#include "SgGameWriter.h"
#include "YGtpEngine.h"
#include <fstream>

//----------------------------------------------------------------------------

YGtpEngine::YGtpEngine(int boardSize)
    : GtpEngine(),
      m_brd(boardSize),
      m_search(),
      m_hashTable(16000000), // 64MB
      m_uctSearch(new YUctThreadStateFactory()),
      m_uctMaxGames(99999999),
      m_uctMaxTime(10.0),
      m_playerName("uct"),
      m_mainTime(0),
      m_timeLeft(0),
      m_timeSettingsSpecified(false),
      m_ignoreClock(false)
{
    RegisterCmd("exec", &YGtpEngine::CmdExec);
    RegisterCmd("name", &YGtpEngine::CmdName);
    RegisterCmd("play", &YGtpEngine::CmdPlay);
    RegisterCmd("boardsize", &YGtpEngine::CmdBoardSize);
    RegisterCmd("showboard", &YGtpEngine::CmdShowBoard);
    RegisterCmd("showborders", &YGtpEngine::CmdShowBorders);
    RegisterCmd("clear_board", &YGtpEngine::CmdClearBoard);
    RegisterCmd("genmove", &YGtpEngine::CmdGenMove);
    RegisterCmd("hexgui-analyze_commands", 
                &YGtpEngine::CmdAnalyzeCommands);
    RegisterCmd("set_player", &YGtpEngine::CmdSetPlayer);
    RegisterCmd("time_left", &YGtpEngine::CmdTimeLeft);
    RegisterCmd("time_settings", &YGtpEngine::CmdTimeSettings);
    RegisterCmd("undo", &YGtpEngine::CmdUndo);
    RegisterCmd("uct_param_search", &YGtpEngine::CmdUctParamSearch);
    RegisterCmd("uct_proven_nodes", &YGtpEngine::CmdUctProvenNodes);
    RegisterCmd("uct_scores", &YGtpEngine::CmdUctScores);
    RegisterCmd("uct_rave_scores", &YGtpEngine::CmdRaveScores);
    RegisterCmd("y_winner", &YGtpEngine::CmdWinner);
    RegisterCmd("y_solve", &YGtpEngine::CmdSolve);
    RegisterCmd("version", &YGtpEngine::CmdVersion);

    NewGame();
}

YGtpEngine::~YGtpEngine()
{
}

//----------------------------------------------------------------------------

void YGtpEngine::RegisterCmd(const std::string& name,
                                GtpCallback<YGtpEngine>::Method method)
{
    Register(name, new GtpCallback<YGtpEngine>(this, method));
}

void YGtpEngine::CmdAnalyzeCommands(GtpCommand& cmd)
{
    cmd.CheckArgNone();
    cmd <<
        "param/Game Param/param_game\n"
        "plist/All Legal Moves/all_legal_moves %c\n"
        "string/ShowBoard/showboard\n"
        "string/Final Score/final_score\n"
        "varc/Reg GenMove/reg_genmove %c\n";
}

SgBlackWhite YGtpEngine::ColorArg(const GtpCommand& cmd, 
                                  std::size_t number) const
{
    std::string value = cmd.ArgToLower(number);
    if (value == "b" || value == "black")
        return SG_BLACK;
    if (value == "w" || value == "white")
        return SG_WHITE;
    throw GtpFailure() << "argument " << (number + 1)
                       << " must be black or white";
}

int YGtpEngine::CellArg(const GtpCommand& cmd, 
                        std::size_t number) const
{
    return m_brd.Const().FromString(cmd.ArgToLower(number));
}

void YGtpEngine::BeforeHandleCommand()
{
    SgSetUserAbort(false);
}

void YGtpEngine::BeforeWritingResponse()
{
}

void YGtpEngine::ApplyTimeSettings()
{
    m_timeLeft[SG_BLACK] = m_mainTime;
    m_timeLeft[SG_WHITE] = m_mainTime;
}

void YGtpEngine::NewGame()
{
    m_brd = Board(); //m_brd.Clear();
    m_history.clear();
    ApplyTimeSettings();
}

void YGtpEngine::Play(int color, int cell)
{
    if (cell == Y_SWAP && m_brd.CanSwap())
    	m_brd.Swap();
    else if (cell != Y_NULL_MOVE)
    {
        if (! m_brd.Const().IsOnBoard(cell))
            throw GtpFailure("Cell not on board!");
        if (m_brd.IsOccupied(cell))
            throw GtpFailure("Cell is occupied!");

        //m_brd.move(color, cell);
        int bdset;
        m_brd.move(Move(color,cell), false, bdset);
    }
    m_history.push_back(cell);
}

void YGtpEngine::Undo()
{
    SG_ASSERT(! m_history.empty());
    int cell = m_history.back();
    if (cell == Y_SWAP)
    	m_brd.UndoSwap();
    else if (cell != Y_NULL_MOVE)
    	m_brd.RemoveStone(cell);
    m_history.pop_back();
}

int YGtpEngine::GenMove(bool useGameClock, SgBlackWhite toPlay)
{
#if 0
    // Size 4: open with 'b2'
    if (m_history.empty() && m_brd.Const().Size() == 4)
        return HvCellUtil::CoordsToCell(1, 1);
    // Size 5: open with 'a3'
    if (m_history.empty() && m_brd.Const().Size() == 5)
        return HvCellUtil::CoordsToCell(0, 2);
#endif

    if (m_playerName == "uct")
    {
        m_brd.SetToPlay(toPlay);        
        m_uctSearch.SetBoard(m_brd);
        std::vector<SgMove> sequence;
        double maxTime = m_uctMaxTime;
        std::size_t maxGames = m_uctMaxGames;
        if (useGameClock)
        {
            double timeLeft = m_timeLeft[toPlay];
            std::cerr << "timeLeft=" << timeLeft << ' ';
            if (timeLeft > 0)
            {
                maxTime = timeLeft / 10.0;
                maxGames = 99999999;
            }
            else
                maxTime = 1;
        }
        if (maxTime < 0.5)
            maxTime = 0.5;
        std::cerr << "maxTime=" << maxTime << " maxGames=" << maxGames << '\n';
        float score = m_uctSearch.Search(maxGames, maxTime, sequence);
        m_uctSearch.WriteStatistics(std::cerr);
        std::cerr << "Score          " << std::setprecision(2) << score << '\n';
        for (std::size_t i = 0; i < sequence.size(); i++) 
            std::cerr << ' ' << m_brd.Const().ToString(sequence[i]);
        std::cerr << '\n';

        if (m_brd.CanSwap() && score < 0.5)
            return Y_SWAP;
        return sequence[0];
    }
    else if (m_playerName == "random")
    {
#if 0
        std::vector<int> empty;
        for (BoardIterator it(m_brd); it; ++it)
            if (m_brd.IsEmpty(*it))
                empty.push_back(*it);
        return empty[SgRandom::Global().Int(empty.size())];
#endif
        return 1; // FIXME: IMPLEMENT
    }
    throw GtpFailure("Invalid player!");
    return Y_NULL_MOVE;
}

//----------------------------------------------------------------------------

void YGtpEngine::CmdExec(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    std::string filename = cmd.Arg(0);
    try {
        ExecuteFile(filename, std::cerr);
    }
    catch (std::exception& e) {
        std::cerr << "Errors occured.\n";
    }
}

void YGtpEngine::CmdName(GtpCommand& cmd)
{
    cmd << PACKAGE_NAME;
}

void YGtpEngine::CmdVersion(GtpCommand& cmd)
{
    cmd << VERSION;
}

void YGtpEngine::CmdBoardSize(GtpCommand& cmd)
{
    cmd.CheckNuArg(2);  // ignore second argument (so we work in hexgui)
    int size = cmd.IntArg(0, 1, Y_MAX_SIZE);
    m_brd = Board(size);
}

void YGtpEngine::CmdClearBoard(GtpCommand& cmd)
{
    cmd.CheckNuArg(0);
    NewGame();
}

void YGtpEngine::CmdPlay(GtpCommand& cmd)
{
    cmd.CheckNuArg(2);
    int color = ColorArg(cmd, 0);
    if (cmd.ArgToLower(1) == "resign") 
    	// play dummy move to keep HGui in sync with undo
        Play(color, Y_NULL_MOVE);
    // else if (color != m_brd.ToPlay())
    //     throw GtpFailure("It is the other player's turn!");
    else
    {
        int cell = CellArg(cmd, 1);
        Play(color, cell);
    }
}

void YGtpEngine::CmdUndo(GtpCommand& cmd)
{
    cmd.CheckNuArg(0);
    Undo();
}

void YGtpEngine::CmdShowBoard(GtpCommand& cmd)
{
    SG_UNUSED(cmd);
    cmd << m_brd.ToString() << '\n';
}

void YGtpEngine::CmdShowBorders(GtpCommand& cmd)
{
    SG_UNUSED(cmd);
    cmd << m_brd.BorderToString() << '\n';
}

void YGtpEngine::CmdWinner(GtpCommand& cmd)
{
    SgBoardColor winner = m_brd.GetWinner();
    if (winner == SG_BLACK)
        cmd << "black";
    else if (winner == SG_WHITE)
        cmd << "white";
    else
        cmd << "none";
}

void YGtpEngine::CmdGenMove(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int color = ColorArg(cmd, 0);
    // if (color != m_brd.ToPlay())
    //     throw GtpFailure("It is the other player's turn!");
    if (m_brd.IsGameOver()) {
        std::cerr << "GAME OVER!?!\n";
        cmd << "resign";
    }
    else
    {
        SgTimer timer;
        int move(GenMove(m_timeSettingsSpecified && !m_ignoreClock, color));
        timer.Stop();
        if (m_timeSettingsSpecified && !m_ignoreClock)
            m_timeLeft[color] -= timer.GetTime();
        Play(color, move);
        cmd << m_brd.Const().ToString(move);
    }
}

std::string PrintPV(const SgVector<SgMove>& pv, const Board& brd)
{
    std::ostringstream os;
    for (int i = 0; i < pv.Length(); ++i)
    {
        if (i) os << ' ';
        os << brd.Const().ToString(pv[i]);
    }
    return os.str();
}

/** Solves the current position with iterative deepening.
    Usage: "havannah_solve [timelimit] [max depth] [doTrace]"

    Timelimit: max time in seconds for the search.
    Max depth: maximum depth to search to.
    doTrace: whether to perform a search trace (default is off).
*/
void YGtpEngine::CmdSolve(GtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(4);
    if (cmd.NuArg() < 1)
        throw GtpFailure("Must give color to play");
    SgBlackWhite toPlay = ColorArg(cmd, 0);
    boost::scoped_ptr<SgTimeSearchControl> timeControl;
    if (cmd.NuArg() >= 2)
    {
        double timelimit = cmd.FloatArg(0);
        if (timelimit > 0.0)
            timeControl.reset(new SgTimeSearchControl(timelimit));
    }
    int maxDepth = m_brd.Const().TotalCells; //m_brd.Const().NumCells();
    if (cmd.NuArg() >= 3)
        maxDepth = cmd.IntArg(1, 0);
    bool doTrace = false;
    if (cmd.NuArg() == 4)
        doTrace = cmd.BoolArg(2);
    SgVector<SgMove> pv;
    m_search.SetPosition(m_brd);
    m_search.SetToPlay(toPlay);
    m_search.SetSearchControl(timeControl.get());
    SgNode* traceNode = 0;
    if (doTrace)
    {
        m_search.CreateTracer();
        traceNode = new SgNode();
    }
    m_search.SetHashTable(&m_hashTable);
    int value = m_search.IteratedSearch(0, maxDepth, -10, 10, &pv,
                                        true, traceNode);
    m_search.SetSearchControl(0);
    m_search.SetTracer(0);
    const SgSearchStatistics& stats = m_search.Statistics();
    const int depth = stats.DepthReached();
    const int nodes = stats.NumNodes();

    if (traceNode)
    {
        // Add root property: board size.
        // SgPropInt* boardSize = new SgPropInt(SG_PROP_SIZE,
        //                                      m_brd.Const().Size());
        SgPropInt* boardSize = new SgPropInt(SG_PROP_SIZE, 10);

        traceNode->Add(boardSize);
        // TODO: add board position as setup stones

        std::ofstream fs("trace.sgf");
        SgGameWriter gw(fs);
        //gw.WriteGame(*traceNode, true, 4, 11, m_brd.Const().Width());
        gw.WriteGame(*traceNode, true, 4, 11, 10);
        fs.close();
        traceNode->DeleteTree();
    }

    if (value == 0)
        cmd << "unknown none";
    else if (value == -1 || value == 1)
    {
        cmd << "draw " << (pv.IsEmpty() ? "none" 
                           : m_brd.Const().ToString(pv[0]));
    }
    else if (value == 10)
    {
        cmd << (toPlay == SG_BLACK ? "black" : "white") << ' ' 
            << (pv.IsEmpty() ? "none" : m_brd.Const().ToString(pv[0]));
        std::cerr << "PV: " << PrintPV(pv, m_brd) << '\n';
    }
    else if (value == -10)
        cmd << (toPlay == SG_BLACK ? "white" : "black") << ' '
            << "none";
    cmd << ' ' << depth << ' ' << nodes;
}

void YGtpEngine::CmdUctParamSearch(GtpCommand& cmd)
{
    if (cmd.NuArg() == 0)
    {
        cmd << '\n'
            << "[bool] ignore_clock " << m_ignoreClock << '\n'
            << "[bool] use_rave " << m_uctSearch.Rave() << '\n'
            << "[string] bias_term_constant " 
            << m_uctSearch.BiasTermConstant() << '\n'
            << "[string] num_threads " << m_uctSearch.NumberThreads() << '\n'
            << "[string] max_games " << m_uctMaxGames << '\n'
            << "[string] max_time " << m_uctMaxTime << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "use_rave")
            m_uctSearch.SetRave(cmd.BoolArg(1));
        else if (name == "ignore_clock")
            m_ignoreClock = cmd.BoolArg(1);
        else if (name == "bias_term_constant")
            m_uctSearch.SetBiasTermConstant(cmd.FloatArg(1));
        else if (name == "num_threads")
            m_uctSearch.SetNumberThreads(cmd.IntArg(1, 1));
        else if (name == "max_games")
            m_uctMaxGames = cmd.SizeTypeArg(1, 2);
        else if (name == "max_time")
            m_uctMaxTime = cmd.FloatArg(1);
        else
            throw GtpFailure("Unknown parameter name!");
    }
    else
        throw GtpFailure("Expected 0 or 2 parameters!");
}

void YGtpEngine::CmdSetPlayer(GtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(1);
    if (cmd.NuArg() == 0)
        cmd << m_playerName;
    else if (cmd.NuArg() == 1)
    {
        std::string name(cmd.Arg(0));
        if (name == "uct" || name == "random")
            m_playerName = name;
        else
            throw GtpFailure("Unknown player name!");
    }
}

void YGtpEngine::CmdTimeLeft(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    SgBlackWhite color = ColorArg(cmd, 0);
    cmd << m_timeLeft[color];
}

void YGtpEngine::CmdTimeSettings(GtpCommand& cmd)
{
    if (!m_history.empty())
        throw GtpFailure("Cannot change time during game!");
    cmd.CheckNuArg(3);
    double newTime = cmd.FloatArg(0);
    if (newTime < 0.0)
        throw GtpFailure("Main time must be >= 0!");
    m_mainTime = newTime;
    m_timeSettingsSpecified = m_mainTime > 0.;
    ApplyTimeSettings();
}

//----------------------------------------------------------------------------

void YGtpEngine::CmdUctProvenNodes(GtpCommand& cmd)
{
    const SgUctTree& tree = m_uctSearch.Tree();
    for (SgUctChildIterator it(tree, tree.Root()); it; ++it)
    {
        const SgUctNode& child = *it;
        SgPoint p = child.Move();
        SgUctProvenType type = child.ProvenType();
        std::string pt = "unknown";
        // Flipped because from child's perspective
        if (type == SG_PROVEN_WIN) 
            pt = "loss";
        else if (type == SG_PROVEN_LOSS)
            pt = "win";
        cmd << ' ' << m_brd.Const().ToString(p) << ' ' << pt;
    }
}

void YGtpEngine::CmdUctScores(GtpCommand& cmd)
{
    const SgUctTree& tree = m_uctSearch.Tree();
    for (SgUctChildIterator it(tree, tree.Root()); it; ++it)
    {
        const SgUctNode& child = *it;
        SgPoint p = child.Move();
        std::size_t count = child.MoveCount();
        float mean = 0.0;
        if (count > 0)
            mean = child.Mean();
        cmd << ' ' << m_brd.Const().ToString(p)
            << ' ' << std::fixed << std::setprecision(3) << mean
            << '@' << count;
    }
}

void YGtpEngine::CmdRaveScores(GtpCommand& cmd)
{
    const SgUctTree& tree = m_uctSearch.Tree();
    for (SgUctChildIterator it(tree, tree.Root()); it; ++it)
    {
        const SgUctNode& child = *it;
        SgPoint p = child.Move();
        if (p == SG_PASS || ! child.HasRaveValue())
            continue;
        cmd << ' ' << m_brd.Const().ToString(p)
            << ' ' << std::fixed << std::setprecision(3) << child.RaveValue()
            << '@' << child.RaveCount();
    }
}

//----------------------------------------------------------------------------
