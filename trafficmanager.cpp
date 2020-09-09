// $Id$

#include <sstream>
#include <fstream>
#include <limits>
#include <ctime>

#include "booksim.hpp"
#include "booksim_config.hpp"
#include "trafficmanager.hpp"
#include "steadystatetrafficmanager.hpp"
#include "batchtrafficmanager.hpp"
#include "workloadtrafficmanager.hpp"
#include "random_utils.hpp"
#include "vc.hpp"

TrafficManager *TrafficManager::New(Configuration const &config, vector<Network *> const &net)
{
  TrafficManager *result = NULL;
  string sim_type = config.GetStr("sim_type");
  if ((sim_type == "latency") || (sim_type == "throughput"))
  {
    result = new SteadyStateTrafficManager(config, net);
  }
  else if (sim_type == "batch")
  {
    result = new BatchTrafficManager(config, net);
  }
  else if (sim_type == "workload")
  {
    result = new WorkloadTrafficManager(config, net);
  }
  else
  {
    cerr << "Unknown simulation type: " << sim_type << endl;
  }
  return result;
}

TrafficManager::TrafficManager(const Configuration &config, const vector<Network *> &net)
    : Module(0, "traffic_manager"), _net(net), _empty_network(false), _deadlock_timer(0), _reset_time(0), _drain_time(-1), _cur_id(0), _cur_pid(0), _time(0)
{

  _nodes = _net[0]->NumNodes();
  _routers = _net[0]->NumRouters();

  _vcs = config.GetLongInt("num_vcs");
  _subnets = config.GetLongInt("subnets");

  timeStretch = config.GetLongInt("traceStretch");

  ticksTo1ns=config.GetLongInt("ticksTo1ns");


  //Sneha
  Generated_flits = 0;
  Changed_flits = 0;
  for (long long int histo = 0; histo < 20; histo++)
    Packet_Size_Histogram[histo] = 0;
  //End Sneha

  // ============ Message priorities ============

  string priority = config.GetStr("priority");

  if (priority == "class")
  {
    _pri_type = class_based;
  }
  else if (priority == "age")
  {
    _pri_type = age_based;
  }
  else if (priority == "network_age")
  {
    _pri_type = network_age_based;
  }
  else if (priority == "local_age")
  {
    _pri_type = local_age_based;
  }
  else if (priority == "queue_length")
  {
    _pri_type = queue_length_based;
  }
  else if (priority == "hop_count")
  {
    _pri_type = hop_count_based;
  }
  else if (priority == "sequence")
  {
    _pri_type = sequence_based;
  }
  else if (priority == "none")
  {
    _pri_type = none;
  }
  else
  {
    Error("Unkown priority value: " + priority);
  }

  // ============ Routing ============

  string rf = config.GetStr("routing_function") + "_" + config.GetStr("topology");
  map<string, tRoutingFunction>::const_iterator rf_iter = gRoutingFunctionMap.find(rf);
  if (rf_iter == gRoutingFunctionMap.end())
  {
    Error("Invalid routing function: " + rf);
  }
  _rf = rf_iter->second;

  _lookahead_routing = !config.GetLongInt("routing_delay");
  _noq = config.GetLongInt("noq");
  if (_noq)
  {
    if (!_lookahead_routing)
    {
      Error("NOQ requires lookahead routing to be enabled.");
    }
  }

  // ============ Traffic ============

  _classes = config.GetLongInt("classes");

  _subnet = config.GetIntArray("subnet");
  if (_subnet.empty())
  {
    _subnet.push_back(config.GetLongInt("subnet"));
  }
  _subnet.resize(_classes, _subnet.back());

  _class_priority = config.GetIntArray("class_priority");
  if (_class_priority.empty())
  {
    _class_priority.push_back(config.GetLongInt("class_priority"));
  }
  _class_priority.resize(_classes, _class_priority.back());

  // ============ Injection VC states  ============

  _buf_states.resize(_nodes);
  _last_vc.resize(_nodes);
  _last_class.resize(_nodes);

  for (long long int source = 0; source < _nodes; ++source)
  {
    _buf_states[source].resize(_subnets);
    _last_class[source].resize(_subnets, 0);
    _last_vc[source].resize(_subnets);
    for (long long int subnet = 0; subnet < _subnets; ++subnet)
    {
      ostringstream tmp_name;
      tmp_name << "terminal_buf_state_" << source << "_" << subnet;
      BufferState *bs = new BufferState(config, this, tmp_name.str());
      long long int vc_alloc_delay = config.GetLongInt("vc_alloc_delay");
      long long int sw_alloc_delay = config.GetLongInt("sw_alloc_delay");
      long long int router_latency = config.GetLongInt("routing_delay") + (config.GetLongInt("speculative") ? max(vc_alloc_delay, sw_alloc_delay) : (vc_alloc_delay + sw_alloc_delay));
      long long int min_latency = 1 + _net[subnet]->GetInject(source)->GetLatency() + router_latency + _net[subnet]->GetInjectCred(source)->GetLatency();
      bs->SetMinLatency(min_latency);
      _buf_states[source][subnet] = bs;
      _last_vc[source][subnet] = gEndVCs;
    }
  }

  // ============ Injection queues ============

  _partial_packets.resize(_classes);
  _packet_seq_no.resize(_classes);
  _requests_outstanding.resize(_classes);

  for (long long int c = 0; c < _classes; ++c)
  {
    _partial_packets[c].resize(_nodes);
    _packet_seq_no[c].resize(_nodes);
    _requests_outstanding[c].resize(_nodes);
  }

  _total_in_flight_flits.resize(_classes);
  _measured_in_flight_flits.resize(_classes);
  _retired_packets.resize(_classes);

  _hold_switch_for_packet = config.GetLongInt("hold_switch_for_packet");

  // ============ Simulation parameters ============

  _total_sims = config.GetLongInt("sim_count");

  _router.resize(_subnets);
  for (long long int i = 0; i < _subnets; ++i)
  {
    _router[i] = _net[i]->GetRouters();
  }

  //seed the network
  long long int seed;
  if (config.GetStr("seed") == "time")
  {
    seed = time(NULL);
    cout << "SEED: seed=" << seed << endl;
  }
  else
  {
    seed = config.GetLongInt("seed");
  }
  RandomSeed(seed);

  _measure_stats = config.GetIntArray("measure_stats");
  if (_measure_stats.empty())
  {
    _measure_stats.push_back(config.GetLongInt("measure_stats"));
  }
  _measure_stats.resize(_classes, _measure_stats.back());
  _pair_stats = (config.GetLongInt("pair_stats") == 1);

  _include_queuing = config.GetLongInt("include_queuing");

  _print_csv_results = config.GetLongInt("print_csv_results");
  _deadlock_warn_timeout = config.GetLongInt("deadlock_warn_timeout");

  string watch_file = config.GetStr("watch_file");
  if ((watch_file != "") && (watch_file != "-"))
  {
    _LoadWatchList(watch_file);
  }

  // Orion Power Support
  string orion_file = config.GetStr("orion_file");
  if ((orion_file != "") && (orion_file != "-"))
  {
    _orion_file = orion_file;
  }

  vector<long long int> watch_flits = config.GetIntArray("watch_flits");
  for (size_t i = 0; i < watch_flits.size(); ++i)
  {
    _flits_to_watch.insert(watch_flits[i]);
  }

  vector<long long int> watch_packets = config.GetIntArray("watch_packets");
  for (size_t i = 0; i < watch_packets.size(); ++i)
  {
    _packets_to_watch.insert(watch_packets[i]);
  }

  string stats_out_file = config.GetStr("stats_out");
  if (stats_out_file == "")
  {
    _stats_out = NULL;
  }
  else if (stats_out_file == "-")
  {
    _stats_out = &cout;
  }
  else
  {
    _stats_out = new ofstream(stats_out_file.c_str());
    config.WriteMatlabFile(_stats_out);
  }

  // Orion Power Support
  string orion_out_file = config.GetStr("orion_out");
  if (orion_out_file == "")
  {
    _orion_out = NULL;
  }
  else if (orion_out_file == "-")
  {
    _orion_out = &cout;
  }
  else
  {
    _orion_out = new ofstream(orion_out_file.c_str());
    config.WriteMatlabFile(_orion_out);
  }

  // ============ Statistics ============

  _plat_stats.resize(_classes);
  _overall_min_plat.resize(_classes, 0.0);
  _overall_avg_plat.resize(_classes, 0.0);
  _overall_max_plat.resize(_classes, 0.0);

  _nlat_stats.resize(_classes);
  _overall_min_nlat.resize(_classes, 0.0);
  _overall_avg_nlat.resize(_classes, 0.0);
  _overall_max_nlat.resize(_classes, 0.0);

  _flat_stats.resize(_classes);
  _overall_min_flat.resize(_classes, 0.0);
  _overall_avg_flat.resize(_classes, 0.0);
  _overall_max_flat.resize(_classes, 0.0);

  _frag_stats.resize(_classes);
  _overall_min_frag.resize(_classes, 0.0);
  _overall_avg_frag.resize(_classes, 0.0);
  _overall_max_frag.resize(_classes, 0.0);

  if (_pair_stats)
  {
    _pair_plat.resize(_classes);
    _pair_nlat.resize(_classes);
    _pair_flat.resize(_classes);
  }

  _hop_stats.resize(_classes);
  _overall_hop_stats.resize(_classes, 0.0);

  _sent_packets.resize(_classes);
  _overall_min_sent_packets.resize(_classes, 0.0);
  _overall_avg_sent_packets.resize(_classes, 0.0);
  _overall_max_sent_packets.resize(_classes, 0.0);
  _accepted_packets.resize(_classes);
  _overall_min_accepted_packets.resize(_classes, 0.0);
  _overall_avg_accepted_packets.resize(_classes, 0.0);
  _overall_max_accepted_packets.resize(_classes, 0.0);

  _sent_flits.resize(_classes);
  _overall_min_sent.resize(_classes, 0.0);
  _overall_avg_sent.resize(_classes, 0.0);
  _overall_max_sent.resize(_classes, 0.0);
  _accepted_flits.resize(_classes);
  _overall_min_accepted.resize(_classes, 0.0);
  _overall_avg_accepted.resize(_classes, 0.0);
  _overall_max_accepted.resize(_classes, 0.0);

  _overall_flits_received.resize(_classes, 0);   //Sneha
  _overall_packets_received.resize(_classes, 0); //Sneha

  _overall_throughput_flits.resize(_classes, 0.0);   //Sneha
  _overall_throughput_packets.resize(_classes, 0.0); //Sneha

  for (long long int c = 0; c < _classes; ++c)
  {
    ostringstream tmp_name;

    tmp_name << "plat_stat_" << c;
    _plat_stats[c] = new Stats(this, tmp_name.str(), 1.0, 1000);
    _stats[tmp_name.str()] = _plat_stats[c];
    tmp_name.str("");

    tmp_name << "nlat_stat_" << c;
    _nlat_stats[c] = new Stats(this, tmp_name.str(), 1.0, 1000);
    _stats[tmp_name.str()] = _nlat_stats[c];
    tmp_name.str("");

    tmp_name << "flat_stat_" << c;
    _flat_stats[c] = new Stats(this, tmp_name.str(), 1.0, 1000);
    _stats[tmp_name.str()] = _flat_stats[c];
    tmp_name.str("");

    tmp_name << "frag_stat_" << c;
    _frag_stats[c] = new Stats(this, tmp_name.str(), 1.0, 100);
    _stats[tmp_name.str()] = _frag_stats[c];
    tmp_name.str("");

    tmp_name << "hop_stat_" << c;
    _hop_stats[c] = new Stats(this, tmp_name.str(), 1.0, 20);
    _stats[tmp_name.str()] = _hop_stats[c];
    tmp_name.str("");

    if (_pair_stats)
    {
      _pair_plat[c].resize(_nodes * _nodes);
      _pair_nlat[c].resize(_nodes * _nodes);
      _pair_flat[c].resize(_nodes * _nodes);
    }

    _sent_packets[c].resize(_nodes, 0);
    _accepted_packets[c].resize(_nodes, 0);
    _sent_flits[c].resize(_nodes, 0);
    _accepted_flits[c].resize(_nodes, 0);

    if (_pair_stats)
    {
      for (long long int i = 0; i < _nodes; ++i)
      {
        for (long long int j = 0; j < _nodes; ++j)
        {
          tmp_name << "pair_plat_stat_" << c << "_" << i << "_" << j;
          _pair_plat[c][i * _nodes + j] = new Stats(this, tmp_name.str(), 1.0, 250);
          _stats[tmp_name.str()] = _pair_plat[c][i * _nodes + j];
          tmp_name.str("");

          tmp_name << "pair_nlat_stat_" << c << "_" << i << "_" << j;
          _pair_nlat[c][i * _nodes + j] = new Stats(this, tmp_name.str(), 1.0, 250);
          _stats[tmp_name.str()] = _pair_nlat[c][i * _nodes + j];
          tmp_name.str("");

          tmp_name << "pair_flat_stat_" << c << "_" << i << "_" << j;
          _pair_flat[c][i * _nodes + j] = new Stats(this, tmp_name.str(), 1.0, 250);
          _stats[tmp_name.str()] = _pair_flat[c][i * _nodes + j];
          tmp_name.str("");
        }
      }
    }
  }

  _slowest_flit.resize(_classes, -1);
  _slowest_packet.resize(_classes, -1);
}

