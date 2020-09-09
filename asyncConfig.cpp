// $Id$

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

#include "asyncConfig.hpp"
#include "random_utils.hpp"

using namespace std;

void AsyncConfig::init(unsigned long long int numberOfNodes, const Configuration &config)
{

    //for netrace
    netraceInterCycle = 0;
    traceStretch = config.GetLongInt("traceStretch");

    //for gating
    doGating = config.GetLongInt("doGating");
    gatingMode = config.GetLongInt("gatingMode");
    sleepThreshold = config.GetLongInt("sleepThreshold");
    breakEvenThreshold = config.GetLongInt("breakEvenThreshold");
    sleepThresholdStep=breakEvenThreshold/8;

    for (unsigned long long int i = 0; i < numberOfNodes; i++)
    {
        idleTicksCounter.push_back(0);
        viableIdleTicksSum.push_back(0);
        viableIdleTimesSum.push_back(0);
        routerSleepThreshold.push_back(sleepThresholdStep);
        

        gatedTicksCounter.push_back(0);
        viableGatedTicksSum.push_back(0);
        gatedTimesSum.push_back(0);
    }
};

void AsyncConfig::readCreaditDelays(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("creditDelays");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()
    init(params.size(), config);

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        creditDelays.push_back(atoll(params[i].c_str()));
    }
};
void AsyncConfig::readRoutingDelays(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("routingDelays");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        routingDelays.push_back(atoll(params[i].c_str()));
    }
};
void AsyncConfig::readVCAllocDelays(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("vcAllocDelays");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        vcAllocDelays.push_back(atoll(params[i].c_str()));
    }
};
void AsyncConfig::readSwAllocDelays(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("swAllocDelays");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        swAllocDelays.push_back(atoll(params[i].c_str()));
        vector<long long int> tempVec;
        //now only supporting 5 output ports
        tempVec.push_back(0);
        tempVec.push_back(0);
        tempVec.push_back(0);
        tempVec.push_back(0);
        tempVec.push_back(0);
        previousSwitchAllocation.push_back(tempVec);
    }
};
void AsyncConfig::readSTFinalDelays(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("stFinalDelays");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        stFinalDelays.push_back(atoll(params[i].c_str()));
    }
};

void AsyncConfig::readIsAsync(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("isAsync");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        isAsync.push_back(atoll(params[i].c_str()));
    }
};
void AsyncConfig::readIsMetaStable(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("isMetaUnstable");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        isMetaStable.push_back(atoll(params[i].c_str()));
    }
};

void AsyncConfig::readCreditDelayStdDevs(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("creditDelayStdDevs");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        creditDelayStdDevs.push_back(atoll(params[i].c_str()));
    }
    //setting the random stuff
    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        creditDelaySeed.push_back(std::chrono::system_clock::now().time_since_epoch().count());
        creditDelayRandomGenerator.push_back(default_random_engine(creditDelaySeed[i]));
        creditDelayRandomDistribution.push_back(normal_distribution<double>(creditDelays[i], creditDelayStdDevs[i]));
    }
};

void AsyncConfig::readRoutingDelayStdDevs(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("routingDelayStdDevs");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        routingDelayStdDevs.push_back(atoll(params[i].c_str()));
    }
    //setting the random stuff
    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        routingDelaySeed.push_back(std::chrono::system_clock::now().time_since_epoch().count());
        routingDelayRandomGenerator.push_back(default_random_engine(routingDelaySeed[i]));
        routingDelayRandomDistribution.push_back(normal_distribution<double>(routingDelays[i], routingDelayStdDevs[i]));
    }
};

void AsyncConfig::readVCAllocDelayStdDevs(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("vcAllocDelayStdDevs");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        vCAllocDelayStdDevs.push_back(atoll(params[i].c_str()));
    }
    //setting the random stuff
    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        VCAllocDelaySeed.push_back(std::chrono::system_clock::now().time_since_epoch().count());
        VCAllocDelayRandomGenerator.push_back(default_random_engine(VCAllocDelaySeed[i]));
        VCAllocDelayRandomDistribution.push_back(normal_distribution<double>(vcAllocDelays[i], vCAllocDelayStdDevs[i]));
    }
};

void AsyncConfig::readSwAllocDelayStdDevs(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("swAllocDelayStdDevs");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        swAllocDelayStdDevs.push_back(atoll(params[i].c_str()));
    }
    //setting the random stuff
    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        SwAllocDelaySeed.push_back(std::chrono::system_clock::now().time_since_epoch().count());
        SwAllocDelayRandomGenerator.push_back(default_random_engine(SwAllocDelaySeed[i]));
        SwAllocDelayRandomDistribution.push_back(normal_distribution<double>(swAllocDelays[i], swAllocDelayStdDevs[i]));
    }
};

void AsyncConfig::readSTFinalDelayStdDevs(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("stFinalDelayStdDevs");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        sTFinalDelayStdDevs.push_back(atoll(params[i].c_str()));
    }
    //setting the random stuff
    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        sTFinalDelaySeed.push_back(std::chrono::system_clock::now().time_since_epoch().count());
        sTFinalDelayRandomGenerator.push_back(default_random_engine(sTFinalDelaySeed[i]));
        sTFinalDelayRandomDistribution.push_back(normal_distribution<double>(stFinalDelays[i], sTFinalDelayStdDevs[i]));
    }
};

