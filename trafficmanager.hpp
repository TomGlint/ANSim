// $Id$

#ifndef _TRAFFICMANAGER_HPP_
#define _TRAFFICMANAGER_HPP_

#include <list>
#include <map>
#include <set>

#include "module.hpp"
#include "config_utils.hpp"
#include "network.hpp"
#include "flit.hpp"
#include "buffer_state.hpp"
#include "stats.hpp"
#include "routefunc.hpp"
#include "outputset.hpp"

class TrafficManager : public Module
{

protected:
  long long int _nodes;
  long long int _routers;
  long long int _vcs;

  long long int timeStretch;

  long long int ticksTo1ns;

  vector<Network *> _net;
  vector<vector<Router *>> _router;

  // ============ Traffic ============

  long long int _classes;

  long long int _subnets;
  vector<long long int> _subnet;

  vector<long long int> _class_priority;

  vector<vector<long long int>> _last_class;

  // ============ Message priorities ============

  enum ePriority
  {
    class_based,
    age_based,
    network_age_based,
    local_age_based,
    queue_length_based,
    hop_count_based,
    sequence_based,
    none
  };

  ePriority _pri_type;

  // ============ Injection VC states  ============

  vector<vector<BufferState *>> _buf_states;
#ifdef TRACK_FLOWS
  vector<vector<vector<long long int>>> _outstanding_credits;
  vector<vector<vector<queue<long long int>>>> _outstanding_classes;
#endif
  vector<vector<vector<long long int>>> _last_vc;

  // ============ Routing ============

  tRoutingFunction _rf;
  bool _lookahead_routing;
  bool _noq;

  // ============ Injection queues ============

  vector<vector<list<Flit *>>> _partial_packets;

  vector<map<long long int, Flit *>> _total_in_flight_flits;
  vector<map<long long int, Flit *>> _measured_in_flight_flits;
  vector<map<long long int, Flit *>> _retired_packets;

  bool _empty_network;

  bool _hold_switch_for_packet;

  // ============ deadlock ==========

  long long int _deadlock_timer;
  long long int _deadlock_warn_timeout;

  // ============ request & replies ==========================

  vector<vector<long long int>> _packet_seq_no;
  vector<vector<long long int>> _requests_outstanding;

  // ============ Statistics ============

  vector<Stats *> _plat_stats;
  vector<double> _overall_min_plat;
  vector<double> _overall_avg_plat;
  vector<double> _overall_max_plat;

  vector<Stats *> _nlat_stats;
  vector<double> _overall_min_nlat;
  vector<double> _overall_avg_nlat;
  vector<double> _overall_max_nlat;

  vector<Stats *> _flat_stats;
  vector<double> _overall_min_flat;
  vector<double> _overall_avg_flat;
  vector<double> _overall_max_flat;

  vector<Stats *> _frag_stats;
  vector<double> _overall_min_frag;
  vector<double> _overall_avg_frag;
  vector<double> _overall_max_frag;

  vector<vector<Stats *>> _pair_plat;
  vector<vector<Stats *>> _pair_nlat;
  vector<vector<Stats *>> _pair_flat;

  vector<Stats *> _hop_stats;
  vector<double> _overall_hop_stats;

  vector<vector<long long int>> _sent_packets;
  vector<double> _overall_min_sent_packets;
  vector<double> _overall_avg_sent_packets;
  vector<double> _overall_max_sent_packets;
  vector<vector<long long int>> _accepted_packets;
  vector<double> _overall_min_accepted_packets;
  vector<double> _overall_avg_accepted_packets;
  vector<double> _overall_max_accepted_packets;
  vector<vector<long long int>> _sent_flits;
  vector<double> _overall_min_sent;
  vector<double> _overall_avg_sent;
  vector<double> _overall_max_sent;
  vector<vector<long long int>> _accepted_flits;
  vector<double> _overall_min_accepted;
  vector<double> _overall_avg_accepted;
  vector<double> _overall_max_accepted;

