// $Id$

#include "iq_router.hpp"

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <cassert>
#include <limits>

//Orion Power Support
#include <math.h>

#include "globals.hpp"
#include "random_utils.hpp"
#include "vc.hpp"
#include "routefunc.hpp"
#include "outputset.hpp"
#include "buffer.hpp"
#include "buffer_state.hpp"
#include "roundrobin_arb.hpp"
#include "allocator.hpp"
#include "switch_monitor.hpp"
#include "buffer_monitor.hpp"

IQRouter::IQRouter(Configuration const &config, Module *parent, string const &name, long long int id, long long int inputs, long long int outputs)
    : Router(config, parent, name, id, inputs, outputs), _active(false)
{

  _vcs = config.GetLongInt("num_vcs");

  _vc_busy_when_full = (config.GetLongInt("vc_busy_when_full") > 0);
  _vc_prioritize_empty = (config.GetLongInt("vc_prioritize_empty") > 0);
  _vc_shuffle_requests = (config.GetLongInt("vc_shuffle_requests") > 0);

  _speculative = (config.GetLongInt("speculative") > 0);
  _spec_check_elig = (config.GetLongInt("spec_check_elig") > 0);
  _spec_check_cred = (config.GetLongInt("spec_check_cred") > 0);
  _spec_mask_by_reqs = (config.GetLongInt("spec_mask_by_reqs") > 0);

  //glint
  _routing_delay = asyncConfig->getRoutingDelay(id);
  _vc_alloc_delay = asyncConfig->getVcAllocDelay(id);

  // _routing_delay    = config.GetLongInt( "routing_delay" );
  // _vc_alloc_delay   = config.GetLongInt( "vc_alloc_delay" );
  if (!_vc_alloc_delay)
  {
    Error("VC allocator cannot have zero delay.");
  }

  //glint

  _sw_alloc_delay = asyncConfig->getSwAllocDelay(id);
  // _sw_alloc_delay   = config.GetLongInt( "sw_alloc_delay" );
  if (!_sw_alloc_delay)
  {
    Error("Switch allocator cannot have zero delay.");
  }

  // Routing
  string const rf = config.GetStr("routing_function") + "_" + config.GetStr("topology");
  map<string, tRoutingFunction>::const_iterator rf_iter = gRoutingFunctionMap.find(rf);
  if (rf_iter == gRoutingFunctionMap.end())
  {
    Error("Invalid routing function: " + rf);
  }
  _rf = rf_iter->second;

  // Alloc VC's
  _buf.resize(_inputs);
  for (long long int i = 0; i < _inputs; ++i)
  {
    ostringstream module_name;
    module_name << "buf_" << i;
    _buf[i] = new Buffer(config, _outputs, this, module_name.str());
    module_name.str("");
  }

  // Alloc next VCs' buffer state
  _next_buf.resize(_outputs);
  for (long long int j = 0; j < _outputs; ++j)
  {
    ostringstream module_name;
    module_name << "next_vc_o" << j;
    _next_buf[j] = new BufferState(config, this, module_name.str());
    module_name.str("");
  }

  // Alloc allocators
  string vc_alloc_type = config.GetStr("vc_allocator");
  if (vc_alloc_type == "piggyback")
  {
    if (!_speculative)
    {
      Error("Piggyback VC allocation requires speculative switch allocation to be enabled.");
    }
    _vc_allocator = NULL;
    _vc_rr_offset.resize(_outputs * _classes, -1);
  }
  else
  {
    _vc_allocator = Allocator::NewAllocator(this, "vc_allocator", vc_alloc_type, _vcs * _inputs, _vcs * _outputs);
    if (!_vc_allocator)
    {
      Error("Unknown vc_allocator type: " + vc_alloc_type);
    }
  }

  string sw_alloc_type = config.GetStr("sw_allocator");
  _sw_allocator = Allocator::NewAllocator(this, "sw_allocator", sw_alloc_type, _inputs * _input_speedup, _outputs * _output_speedup);

  if (!_sw_allocator)
  {
    Error("Unknown sw_allocator type: " + sw_alloc_type);
  }

  string spec_sw_alloc_type = config.GetStr("spec_sw_allocator");
  if (_speculative && (spec_sw_alloc_type != "prio"))
  {
    _spec_sw_allocator = Allocator::NewAllocator(this, "spec_sw_allocator", spec_sw_alloc_type, _inputs * _input_speedup, _outputs * _output_speedup);
    if (!_spec_sw_allocator)
    {
      Error("Unknown spec_sw_allocator type: " + spec_sw_alloc_type);
    }
  }
  else
  {
    _spec_sw_allocator = NULL;
  }

  _sw_rr_offset.resize(_inputs * _input_speedup);
  for (long long int i = 0; i < _inputs * _input_speedup; ++i)
    _sw_rr_offset[i] = i % _input_speedup;

  _noq = config.GetLongInt("noq") > 0;
  if (_noq)
  {
    if (_routing_delay)
    {
      Error("NOQ requires lookahead routing to be enabled.");
    }
    if (_vcs < _outputs)
    {
      Error("NOQ requires at least as many VCs as router outputs.");
    }
  }
  _noq_next_output_port.resize(_inputs, vector<long long int>(_vcs, -1));
  _noq_next_vc_start.resize(_inputs, vector<long long int>(_vcs, -1));
  _noq_next_vc_end.resize(_inputs, vector<long long int>(_vcs, -1));

  // Output queues
  _output_buffer_size = config.GetLongInt("output_buffer_size");
  _output_buffer.resize(_outputs);
  _credit_buffer.resize(_inputs);

  // Switch configuration (when held for multiple cycles)
  _hold_switch_for_packet = (config.GetLongInt("hold_switch_for_packet") > 0);
  _switch_hold_in.resize(_inputs * _input_speedup, -1);
  _switch_hold_out.resize(_outputs * _output_speedup, -1);
  _switch_hold_vc.resize(_inputs * _input_speedup, -1);

  _bufferMonitor = new BufferMonitor(inputs, _classes);
  _switchMonitor = new SwitchMonitor(inputs, outputs, _classes);
}

IQRouter::~IQRouter()
{
  if (gPrintActivity)
  {
    cout << Name() << ".bufferMonitor:" << endl;
    cout << *_bufferMonitor << endl;

    cout << Name() << ".switchMonitor:" << endl;
    cout << "Inputs=" << _inputs;
    cout << "Outputs=" << _outputs;
    cout << *_switchMonitor << endl;
  }

  for (long long int i = 0; i < _inputs; ++i)
    delete _buf[i];

  for (long long int j = 0; j < _outputs; ++j)
    delete _next_buf[j];

  delete _vc_allocator;
  delete _sw_allocator;
  if (_spec_sw_allocator)
    delete _spec_sw_allocator;

  delete _bufferMonitor;
  delete _switchMonitor;
}

void IQRouter::AddOutputChannel(FlitChannel *channel, CreditChannel *backchannel)
{
  //long long int alloc_delay = _speculative ? max(_vc_alloc_delay, _sw_alloc_delay) : (_vc_alloc_delay + _sw_alloc_delay);
  //long long int min_latency = 1 + _crossbar_delay + channel->GetLatency() + _routing_delay + alloc_delay + backchannel->GetLatency() + _credit_delay;
  long long int min_latency = 1;
  _next_buf[_output_channels.size()]->SetMinLatency(min_latency);
  Router::AddOutputChannel(channel, backchannel);
}