TrafficManager::~TrafficManager()
{

  for (long long int source = 0; source < _nodes; ++source)
  {
    for (long long int subnet = 0; subnet < _subnets; ++subnet)
    {
      delete _buf_states[source][subnet];
    }
  }

  for (long long int c = 0; c < _classes; ++c)
  {
    delete _plat_stats[c];
    delete _nlat_stats[c];
    delete _flat_stats[c];
    delete _frag_stats[c];
    delete _hop_stats[c];

    if (_pair_stats)
    {
      for (long long int i = 0; i < _nodes; ++i)
      {
        for (long long int j = 0; j < _nodes; ++j)
        {
          delete _pair_plat[c][i * _nodes + j];
          delete _pair_nlat[c][i * _nodes + j];
          delete _pair_flat[c][i * _nodes + j];
        }
      }
    }
  }

  if (gWatchOut && (gWatchOut != &cout))
    delete gWatchOut;
  if (_stats_out && (_stats_out != &cout))
    delete _stats_out;
  // Orion Power Support
  if (_orion_out && (_orion_out != &cout))
    delete _orion_out;

  Flit::FreeAll();
  Credit::FreeAll();
}

void TrafficManager::_RetireFlit(Flit *f, long long int dest)
{
  _deadlock_timer = 0;
  //  printf("\nTime:,%lld,%lld,[%lld][%lld],RetFlit,%lld\n", GetSimTime(), f->dest, f->id, f->pid, f->vc); //Sneha
  //  printf("\nTime:,%lld,%lld,[%lld][%lld],RetFlit,%lld, Time taken = %llds\n", GetSimTime(), f->dest, f->id, f->pid, f->vc,((long long int)GetSimTime() - (long long int)f->starttime)); //*Sneha
  _total_in_flight_flits[f->cl].erase(f->id);

  _overall_flits_received[f->cl]++; //Sneha

  if (f->record)
  {
    _measured_in_flight_flits[f->cl].erase(f->id);
  }

  if (f->head && (f->dest != dest))
  {
    ostringstream err;
    err << "Flit " << f->id << " arrived at incorrect output " << dest;
    Error(err.str());
  }

  if ((_slowest_flit[f->cl] < 0) ||
      (_flat_stats[f->cl]->Max() < (f->atime - f->itime)))
    _slowest_flit[f->cl] = f->id;
  _flat_stats[f->cl]->AddSample(f->atime - f->itime);
  if (_pair_stats)
  {
    _pair_flat[f->cl][f->src * _nodes + dest]->AddSample(f->atime - f->itime);
  }

  if (f->tail)
  {
    Flit *head;
    if (f->head)
    {
      head = f;
    }
    else
    {
      map<long long int, Flit *>::iterator iter = _retired_packets[f->cl].find(f->pid);
      head = iter->second;
      _retired_packets[f->cl].erase(iter);
    }
    _overall_packets_received[f->cl]++; //Sneha

    _RetirePacket(head, f);

    // Only record statistics once per packet (at tail)
    // and based on the simulation state
    if ((_sim_state == warming_up) || f->record)
    {
      _hop_stats[f->cl]->AddSample(f->hops);
      if ((_slowest_packet[f->cl] < 0) || (_plat_stats[f->cl]->Max() < (f->atime - head->itime)))
        _slowest_packet[f->cl] = f->pid;
      _plat_stats[f->cl]->AddSample(f->atime - head->ctime);
      _nlat_stats[f->cl]->AddSample(f->atime - head->itime);
      _frag_stats[f->cl]->AddSample((f->atime - head->atime) - (f->id - head->id));
      if (_pair_stats)
      {
        _pair_plat[f->cl][f->src * _nodes + dest]->AddSample(f->atime - head->ctime);
        _pair_nlat[f->cl][f->src * _nodes + dest]->AddSample(f->atime - head->itime);
      }
    }

    if (f != head)
    {
      head->Free();
    }
  }

  if (f->head && !f->tail)
  {
    _retired_packets[f->cl].insert(make_pair(f->pid, f));
  }
  else
  {
    f->Free();
  }

  // Orion Power Support
  g_number_of_retired_flits++;
}