  vector<double> _overall_throughput_flits;        //Sneha
  vector<double> _overall_throughput_packets;      //Sneha
  vector<long long int> _overall_flits_received;   //Sneha
  vector<long long int> _overall_packets_received; //Sneha

#ifdef TRACK_STALLS
  vector<vector<long long int>> _buffer_busy_stalls;
  vector<vector<long long int>> _buffer_conflict_stalls;
  vector<vector<long long int>> _buffer_full_stalls;
  vector<vector<long long int>> _buffer_reserved_stalls;
  vector<vector<long long int>> _crossbar_conflict_stalls;
  vector<double> _overall_buffer_busy_stalls;
  vector<double> _overall_buffer_conflict_stalls;
  vector<double> _overall_buffer_full_stalls;
  vector<double> _overall_buffer_reserved_stalls;
  vector<double> _overall_crossbar_conflict_stalls;
#endif

  vector<long long int> _slowest_packet;
  vector<long long int> _slowest_flit;

  map<string, Stats *> _stats;

  // ============ Simulation parameters ============

  enum eSimState
  {
    warming_up,
    running,
    draining,
    done
  };
  eSimState _sim_state;

  long long int _reset_time;
  long long int _drain_time;

  long long int _total_sims;

  long long int _include_queuing;

  vector<long long int> _measure_stats;
  bool _pair_stats;

  long long int _cur_id;
  long long int _cur_pid;
  long long int _time;

  set<long long int> _flits_to_watch;
  set<long long int> _packets_to_watch;

  bool _print_csv_results;

  //flits to watch
  ostream *_stats_out;

  // Orion Power Support
  ostream *_orion_out;
  string _orion_file;

  //Sneha
  long long int Generated_flits;
  long long int Changed_flits;
  long long int Packet_Size_Histogram[20];

#ifdef TRACK_FLOWS
  vector<vector<long long int>> _injected_flits;
  vector<vector<long long int>> _ejected_flits;
  ostream *_injected_flits_out;
  ostream *_received_flits_out;
  ostream *_stored_flits_out;
  ostream *_sent_flits_out;
  ostream *_outstanding_credits_out;
  ostream *_ejected_flits_out;
  ostream *_active_packets_out;
#endif

#ifdef TRACK_CREDITS
  ostream *_used_credits_out;
  ostream *_free_credits_out;
  ostream *_max_credits_out;
#endif

  // ============ Internal methods ============

  virtual void _RetireFlit(Flit *f, long long int dest);
  virtual void _RetirePacket(Flit *head, Flit *tail);

  virtual void _Inject() = 0;

  void _Step();

  virtual bool _PacketsOutstanding() const;

  long long int _GeneratePacket(long long int source, long long int dest, long long int size, long long int cl, long long int time);

  virtual void _ResetSim();

  virtual void _ClearStats();

  void _ComputeStats(const vector<long long int> &stats, long long int *sum, long long int *min = NULL, long long int *max = NULL, long long int *min_pos = NULL, long long int *max_pos = NULL) const;

  virtual bool _SingleSim() = 0;

  void _DisplayRemaining(ostream &os = cout) const;

  void _LoadWatchList(const string &filename);

  virtual void _UpdateOverallStats();

  virtual string _OverallStatsHeaderCSV() const;
  virtual string _OverallClassStatsCSV(long long int c) const;

  virtual void _DisplayClassStats(long long int c, ostream &os) const;
  virtual void _WriteClassStats(long long int c, ostream &os) const;
  virtual void _DisplayOverallClassStats(long long int c, ostream &os) const;

  // Orion Power Support
  void _ComputeOrionPower();

  TrafficManager(const Configuration &config, const vector<Network *> &net);

public:
  virtual ~TrafficManager();

  static TrafficManager *New(Configuration const &config,
                             vector<Network *> const &net);

  bool Run();

  void UpdateStats();
  void DisplayStats(ostream &os = cout) const;
  void WriteStats(ostream &os = cout) const;
  void DisplayOverallStats(ostream &os = cout) const;
  void DisplayOverallStatsCSV(ostream &os = cout) const;

  inline long long int getTime() { return _time; }
  Stats *getStats(const string &name) { return _stats[name]; }
};

template <class T>
ostream &operator<<(ostream &os, const vector<T> &v)
{
  for (size_t i = 0; i < v.size() - 1; ++i)
  {
    os << v[i] << ",";
  }
  os << v[v.size() - 1];
  return os;
}

#endif