void AsyncConfig::readSwAllocThresholds(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("swAllocThresholds");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        swAllocThresholds.push_back(atoll(params[i].c_str()));
    }
};
void AsyncConfig::readSwAllocThresholdPenalityStdDevs(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("swAllocThresholdStdDevs");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        swAllocThresholdStdDevs.push_back(atoll(params[i].c_str()));
    }

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        seedMetaStable.push_back(std::chrono::system_clock::now().time_since_epoch().count());
        generatorMetaStable.push_back(default_random_engine(seedMetaStable[i]));
        distributionMetaStable.push_back(normal_distribution<double>(0, swAllocThresholdStdDevs[i]));
    }
};

void AsyncConfig::readSwAllocMetaStableThresholds(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("swAllocMetaStableThresholds");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()
    metaStabiliyNormaliser = log(1000);
    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        swAllocMetaStableThresholds.push_back(atoll(params[i].c_str()));
    }
};

void AsyncConfig::readswAllocMetaStableMaxPenalities(const Configuration &config)
{
    vector<string> workloads = config.GetStrArray("swAllocMetaStableMaxPenalities");
    string workload = workloads[0];
    string workload_name;
    string param_str;
    size_t left = workload.find_first_of('(');
    if (left == string::npos)
    {
        cout << "Error: Missing parameter in" << workload
             << endl;
        exit(-1);
    }
    workload_name = workload.substr(0, left);
    size_t right = workload.find_last_of(')');
    if (right == string::npos)
    {
        param_str = workload.substr(left + 1);
    }
    else
    {
        param_str = workload.substr(left + 1, right - left - 1);
    }
    vector<string> params = tokenize_str(param_str);

    //params.size()

    for (unsigned long long int i = 0; i < params.size(); i++)
    {
        swAllocMetaStableMaxPenality.push_back(atoll(params[i].c_str()));
    }
};

AsyncConfig::AsyncConfig(const Configuration &config)
{
    readCreaditDelays(config);
    readRoutingDelays(config);
    readVCAllocDelays(config);
    readSwAllocDelays(config);
    readSTFinalDelays(config);

    readIsAsync(config);
    readIsMetaStable(config);

    readCreditDelayStdDevs(config);
    readRoutingDelayStdDevs(config);
    readVCAllocDelayStdDevs(config);
    readSwAllocDelayStdDevs(config);
    readSTFinalDelayStdDevs(config);

    readSwAllocThresholds(config);
    readSwAllocThresholdPenalityStdDevs(config);
    readSwAllocMetaStableThresholds(config);
    readswAllocMetaStableMaxPenalities(config);

    queueTicks=0;
	routeTicks=0;
	vcaTicks=0;
	swaTicks=0;
	crossbarTicks=0;
}
AsyncConfig::AsyncConfig()
{
}

long long int AsyncConfig::getCreditDelay(long long int routerID)
{

    if (isAsync[routerID])
    {
        long long int temp = creditDelayRandomDistribution[routerID](creditDelayRandomGenerator[routerID]);

        if (temp >= 1)
        {

            return temp;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        // cout<<"The credit delay returned is "<<creditDelays[routerID]<<endl;
        return creditDelays[routerID];
    }
};

long long int AsyncConfig::getRoutingDelay(long long int routerID)
{
    if (isAsync[routerID])
    {
        long long int temp = routingDelayRandomDistribution[routerID](routingDelayRandomGenerator[routerID]);

        if (temp >= 1)
        {

            return temp;
        }
        else
        {
            return 1;
        }
    }
    else
    {

        return routingDelays[routerID];
    }
};
long long int AsyncConfig::getVcAllocDelay(long long int routerID)
{
    if (isAsync[routerID])
    {
        long long int temp = VCAllocDelayRandomDistribution[routerID](VCAllocDelayRandomGenerator[routerID]);

        if (temp >= 1)
        {

            return temp;
        }
        else
        {
            return 1;
        }
    }
    else
    {

        return vcAllocDelays[routerID];
    }
};

long long int AsyncConfig::getSwAllocDelay(long long int routerID, long long int output)
{
    long long int delay = 0;
    // swAllocDelays[routerID];

    if (isAsync[routerID])
    {
        long long int temp = SwAllocDelayRandomDistribution[routerID](SwAllocDelayRandomGenerator[routerID]);

        if (temp >= 1)
        {

            delay = temp;
        }
        else
        {
            delay = 1;
        }
    }
    else
    {

        delay = swAllocDelays[routerID];
    }

    if (isAsync[routerID])
    {
        if ((GetSimTime() - previousSwitchAllocation[routerID][output]) < swAllocThresholds[routerID])
        {
            //cout<<"Hit++++++++++++++++++++"<<endl;
            long long int additionalDelay = distributionMetaStable[routerID](generatorMetaStable[routerID]);
            if (additionalDelay < 0)
            {
                additionalDelay = -additionalDelay;
            }
            delay += additionalDelay;
        }
    }

    if (isMetaStable[routerID])
    {
        if ((GetSimTime() - previousSwitchAllocation[routerID][output]) < swAllocMetaStableThresholds[routerID])
        {
            long long int additionalDelay;
            double x = rand() % 1000 + 1;
            x = log(1000 / x) / metaStabiliyNormaliser;
            additionalDelay = x * swAllocMetaStableMaxPenality[routerID];
            delay += additionalDelay;
        }
    }

    previousSwitchAllocation[routerID][output] = GetSimTime();
    return delay;
};

long long int AsyncConfig::getStFinalDelay(long long int routerID)
{
    if (isAsync[routerID])
    {
        long long int temp = sTFinalDelayRandomDistribution[routerID](sTFinalDelayRandomGenerator[routerID]);

        if (temp >= 1)
        {

            return temp;
        }
        else
        {
            return 1;
        }
    }
    else
    {

        return stFinalDelays[routerID];
    }
};

long long int AsyncConfig::getSwAllocDelay(long long int routerID)
{
    long long int delay = swAllocDelays[routerID];

    return delay;
};
