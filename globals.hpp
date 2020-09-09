// $Id$

#ifndef _GLOBALS_HPP_
#define _GLOBALS_HPP_
#include <string>
#include <vector>
#include <iostream>

/*all declared in main.cpp*/

long long int GetSimTime();

class Stats;
Stats *GetStats(const std::string &name);

extern bool gPrintActivity;

extern long long int gK;
extern long long int gN;
extern long long int gC;

extern long long int gNodes;

extern bool gTrace;

extern std::ostream *gWatchOut;

// Orion Power Support
extern int g_number_of_cache;
extern int g_number_of_injected_flits;
extern int g_number_of_retired_flits;
extern int g_total_cs_register_writes;

#endif