void TrafficManager::_RetirePacket(Flit *head, Flit *tail)
{
  _requests_outstanding[head->cl][head->src]--;
}

long long int TrafficManager::_GeneratePacket(long long int source, long long int dest, long long int size, long long int cl, long long int time)
{
  assert(size > 0);
  assert((source >= 0) && (source < _nodes));
  assert((dest >= 0) && (dest < _nodes));

  long long int pid = _cur_pid++;
  assert(_cur_pid);

  bool watch = gWatchOut && (_packets_to_watch.count(pid) > 0);

  bool record = (((_sim_state == running) || ((_sim_state == draining) && (time < _drain_time))) && _measure_stats[cl]);
  Packet_Size_Histogram[size]++; //Sneha
                                 //  printf("\n Generating packet: Source : %lld, Destination : %lld, Time: %lld\n", source, dest, time);		//Sneha

  for (long long int i = 0; i < size; ++i)
  {

    long long int id = _cur_id++;

    Flit *f = Flit::New();

    f->id = id;
    f->pid = pid;
    f->watch = watch | (gWatchOut && (_flits_to_watch.count(f->id) > 0));
    f->src = source;
    f->dest = dest;
    f->ctime = time;
    f->record = record;
    f->cl = cl;
    f->head = (i == 0);
    f->tail = (i == (size - 1));
    f->vc = -1;
    Generated_flits++;                            //Sneha
    f->starttime = (long long int)(GetSimTime()); //Sarab
                                                  //    printf("\nTime:,%lld,%lld,[%lld][%lld],GenFlit,%lld\n", GetSimTime(), f->src, f->id, f->pid, f->vc); //Sneha

    switch (_pri_type)
    {
    case class_based:
      f->pri = _class_priority[cl];
      break;
    case age_based:
      f->pri = numeric_limits<long long int>::max() - time;
      assert(f->pri >= 0);
      break;
    case sequence_based:
      f->pri = numeric_limits<long long int>::max() - _packet_seq_no[cl][source];
      break;
    default:
      f->pri = 0;
    }

    _total_in_flight_flits[f->cl].insert(make_pair(f->id, f));
    if (record)
    {
      _measured_in_flight_flits[f->cl].insert(make_pair(f->id, f));
    }

    if (gTrace)
    {
      cout << "New Flit " << f->src << endl;
    }

    _partial_packets[cl][source].push_back(f);
  }
  return pid;
}