void IQRouter::ReadInputs()
{
  bool have_flits = _ReceiveFlits();
  bool have_credits = _ReceiveCredits();
  _active = _active || have_flits || have_credits;
}

void IQRouter::_InternalStep()
{
  //==========================gating===================================
  if (asyncConfig->doGating)
  {
    if (asyncConfig->gatingMode == 0)
    {
      if (!_active)
      {
        asyncConfig->idleTicksCounter[_id]++;
        if (asyncConfig->idleTicksCounter[_id] >= asyncConfig->sleepThreshold)
        {
          asyncConfig->gatedTicksCounter[_id]++;
        }
      }
      else
      {
        if (asyncConfig->idleTicksCounter[_id] > 0)
        {

          if (asyncConfig->idleTicksCounter[_id] >= asyncConfig->breakEvenThreshold)
          {

            asyncConfig->viableIdleTicksSum[_id] = asyncConfig->viableIdleTicksSum[_id] + asyncConfig->idleTicksCounter[_id] - asyncConfig->breakEvenThreshold;
            asyncConfig->viableIdleTimesSum[_id]++;
          }

          asyncConfig->idleTicksCounter[_id] = 0;

          if (asyncConfig->gatedTicksCounter[_id] > 0)
          {
            asyncConfig->viableGatedTicksSum[_id] = asyncConfig->viableGatedTicksSum[_id] + asyncConfig->gatedTicksCounter[_id] - asyncConfig->breakEvenThreshold;
            asyncConfig->gatedTicksCounter[_id] = 0;
            asyncConfig->gatedTimesSum[_id]++;
          }
        }
      }
    }
    else if (asyncConfig->gatingMode == 1)
    {
      if (!_active)
      {
        //deal with oracular finding all windows of idle times
        asyncConfig->idleTicksCounter[_id]++;

        //deals with gating depending on threshold set counting all idle ticks about the sleep threshold set for that router
        if (asyncConfig->idleTicksCounter[_id] >= asyncConfig->routerSleepThreshold[_id])
        {
          asyncConfig->gatedTicksCounter[_id]++;
        }
      }
      else
      {
        //seeing if there was idle time
        if (asyncConfig->idleTicksCounter[_id] > 0)
        {
          long long int flagSpuriousCase = 0;

          //adding the idle window to the oracular gated tick if it was indeed a window that could be gated.

          if (asyncConfig->idleTicksCounter[_id] >= asyncConfig->breakEvenThreshold)
          {
            //there is an interesting scenario here - because of large sleepthreshold window there could be op missing

            if ((asyncConfig->idleTicksCounter[_id] - asyncConfig->breakEvenThreshold) < asyncConfig->routerSleepThreshold[_id])
            {
              flagSpuriousCase = 1;
            }

            asyncConfig->viableIdleTicksSum[_id] = asyncConfig->viableIdleTicksSum[_id] + asyncConfig->idleTicksCounter[_id] - asyncConfig->breakEvenThreshold;

            asyncConfig->viableIdleTimesSum[_id]++;
          }
          //reseting the  oracular idle tick counter
          asyncConfig->idleTicksCounter[_id] = 0;

          //seeing if the previous window was gated, satisfying the threshold
          if (asyncConfig->gatedTicksCounter[_id] > 0)
          {
            //adding the effective time of gating.
            //logic to handle dynamic sleep threshold
            if (asyncConfig->gatedTicksCounter[_id] > asyncConfig->breakEvenThreshold)
            {
              //successfull gating
              if (asyncConfig->routerSleepThreshold[_id] <= asyncConfig->sleepThresholdStep)
              {
                //do nothing..
              }
              else
              {
                //decrease the sleepThreshold
                asyncConfig->routerSleepThreshold[_id] = asyncConfig->routerSleepThreshold[_id] - asyncConfig->sleepThresholdStep;
              }
            }
            else
            { //failed gating
              if (flagSpuriousCase == 1)
              {
                 asyncConfig->routerSleepThreshold[_id] = asyncConfig->sleepThresholdStep;
              }
              else
              {
                if (asyncConfig->routerSleepThreshold[_id] >= asyncConfig->breakEvenThreshold)
                {
                  //do nothing..
                }
                else
                {
                  //increase the sleepThreshold
                  asyncConfig->routerSleepThreshold[_id] = asyncConfig->routerSleepThreshold[_id] + asyncConfig->sleepThresholdStep;
                }
              }
            }

            //stat collection
            asyncConfig->viableGatedTicksSum[_id] = asyncConfig->viableGatedTicksSum[_id] + asyncConfig->gatedTicksCounter[_id] - asyncConfig->breakEvenThreshold;
            //reseting
            asyncConfig->gatedTicksCounter[_id] = 0;
            asyncConfig->gatedTimesSum[_id]++;
          }
        }
      }
    }
  }

  //===============================end gating==========================

  if (!_active)
  {
    return;
  }
  //_proc_credits
  //_route_vcs
  //_sw_hold_vcs
  //_sw_alloc_vcs
  //_crossbar_flits
  //_in_queue_flits
  //_vc_alloc_vcs

  asyncConfig->queueTicks += _in_queue_flits.size();
  asyncConfig->routeTicks += _route_vcs.size();
  asyncConfig->vcaTicks += _vc_alloc_vcs.size();
  asyncConfig->swaTicks += _sw_alloc_vcs.size();
  asyncConfig->crossbarTicks += _crossbar_flits.size();

  _InputQueuing();
  bool activity = !_proc_credits.empty();

  if (!_route_vcs.empty())
    _RouteEvaluate();
  if (_vc_allocator)
  {
    _vc_allocator->Clear();
    if (!_vc_alloc_vcs.empty())
      _VCAllocEvaluate();
  }

  if (_hold_switch_for_packet)
  {
    if (!_sw_hold_vcs.empty())
      _SWHoldEvaluate();
  }
  _sw_allocator->Clear();
  if (_spec_sw_allocator)
    _spec_sw_allocator->Clear();
  if (!_sw_alloc_vcs.empty())
    _SWAllocEvaluate();
  if (!_crossbar_flits.empty())
    _SwitchEvaluate();

  if (!_route_vcs.empty())
  {
    _RouteUpdate();
    activity = activity || !_route_vcs.empty();
  }
  if (!_vc_alloc_vcs.empty())
  {
    _VCAllocUpdate();
    activity = activity || !_vc_alloc_vcs.empty();
  }
  if (_hold_switch_for_packet)
  {
    if (!_sw_hold_vcs.empty())
    {
      _SWHoldUpdate();
      activity = activity || !_sw_hold_vcs.empty();
    }
  }
  if (!_sw_alloc_vcs.empty())
  {
    _SWAllocUpdate();
    activity = activity || !_sw_alloc_vcs.empty();
  }
  if (!_crossbar_flits.empty())
  {
    _SwitchUpdate();
    activity = activity || !_crossbar_flits.empty();
  }

  _active = activity;
  _OutputQueuing();
  _bufferMonitor->cycle();
  _switchMonitor->cycle();
}

void IQRouter::WriteOutputs()
{
  _SendFlits();
  _SendCredits();
}

//------------------------------------------------------------------------------
// read inputs
//------------------------------------------------------------------------------

bool IQRouter::_ReceiveFlits()
{
  bool activity = false;
  for (long long int input = 0; input < _inputs; ++input)
  {
    Flit *const f = _input_channels[input]->Receive();
    if (f)
    {
      _in_queue_flits.insert(make_pair(input, f));
      activity = true;
      //      printf("\nTime:,%lld,%lld,[%lld][%lld],ReceiveFlit,%lld\n", GetSimTime(), this->GetID(), f->id, f->pid, f->vc); //Sneha
    }
  }
  return activity;
}

