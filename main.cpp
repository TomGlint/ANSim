// $Id$

/*main.cpp
 *
 *The starting point of the network simulator
 *-Include all network header files
 *-initilize the network
 *-initialize the traffic manager and set it to run
 *
 *
 */
#include <sys/time.h>

#include <string>
#include <cstdlib>
#include <iostream>
#include <fstream>

#include <sstream>
#include "booksim.hpp"
#include "routefunc.hpp"
#include "traffic.hpp"
#include "booksim_config.hpp"
#include "trafficmanager.hpp"
#include "random_utils.hpp"
#include "network.hpp"
#include "injection.hpp"
#include "power_module.hpp"

#include "asyncConfig.hpp"

///////////////////////////////////////////////////////////////////////////////
//Global declarations
//////////////////////

AsyncConfig *asyncConfig = NULL;

/* the current traffic manager instance */
TrafficManager *trafficManager = NULL;

long long int GetSimTime()
{
  return trafficManager->getTime();
}

class Stats;
Stats *GetStats(const std::string &name)
{
  Stats *test = trafficManager->getStats(name);
  if (test == 0)
  {
    cout << "warning statistics " << name << " not found" << endl;
  }
  return test;
}

/* printing activity factor*/
bool gPrintActivity;

long long int gK; //radix
long long int gN; //dimension
long long int gC; //concentration

long long int gNodes;

//generate nocviewer trace
bool gTrace;

ostream *gWatchOut;

// Orion Power Support
int g_number_of_injected_flits = 0;
int g_number_of_retired_flits = 0;
int g_total_cs_register_writes = 0;

/////////////////////////////////////////////////////////////////////////////

bool Simulate(BookSimConfig const &config)
{
  vector<Network *> net;

  long long int subnets = config.GetLongInt("subnets");
  /*To include a new network, must register the network here
   *add an else if statement with the name of the network
   */
  net.resize(subnets);
  for (long long int i = 0; i < subnets; ++i)
  {
    ostringstream name;
    name << "network_" << i;
    net[i] = Network::New(config, name.str());
    //    net[i]->DumpChannelMap();		//Sneha
    //    net[i]->DumpNodeMap();		//Sneha
  }

  /*tcc and characterize are legacy not sure how to use them */

  trafficManager = TrafficManager::New(config, net);

  /*Start the simulation run */

  double total_time;                   /* Amount of time we've run */
  struct timeval start_time, end_time; /* Time before/after user code */
  total_time = 0.0;
  gettimeofday(&start_time, NULL);

  bool result = trafficManager->Run();

  gettimeofday(&end_time, NULL);
  total_time = ((double)(end_time.tv_sec) + (double)(end_time.tv_usec) / 1000000.0) - ((double)(start_time.tv_sec) + (double)(start_time.tv_usec) / 1000000.0);

  cout << "\n*****************************************\n";
  cout << "Total run time " << total_time << endl;

  for (long long int i = 0; i < subnets; ++i)
  {

    ///Power analysis
    if (config.GetLongInt("sim_power") > 0)
    {
      Power_Module pnet(net[i], config);
      pnet.run();
    }

    delete net[i];
  }

  delete trafficManager;
  trafficManager = NULL;

  return result;
}

int main(int argc, char **argv)
{

  BookSimConfig config;

  if (!ParseArgs(&config, argc, argv))
  {
    cerr << "Usage: " << argv[0] << " configfile... [param=value...]" << endl;
    return 0;
  }
  asyncConfig = new AsyncConfig(config);
  /*initialize routing, traffic, injection functions  */
  InitializeRoutingMap(config);

  gPrintActivity = (config.GetLongInt("print_activity") > 0);
  gTrace = (config.GetLongInt("viewer_trace") > 0);

  string watch_out_file = config.GetStr("watch_out");
  if (watch_out_file == "")
  {
    gWatchOut = NULL;
  }
  else if (watch_out_file == "-")
  {
    gWatchOut = &cout;
  }
  else
  {
    gWatchOut = new ofstream(watch_out_file.c_str());
  }

  /*configure and run the simulator */
  bool result = Simulate(config);

  if (asyncConfig->doGating)
  {
    long long int totalViableIdleTicksSum = 0;
    long long int totalViableGatedTicksSum = 0;

    long long int totalViableIdleTimesSum = 0;
    long long int totalGatedTimesSum = 0;
    cout << "\n--------------Gating Results---------------------\n";
    //per router viable idle tick sum
    cout << "\nViable idle Ticks Sum, ";

    for (unsigned int i = 0; i < asyncConfig->viableIdleTicksSum.size(); i++)
    {
      cout << asyncConfig->viableIdleTicksSum[i] << ", ";
      totalViableIdleTicksSum = totalViableIdleTicksSum + asyncConfig->viableIdleTicksSum[i];
    }

    //per router viable idle Times sum
    cout << "\nViable idle Times Sum, ";

    for (unsigned int i = 0; i < asyncConfig->viableIdleTimesSum.size(); i++)
    {
      cout << asyncConfig->viableIdleTimesSum[i] << ", ";
      totalViableIdleTimesSum = totalViableIdleTimesSum + asyncConfig->viableIdleTimesSum[i];
    }

    //per router gated Ticks Sum
    cout << "\nViable gated Ticks Sum, ";

    for (unsigned int i = 0; i < asyncConfig->viableGatedTicksSum.size(); i++)
    {
      cout << asyncConfig->viableGatedTicksSum[i] << ", ";
      totalViableGatedTicksSum = totalViableGatedTicksSum + asyncConfig->viableGatedTicksSum[i];
    }

    //per router gated Times Sum
    cout << "\nViable gated Times Sum, ";

    for (unsigned int i = 0; i < asyncConfig->gatedTimesSum.size(); i++)
    {
      cout << asyncConfig->gatedTimesSum[i] << ", ";
      totalGatedTimesSum = totalGatedTimesSum + asyncConfig->gatedTimesSum[i];
    }

    //Overall Result

    cout << "\nOverall viable idle ticks, " << totalViableIdleTicksSum << endl;
    cout << "Overall viable idle times, " << totalViableIdleTimesSum << endl;
    cout << "Overall gated ticks, " << totalViableGatedTicksSum << endl;
    cout << "Overall gated times, " << totalGatedTimesSum << endl;
  }

  delete asyncConfig;
  asyncConfig = NULL;
  return result ? -1 : 0;
}
