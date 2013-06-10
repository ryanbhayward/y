//---------------------------------------------------------------------------

#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

#include "config.h"
#include "SgSystem.h"
#include "SgRandom.h"
#include "SgInit.h"
#include "YGtpEngine.h"
//#include "YInit.h"

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

bool g_useGridCoords = false;

int g_boardSize = 8;

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
        ("boardsize", 
         po::value<int>(&g_boardSize)->default_value(8),
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
        std::cout << "Havannah " << VERSION " " << g_build_date << ".\n";
        exit(0);
    }
    if (vm.count("coords"))
        g_useGridCoords = true;
}

}

int main(int argc, char** argv)
{
    AddCommandLineArguments();
    ProcessCommandLineArguments(argc, argv);

    SgInit();
    //HavannahInit();
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