bool IQRouter::_ReceiveCredits()
{
  bool activity = false;
  for (long long int output = 0; output < _outputs; ++output)
  {
    Credit *const c = _output_credits[output]->Receive();
    if (c)
    {
      //glint
      _proc_credits.push_back(make_pair(GetSimTime() + asyncConfig->getCreditDelay(_id), make_pair(c, output)));
      // _proc_credits.push_back(make_pair(GetSimTime() + _credit_delay, make_pair(c, output)));
      activity = true;
    }
  }
  return activity;
}

//------------------------------------------------------------------------------
// input queuing
//------------------------------------------------------------------------------

void IQRouter::_InputQueuing()
{
  for (map<long long int, Flit *>::const_iterator iter = _in_queue_flits.begin(); iter != _in_queue_flits.end(); ++iter)
  {
    long long int const input = iter->first;
    Flit *const f = iter->second;
    long long int const vc = f->vc;
    Buffer *const cur_buf = _buf[input];
    cur_buf->AddFlit(vc, f);
    // MoRi
    /* added by [a.mazloumi and modarressi] */
    _number_of_crossed_flits++;
    if (f->head)
      _number_of_crossed_headerFlits++;
    u_char x[8] = "abcdefg";
    u_char y[8] = "pqrstuv";
    u_char z[8] = "abcdtuv";
    SIM_buf_power_data_write(&(_orion_router_info.in_buf_info), &(_orion_router_power.in_buf), x, y, z);
    _number_of_calls_of_power_functions++;
    /* end [a.mazloumi and modarressi]codes */
    _bufferMonitor->write(input, f);
    //    printf("\nTime:,%lld,%lld,[%lld][%lld],InputQueueing,%lld\n", GetSimTime(), this->GetID(), f->id, f->pid, f->vc); //Sneha
    if (cur_buf->GetState(vc) == VC::idle)
    {
      if (_routing_delay)
      {
        cur_buf->SetState(vc, VC::routing);
        _route_vcs.push_back(make_pair(-1, make_pair(input, vc)));
      }
      else
      {
        cur_buf->SetRouteSet(vc, &f->la_route_set);
        cur_buf->SetState(vc, VC::vc_alloc);
        if (_speculative)
        {
          _sw_alloc_vcs.push_back(make_pair(-1, make_pair(make_pair(input, vc), -1)));
        }
        if (_vc_allocator)
        {
          _vc_alloc_vcs.push_back(make_pair(-1, make_pair(make_pair(input, vc), -1)));
        }
        if (_noq)
        {
          _UpdateNOQ(input, vc, f);
        }
      }
    }
    else if ((cur_buf->GetState(vc) == VC::active) && (cur_buf->FrontFlit(vc) == f))
    {
      if (_switch_hold_vc[input * _input_speedup + vc % _input_speedup] == vc)
      {
        _sw_hold_vcs.push_back(make_pair(-1, make_pair(make_pair(input, vc), -1)));
      }
      else
      {
        _sw_alloc_vcs.push_back(make_pair(-1, make_pair(make_pair(input, vc), -1)));
      }
    }
  }
  _in_queue_flits.clear();

  while (!_proc_credits.empty())
  {
    pair<long long int, pair<Credit *, long long int>> const &item = _proc_credits.front();
    long long int const time = item.first;
    // cout<<"The time and time in packet is"<<GetSimTime()<<","<<time<<endl;
    if (GetSimTime() < time)
    {
      break;
    }

    Credit *const c = item.second.first;
    long long int const output = item.second.second;
    BufferState *const dest_buf = _next_buf[output];
    dest_buf->ProcessCredit(c);
    c->Free();
    _proc_credits.pop_front();
  }
}

//------------------------------------------------------------------------------
// routing
//------------------------------------------------------------------------------

void IQRouter::_RouteEvaluate()
{
  for (deque<pair<long long int, pair<long long int, long long int>>>::iterator iter = _route_vcs.begin(); iter != _route_vcs.end(); ++iter)
  {
    long long int const time = iter->first;
    if (time >= 0)
    {
      break;
    }
    iter->first = GetSimTime() + asyncConfig->getRoutingDelay(_id) - 1;
  }
}

void IQRouter::_RouteUpdate()
{
  while (!_route_vcs.empty())
  {
    pair<long long int, pair<long long int, long long int>> const &item = _route_vcs.front();
    long long int const time = item.first;
    if ((time < 0) || (GetSimTime() < time))
    {
      break;
    }
    long long int const input = item.second.first;
    long long int const vc = item.second.second;
    Buffer *const cur_buf = _buf[input];
    Flit *const f = cur_buf->FrontFlit(vc);
    cur_buf->Route(vc, _rf, this, f, input);
    cur_buf->SetState(vc, VC::vc_alloc);
    if (_speculative)
    {
      _sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second, -1)));
    }
    if (_vc_allocator)
    {
      _vc_alloc_vcs.push_back(make_pair(-1, make_pair(item.second, -1)));
    }
    // NOTE: No need to handle NOQ here, as it requires lookahead routing!
    _route_vcs.pop_front();
  }
}

//------------------------------------------------------------------------------
// VC allocation
//------------------------------------------------------------------------------

