// $Id$

#ifndef _BATCHTRAFFICMANAGER_HPP_
#define _BATCHTRAFFICMANAGER_HPP_

#include <iostream>
#include <vector>

#include "synthetictrafficmanager.hpp"

class BatchTrafficManager : public SyntheticTrafficManager
{

protected:
  vector<long long int> _max_outstanding;
  vector<long long int> _batch_size;
  long long int _batch_count;
  long long int _last_id;
  long long int _last_pid;

  Stats *_batch_time;
  double _overall_min_batch_time;
  double _overall_avg_batch_time;
  double _overall_max_batch_time;

  ostream *_sent_packets_out;

  virtual void _RetireFlit(Flit *f, long long int dest);

  virtual long long int _IssuePacket(long long int source, long long int cl);
  virtual void _ClearStats();
  virtual bool _SingleSim();

  virtual void _UpdateOverallStats();

  virtual string _OverallStatsHeaderCSV() const;
  virtual string _OverallClassStatsCSV(long long int c) const;
  virtual void _DisplayOverallClassStats(long long int c, ostream &os) const;
  virtual void _DisplayClassStats(long long int c, ostream &os) const;
  virtual void _WriteClassStats(long long int c, ostream &os) const;

public:
  BatchTrafficManager(const Configuration &config, const vector<Network *> &net);
  virtual ~BatchTrafficManager();
};

#endif