void TrafficManager::_Step()
{
  bool flits_in_flight = false;
  for (long long int c = 0; c < _classes; ++c)
  {
    flits_in_flight |= !_total_in_flight_flits[c].empty();
  }
  if (flits_in_flight && (_deadlock_timer++ >= _deadlock_warn_timeout))
  {
    _deadlock_timer = 0;
    cout << "WARNING: Possible network deadlock." << endl;
  }

  vector<map<long long int, Flit *>> flits(_subnets);
  for (long long int subnet = 0; subnet < _subnets; ++subnet)
  {
    for (long long int n = 0; n < _nodes; ++n)
    {
      Flit *const f = _net[subnet]->ReadFlit(n);
      if (f)
      {
        flits[subnet].insert(make_pair(n, f));
        if ((_sim_state == warming_up) || (_sim_state == running))
        {
          ++_accepted_flits[f->cl][n];
          if (f->tail)
          {
            ++_accepted_packets[f->cl][n];
          }
        }
      }

      Credit *const c = _net[subnet]->ReadCredit(n);
      if (c)
      {
        _buf_states[n][subnet]->ProcessCredit(c);
        c->Free();
      }
    }
    _net[subnet]->ReadInputs();
  }

  if (!_empty_network)
  {
    _Inject();
  }

  for (long long int subnet = 0; subnet < _subnets; ++subnet)
  {

    for (long long int n = 0; n < _nodes; ++n)
    {
      Flit *f = NULL;
      BufferState *const dest_buf = _buf_states[n][subnet];
      long long int const last_class = _last_class[n][subnet];
      long long int class_limit = _classes;
      if (_hold_switch_for_packet)
      {
        list<Flit *> const &pp = _partial_packets[last_class][n];
        if (!pp.empty() && !pp.front()->head &&
            !dest_buf->IsFullFor(pp.front()->vc))
        {
          f = pp.front();
          --class_limit;
        }
      }

      for (long long int i = 1; i <= class_limit; ++i)
      {
        long long int const c = (last_class + i) % _classes;
        if (_subnet[c] != subnet)
        {
          continue;
        }
        list<Flit *> const &pp = _partial_packets[c][n];

        if (pp.empty())
        {
          continue;
        }

        Flit *const cf = pp.front();
        if (f && (f->pri >= cf->pri))
        {
          continue;
        }

        if (cf->head && cf->vc == -1)
        { // Find first available VC
          OutputSet route_set;
          _rf(NULL, cf, -1, &route_set, true);
          set<OutputSet::sSetElement> const &os = route_set.GetSet();
          OutputSet::sSetElement const &se = *os.begin();
          long long int vcBegin = se.vc_start;
          long long int vcEnd = se.vc_end;
          long long int vc_count = vcEnd - vcBegin + 1;
          if (_noq)
          {
            const FlitChannel *inject = _net[subnet]->GetInject(n);
            const Router *router = inject->GetSink();
            long long int in_channel = inject->GetSinkPort();
            cf->vc = vcBegin;
            _rf(router, cf, in_channel, &cf->la_route_set, false);
            cf->vc = -1;

            set<OutputSet::sSetElement> const sl = cf->la_route_set.GetSet();
            long long int next_output = sl.begin()->output_port;
            vc_count /= router->NumOutputs();
            vcBegin += next_output * vc_count;
            vcEnd = vcBegin + vc_count - 1;
          }
          for (long long int i = 1; i <= vc_count; ++i)
          {
            long long int const lvc = _last_vc[n][subnet][c];
            long long int const vc =
                (lvc < vcBegin || lvc > vcEnd) ? vcBegin : (vcBegin + (lvc - vcBegin + i) % vc_count);
            if (!dest_buf->IsAvailableFor(vc))
            {
            }
            else
            {
              if (dest_buf->IsFullFor(vc))
              {
              }
              else
              {
                cf->vc = vc;
                break;
              }
            }
          }
        }

        if (cf->vc == -1)
        {
        }
        else
        {
          if (dest_buf->IsFullFor(cf->vc))
          {
          }
          else
          {
            f = cf;
          }
        }
      }

      if (f)
      {
        long long int const c = f->cl;
        if (f->head)
        {
          if (_lookahead_routing)
          {
            if (!_noq)
            {
              const FlitChannel *inject = _net[subnet]->GetInject(n);
              const Router *router = inject->GetSink();
              long long int in_channel = inject->GetSinkPort();
              _rf(router, f, in_channel, &f->la_route_set, false);
            }
            else if (f->watch)
            {
              *gWatchOut << GetSimTime() << " | "
                         << "node" << n << " | "
                         << "Already generated lookahead routing info for flit " << f->id
                         << " (NOQ)." << endl;
            }
          }
          else
          {
            f->la_route_set.Clear();
          }

          dest_buf->TakeBuffer(f->vc);
          _last_vc[n][subnet][c] = f->vc;
        }
        _last_class[n][subnet] = c;
        _partial_packets[c][n].pop_front();
        dest_buf->SendingFlit(f);
        if (_pri_type == network_age_based)
        {
          f->pri = numeric_limits<long long int>::max() - _time;
        }
        f->itime = _time;
        // Pass VC "back"
        if (!_partial_packets[c][n].empty() && !f->tail)
        {
          Flit *const nf = _partial_packets[c][n].front();
          nf->vc = f->vc;
        }

        if ((_sim_state == warming_up) || (_sim_state == running))
        {
          ++_sent_flits[c][n];
          if (f->head)
          {
            ++_sent_packets[c][n];
          }
        }
        _net[subnet]->WriteFlit(f, n);
      }
    }
  }

  for (long long int subnet = 0; subnet < _subnets; ++subnet)
  {
    for (long long int n = 0; n < _nodes; ++n)
    {
      map<long long int, Flit *>::const_iterator iter = flits[subnet].find(n);
      if (iter != flits[subnet].end())
      {
        Flit *const f = iter->second;

        f->atime = _time;
        Credit *const c = Credit::New();
        c->vc.insert(f->vc);
        _net[subnet]->WriteCredit(c, n);

#ifdef TRACK_FLOWS
        ++_ejected_flits[f->cl][n];
#endif

        _RetireFlit(f, n);
      }
    }
    flits[subnet].clear();
    _net[subnet]->Evaluate();
    _net[subnet]->WriteOutputs();
  }

  ++_time;
  assert(_time);
  if (gTrace)
  {
    cout << "TIME " << _time << endl;
  }
}