void IQRouter::_VCAllocEvaluate()
{
  // MoRi
  /* added by a.mazloumi */
  unsigned int _orion_current_vc_requset[_outputs * _vcs];
  for (int i = 0; i < _outputs * _vcs; i++)
    _orion_current_vc_requset[i] = 0;
  int _orion_current_vc_grant = 0;
  /* end of a.mazloumicodes */

  for (deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>>::iterator iter = _vc_alloc_vcs.begin(); iter != _vc_alloc_vcs.end(); ++iter)
  {
    long long int const time = iter->first;
    if (time >= 0)
    {
      break;
    }
    long long int const input = iter->second.first.first;
    long long int const vc = iter->second.first.second;
    Buffer const *const cur_buf = _buf[input];
    OutputSet const *const route_set = cur_buf->GetRouteSet(vc);
    long long int const out_priority = cur_buf->GetPriority(vc);
    set<OutputSet::sSetElement> const setlist = route_set->GetSet();
    bool elig = false;
    bool cred = false;
    bool reserved = false;
    for (set<OutputSet::sSetElement>::const_iterator iset = setlist.begin(); iset != setlist.end(); ++iset)
    {
      long long int const out_port = iset->output_port;
      BufferState const *const dest_buf = _next_buf[out_port];
      long long int vc_start;
      long long int vc_end;
      if (_noq && _noq_next_output_port[input][vc] >= 0)
      {
        vc_start = _noq_next_vc_start[input][vc];
        vc_end = _noq_next_vc_end[input][vc];
      }
      else
      {
        vc_start = iset->vc_start;
        vc_end = iset->vc_end;
      }

      for (long long int out_vc = vc_start; out_vc <= vc_end; ++out_vc)
      {
        long long int in_priority = iset->pri;
        if (_vc_prioritize_empty && !dest_buf->IsEmptyFor(out_vc))
        {
          in_priority += numeric_limits<long long int>::min();
        }

        // On the input input side, a VC might request several output VCs.
        // These VCs can be prioritized by the routing function, and this is
        // reflected in "in_priority". On the output side, if multiple VCs are
        // requesting the same output VC, the priority of VCs is based on the
        // actual packet priorities, which is reflected in "out_priority".

        if (!dest_buf->IsAvailableFor(out_vc))
        {
        }
        else
        {
          elig = true;
          if (_vc_busy_when_full && dest_buf->IsFullFor(out_vc))
          {
            reserved |= !dest_buf->IsFull();
          }
          else
          {
            cred = true;
            long long int const input_and_vc = _vc_shuffle_requests ? (vc * _inputs + input) : (input * _vcs + vc);
            _vc_allocator->AddRequest(input_and_vc, out_port * _vcs + out_vc, 0, in_priority, out_priority);
            //MoRi
            /* added by [a.mazloumi and modarressi] */
            _orion_current_vc_requset[out_port * _vcs + out_vc] = _orion_current_vc_requset[out_port * _vcs + out_vc] | ((unsigned int)pow(2, input_and_vc));
            /* end of [a.mazloumi and modarressi] */
          }
        }
      }
    }
    if (!elig)
    {
      iter->second.second = STALL_BUFFER_BUSY;
    }
    else if (_vc_busy_when_full && !cred)
    {
      iter->second.second = reserved ? STALL_BUFFER_RESERVED : STALL_BUFFER_FULL;
    }
  }

  _vc_allocator->Allocate();

  for (deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>>::iterator iter = _vc_alloc_vcs.begin(); iter != _vc_alloc_vcs.end(); ++iter)
  {
    long long int const time = iter->first;
    if (time >= 0)
    {
      break;
    }
    iter->first = GetSimTime() + asyncConfig->getVcAllocDelay(_id) - 1;
    long long int const input = iter->second.first.first;
    long long int const vc = iter->second.first.second;
    if (iter->second.second < -1)
    {
      continue;
    }
    //Buffer const *const cur_buf = _buf[input];
    //Flit *f = cur_buf->FrontFlit(vc);
    long long int const input_and_vc = _vc_shuffle_requests ? (vc * _inputs + input) : (input * _vcs + vc);
    long long int const output_and_vc = _vc_allocator->OutputAssigned(input_and_vc);
    if (output_and_vc >= 0)
    {
      //long long int const match_vc = output_and_vc % _vcs;

      // MoRi
      /* added by [a.mazloumi and modarressi] */
      _orion_current_vc_grant = input_and_vc;

      if (!(PARM_v_channel < 2 && PARM_v_class < 2))
        SIM_arbiter_record(&(_orion_router_power.vc_out_arb), _orion_current_vc_requset[output_and_vc], _orion_last_vc_reguest[output_and_vc], _orion_current_vc_grant, _orion_last_vc_grant[output_and_vc]);
      _number_of_calls_of_power_functions++;
      _orion_last_vc_reguest[output_and_vc] = _orion_current_vc_requset[output_and_vc];
      _orion_last_vc_grant[output_and_vc] = _orion_current_vc_grant;
      /* end of [a.mazloumi and modarressi] */
      iter->second.second = output_and_vc;
    }
    else
    {
      iter->second.second = STALL_BUFFER_CONFLICT;
    }
  }

  if (_vc_alloc_delay <= 1)
  {
    return;
  }

  for (deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>>::iterator iter = _vc_alloc_vcs.begin(); iter != _vc_alloc_vcs.end(); ++iter)
  {
    long long int const time = iter->first;
    if (GetSimTime() < time)
    {
      break;
    }
    long long int const output_and_vc = iter->second.second;
    if (output_and_vc >= 0)
    {
      long long int const match_output = output_and_vc / _vcs;
      long long int const match_vc = output_and_vc % _vcs;
      BufferState const *const dest_buf = _next_buf[match_output];
      //long long int const input = iter->second.first.first;
      if (!dest_buf->IsAvailableFor(match_vc))
      {
        iter->second.second = STALL_BUFFER_BUSY;
      }
      else if (_vc_busy_when_full && dest_buf->IsFullFor(match_vc))
      {
        iter->second.second = dest_buf->IsFull() ? STALL_BUFFER_FULL : STALL_BUFFER_RESERVED;
      }
    }
  }
}

void IQRouter::_VCAllocUpdate()
{
  while (!_vc_alloc_vcs.empty())
  {
    pair<long long int, pair<pair<long long int, long long int>, long long int>> const &item = _vc_alloc_vcs.front();
    long long int const time = item.first;
    if ((time < 0) || (GetSimTime() < time))
    {
      break;
    }
    long long int const input = item.second.first.first;
    long long int const vc = item.second.first.second;
    Buffer *const cur_buf = _buf[input];
    //Flit const *const f = cur_buf->FrontFlit(vc);
    long long int const output_and_vc = item.second.second;
    if (output_and_vc >= 0)
    {
      long long int const match_output = output_and_vc / _vcs;
      long long int const match_vc = output_and_vc % _vcs;
      BufferState *const dest_buf = _next_buf[match_output];
      dest_buf->TakeBuffer(match_vc, input * _vcs + vc);
      cur_buf->SetOutput(vc, match_output, match_vc);
      cur_buf->SetState(vc, VC::active);
      if (!_speculative)
      {
        _sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
      }
    }
    else
    {
      _vc_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
    }
    _vc_alloc_vcs.pop_front();
    //    printf("\nTime:,%lld,%lld,[%lld][%lld],VCUpdate,%lld\n", GetSimTime(), this->GetID(), f->id, f->pid, f->vc); //Sneha
  }
}

//------------------------------------------------------------------------------
// switch holding
//------------------------------------------------------------------------------

void IQRouter::_SWHoldEvaluate()
{
  //printf("\n Looks like we come here too\n"); //Sneha
  for (deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>>::iterator iter = _sw_hold_vcs.begin(); iter != _sw_hold_vcs.end(); ++iter)
  {
    long long int const time = iter->first;
    if (time >= 0)
    {
      break;
    }
    iter->first = GetSimTime();
    long long int const input = iter->second.first.first;
    long long int const vc = iter->second.first.second;
    Buffer const *const cur_buf = _buf[input];
    //Flit const *const f = cur_buf->FrontFlit(vc);
    //long long int const expanded_input = input * _input_speedup + vc % _input_speedup;
    long long int const match_port = cur_buf->GetOutputPort(vc);
    long long int const match_vc = cur_buf->GetOutputVC(vc);
    long long int const expanded_output = match_port * _output_speedup + input % _output_speedup;
    BufferState const *const dest_buf = _next_buf[match_port];
    if (dest_buf->IsFullFor(match_vc))
    {
      iter->second.second = dest_buf->IsFull() ? STALL_BUFFER_FULL : STALL_BUFFER_RESERVED;
    }
    else
    {
      iter->second.second = expanded_output;
    }
  }
}

