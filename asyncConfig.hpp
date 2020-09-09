// $Id$

#ifndef _ASYNCCONFIG_HPP_
#define _ASYNCCONFIG_HPP_

#include <iostream>
#include <vector>
#include <list>
#include <sstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <list>
#include <fstream>
#include <chrono>
#include <random>

#include "config_utils.hpp"
#include "booksim_config.hpp"
#include "globals.hpp"

using namespace std;

class AsyncConfig
{

private:
	vector<unsigned> creditDelaySeed;
	vector<std::default_random_engine> creditDelayRandomGenerator;
	vector<std::normal_distribution<double>> creditDelayRandomDistribution;

	vector<unsigned> routingDelaySeed;
	vector<std::default_random_engine> routingDelayRandomGenerator;
	vector<std::normal_distribution<double>> routingDelayRandomDistribution;

	vector<unsigned> VCAllocDelaySeed;
	vector<std::default_random_engine> VCAllocDelayRandomGenerator;
	vector<std::normal_distribution<double>> VCAllocDelayRandomDistribution;

	vector<unsigned> SwAllocDelaySeed;
	vector<std::default_random_engine> SwAllocDelayRandomGenerator;
	vector<std::normal_distribution<double>> SwAllocDelayRandomDistribution;

	vector<unsigned> sTFinalDelaySeed;
	vector<std::default_random_engine> sTFinalDelayRandomGenerator;
	vector<std::normal_distribution<double>> sTFinalDelayRandomDistribution;

	vector<unsigned> seedMetaStable;
	vector<std::default_random_engine> generatorMetaStable;
	vector<std::normal_distribution<double>> distributionMetaStable;

	vector<vector<long long int>> previousSwitchAllocation;

	double metaStabiliyNormaliser;

	vector<long long int> creditDelays;
	vector<long long int> routingDelays;
	vector<long long int> vcAllocDelays;
	vector<long long int> swAllocDelays;
	vector<long long int> stFinalDelays;

	vector<long long int> isAsync;
	vector<long long int> isMetaStable;

	vector<long long int> creditDelayStdDevs;
	vector<long long int> routingDelayStdDevs;
	vector<long long int> vCAllocDelayStdDevs;
	vector<long long int> swAllocDelayStdDevs;
	vector<long long int> sTFinalDelayStdDevs;

	vector<long long int> swAllocThresholds;
	vector<long long int> swAllocThresholdStdDevs;

	vector<long long int> swAllocMetaStableThresholds;
	vector<long long int> swAllocMetaStableMaxPenality;

public:
	//for router gating
	long long int doGating;
	long long int gatingMode;
	long long int sleepThreshold;
	long long int breakEvenThreshold;
	long long int sleepThresholdStep;

	vector<long long int> routerSleepThreshold;

	vector<long long int> idleTicksCounter;
	vector<long long int> viableIdleTicksSum;
	vector<long long int> viableIdleTimesSum;

	vector<long long int> gatedTicksCounter;
	vector<long long int> viableGatedTicksSum;
	vector<long long int> gatedTimesSum;

	//for netrace
	long long int traceStretch;
	long long int netraceInterCycle;

	long long int queueTicks;
	long long int routeTicks;
	long long int vcaTicks;
	long long int swaTicks;
	long long int crossbarTicks;

private:
	void init(unsigned long long int numberOfNodes, const Configuration &config);
	void readCreaditDelays(const Configuration &config);
	void readRoutingDelays(const Configuration &config);
	void readVCAllocDelays(const Configuration &config);
	void readSwAllocDelays(const Configuration &config);
	void readSTFinalDelays(const Configuration &config);

	void readIsAsync(const Configuration &config);
	void readIsMetaStable(const Configuration &config);

	void readCreditDelayStdDevs(const Configuration &config);
	void readRoutingDelayStdDevs(const Configuration &config);
	void readVCAllocDelayStdDevs(const Configuration &config);
	void readSwAllocDelayStdDevs(const Configuration &config);
	void readSTFinalDelayStdDevs(const Configuration &config);

	void readSwAllocThresholds(const Configuration &config);
	void readSwAllocThresholdPenalityStdDevs(const Configuration &config);

	void readSwAllocMetaStableThresholds(const Configuration &config);
	void readswAllocMetaStableMaxPenalities(const Configuration &config);

public:
	AsyncConfig(const Configuration &config);
	AsyncConfig();

	long long int getCreditDelay(long long int routerID);
	long long int getRoutingDelay(long long int routerID);
	long long int getVcAllocDelay(long long int routerID);
	long long int getSwAllocDelay(long long int routerID, long long int output);
	long long int getStFinalDelay(long long int routerID);
	long long int getSwAllocDelay(long long int routerID);
};

extern AsyncConfig *asyncConfig;

#endif
