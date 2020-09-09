// $Id$

#ifndef _WORKLOADTRAFFICMANAGER_HPP_
#define _WORKLOADTRAFFICMANAGER_HPP_

#include <iostream>
#include <vector>
#include <list>

#include "trafficmanager.hpp"
#include "workload.hpp"

class WorkloadTrafficManager : public TrafficManager
{

protected:
  long long int _sample_period;
  long long int _max_samples;
  long long int _warmup_periods;

  vector<Workload *> _workload;

  long long int _overall_runtime;
  long long int source_map_1[64]; //Sneha
  long long int dest_map_1[64];   //Sneha
  long long int source_map_5[64]; //Sneha
  long long int dest_map_5[64];   //Sneha

  virtual void _Inject();
  virtual void _RetirePacket(Flit *head, Flit *tail);
  virtual void _ResetSim();
  virtual bool _SingleSim();

  bool _Completed();

  virtual void _UpdateOverallStats();

  virtual string _OverallStatsHeaderCSV() const;
  virtual string _OverallClassStatsCSV(long long int c) const;

  virtual void _DisplayClassStats(long long int c, ostream &os) const;
  virtual void _DisplayOverallClassStats(long long int c, ostream &os) const;

public:
  WorkloadTrafficManager(const Configuration &config, const vector<Network *> &net);
  virtual ~WorkloadTrafficManager();
};

#endif