void IQRouter::_SWHoldUpdate()
{
  //printf("\n Looks like we come here too\n"); //Sneha
  while (!_sw_hold_vcs.empty())
  {
    pair<long long int, pair<pair<long long int, long long int>, long long int>> const &item = _sw_hold_vcs.front();
    long long int const time = item.first;
    if (time < 0)
    {
      break;
    }
    long long int const input = item.second.first.first;
    long long int const vc = item.second.first.second;
    Buffer *const cur_buf = _buf[input];
    Flit *const f = cur_buf->FrontFlit(vc);
    long long int const expanded_input = input * _input_speedup + vc % _input_speedup;
    long long int const expanded_output = item.second.second;
    if (expanded_output >= 0 && (_output_buffer_size == -1 || _output_buffer[expanded_output].size() < size_t(_output_buffer_size)))
    {
      long long int const output = expanded_output / _output_speedup;
      long long int const match_vc = cur_buf->GetOutputVC(vc);
      BufferState *const dest_buf = _next_buf[output];
      cur_buf->RemoveFlit(vc);
      // MoRi
      /* added by [a.mazloumi and modarressi] */
      SIM_buf_power_data_read(&(_orion_router_info.in_buf_info), &(_orion_router_power.in_buf), 0xffff0000);
      _number_of_calls_of_power_functions++;
      /* end [a.mazloumi and modarressi] codes */
      _bufferMonitor->read(input, f);
      f->hops++;
      f->vc = match_vc;
      if (!_routing_delay && f->head)
      {
        const FlitChannel *channel = _output_channels[output];
        const Router *router = channel->GetSink();
        if (router)
        {
          if (_noq)
          {
            long long int next_output_port = _noq_next_output_port[input][vc];
            _noq_next_output_port[input][vc] = -1;
            long long int next_vc_start = _noq_next_vc_start[input][vc];
            _noq_next_vc_start[input][vc] = -1;
            long long int next_vc_end = _noq_next_vc_end[input][vc];
            _noq_next_vc_end[input][vc] = -1;
            f->la_route_set.Clear();
            f->la_route_set.AddRange(next_output_port, next_vc_start, next_vc_end);
          }
          else
          {
            long long int in_channel = channel->GetSinkPort();
            _rf(router, f, in_channel, &f->la_route_set, false);
          }
        }
        else
        {
          f->la_route_set.Clear();
        }
      }
      dest_buf->SendingFlit(f);

      // MoRi
      /* added by [a.mazloumi and modarressi]@ */
      SIM_crossbar_record(&(_orion_router_power.crossbar), 1, 0xffffffff, 0x0000ffff, 0, 0); //sending to crossbar
      _number_of_calls_of_power_functions++;
      /* end [a.mazloumi and modarressi]@ codes */
      _crossbar_flits.push_back(make_pair(-1, make_pair(f, make_pair(expanded_input, expanded_output))));
      if (_out_queue_credits.count(input) == 0)
      {
        _out_queue_credits.insert(make_pair(input, Credit::New()));
      }
      _out_queue_credits.find(input)->second->vc.insert(vc);

      if (cur_buf->Empty(vc))
      {
        _switch_hold_vc[expanded_input] = -1;
        _switch_hold_in[expanded_input] = -1;
        _switch_hold_out[expanded_output] = -1;
        if (f->tail)
        {
          cur_buf->SetState(vc, VC::idle);
        }
      }
      else
      {
        Flit *const nf = cur_buf->FrontFlit(vc);
        if (f->tail)
        {
          _switch_hold_vc[expanded_input] = -1;
          _switch_hold_in[expanded_input] = -1;
          _switch_hold_out[expanded_output] = -1;
          if (_routing_delay)
          {
            cur_buf->SetState(vc, VC::routing);
            _route_vcs.push_back(make_pair(-1, item.second.first));
          }
          else
          {
            cur_buf->SetRouteSet(vc, &nf->la_route_set);
            cur_buf->SetState(vc, VC::vc_alloc);
            if (_speculative)
            {
              _sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
            }
            if (_vc_allocator)
            {
              _vc_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
            }
            if (_noq)
            {
              _UpdateNOQ(input, vc, nf);
            }
          }
        }
        else
        {
          _sw_hold_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
        }
      }
    }
    else
    {
      //when internal speedup >1.0, the buffer stall stats may not be accruate
      long long int const held_expanded_output = _switch_hold_in[expanded_input];
      _switch_hold_vc[expanded_input] = -1;
      _switch_hold_in[expanded_input] = -1;
      _switch_hold_out[held_expanded_output] = -1;
      _sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
    }
    _sw_hold_vcs.pop_front();
  }
}

//------------------------------------------------------------------------------
// switch allocation
//------------------------------------------------------------------------------

bool IQRouter::_SWAllocAddReq(unsigned int *_orion_current_sw_requset, long long int input, long long int vc, long long int output)
{
  // When input_speedup > 1, the virtual channel buffers are interleaved to
  // create multiple input ports to the switch. Similarily, the output ports
  // are interleaved based on their originating input when output_speedup > 1.
  long long int const expanded_input = input * _input_speedup + vc % _input_speedup;
  long long int const expanded_output = output * _output_speedup + input % _output_speedup;
  Buffer const *const cur_buf = _buf[input];
  //Flit const *const f = cur_buf->FrontFlit(vc);
  if ((_switch_hold_in[expanded_input] < 0) && (_switch_hold_out[expanded_output] < 0))
  {
    Allocator *allocator = _sw_allocator;
    long long int prio = cur_buf->GetPriority(vc);
    if (_speculative && (cur_buf->GetState(vc) == VC::vc_alloc))
    {
      if (_spec_sw_allocator)
      {
        allocator = _spec_sw_allocator;
      }
      else
      {
        prio += numeric_limits<long long int>::min();
      }
    }
    Allocator::sRequest req;
    if (allocator->ReadRequest(req, expanded_input, expanded_output))
    {
      if (RoundRobinArbiter::Supersedes(vc, prio, req.label, req.in_pri, _sw_rr_offset[expanded_input], _vcs))
      {
        allocator->RemoveRequest(expanded_input, expanded_output, req.label);
        allocator->AddRequest(expanded_input, expanded_output, vc, prio, prio);

        // MoRi
        /* added by [a.mazloumi and modarressi]@ */
        _orion_current_sw_requset[output] = _orion_current_sw_requset[output] | ((unsigned int)pow(2, (expanded_input)));
        /* end [a.mazloumi and modarressi]@ */
        return true;
      }
      return false;
    }
    allocator->AddRequest(expanded_input, expanded_output, vc, prio, prio);

    // MoRi
    /* added by [a.mazloumi and modarressi] */
    _orion_current_sw_requset[output] = _orion_current_sw_requset[output] | ((unsigned int)pow(2, (expanded_input)));
    /* end [a.mazloumi and modarressi] */

    return true;
  }
  return false;
}