bool TrafficManager::_PacketsOutstanding() const
{
  for (long long int c = 0; c < _classes; ++c)
  {
    if (!_measured_in_flight_flits[c].empty())
    {
      return true;
    }
  }
  return false;
}

void TrafficManager::_ResetSim()
{
  _time = 0;

  //remove any pending request from the previous simulations
  for (long long int c = 0; c < _classes; ++c)
  {
    _requests_outstanding[c].assign(_nodes, 0);
  }
}

void TrafficManager::_ClearStats()
{
  _slowest_flit.assign(_classes, -1);
  _slowest_packet.assign(_classes, -1);

  for (long long int c = 0; c < _classes; ++c)
  {

    _plat_stats[c]->Clear();
    _nlat_stats[c]->Clear();
    _flat_stats[c]->Clear();

    _frag_stats[c]->Clear();

    _sent_packets[c].assign(_nodes, 0);
    _accepted_packets[c].assign(_nodes, 0);
    _sent_flits[c].assign(_nodes, 0);
    _accepted_flits[c].assign(_nodes, 0);

#ifdef TRACK_STALLS
    _buffer_busy_stalls[c].assign(_subnets * _routers, 0);
    _buffer_conflict_stalls[c].assign(_subnets * _routers, 0);
    _buffer_full_stalls[c].assign(_subnets * _routers, 0);
    _buffer_reserved_stalls[c].assign(_subnets * _routers, 0);
    _crossbar_conflict_stalls[c].assign(_subnets * _routers, 0);
#endif
    if (_pair_stats)
    {
      for (long long int i = 0; i < _nodes; ++i)
      {
        for (long long int j = 0; j < _nodes; ++j)
        {
          _pair_plat[c][i * _nodes + j]->Clear();
          _pair_nlat[c][i * _nodes + j]->Clear();
          _pair_flat[c][i * _nodes + j]->Clear();
        }
      }
    }
    _hop_stats[c]->Clear();
  }

  _reset_time = _time;
}

void TrafficManager::_ComputeStats(const vector<long long int> &stats, long long int *sum, long long int *min, long long int *max, long long int *min_pos, long long int *max_pos) const
{
  long long int const count = stats.size();
  assert(count > 0);

  if (min_pos)
  {
    *min_pos = 0;
  }
  if (max_pos)
  {
    *max_pos = 0;
  }

  if (min)
  {
    *min = stats[0];
  }
  if (max)
  {
    *max = stats[0];
  }

  *sum = stats[0];

  for (long long int i = 1; i < count; ++i)
  {
    long long int curr = stats[i];
    if (min && (curr < *min))
    {
      *min = curr;
      if (min_pos)
      {
        *min_pos = i;
      }
    }
    if (max && (curr > *max))
    {
      *max = curr;
      if (max_pos)
      {
        *max_pos = i;
      }
    }
    *sum += curr;
  }
}

void TrafficManager::_DisplayRemaining(ostream &os) const
{
  for (long long int c = 0; c < _classes; ++c)
  {

    map<long long int, Flit *>::const_iterator iter;
    long long int i;

    os << "Class " << c << ":" << endl;

    os << "Remaining flits: ";
    for (iter = _total_in_flight_flits[c].begin(), i = 0;
         (iter != _total_in_flight_flits[c].end()) && (i < 10);
         iter++, i++)
    {
      os << iter->first << " ";
    }
    if (_total_in_flight_flits[c].size() > 10)
      os << "[...] ";

    os << "(" << _total_in_flight_flits[c].size() << " flits)" << endl;

    os << "Measured flits: ";
    for (iter = _measured_in_flight_flits[c].begin(), i = 0;
         (iter != _measured_in_flight_flits[c].end()) && (i < 10);
         iter++, i++)
    {
      os << iter->first << " ";
    }
    if (_measured_in_flight_flits[c].size() > 10)
      os << "[...] ";

    os << "(" << _measured_in_flight_flits[c].size() << " flits)" << endl;
  }
}

bool TrafficManager::Run()
{
  for (long long int sim = 0; sim < _total_sims; ++sim)
  {

    _ResetSim();

    _ClearStats();

    if (!_SingleSim())
    {
      cout << "Simulation unstable, ending ..." << endl;
      return false;
    }

    // Empty any remaining packets
    cout << "Draining remaining packets ..." << endl;
    _empty_network = true;
    long long int empty_steps = 0;

    bool packets_left = false;
    for (long long int c = 0; c < _classes; ++c)
    {
      packets_left |= !_total_in_flight_flits[c].empty();
    }

    while (packets_left)
    {
      _Step();

      ++empty_steps;

      if (empty_steps % 1000 == 0)
      {
        _DisplayRemaining();
      }

      packets_left = false;
      for (long long int c = 0; c < _classes; ++c)
      {
        packets_left |= !_total_in_flight_flits[c].empty();
      }
    }
    //wait until all the credits are drained as well
    while (Credit::OutStanding() != 0)
    {
      _Step();
    }
    _empty_network = false;

    //for the love of god don't ever say "Time taken" anywhere else the power script depend on it
    cout << "Time taken is " << _time << " cycles" << endl;

    if (_stats_out)
    {
      WriteStats(*_stats_out);
    }
    _UpdateOverallStats();
  }

  DisplayOverallStats();
  if (_print_csv_results)
  {
    DisplayOverallStatsCSV();
  }

  // Orion Power Support
  _ComputeOrionPower();
  return true;
}

void TrafficManager::DisplayOverallStats(ostream &os) const
{
  for (long long int c = 0; c < _classes; ++c)
  {
    if (_measure_stats[c])
    {
      cout << "====== Traffic class " << c << " ======" << endl;
      _DisplayOverallClassStats(c, os);
    }
  }
}

void TrafficManager::DisplayOverallStatsCSV(ostream &os) const
{
  os << "header:class," << _OverallStatsHeaderCSV() << endl;
  for (long long int c = 0; c < _classes; ++c)
  {
    if (_measure_stats[c])
    {
      os << "results:" << c << ',' << _OverallClassStatsCSV(c) << endl;
    }
  }
}

