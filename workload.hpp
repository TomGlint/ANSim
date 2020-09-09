// $Id$

#ifndef _WORKLOAD_HPP_
#define _WORKLOAD_HPP_

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <list>
#include <fstream>

#include "injection.hpp"
#include "traffic.hpp"

extern "C"
{
#include "netrace/netrace.h"
}

using namespace std;

class Workload
{
protected:
  long long int const _nodes;
  queue<long long int> _pending_nodes;
  queue<long long int> _deferred_nodes;
  Workload(long long int nodes);

public:
  virtual ~Workload();
  static Workload *New(string const &workload, long long int nodes,
                       Configuration const *const config = NULL);
  virtual void reset();
  virtual void advanceTime();
  virtual bool empty() const;
  virtual bool completed() const = 0;
  virtual long long int source() const;
  virtual long long int dest() const = 0;
  virtual long long int size() const = 0;
  virtual long long int time() const = 0;
  virtual void inject(long long int pid) = 0;
  virtual void defer();
  virtual void retire(long long int pid) = 0;
  virtual void printStats(ostream &os) const;
};

class NullWorkload : public Workload
{
public:
  NullWorkload(long long int nodes) : Workload(nodes) {}
  virtual bool completed() const { return true; }
  virtual long long int dest() const { return -1; }
  virtual long long int size() const { return -1; }
  virtual long long int time() const { return -1; }
  virtual void inject(long long int pid) { assert(false); }
  virtual void retire(long long int pid) { assert(false); }
};

class SyntheticWorkload : public Workload
{
protected:
  unsigned long long int _time;
  vector<long long int> _sizes;
  vector<long long int> _rates;
  long long int _max_val;
  InjectionProcess *_injection;
  TrafficPattern *_traffic;
  vector<unsigned long long int> _qtime;
  queue<long long int> _sleeping_nodes;

public:
  SyntheticWorkload(long long int nodes, double load, string const &traffic,
                    string const &injection,
                    vector<long long int> const &sizes,
                    vector<long long int> const &rates,
                    Configuration const *const config = NULL);
  virtual ~SyntheticWorkload();
  virtual void reset();
  virtual void advanceTime();
  virtual bool completed() const;
  virtual long long int dest() const;
  virtual long long int size() const;
  virtual long long int time() const;
  virtual void inject(long long int pid);
  virtual void retire(long long int pid) {}
};

class TraceWorkload : public Workload
{

protected:
  unsigned long long int _time;

  vector<long long int> _packet_sizes;

  struct PacketInfo
  {
    unsigned long long int time;
    long long int dest;
    long long int type;
  };

  long long int _next_source;
  PacketInfo _next_packet;
  vector<queue<PacketInfo>> _ready_packets;

  ifstream *_trace;

  unsigned long long int _count;
  long long int _limit;

  unsigned long long int _scale;
  unsigned long long int _skip;

  void _refill();

public:
  TraceWorkload(long long int nodes, string const &filename,
                vector<long long int> const &packet_size, long long int limit = -1,
                unsigned long long int skip = 0, unsigned long long int scale = 1);

  virtual ~TraceWorkload();
  virtual void reset();
  virtual void advanceTime();
  virtual bool completed() const;
  virtual long long int dest() const;
  virtual long long int size() const;
  virtual long long int time() const;
  virtual void inject(long long int pid);
  virtual void retire(long long int pid);
  virtual void printStats(ostream &os) const;
};

class NetraceWorkload : public Workload
{

protected:
  unsigned long long int _time;

  nt_context_t *_ctx;

  nt_packet_t *_next_packet;

  vector<queue<nt_packet_t *>> _ready_packets;
  list<nt_packet_t *> _future_packets;
  set<unsigned long long int> _check_packets;
  map<unsigned long long int, nt_packet_t *> _stalled_packets;
  map<long long int, nt_packet_t *> _in_flight_packets;

  map<unsigned long long int, unsigned long long int> _response_eject_time;
  vector<unsigned long long int> _last_response_eject_time;

  unsigned long long int _channel_width;
  unsigned long long int _size_offset;

  unsigned long long int _region;

  unsigned long long int _window_size;

  unsigned long long int _count;
  unsigned long long int _limit;

  unsigned long long int _scale;

  unsigned long long int _skip;

  bool _enforce_deps;
  bool _enforce_lats;

  unsigned long long int _l2_tag_latency;
  unsigned long long int _l2_data_latency;
  unsigned long long int _mem_latency;
  unsigned long long int _trace_net_delay;

  void _refill();

public:
  NetraceWorkload(long long int nodes, string const &filename,
                  unsigned long long int channel_width, long long int limit = -1ll,
                  unsigned long long int scale = 1, long long int region = -1,
                  bool enforce_deps = true, bool enforce_lats = false,
                  unsigned long long int size_offset = 0);

  virtual ~NetraceWorkload();
  virtual void reset();
  virtual void advanceTime();
  virtual bool completed() const;
  virtual long long int dest() const;
  virtual long long int size() const;
  virtual long long int time() const;
  virtual void inject(long long int pid);
  virtual void retire(long long int pid);
  virtual void printStats(ostream &os) const;
};

#endif