void IQRouter::_SWAllocEvaluate()
{
  // MoRi
  /* added by a.mazloumi */
  unsigned int _orion_current_sw_requset[_outputs];
  for (int i = 0; i < _outputs; i++)
    _orion_current_sw_requset[i] = 0;
  int _orion_current_sw_grant = 0;
  /* end of a.mazloum*/
  bool watched = false;

  for (deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>>::iterator iter = _sw_alloc_vcs.begin(); iter != _sw_alloc_vcs.end(); ++iter)
  {
    long long int const time = iter->first;
    if (time >= 0)
    {
      break;
    }
    long long int const input = iter->second.first.first;
    long long int const vc = iter->second.first.second;
    Buffer const *const cur_buf = _buf[input];
    Flit const *const f = cur_buf->FrontFlit(vc);
    if (cur_buf->GetState(vc) == VC::active)
    {
      long long int const dest_output = cur_buf->GetOutputPort(vc);
      long long int const dest_vc = cur_buf->GetOutputVC(vc);
      BufferState const *const dest_buf = _next_buf[dest_output];
      if (dest_buf->IsFullFor(dest_vc) || (_output_buffer_size != -1 && _output_buffer[dest_output].size() >= (size_t)(_output_buffer_size)))
      {
        iter->second.second = dest_buf->IsFull() ? STALL_BUFFER_FULL : STALL_BUFFER_RESERVED;
        continue;
      }

      //glint

      bool const requested = _SWAllocAddReq(_orion_current_sw_requset, input, vc, dest_output);
      watched |= requested && f->watch;
      continue;
    }
    // The following models the speculative VC allocation aspects of the pipeline. An input VC with a request in for an egress virtual channel
    // will also speculatively bid for the switch regardless of whether the VC  allocation succeeds.

    OutputSet const *const route_set = cur_buf->GetRouteSet(vc);
    set<OutputSet::sSetElement> const setlist = route_set->GetSet();

    for (set<OutputSet::sSetElement>::const_iterator iset = setlist.begin(); iset != setlist.end(); ++iset)
    {
      long long int const dest_output = iset->output_port;
      // for lower levels of speculation, ignore credit availability and always issue requests for all output ports in route set

      BufferState const *const dest_buf = _next_buf[dest_output];
      bool elig = false;
      bool cred = false;

      if (_spec_check_elig)
      {
        // for higher levels of speculation, check if at least one suitable VC is available at the current output
        long long int vc_start;
        long long int vc_end;

        if (_noq && _noq_next_output_port[input][vc] >= 0)
        {
          vc_start = _noq_next_vc_start[input][vc];
          vc_end = _noq_next_vc_end[input][vc];
        }
        else
        {
          vc_start = iset->vc_start;
          vc_end = iset->vc_end;
        }
        for (long long int dest_vc = vc_start; dest_vc <= vc_end; ++dest_vc)
        {
          if (dest_buf->IsAvailableFor(dest_vc) && (_output_buffer_size == -1 || _output_buffer[dest_output].size() < (size_t)(_output_buffer_size)))
          {
            elig = true;
            if (!_spec_check_cred || !dest_buf->IsFullFor(dest_vc))
            {
              cred = true;
              break;
            }
          }
        }
      }

      if (_spec_check_elig && !elig)
      {
        iter->second.second = STALL_BUFFER_BUSY;
      }
      else if (_spec_check_cred && !cred)
      {
        iter->second.second = dest_buf->IsFull() ? STALL_BUFFER_FULL : STALL_BUFFER_RESERVED;
      }
      else
      {
        bool const requested = _SWAllocAddReq(_orion_current_sw_requset, input, vc, dest_output);
        watched |= requested && f->watch;
      }
    }
  }
  _sw_allocator->Allocate();

  if (_spec_sw_allocator)
    _spec_sw_allocator->Allocate();

  for (deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>>::iterator iter = _sw_alloc_vcs.begin(); iter != _sw_alloc_vcs.end(); ++iter)
  {
    long long int const time = iter->first;
    if (time >= 0)
    {
      break;
    }
    //here
    long long int const input = iter->second.first.first;
    long long int const vc = iter->second.first.second;
    Buffer const *const cur_buf = _buf[input];
    long long dest = cur_buf->GetOutputPort(vc);

    iter->first = GetSimTime() + asyncConfig->getSwAllocDelay(_id, dest) - 1;

    if (iter->second.second < -1)
    {
      continue;
    }
    //Buffer const *const cur_buf = _buf[input];
    //Flit const *const f = cur_buf->FrontFlit(vc);
    long long int const expanded_input = input * _input_speedup + vc % _input_speedup;
    long long int expanded_output = _sw_allocator->OutputAssigned(expanded_input);

    if (expanded_output >= 0)
    {
      long long int const granted_vc = _sw_allocator->ReadRequest(expanded_input, expanded_output);
      if (granted_vc == vc)
      {
        // MoRi
        /* added by [a.mazloumi and modarressi] */
        _orion_current_sw_grant = expanded_input;
        SIM_arbiter_record(&(_orion_router_power.sw_out_arb), _orion_current_sw_requset[expanded_output / _output_speedup], _orion_last_sw_request[expanded_output / _output_speedup], _orion_current_sw_grant, _orion_last_sw_grant[expanded_output / _output_speedup]);
        _number_of_calls_of_power_functions++;
        _orion_last_sw_request[expanded_output / _output_speedup] = _orion_current_sw_requset[expanded_output / _output_speedup];
        _orion_last_sw_grant[expanded_output / _output_speedup] = _orion_current_sw_grant;
        /* end of [a.mazloumi and modarressi] codes */

        _sw_rr_offset[expanded_input] = (vc + _input_speedup) % _vcs;
        iter->second.second = expanded_output;
      }
      else
      {
        iter->second.second = STALL_CROSSBAR_CONFLICT;
      }
    }
    else if (_spec_sw_allocator)
    {
      expanded_output = _spec_sw_allocator->OutputAssigned(expanded_input);
      if (expanded_output >= 0)
      {
        if (_spec_mask_by_reqs && _sw_allocator->OutputHasRequests(expanded_output))
        {
          iter->second.second = STALL_CROSSBAR_CONFLICT;
        }
        else if (!_spec_mask_by_reqs && (_sw_allocator->InputAssigned(expanded_output) >= 0))
        {
          iter->second.second = STALL_CROSSBAR_CONFLICT;
        }
        else
        {
          long long int const granted_vc = _spec_sw_allocator->ReadRequest(expanded_input, expanded_output);
          if (granted_vc == vc)
          {
            _sw_rr_offset[expanded_input] = (vc + _input_speedup) % _vcs;
            iter->second.second = expanded_output;
          }
          else
          {
            iter->second.second = STALL_CROSSBAR_CONFLICT;
          }
        }
      }
      else
      {
        iter->second.second = STALL_CROSSBAR_CONFLICT;
      }
    }
    else
    {
      iter->second.second = STALL_CROSSBAR_CONFLICT;
    }
  }

  if (!_speculative && (_sw_alloc_delay <= 1))
  {
    return;
  }

  for (deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>>::iterator iter = _sw_alloc_vcs.begin(); iter != _sw_alloc_vcs.end(); ++iter)
  {
    long long int const time = iter->first;
    if (GetSimTime() < time)
    {
      break;
    }
    long long int const expanded_output = iter->second.second;
    if (expanded_output >= 0)
    {
      long long int const output = expanded_output / _output_speedup;
      BufferState const *const dest_buf = _next_buf[output];
      long long int const input = iter->second.first.first;
      long long int const vc = iter->second.first.second;
      long long int const expanded_input = input * _input_speedup + vc % _input_speedup;
      Buffer const *const cur_buf = _buf[input];
      //Flit const *const f = cur_buf->FrontFlit(vc);
      if ((_switch_hold_in[expanded_input] >= 0) || (_switch_hold_out[expanded_output] >= 0))
      {
        iter->second.second = STALL_CROSSBAR_CONFLICT;
      }
      else if (_speculative && (cur_buf->GetState(vc) == VC::vc_alloc))
      {
        if (_vc_allocator)
        { // separate VC and switch allocators
          long long int const input_and_vc = _vc_shuffle_requests ? (vc * _inputs + input) : (input * _vcs + vc);
          long long int const output_and_vc = _vc_allocator->OutputAssigned(input_and_vc);
          if (output_and_vc < 0)
          {
            iter->second.second = -1; // stall is counted in VC allocation path!
          }
          else if ((output_and_vc / _vcs) != output)
          {
            iter->second.second = STALL_BUFFER_CONFLICT; // count this case as if we had failed allocation
          }
          else if (dest_buf->IsFullFor((output_and_vc % _vcs)))
          {
            iter->second.second = dest_buf->IsFull() ? STALL_BUFFER_FULL : STALL_BUFFER_RESERVED;
          }
        }
        else
        { // VC allocation is piggybacked onto switch allocation
          OutputSet const *const route_set = cur_buf->GetRouteSet(vc);
          set<OutputSet::sSetElement> const setlist = route_set->GetSet();
          bool busy = true;
          bool full = true;
          bool reserved = false;
          for (set<OutputSet::sSetElement>::const_iterator iset = setlist.begin(); iset != setlist.end(); ++iset)
          {
            if (iset->output_port == output)
            {
              long long int vc_start;
              long long int vc_end;
              if (_noq && _noq_next_output_port[input][vc] >= 0)
              {
                vc_start = _noq_next_vc_start[input][vc];
                vc_end = _noq_next_vc_end[input][vc];
              }
              else
              {
                vc_start = iset->vc_start;
                vc_end = iset->vc_end;
              }
              for (long long int out_vc = vc_start; out_vc <= vc_end; ++out_vc)
              {
                if (dest_buf->IsAvailableFor(out_vc))
                {
                  busy = false;
                  if (!dest_buf->IsFullFor(out_vc))
                  {
                    full = false;
                    break;
                  }
                  else if (!dest_buf->IsFull())
                  {
                    reserved = true;
                  }
                }
              }
              if (!full)
              {
                break;
              }
            }
          }

          if (busy)
          {
            iter->second.second = STALL_BUFFER_BUSY;
          }
          else if (full)
          {
            iter->second.second = reserved ? STALL_BUFFER_RESERVED : STALL_BUFFER_FULL;
          }
        }
      }
      else
      {
        long long int const match_vc = cur_buf->GetOutputVC(vc);
        if (dest_buf->IsFullFor(match_vc))
        {
          iter->second.second = dest_buf->IsFull() ? STALL_BUFFER_FULL : STALL_BUFFER_RESERVED;
        }
      }
    }
  }
}