void TrafficManager::_UpdateOverallStats()
{

  for (long long int c = 0; c < _classes; ++c)
  {

    if (_measure_stats[c] == 0)
    {
      continue;
    }

    _overall_min_plat[c] += _plat_stats[c]->Min();
    _overall_avg_plat[c] += _plat_stats[c]->Average();
    _overall_max_plat[c] += _plat_stats[c]->Max();
    _overall_min_nlat[c] += _nlat_stats[c]->Min();
    _overall_avg_nlat[c] += _nlat_stats[c]->Average();
    _overall_max_nlat[c] += _nlat_stats[c]->Max();
    _overall_min_flat[c] += _flat_stats[c]->Min();
    _overall_avg_flat[c] += _flat_stats[c]->Average();
    _overall_max_flat[c] += _flat_stats[c]->Max();

    _overall_min_frag[c] += _frag_stats[c]->Min();
    _overall_avg_frag[c] += _frag_stats[c]->Average();
    _overall_max_frag[c] += _frag_stats[c]->Max();

    _overall_hop_stats[c] += _hop_stats[c]->Average();

    long long int count_min, count_sum, count_max;
    double rate_min, rate_sum, rate_max;
    double rate_avg;
    double time_delta = (double)(_drain_time - _reset_time);
    _ComputeStats(_sent_flits[c], &count_sum, &count_min, &count_max);
    rate_min = (double)count_min / time_delta;
    rate_sum = (double)count_sum / time_delta;
    rate_max = (double)count_max / time_delta;
    rate_avg = rate_sum / (double)_nodes;
    _overall_min_sent[c] += rate_min;
    _overall_avg_sent[c] += rate_avg;
    _overall_max_sent[c] += rate_max;
    _ComputeStats(_sent_packets[c], &count_sum, &count_min, &count_max);
    rate_min = (double)count_min / time_delta;
    rate_sum = (double)count_sum / time_delta;
    rate_max = (double)count_max / time_delta;
    rate_avg = rate_sum / (double)_nodes;
    _overall_min_sent_packets[c] += rate_min;
    _overall_avg_sent_packets[c] += rate_avg;
    _overall_max_sent_packets[c] += rate_max;
    _ComputeStats(_accepted_flits[c], &count_sum, &count_min, &count_max);
    rate_min = (double)count_min / time_delta;
    rate_sum = (double)count_sum / time_delta;
    rate_max = (double)count_max / time_delta;
    rate_avg = rate_sum / (double)_nodes;
    _overall_min_accepted[c] += rate_min;
    _overall_avg_accepted[c] += rate_avg;
    _overall_max_accepted[c] += rate_max;
    _ComputeStats(_accepted_packets[c], &count_sum, &count_min, &count_max);
    rate_min = (double)count_min / time_delta;
    rate_sum = (double)count_sum / time_delta;
    rate_max = (double)count_max / time_delta;
    rate_avg = rate_sum / (double)_nodes;
    _overall_min_accepted_packets[c] += rate_min;
    _overall_avg_accepted_packets[c] += rate_avg;
    _overall_max_accepted_packets[c] += rate_max;

    _overall_throughput_flits[c] = (double)_overall_flits_received[c] / time_delta;     //Sneha
    _overall_throughput_packets[c] = (double)_overall_packets_received[c] / time_delta; //Sneha
  }
}

void TrafficManager::UpdateStats()
{
}

void TrafficManager::DisplayStats(ostream &os) const
{
  os << "===== Time: " << _time << " =====" << endl;

  for (long long int c = 0; c < _classes; ++c)
  {
    if (_measure_stats[c])
    {
      os << "Class " << c << ":" << endl;
      _DisplayClassStats(c, os);
    }
  }
}

void TrafficManager::_DisplayClassStats(long long int c, ostream &os) const
{

  os << "Minimum packet latency = " << _plat_stats[c]->Min() << endl;
  os << "Average packet latency = " << _plat_stats[c]->Average() << endl;
  os << "Maximum packet latency = " << _plat_stats[c]->Max() << endl;
  os << "Minimum network latency = " << _nlat_stats[c]->Min() << endl;
  os << "Average network latency = " << _nlat_stats[c]->Average() << endl;
  os << "Maximum network latency = " << _nlat_stats[c]->Max() << endl;
  os << "Slowest packet = " << _slowest_packet[c] << endl;
  os << "Minimum flit latency = " << _flat_stats[c]->Min() << endl;
  os << "Average flit latency = " << _flat_stats[c]->Average() << endl;
  os << "Maximum flit latency = " << _flat_stats[c]->Max() << endl;
  os << "Slowest flit = " << _slowest_flit[c] << endl;
  os << "Minimum fragmentation = " << _frag_stats[c]->Min() << endl;
  os << "Average fragmentation = " << _frag_stats[c]->Average() << endl;
  os << "Maximum fragmentation = " << _frag_stats[c]->Max() << endl;

  long long int count_sum, count_min, count_max;
  double rate_sum, rate_min, rate_max;
  double rate_avg;
  long long int sent_packets, sent_flits, accepted_packets, accepted_flits;
  long long int min_pos, max_pos;
  double time_delta = (double)(_time - _reset_time);
  _ComputeStats(_sent_packets[c], &count_sum, &count_min, &count_max, &min_pos, &max_pos);
  rate_sum = (double)count_sum / time_delta;
  rate_min = (double)count_min / time_delta;
  rate_max = (double)count_max / time_delta;
  rate_avg = rate_sum / (double)_nodes;
  sent_packets = count_sum;
  os << "Minimum injected packet rate = " << rate_min
     << " (at node " << min_pos << ")" << endl
     << "Average injected packet rate = " << rate_avg << endl
     << "Maximum injected packet rate = " << rate_max
     << " (at node " << max_pos << ")" << endl;
  _ComputeStats(_accepted_packets[c], &count_sum, &count_min, &count_max, &min_pos, &max_pos);
  rate_sum = (double)count_sum / time_delta;
  rate_min = (double)count_min / time_delta;
  rate_max = (double)count_max / time_delta;
  rate_avg = rate_sum / (double)_nodes;
  accepted_packets = count_sum;
  os << "Minimum accepted packet rate = " << rate_min
     << " (at node " << min_pos << ")" << endl
     << "Average accepted packet rate = " << rate_avg << endl
     << "Maximum accepted packet rate = " << rate_max
     << " (at node " << max_pos << ")" << endl;
  _ComputeStats(_sent_flits[c], &count_sum, &count_min, &count_max, &min_pos, &max_pos);
  rate_sum = (double)count_sum / time_delta;
  rate_min = (double)count_min / time_delta;
  rate_max = (double)count_max / time_delta;
  rate_avg = rate_sum / (double)_nodes;
  sent_flits = count_sum;
  os << "Minimum injected flit rate = " << rate_min
     << " (at node " << min_pos << ")" << endl
     << "Average injected flit rate = " << rate_avg << endl
     << "Maximum injected flit rate = " << rate_max
     << " (at node " << max_pos << ")" << endl;
  _ComputeStats(_accepted_flits[c], &count_sum, &count_min, &count_max, &min_pos, &max_pos);
  rate_sum = (double)count_sum / time_delta;
  rate_min = (double)count_min / time_delta;
  rate_max = (double)count_max / time_delta;
  rate_avg = rate_sum / (double)_nodes;
  accepted_flits = count_sum;
  os << "Minimum accepted flit rate = " << rate_min
     << " (at node " << min_pos << ")" << endl
     << "Average accepted flit rate = " << rate_avg << endl
     << "Maximum accepted flit rate = " << rate_max
     << " (at node " << max_pos << ")" << endl;

  os << "Total number of injected packets = " << sent_packets << endl
     << "Total number of injected flits = " << sent_flits << endl
     << "Average injected packet length = " << (double)sent_flits / (double)sent_packets << endl;

  os << "Total number of accepted packets = " << accepted_packets << endl
     << "Total number of accepted flits = " << accepted_flits << endl
     << "Average accepted packet length = " << (double)accepted_flits / (double)accepted_packets << endl;

  os << "Total in-flight flits = " << _total_in_flight_flits[c].size()
     << " (" << _measured_in_flight_flits[c].size() << " measured)"
     << endl;
}

