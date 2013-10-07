//----------------------------------------------------------------------------
/** @file YGtpEngine.cpp */
//----------------------------------------------------------------------------

#include <fstream>

#include "SgSystem.h"
#include "SgRandom.h"
#include "SgSearchControl.h"
#include "SgSearch.h"
#include "SgTimer.h"
#include "SgGameReader.h"
#include "SgGameWriter.h"

#include "YGtpEngine.h"
#include "YSgUtil.h"

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
      m_ignoreClock(false),
      m_allowSwap(true)
{
    RegisterCmd("exec", &YGtpEngine::CmdExec);
    RegisterCmd("name", &YGtpEngine::CmdName);
    RegisterCmd("play", &YGtpEngine::CmdPlay);
    RegisterCmd("boardsize", &YGtpEngine::CmdBoardSize);
    RegisterCmd("showboard", &YGtpEngine::CmdShowBoard);
    RegisterCmd("clear_board", &YGtpEngine::CmdClearBoard);
    RegisterCmd("genmove", &YGtpEngine::CmdGenMove);
    RegisterCmd("loadsgf", &YGtpEngine::CmdLoadSgf);
#if GTPENGINE_INTERRUPT
    RegisterCmd("gogui-interrupt", &YGtpEngine::CmdInterrupt);
#endif
    RegisterCmd("hexgui-analyze_commands", 
                &YGtpEngine::CmdAnalyzeCommands);
    RegisterCmd("set_player", &YGtpEngine::CmdSetPlayer);
    RegisterCmd("time_left", &YGtpEngine::CmdTimeLeft);
    RegisterCmd("time_settings", &YGtpEngine::CmdTimeSettings);
    RegisterCmd("undo", &YGtpEngine::CmdUndo);
    RegisterCmd("final_score", &YGtpEngine::CmdFinalScore);
    RegisterCmd("y_solve", &YGtpEngine::CmdSolve);
    RegisterCmd("y_param", &YGtpEngine::CmdParam);
    RegisterCmd("version", &YGtpEngine::CmdVersion);

    RegisterCmd("uct_proven_nodes", &YGtpEngine::CmdUctProvenNodes);
    RegisterCmd("uct_scores", &YGtpEngine::CmdUctScores);
    RegisterCmd("uct_rave_scores", &YGtpEngine::CmdRaveScores);
    RegisterCmd("playout_move", &YGtpEngine::CmdPlayoutMove);
    RegisterCmd("playout_weights", &YGtpEngine::CmdPlayoutWeights);
    RegisterCmd("playout_statistics", &YGtpEngine::CmdPlayoutStatistics);

    RegisterCmd("board_statistics", &YGtpEngine::CmdBoardStatistics);
    
    RegisterCmd("cell_info", &YGtpEngine::CmdCellInfo);
    RegisterCmd("full_connected_with", 
                &YGtpEngine::CmdFullConnectedWith);
    RegisterCmd("semi_connected_with",
		&YGtpEngine::CmdSemiConnectedWith);
    RegisterCmd("carrier_between", &YGtpEngine::CmdCarrierBetween);
    RegisterCmd("semis_between", &YGtpEngine::CmdSemisBetween);
    RegisterCmd("print_semitable", &YGtpEngine::CmdPrintSemiTable);
    RegisterCmd("full_connects_multiple", 
		&YGtpEngine::CmdFullConnectsMultipleBlocks);

    RegisterCmd("dirty_con_cells", &YGtpEngine::CmdDirtyConCells);
    RegisterCmd("dirty_weight_cells", &YGtpEngine::CmdDirtyWeightCells);

    RegisterCmd("block_info", &YGtpEngine::CmdBlockInfo);
    RegisterCmd("block_stones", &YGtpEngine::CmdBlockStones);
    RegisterCmd("block_liberties", &YGtpEngine::CmdBlockLiberties);

    RegisterCmd("group_info", &YGtpEngine::CmdGroupInfo);
    RegisterCmd("group_blocks", &YGtpEngine::CmdGroupBlocks);
    RegisterCmd("group_value", &YGtpEngine::CmdGroupValue);
    RegisterCmd("group_carrier", &YGtpEngine::CmdGroupCarrier);

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
        "param/Search Parameters/y_param\n"
        "plist/All Legal Moves/all_legal_moves %c\n"
        "string/Final Score/final_score\n"
        "varc/Reg GenMove/reg_genmove %c\n"
        "string/ShowBoard/showboard\n"
        "string/Board Statistics/board_statistics\n"
	"string/Cell Info/cell_info %p\n"
        "plist/Full Connected With/full_connected_with %p %c\n"
	"plist/Semi Connected With/semi_connected_with %p %c\n"
	"plist/Carrier Between/carrier_between %P\n"
	"vc/Semis Between/semis_between %P\n"
        "string/Print Semitable/print_semitable\n"
	"plist/Full Connects Multiple Blocks/full_connects_multiple %c\n"
        "plist/Dirty Con Cells/dirty_con_cells\n"
        "plist/Dirty Weight Cells/dirty_weight_cells\n"
        "string/Block Info/block_info %p\n"
        "group/Block Stones/block_stones %p\n"
        "plist/Block Liberties/block_liberties %p\n"
	"string/Group Info/group_info %p\n"
	"string/Group Blocks/group_blocks %p\n"
        "plist/Group Carrier/group_carrier %p\n"
        "string/Playout Statistics/playout_statistics\n"
        "pspairs/Playout Weights/playout_weights\n"
        "move/Playout Move/playout_move\n";
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

SgMove YGtpEngine::CellArg(const GtpCommand& cmd, 
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

void YGtpEngine::NewGame(int size)
{
    m_brd.SetSize(size);
    ApplyTimeSettings();
}

void YGtpEngine::NewGame()
{
    m_brd.SetSize(m_brd.Size());
    ApplyTimeSettings();
}

void YGtpEngine::Play(int color, int move)
{
    if (move == Y_SWAP) {
        if (!m_allowSwap)
            throw GtpFailure("Swap setting is disabled!");
        if (m_brd.NumMoves() != 1)
            throw GtpFailure("Cannot swap in this position!");
    	m_brd.Swap();
    }
    else if (move != SG_RESIGN)
    {
        if (! m_brd.Const().IsOnBoard(move))
            throw GtpFailure("Cell not on board!");
        if (m_brd.IsOccupied(move))
            throw GtpFailure("Cell is occupied!");
        m_brd.Play(color, move);
    }
}

void YGtpEngine::Undo()
{
    SG_ASSERT(m_brd.NumMoves() > 0);
    m_brd.Undo();
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
        if (m_brd.HasWinningVC()) {
            std::cerr << "VC Win Detected! Playing into carrier...\n";
            MarkedCells carrier = m_brd.GroupCarrier(m_brd.WinningVCGroup());
            std::vector<cell_t> car;
            Include(car, carrier);
            return car[SgRandom::Global().Int(car.size())];
        }

        m_brd.SetToPlay(toPlay);        
        m_uctSearch.SetPosition(m_brd);
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
            std::cerr << ' ' << m_brd.ToString(sequence[i]);
        std::cerr << '\n';
	
        if (m_allowSwap && m_brd.NumMoves()==1 && score < 0.5)
            return Y_SWAP;
        return sequence[0];
    }
    else if (m_playerName == "random")
    {
#if 0
        std::vector<cell_t> empty;
        for (Board::EmptyIterator it(m_brd); it; ++it)
            empty.push_back(*it);
        return empty[SgRandom::Global().Int(empty.size())];
#endif
        return 1; // FIXME: IMPLEMENT
    }
    throw GtpFailure("Invalid player!");
    return SG_RESIGN;
}

#if GTPENGINE_INTERRUPT

void YGtpEngine::Interrupt()
{
    SgSetUserAbort(true);
}

#endif // GTPENGINE_INTERRUPT


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

#if GTPENGINE_INTERRUPT

/** Does nothing, but lets gogui know we can be interrupted with the 
    "# interrupt" gtp command. */
void YGtpEngine::CmdInterrupt(GtpCommand& cmd)
{
    cmd.CheckArgNone();
}

#endif

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
    m_brd.SetSize(size);
    NewGame();
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
        Play(color, SG_RESIGN);
    // else if (color != m_brd.ToPlay())
    //     throw GtpFailure("It is the other player's turn!");
    else
    {
        SgMove move = CellArg(cmd, 1);
        Play(color, move);
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

void YGtpEngine::CmdFinalScore(GtpCommand& cmd)
{
    SgBoardColor winner = m_brd.GetWinner();
    if (winner == SG_BLACK)
        cmd << "B+";
    else if (winner == SG_WHITE)
        cmd << "W+";
    else
        cmd << "cannot score";
}

void YGtpEngine::CmdGenMove(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int color = ColorArg(cmd, 0);
    // if (color != m_brd.ToPlay())
    //     throw GtpFailure("It is the other player's turn!");
    if (m_brd.IsGameOver()) {
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
        if (move == Y_SWAP)
            cmd << "swap";
        else
            cmd << m_brd.ToString(move);
    }
}

std::string PrintPV(const SgVector<SgMove>& pv, const Board& brd)
{
    std::ostringstream os;
    for (int i = 0; i < pv.Length(); ++i)
    {
        if (i) os << ' ';
        os << brd.ToString(pv[i]);
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
        cmd << "draw " << (pv.IsEmpty() ? "none" : m_brd.ToString(pv[0]));
    }
    else if (value == 10)
    {
        cmd << (toPlay == SG_BLACK ? "black" : "white") << ' ' 
            << (pv.IsEmpty() ? "none" : m_brd.ToString(pv[0]));
        std::cerr << "PV: " << PrintPV(pv, m_brd) << '\n';
    }
    else if (value == -10)
        cmd << (toPlay == SG_BLACK ? "white" : "black") << ' '
            << "none";
    cmd << ' ' << depth << ' ' << nodes;
}

void YGtpEngine::CmdParam(GtpCommand& cmd)
{
    if (cmd.NuArg() == 0)
    {
        cmd << '\n'
            << "[bool] allow_swap " << m_allowSwap << '\n'
            << "[bool] ignore_clock " << m_ignoreClock << '\n'
            << "[bool] use_livegfx " << m_uctSearch.LiveGfx() << '\n'
            << "[bool] use_rave " << m_uctSearch.Rave() << '\n'
            << "[bool] use_savebridge " << m_uctSearch.UseSaveBridge() << '\n'
            << "[string] bias_term_constant " 
            << m_uctSearch.BiasTermConstant() << '\n'
            << "[string] expand_threshold " 
            << m_uctSearch.ExpandThreshold() << '\n'
            << "[string] number_playouts " 
            << m_uctSearch.NumberPlayouts() << '\n'
            << "[string] num_threads " << m_uctSearch.NumberThreads() << '\n'
            << "[string] max_games " << m_uctMaxGames << '\n'
            << "[string] max_memory "
            << m_uctSearch.MaxNodes() * 2 * sizeof(SgUctNode) << '\n'
            << "[string] max_nodes "
            << m_uctSearch.MaxNodes() << '\n'
            << "[string] max_time " << m_uctMaxTime << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "use_rave")
            m_uctSearch.SetRave(cmd.BoolArg(1));
        else if (name == "use_livegfx")
            m_uctSearch.SetLiveGfx(cmd.BoolArg(1));
        else if (name == "use_savebridge")
            m_uctSearch.SetUseSaveBridge(cmd.BoolArg(1));
        else if (name == "allow_swap")
            m_allowSwap = cmd.BoolArg(1);
        else if (name == "ignore_clock")
            m_ignoreClock = cmd.BoolArg(1);
        else if (name == "bias_term_constant")
            m_uctSearch.SetBiasTermConstant(cmd.FloatArg(1));
        else if (name == "expand_threshold")
            m_uctSearch.SetExpandThreshold(cmd.IntArg(1, 1));
        else if (name == "number_playouts")
            m_uctSearch.SetNumberPlayouts(cmd.IntArg(1,1));
        else if (name == "num_threads")
            m_uctSearch.SetNumberThreads(cmd.IntArg(1, 1));
        else if (name == "max_games")
            m_uctMaxGames = cmd.SizeTypeArg(1, 1);
        else if (name == "max_memory")
            m_uctSearch.SetMaxNodes(cmd.ArgMin<std::size_t>(1, 1) 
                                 / sizeof(SgUctNode) / 2);
        else if (name == "max_nodes")
            m_uctSearch.SetMaxNodes(cmd.ArgMin<std::size_t>(1, 1));
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
    if (m_brd.NumMoves() > 0)
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
        cmd << ' ' << m_brd.ToString(p) << ' ' << pt;
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
        cmd << ' ' << m_brd.ToString(p)
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
        cmd << ' ' << m_brd.ToString(p)
            << ' ' << std::fixed << std::setprecision(3) << child.RaveValue()
            << '@' << child.RaveCount();
    }
}

void YGtpEngine::CmdPlayoutStatistics(GtpCommand& cmd)
{
    cmd << YUctSearch::PlayoutStatistics::Get().ToString();
}

//----------------------------------------------------------------------------

void YGtpEngine::CmdBoardStatistics(GtpCommand& cmd)
{
    cmd << Board::Statistics::Get().ToString();
}

//----------------------------------------------------------------------------

void YGtpEngine::CmdCellInfo(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int p = CellArg(cmd, 0);
    if (m_brd.IsOccupied(p))
        throw GtpFailure("Invalid cell");
    cmd << m_brd.CellInfo(p);
}

void YGtpEngine::CmdFullConnectedWith(GtpCommand& cmd)
{
    cmd.CheckNuArg(2);
    int p = CellArg(cmd, 0);
    SgBlackWhite color = ColorArg(cmd, 1);
    std::vector<cell_t> blocks = m_brd.FullConnectedTo(p, color);
    std::stable_sort(blocks.begin(), blocks.end());
    for (size_t i = 0; i < blocks.size(); ++i) {
        cmd << ' ' << m_brd.ToString(blocks[i]);
    }
}

void YGtpEngine::CmdSemiConnectedWith(GtpCommand& cmd)
{
    cmd.CheckNuArg(2);
    int p = CellArg(cmd, 0);
    SgBlackWhite color = ColorArg(cmd, 1);
    std::vector<cell_t> blocks = m_brd.SemiConnectedTo(p, color);
    std::stable_sort(blocks.begin(), blocks.end());
    for (size_t i = 0; i < blocks.size(); ++i) {
        cmd << ' ' << m_brd.ToString(blocks[i]);
    }
}

void YGtpEngine::CmdCarrierBetween(GtpCommand& cmd)
{
    cmd.CheckNuArg(2);
    int p1 = CellArg(cmd, 0);
    int p2 = CellArg(cmd, 1);
    std::vector<cell_t> cells = m_brd.GetCarrierBetween(p1, p2);
    std::stable_sort(cells.begin(), cells.end());
    for (size_t i = 0; i < cells.size(); ++i) {
        cmd << ' ' << m_brd.ToString(cells[i]);
    }
}

void YGtpEngine::CmdSemisBetween(GtpCommand& cmd)
{
    cmd.CheckNuArg(2);
    int p1 = CellArg(cmd, 0);
    int p2 = CellArg(cmd, 1);
    // handle both p1 and p2 being an edge
    SgBlackWhite color = SG_BLACK;
    if (m_brd.GetColor(p1) == SG_WHITE || m_brd.GetColor(p2) == SG_WHITE)
        color = SG_WHITE;
    std::vector<SemiConnection> semis = m_brd.GetSemisBetween(p1, p2);
    for (size_t i = 0; i < semis.size(); ++i) {
        cmd << ConstBoard::ColorToString(color) << ' ' 
            << semis[i].ToString() << '\n';
    }
}

void YGtpEngine::CmdPrintSemiTable(GtpCommand& cmd)
{
    cmd << m_brd.GetSemis().ToString();
}

void YGtpEngine::CmdFullConnectsMultipleBlocks(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    SgBlackWhite color = ColorArg(cmd, 0);
    std::vector<cell_t> blocks = m_brd.FullConnectsMultipleBlocks(color);
    std::stable_sort(blocks.begin(), blocks.end());
    for (size_t i = 0; i < blocks.size(); ++i) {
        cmd << ' ' << m_brd.ToString(blocks[i]);
    }
}

void YGtpEngine::CmdDirtyWeightCells(GtpCommand& cmd)
{
    const MarkedCellsWithList& dirty = m_brd.GetAllDirtyWeightCells();
    for (MarkedCellsWithList::Iterator it(dirty); it; ++it) {
        cmd << ' ' << m_brd.ToString(*it);
    }
}

void YGtpEngine::CmdDirtyConCells(GtpCommand& cmd)
{
    const MarkedCellsWithList& dirty = m_brd.GetAllDirtyConCells();
    for (MarkedCellsWithList::Iterator it(dirty); it; ++it) {
        cmd << ' ' << m_brd.ToString(*it);
    }
}

//----------------------------------------------------------------------------

void YGtpEngine::CmdBlockInfo(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int p = CellArg(cmd, 0);
    if (!m_brd.IsOccupied(p))
        throw GtpFailure("Invalid block");
    cmd << m_brd.BlockInfo(p);
}

void YGtpEngine::CmdBlockStones(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int p = CellArg(cmd, 0);
    if (m_brd.GetColor(p) == SG_EMPTY)
        return;
    int anchor = m_brd.BlockAnchor(p);
    SgBlackWhite color = m_brd.GetColor(anchor);
    cmd << m_brd.ToString(anchor);
    for (CellIterator it(m_brd.Const()); it; ++it) {
        if (*it != anchor
            && m_brd.GetColor(*it) == color 
            && m_brd.IsInBlock(*it, anchor))
            cmd << ' ' << m_brd.ToString(*it);
    }
}

void YGtpEngine::CmdBlockLiberties(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int p = CellArg(cmd, 0);
    if (m_brd.GetColor(p) == SG_EMPTY)
        return;
    int anchor = m_brd.BlockAnchor(p);
    for (CellIterator it(m_brd.Const()); it; ++it) {
        if (m_brd.GetColor(*it) == SG_EMPTY 
            && m_brd.IsLibertyOfBlock(*it, anchor))
            cmd << ' ' << m_brd.ToString(*it);
    }
}

void YGtpEngine::CmdGroupInfo(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int p = CellArg(cmd, 0);
    if (!m_brd.IsOccupied(p))
        throw GtpFailure("Invalid group");
    cmd << m_brd.GroupInfo(p);
}

void YGtpEngine::CmdGroupBlocks(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int p = CellArg(cmd, 0);
    if (m_brd.GetColor(p) == SG_EMPTY)
	return;
    cmd << m_brd.GroupStructure(p);
}

void YGtpEngine::CmdGroupValue(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int p = CellArg(cmd, 0);
    if (m_brd.GetColor(p) == SG_EMPTY)
	return;
    cmd << ' ' << m_brd.GroupBorder(p);
}

void YGtpEngine::CmdGroupCarrier(GtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int p = CellArg(cmd, 0);
    if (m_brd.GetColor(p) == SG_EMPTY)
	return;
    MarkedCells carrier = m_brd.GroupCarrier(p);
    for(MarkedCells::Iterator i(carrier); i; ++i)
	cmd << ' ' << m_brd.ToString(*i);
}

//----------------------------------------------------------------------

void YGtpEngine::CmdPlayoutMove(GtpCommand& cmd)
{
    if (m_brd.IsGameOver())
        throw GtpFailure("Game over!");
    if (m_brd.NumMoves() == 0)
        throw GtpFailure("Must play at least one move first!");
    if (!m_uctSearch.ThreadsCreated())
        m_uctSearch.CreateThreads();
    YUctThreadState* thread 
        = dynamic_cast<YUctThreadState*>(&m_uctSearch.ThreadState(0));
    if (!thread)
        throw GtpFailure() << "Thread not a YUctThreadState!";
    thread->StartPlayout(m_brd);
    bool skipRaveUpdate;
    int move = thread->GeneratePlayoutMove(skipRaveUpdate);
    Play(m_brd.ToPlay(), move);
    cmd << m_brd.ToString(move);
}

void YGtpEngine::CmdPlayoutWeights(GtpCommand& cmd)
{
    if (m_brd.IsGameOver())
        throw GtpFailure("Game over!");
    if (m_brd.NumMoves() == 0)
        throw GtpFailure("Must play at least one move first!");
    if (!m_uctSearch.ThreadsCreated())
        m_uctSearch.CreateThreads();
    YUctThreadState* thread 
        = dynamic_cast<YUctThreadState*>(&m_uctSearch.ThreadState(0));
    if (!thread)
        throw GtpFailure() << "Thread not a YUctThreadState!";
    thread->StartPlayout(m_brd);
    bool skipRaveUpdate;
    thread->GeneratePlayoutMove(skipRaveUpdate);

    std::vector<float> weights;
    thread->GetWeightsForLastMove(weights, m_brd.ToPlay());
    for (CellIterator i(m_brd.Const()); i; ++i) {
        if (weights[*i] > 0.0f)
            cmd << ' ' << m_brd.ToString(*i)
                << ' ' << weights[*i];
        //<< ' ' << std::fixed << std::setprecision(3) << weights[*i];
    }
}

//----------------------------------------------------------------------

/** Plays setup stones to the board.                                            
    Plays all black moves first then all white moves as actuall game            
    moves.                                                                      
    @bug This will not work if the setup stones intersect previously            
    played stones! The current only works if we expect only a single            
    node with setup information. If multiple nodes in the game tree             
    are adding/removing stones this will break horribly. */
void YGtpEngine::SetPosition(const SgNode* node)
{
    std::vector<cell_t> black, white, empty;
    YSgUtil::GetSetupPosition(node, m_brd.Size(), black, white, empty);
    for (std::size_t i = 0; ; ++i)
    {
        bool bdone = (i >= black.size());
        bool wdone = (i >= white.size());
        if (!bdone)
            Play(SG_BLACK, black[i]);
        if (!wdone)
            Play(SG_WHITE, white[i]);
        if (bdone && wdone)
            break;
    }
}

/** Loads game or position from given sgf.                                      
    Sets position to given move number or the last move of the game if          
    none is given. */
void YGtpEngine::CmdLoadSgf(GtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(2);
    std::string filename = cmd.Arg(0);
    int moveNumber = std::numeric_limits<int>::max();
    if (cmd.NuArg() == 2)
	moveNumber = cmd.ArgMin<int>(1, 0);
    std::ifstream file(filename.c_str());
    if (!file)
        throw GtpFailure() << "cannot load file";
    SgGameReader sgreader(file, 13);
    SgNode* root = sgreader.ReadGame();
    if (root == 0)
	throw GtpFailure() << "cannot load file";
    sgreader.PrintWarnings(std::cerr);

    int size = root->GetIntProp(SG_PROP_SIZE);
    NewGame(size);
    if (YSgUtil::NodeHasSetupInfo(root))
    {
        std::cerr << "Root has setup info!\n";
        SetPosition(root);
    }
    // Play moveNumber moves; stop if we hit the end of the game                
    SgNode* cur = root;
    for (int mn = 0; mn < moveNumber;)
    {
        cur = cur->NodeInDirection(SgNode::NEXT);
        if (!cur)
            break;
        if (YSgUtil::NodeHasSetupInfo(cur))
        {
            SetPosition(cur);
            continue;
        }
        else if (!cur->HasNodeMove())
            continue;
        SgBlackWhite color = cur->NodePlayer();
        int point = YSgUtil::SgPointToYPoint(cur->NodeMove(), m_brd.Const());
        Play(color, point);
        ++mn;
    }
}