void IQRouter::_SWAllocUpdate()
{
  while (!_sw_alloc_vcs.empty())
  {
    pair<long long int, pair<pair<long long int, long long int>, long long int>> const &item = _sw_alloc_vcs.front();
    long long int const time = item.first;
    if ((time < 0) || (GetSimTime() < time))
    {
      break;
    }
    long long int const input = item.second.first.first;
    long long int const vc = item.second.first.second;
    Buffer *const cur_buf = _buf[input];
    Flit *const f = cur_buf->FrontFlit(vc);
    long long int const expanded_output = item.second.second;
    if (expanded_output >= 0)
    {
      long long int const expanded_input = input * _input_speedup + vc % _input_speedup;
      long long int const output = expanded_output / _output_speedup;
      BufferState *const dest_buf = _next_buf[output];
      long long int match_vc;
      if (!_vc_allocator && (cur_buf->GetState(vc) == VC::vc_alloc))
      {
        long long int const cl = f->cl;
        long long int const vc_offset = _vc_rr_offset[output * _classes + cl];
        match_vc = -1;
        long long int match_prio = numeric_limits<long long int>::min();
        const OutputSet *route_set = cur_buf->GetRouteSet(vc);
        set<OutputSet::sSetElement> const setlist = route_set->GetSet();
        for (set<OutputSet::sSetElement>::const_iterator iset = setlist.begin(); iset != setlist.end(); ++iset)
        {
          if (iset->output_port == output)
          {
            long long int vc_start;
            long long int vc_end;
            if (_noq && _noq_next_output_port[input][vc] >= 0)
            {
              vc_start = _noq_next_vc_start[input][vc];
              vc_end = _noq_next_vc_end[input][vc];
            }
            else
            {
              vc_start = iset->vc_start;
              vc_end = iset->vc_end;
            }
            for (long long int out_vc = vc_start; out_vc <= vc_end; ++out_vc)
            {
              long long int vc_prio = iset->pri;
              if (_vc_prioritize_empty && !dest_buf->IsEmptyFor(out_vc))
              {
                vc_prio += numeric_limits<long long int>::min();
              }

              // FIXME: This check should probably be performed in Evaluate(),
              // not Update(), as the latter can cause the outcome to depend on
              // the order of evaluation!
              if (dest_buf->IsAvailableFor(out_vc) && !dest_buf->IsFullFor(out_vc) && ((match_vc < 0) || RoundRobinArbiter::Supersedes(out_vc, vc_prio, match_vc, match_prio, vc_offset, _vcs)))
              {
                match_vc = out_vc;
                match_prio = vc_prio;
              }
            }
          }
        }
        cur_buf->SetState(vc, VC::active);
        cur_buf->SetOutput(vc, output, match_vc);
        dest_buf->TakeBuffer(match_vc, input * _vcs + vc);
        _vc_rr_offset[output * _classes + cl] = (match_vc + 1) % _vcs;
      }
      else
      {
        match_vc = cur_buf->GetOutputVC(vc);
      }
      cur_buf->RemoveFlit(vc);

      // MoRi
      /* added by [a.mazloumi and modarressi] */
      SIM_buf_power_data_read(&(_orion_router_info.in_buf_info), &(_orion_router_power.in_buf), 0xffff0000);
      _number_of_calls_of_power_functions++;
      /* end [a.mazloumi and modarressi]@ */

      _bufferMonitor->read(input, f);
      f->hops++;
      f->vc = match_vc;
      if (!_routing_delay && f->head)
      {
        const FlitChannel *channel = _output_channels[output];
        const Router *router = channel->GetSink();
        if (router)
        {
          if (_noq)
          {
            long long int next_output_port = _noq_next_output_port[input][vc];
            _noq_next_output_port[input][vc] = -1;
            long long int next_vc_start = _noq_next_vc_start[input][vc];
            _noq_next_vc_start[input][vc] = -1;
            long long int next_vc_end = _noq_next_vc_end[input][vc];
            _noq_next_vc_end[input][vc] = -1;
            f->la_route_set.Clear();
            f->la_route_set.AddRange(next_output_port, next_vc_start, next_vc_end);
          }
          else
          {
            long long int in_channel = channel->GetSinkPort();
            _rf(router, f, in_channel, &f->la_route_set, false);
          }
        }
        else
        {
          f->la_route_set.Clear();
        }
      }
      dest_buf->SendingFlit(f);

      // MoRi
      /* added by [a.mazloumi and modarressi] */
      SIM_crossbar_record(&(_orion_router_power.crossbar), 1, 0xffffffff, 0x0000ffff, 0, 0); //sending to crossbar
      _number_of_calls_of_power_functions++;
      /* end [a.mazloumi and modarressi]codes */

      _crossbar_flits.push_back(make_pair(-1, make_pair(f, make_pair(expanded_input, expanded_output))));
      if (_out_queue_credits.count(input) == 0)
      {
        _out_queue_credits.insert(make_pair(input, Credit::New()));
      }
      _out_queue_credits.find(input)->second->vc.insert(vc);
      if (cur_buf->Empty(vc))
      {
        if (f->tail)
        {
          cur_buf->SetState(vc, VC::idle);
        }
      }
      else
      {
        Flit *const nf = cur_buf->FrontFlit(vc);
        if (f->tail)
        {
          if (_routing_delay)
          {
            cur_buf->SetState(vc, VC::routing);
            _route_vcs.push_back(make_pair(-1, item.second.first));
          }
          else
          {
            cur_buf->SetRouteSet(vc, &nf->la_route_set);
            cur_buf->SetState(vc, VC::vc_alloc);
            if (_speculative)
            {
              _sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
            }
            if (_vc_allocator)
            {
              _vc_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
            }
            if (_noq)
            {
              _UpdateNOQ(input, vc, nf);
            }
          }
        }
        else
        {
          if (_hold_switch_for_packet)
          {
            _switch_hold_vc[expanded_input] = vc;
            _switch_hold_in[expanded_input] = expanded_output;
            _switch_hold_out[expanded_output] = expanded_input;
            _sw_hold_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
          }
          else
          {
            _sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
          }
        }
      }
    }
    else
    {
      _sw_alloc_vcs.push_back(make_pair(-1, make_pair(item.second.first, -1)));
    }
    _sw_alloc_vcs.pop_front();
    //    printf("\nTime:,%lld,%lld,[%lld][%lld],SWAllocUpdate,%lld\n", GetSimTime(), this->GetID(), f->id, f->pid, f->vc); //Sneha
  }
}

