//---------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include "config.h"
#include "SgSystem.h"
#include "SgRandom.h"
#include "SgInit.h"
#include "YSystem.h"
#include "YGtpEngine.h"
#include "YSgUtil.h"
#include "Board.h"

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/cmdline.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

//---------------------------------------------------------------------------

namespace 
{

const char* g_build_date = __DATE__;

boost::program_options::options_description g_options_desc("Options");

std::string g_config_file;

int g_seed = 0;

int g_tracing_level = 0;

int g_boardSize = 13;

void Usage()
{
    std::cout << '\n'
              << "Usage: " 
              << '\n' 
              << "       y [Options]" 
              << '\n' 
              << '\n'
              << "[OPTIONS] is any number of the following:"
              << '\n' 
              << '\n'
              << g_options_desc << '\n';
}
    
void AddCommandLineArguments()
{
    namespace po = boost::program_options;
    g_options_desc.add_options()
        ("help", "Displays this usage information.")
        ("usage", "Displays this usage information.")
        ("version", "Displays version information.")
        ("seed", 
         po::value<int>(&g_seed)->default_value(0), 
         "RNG seed (0=use system time).")
        ("verbose", "Turn tracing on")
        ("boardsize", 
         po::value<int>(&g_boardSize)->default_value(13),
         "Boardsize to use at startup.")
        ("config", 
         po::value<std::string>(&g_config_file)->default_value(""),
         "Sets the config file to parse.");
}

void ProcessCommandLineArguments(int argc, char** argv)
{
    namespace po = boost::program_options;
    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, g_options_desc), vm);
        po::notify(vm);
    }
    catch(...)
    {
        Usage();
        exit(1);
    }
    if (vm.count("usage") || vm.count("help"))
    {
        Usage();
        exit(1);
    }
    if (vm.count("version"))
    {
        std::cout << "Y " << VERSION " " << g_build_date << ".\n";
        exit(0);
    }
    if (vm.count("verbose"))
        g_tracing_level = 1;
}

}

int main(int argc, char** argv)
{
    AddCommandLineArguments();
    ProcessCommandLineArguments(argc, argv);

    SgInit();
    YSystem::Init(g_tracing_level);
    YSgUtil::Init();
    SemiTable::Init();
    SgRandom::SetSeed(g_seed);

    YGtpEngine engine(g_boardSize);
    if (g_config_file != "")
        engine.ExecuteFile(g_config_file);
    GtpInputStream gin(std::cin);
    GtpOutputStream gout(std::cout);
    engine.MainLoop(gin, gout);

    SgFini();
    //HavannahFini();

    return 0;
}

//---------------------------------------------------------------------------