void TrafficManager::WriteStats(ostream &os) const
{
  os << "%=================================" << endl;
  for (long long int c = 0; c < _classes; ++c)
  {
    if (_measure_stats[c])
    {
      _WriteClassStats(c, os);
    }
  }
}

void TrafficManager::_WriteClassStats(long long int c, ostream &os) const
{

  os << "plat(" << c + 1 << ") = " << _plat_stats[c]->Average() << ";" << endl
     << "plat_hist(" << c + 1 << ",:) = " << *_plat_stats[c] << ";" << endl
     << "nlat(" << c + 1 << ") = " << _nlat_stats[c]->Average() << ";" << endl
     << "nlat_hist(" << c + 1 << ",:) = " << *_nlat_stats[c] << ";" << endl
     << "flat(" << c + 1 << ") = " << _flat_stats[c]->Average() << ";" << endl
     << "flat_hist(" << c + 1 << ",:) = " << *_flat_stats[c] << ";" << endl
     << "frag_hist(" << c + 1 << ",:) = " << *_frag_stats[c] << ";" << endl
     << "hops(" << c + 1 << ",:) = " << *_hop_stats[c] << ";" << endl;
  if (_pair_stats)
  {
    os << "pair_sent(" << c + 1 << ",:) = [ ";
    for (long long int i = 0; i < _nodes; ++i)
    {
      for (long long int j = 0; j < _nodes; ++j)
      {
        os << _pair_plat[c][i * _nodes + j]->NumSamples() << " ";
      }
    }
    os << "];" << endl
       << "pair_plat(" << c + 1 << ",:) = [ ";
    for (long long int i = 0; i < _nodes; ++i)
    {
      for (long long int j = 0; j < _nodes; ++j)
      {
        os << _pair_plat[c][i * _nodes + j]->Average() << " ";
      }
    }
    os << "];" << endl
       << "pair_nlat(" << c + 1 << ",:) = [ ";
    for (long long int i = 0; i < _nodes; ++i)
    {
      for (long long int j = 0; j < _nodes; ++j)
      {
        os << _pair_nlat[c][i * _nodes + j]->Average() << " ";
      }
    }
    os << "];" << endl
       << "pair_flat(" << c + 1 << ",:) = [ ";
    for (long long int i = 0; i < _nodes; ++i)
    {
      for (long long int j = 0; j < _nodes; ++j)
      {
        os << _pair_flat[c][i * _nodes + j]->Average() << " ";
      }
    }
  }

  double time_delta = (double)(_drain_time - _reset_time);

  os << "];" << endl
     << "sent_packets(" << c + 1 << ",:) = [ ";
  for (long long int d = 0; d < _nodes; ++d)
  {
    os << (double)_sent_packets[c][d] / time_delta << " ";
  }
  os << "];" << endl
     << "accepted_packets(" << c + 1 << ",:) = [ ";
  for (long long int d = 0; d < _nodes; ++d)
  {
    os << (double)_accepted_packets[c][d] / time_delta << " ";
  }
  os << "];" << endl
     << "sent_flits(" << c + 1 << ",:) = [ ";
  for (long long int d = 0; d < _nodes; ++d)
  {
    os << (double)_sent_flits[c][d] / time_delta << " ";
  }
  os << "];" << endl
     << "accepted_flits(" << c + 1 << ",:) = [ ";
  for (long long int d = 0; d < _nodes; ++d)
  {
    os << (double)_accepted_flits[c][d] / time_delta << " ";
  }
  os << "];" << endl
     << "sent_packet_size(" << c + 1 << ",:) = [ ";
  for (long long int d = 0; d < _nodes; ++d)
  {
    os << (double)_sent_flits[c][d] / (double)_sent_packets[c][d] << " ";
  }
  os << "];" << endl
     << "accepted_packet_size(" << c + 1 << ",:) = [ ";
  for (long long int d = 0; d < _nodes; ++d)
  {
    os << (double)_accepted_flits[c][d] / (double)_accepted_packets[c][d] << " ";
  }
  os << "];" << endl;
}