//------------------------------------------------------------------------------
// switch traversal
//------------------------------------------------------------------------------

void IQRouter::_SwitchEvaluate()
{
  for (deque<pair<long long int, pair<Flit *, pair<long long int, long long int>>>>::iterator iter = _crossbar_flits.begin(); iter != _crossbar_flits.end(); ++iter)
  {
    long long int const time = iter->first;
    if (time >= 0)
    {
      break;
    }
    iter->first = GetSimTime() + asyncConfig->getStFinalDelay(_id) - 1;
    //Flit const *const f = iter->second.first;
    //    printf("\nTime:,%lld,%lld,[%lld][%lld],SWEvaluate,%lld\n", GetSimTime(), this->GetID(), f->id, f->pid, f->vc); //Sneha
    //long long int const expanded_input = iter->second.second.first;
    //long long int const expanded_output = iter->second.second.second;
  }
}

void IQRouter::_SwitchUpdate()
{
  while (!_crossbar_flits.empty())
  {
    pair<long long int, pair<Flit *, pair<long long int, long long int>>> const &item = _crossbar_flits.front();
    long long int const time = item.first;
    if ((time < 0) || (GetSimTime() < time))
    {
      break;
    }
    Flit *const f = item.second.first;
    long long int const expanded_input = item.second.second.first;
    long long int const input = expanded_input / _input_speedup;
    long long int const expanded_output = item.second.second.second;
    long long int const output = expanded_output / _output_speedup;
    //    printf("\nTime:,%lld,%lld,[%lld][%lld],SWUpdate,%lld\n", GetSimTime(), this->GetID(), f->id, f->pid, f->vc); //Sneha
    _switchMonitor->traversal(input, output, f);
    // MoRi
    /* added by [a.mazloumi and modarressi]@*/
    SIM_crossbar_record(&(_orion_router_power.crossbar), 0, 0xffffffff, 0x0000ffff, expanded_input, _orion_crosbar_last_match[expanded_output]);
    _number_of_calls_of_power_functions++;
    _orion_crosbar_last_match[expanded_output] = expanded_input;
    /* end [a.mazloumi and modarressi]@ codes */
    _output_buffer[output].push(f);
    //the output buffer size isn't precise due to flits in flight but there is a maximum bound based on output speed up and ST traversal
    _crossbar_flits.pop_front();
  }
}

//------------------------------------------------------------------------------
// output queuing
//------------------------------------------------------------------------------

void IQRouter::_OutputQueuing()
{
  for (map<long long int, Credit *>::const_iterator iter = _out_queue_credits.begin(); iter != _out_queue_credits.end(); ++iter)
  {
    long long int const input = iter->first;
    Credit *const c = iter->second;
    _credit_buffer[input].push(c);
  }
  _out_queue_credits.clear();
}

//------------------------------------------------------------------------------
// write outputs
//------------------------------------------------------------------------------

void IQRouter::_SendFlits()
{
  for (long long int output = 0; output < _outputs; ++output)
  {
    if (!_output_buffer[output].empty())
    {
      Flit *const f = _output_buffer[output].front();
      _output_buffer[output].pop();
      // MoRi
      /* added by a.mazloumi@ */
      if (f)
      {
        if (output != (_outputs - 1))
          _orion_link_output_counters[output]++;
      }
      /* end of a.mazloumi codes*/
      _output_channels[output]->Send(f);
      //      printf("\nTime:,%lld,%lld,[%lld][%lld],SendingFlit,%lld\n", GetSimTime(), this->GetID(), f->id, f->pid, f->vc); //Sneha
    }
  }
}

void IQRouter::_SendCredits()
{
  for (long long int input = 0; input < _inputs; ++input)
  {
    if (!_credit_buffer[input].empty())
    {
      Credit *const c = _credit_buffer[input].front();
      _credit_buffer[input].pop();
      _input_credits[input]->Send(c);
    }
  }
}

//------------------------------------------------------------------------------
// misc.
//------------------------------------------------------------------------------

void IQRouter::Display(ostream &os) const
{
  for (long long int input = 0; input < _inputs; ++input)
  {
    _buf[input]->Display(os);
  }
}

long long int IQRouter::GetUsedCredit(long long int o) const
{
  BufferState const *const dest_buf = _next_buf[o];
  return dest_buf->Occupancy();
}

long long int IQRouter::GetBufferOccupancy(long long int i) const
{
  return _buf[i]->GetOccupancy();
}

vector<long long int> IQRouter::UsedCredits() const
{
  vector<long long int> result(_outputs * _vcs);
  for (long long int o = 0; o < _outputs; ++o)
  {
    for (long long int v = 0; v < _vcs; ++v)
    {
      result[o * _vcs + v] = _next_buf[o]->OccupancyFor(v);
    }
  }
  return result;
}

vector<long long int> IQRouter::FreeCredits() const
{
  vector<long long int> result(_outputs * _vcs);
  for (long long int o = 0; o < _outputs; ++o)
  {
    for (long long int v = 0; v < _vcs; ++v)
    {
      result[o * _vcs + v] = _next_buf[o]->AvailableFor(v);
    }
  }
  return result;
}

vector<long long int> IQRouter::MaxCredits() const
{
  vector<long long int> result(_outputs * _vcs);
  for (long long int o = 0; o < _outputs; ++o)
  {
    for (long long int v = 0; v < _vcs; ++v)
    {
      result[o * _vcs + v] = _next_buf[o]->LimitFor(v);
    }
  }
  return result;
}

void IQRouter::_UpdateNOQ(long long int input, long long int vc, Flit const *f)
{
  set<OutputSet::sSetElement> sl = f->la_route_set.GetSet();
  long long int out_port = sl.begin()->output_port;
  const FlitChannel *channel = _output_channels[out_port];
  const Router *router = channel->GetSink();
  if (router)
  {
    long long int in_channel = channel->GetSinkPort();
    OutputSet nos;
    _rf(router, f, in_channel, &nos, false);
    sl = nos.GetSet();
    OutputSet::sSetElement const &se = *sl.begin();
    long long int next_output_port = se.output_port;
    _noq_next_output_port[input][vc] = next_output_port;
    long long int next_vc_count = (se.vc_end - se.vc_start + 1) / router->NumOutputs();
    long long int next_vc_start = se.vc_start + next_output_port * next_vc_count;
    _noq_next_vc_start[input][vc] = next_vc_start;
    long long int next_vc_end = se.vc_start + (next_output_port + 1) * next_vc_count - 1;
    _noq_next_vc_end[input][vc] = next_vc_end;
  }
}