void TrafficManager::_DisplayOverallClassStats(long long int c, ostream &os) const
{

  
  printf("\nTotal number of flits generated = %lld, changed lanes = %lld\n", Generated_flits, Changed_flits);

  cout<<"FlitTimeBreakup,IQ,RC,VC,SA,ST,"<<asyncConfig->queueTicks/Generated_flits <<","<<asyncConfig->routeTicks/Generated_flits <<","<<asyncConfig->vcaTicks/Generated_flits <<","<<asyncConfig->swaTicks/Generated_flits <<","<<asyncConfig->crossbarTicks/Generated_flits <<endl;
  //  printf("\nHistogram of the packet sizes is as follows\n");
  //  for(long long int histo = 0; histo <20; histo++)
  //  	printf("\nNumber of packets with packet size %lld = %lld", histo, Packet_Size_Histogram[histo]);

  os << "Overall minimum packet latency = " << _overall_min_plat[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl
     << "Overall average packet latency = " << _overall_avg_plat[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl
     << "Overall maximum packet latency = " << _overall_max_plat[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall minimum network latency = " << _overall_min_nlat[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall average network latency = " << _overall_avg_nlat[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall maximum network latency = " << _overall_max_nlat[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall minimum flit latency = " << _overall_min_flat[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall average flit latency = " << _overall_avg_flat[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall maximum flit latency = " << _overall_max_flat[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall minimum fragmentation = " << _overall_min_frag[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall average fragmentation = " << _overall_avg_frag[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall maximum fragmentation = " << _overall_max_frag[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall minimum injected packet rate = " << _overall_min_sent_packets[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall average injected packet rate = " << _overall_avg_sent_packets[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall maximum injected packet rate = " << _overall_max_sent_packets[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall minimum accepted packet rate = " << _overall_min_accepted_packets[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall average accepted packet rate = " << _overall_avg_accepted_packets[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall maximum accepted packet rate = " << _overall_max_accepted_packets[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall minimum injected flit rate = " << _overall_min_sent[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall average injected flit rate = " << _overall_avg_sent[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall maximum injected flit rate = " << _overall_max_sent[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall minimum accepted flit rate = " << _overall_min_accepted[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall average accepted flit rate = " << _overall_avg_accepted[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;
  os << "Overall maximum accepted flit rate = " << _overall_max_accepted[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall average injected packet size = " << _overall_avg_sent[c] / _overall_avg_sent_packets[c]
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall average accepted packet size = " << _overall_avg_accepted[c] / _overall_avg_accepted_packets[c]
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall average hops = " << _overall_hop_stats[c] / (double)_total_sims
     << " (" << _total_sims << " samples)" << endl;

  os << "Overall throughput of the network (flits/cycle) = " << _overall_throughput_flits[c] << endl; //Sneha: For throughput display (flits/cycle)

  os << "Overall throughput of the network (packets/cycle) = " << _overall_throughput_packets[c] << endl; //Sneha: For throughput display (packets/cycle)

  os << "CSV Result Header , "
     << "avgFLat,"
     << "avgFInjRate" << endl;
  os << "CSV Result ,"
     << _overall_avg_flat[c] / (double)_total_sims << ","
     << _overall_avg_sent_packets[c] / (double)_total_sims << endl;
}

string TrafficManager::_OverallStatsHeaderCSV() const
{
  ostringstream os;
  os << "min_plat"
     << ',' << "avg_plat"
     << ',' << "max_plat"
     << ',' << "min_nlat"
     << ',' << "avg_nlat"
     << ',' << "max_nlat"
     << ',' << "min_flat"
     << ',' << "avg_flat"
     << ',' << "max_flat"
     << ',' << "min_frag"
     << ',' << "avg_frag"
     << ',' << "max_frag"
     << ',' << "min_sent_packets"
     << ',' << "avg_sent_packets"
     << ',' << "max_sent_packets"
     << ',' << "min_accepted_packets"
     << ',' << "avg_accepted_packets"
     << ',' << "max_accepted_packets"
     << ',' << "min_sent_flits"
     << ',' << "avg_sent_flits"
     << ',' << "max_sent_flits"
     << ',' << "min_accepted_flits"
     << ',' << "avg_accepted_flits"
     << ',' << "max_accepted_flits"
     << ',' << "avg_sent_packet_size"
     << ',' << "avg_accepted_packet_size"
     << ',' << "hops";
#ifdef TRACK_STALLS
  os << ',' << "buffer_busy"
     << ',' << "buffer_conflict"
     << ',' << "buffer_full"
     << ',' << "buffer_reserved"
     << ',' << "crossbar_conflict";
#endif
  return os.str();
}

string TrafficManager::_OverallClassStatsCSV(long long int c) const
{
  ostringstream os;
  os << _overall_min_plat[c] / (double)_total_sims
     << ',' << _overall_avg_plat[c] / (double)_total_sims
     << ',' << _overall_max_plat[c] / (double)_total_sims
     << ',' << _overall_min_nlat[c] / (double)_total_sims
     << ',' << _overall_avg_nlat[c] / (double)_total_sims
     << ',' << _overall_max_nlat[c] / (double)_total_sims
     << ',' << _overall_min_flat[c] / (double)_total_sims
     << ',' << _overall_avg_flat[c] / (double)_total_sims
     << ',' << _overall_max_flat[c] / (double)_total_sims
     << ',' << _overall_min_frag[c] / (double)_total_sims
     << ',' << _overall_avg_frag[c] / (double)_total_sims
     << ',' << _overall_max_frag[c] / (double)_total_sims
     << ',' << _overall_min_sent_packets[c] / (double)_total_sims
     << ',' << _overall_avg_sent_packets[c] / (double)_total_sims
     << ',' << _overall_max_sent_packets[c] / (double)_total_sims
     << ',' << _overall_min_accepted_packets[c] / (double)_total_sims
     << ',' << _overall_avg_accepted_packets[c] / (double)_total_sims
     << ',' << _overall_max_accepted_packets[c] / (double)_total_sims
     << ',' << _overall_min_sent[c] / (double)_total_sims
     << ',' << _overall_avg_sent[c] / (double)_total_sims
     << ',' << _overall_max_sent[c] / (double)_total_sims
     << ',' << _overall_min_accepted[c] / (double)_total_sims
     << ',' << _overall_avg_accepted[c] / (double)_total_sims
     << ',' << _overall_max_accepted[c] / (double)_total_sims
     << ',' << _overall_avg_sent[c] / _overall_avg_sent_packets[c]
     << ',' << _overall_avg_accepted[c] / _overall_avg_accepted_packets[c]
     << ',' << _overall_hop_stats[c] / (double)_total_sims;
  return os.str();
}

//read the watchlist
void TrafficManager::_LoadWatchList(const string &filename)
{
  ifstream watch_list;
  watch_list.open(filename.c_str());

  string line;
  if (watch_list.is_open())
  {
    while (!watch_list.eof())
    {
      getline(watch_list, line);
      if (line != "")
      {
        if (line[0] == 'p')
        {
          _packets_to_watch.insert(atoll(line.c_str() + 1));
        }
        else
        {
          _flits_to_watch.insert(atoll(line.c_str()));
        }
      }
    }
  }
  else
  {
    Error("Unable to open flit watch file: " + filename);
  }
}

// Orion Power Support
void TrafficManager::_ComputeOrionPower()
{
  ofstream PowerOut;

  //char *file_name = _orion_file.c_str();

  for (int i = 0; i < _subnets; i++)
  {
    PowerOut.open(_orion_file.c_str());
    PowerOut << "======== Energy statistics for sub_network at " << PARM_Freq << " Hz =======\n";
    PowerOut.close();
    _net[i]->_PowerReports(0, _orion_file, (_time / ticksTo1ns));
  }

  PowerOut.open(_orion_file.c_str(), std::ofstream::app);

  PowerOut << "\n================= Number of Filts ==================\n";
  PowerOut << " The Total Number of Injected Flits:\t" << g_number_of_injected_flits << "\n";
  PowerOut << " The Total Number of Retired Flits:\t" << g_number_of_retired_flits << "\n";
  PowerOut << "====================================================\n\n";
  PowerOut.close();
}