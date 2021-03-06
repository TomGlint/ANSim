// $Id$

/*
 Copyright (c) 2007-2012, Trustees of The Leland Stanford Junior University
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this 
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _IQ_ROUTER_HPP_
#define _IQ_ROUTER_HPP_

#include <string>
#include <deque>
#include <queue>
#include <set>
#include <map>

#include "router.hpp"
#include "routefunc.hpp"

using namespace std;

class VC;
class Flit;
class Credit;
class Buffer;
class BufferState;
class Allocator;
class SwitchMonitor;
class BufferMonitor;

class IQRouter : public Router
{

  long long int _vcs;

  bool _vc_busy_when_full;
  bool _vc_prioritize_empty;
  bool _vc_shuffle_requests;

  bool _speculative;
  bool _spec_check_elig;
  bool _spec_check_cred;
  bool _spec_mask_by_reqs;

  bool _active;

  long long int _routing_delay;
  long long int _vc_alloc_delay;
  long long int _sw_alloc_delay;

  map<long long int, Flit *> _in_queue_flits;

  deque<pair<long long int, pair<Credit *, long long int>>> _proc_credits;

  deque<pair<long long int, pair<long long int, long long int>>> _route_vcs;
  deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>> _vc_alloc_vcs;
  deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>> _sw_hold_vcs;
  deque<pair<long long int, pair<pair<long long int, long long int>, long long int>>> _sw_alloc_vcs;

  deque<pair<long long int, pair<Flit *, pair<long long int, long long int>>>> _crossbar_flits;

  map<long long int, Credit *> _out_queue_credits;

  vector<Buffer *> _buf;
  vector<BufferState *> _next_buf;

  Allocator *_vc_allocator;
  Allocator *_sw_allocator;
  Allocator *_spec_sw_allocator;

  vector<long long int> _vc_rr_offset;
  vector<long long int> _sw_rr_offset;

  tRoutingFunction _rf;

  long long int _output_buffer_size;
  vector<queue<Flit *>> _output_buffer;

  vector<queue<Credit *>> _credit_buffer;

  bool _hold_switch_for_packet;
  vector<long long int> _switch_hold_in;
  vector<long long int> _switch_hold_out;
  vector<long long int> _switch_hold_vc;

  bool _noq;
  vector<vector<long long int>> _noq_next_output_port;
  vector<vector<long long int>> _noq_next_vc_start;
  vector<vector<long long int>> _noq_next_vc_end;

#ifdef TRACK_FLOWS
  vector<vector<queue<long long int>>> _outstanding_classes;
#endif

  bool _ReceiveFlits();
  bool _ReceiveCredits();

  virtual void _InternalStep();

  bool _SWAllocAddReq(unsigned int *_orion_current_sw_requset, long long int input, long long int vc, long long int output);

  void _InputQueuing();

  void _RouteEvaluate();
  void _VCAllocEvaluate();
  void _SWHoldEvaluate();
  void _SWAllocEvaluate();
  void _SwitchEvaluate();

  void _RouteUpdate();
  void _VCAllocUpdate();
  void _SWHoldUpdate();
  void _SWAllocUpdate();
  void _SwitchUpdate();

  void _OutputQueuing();

  void _SendFlits();
  void _SendCredits();

  void _UpdateNOQ(long long int input, long long int vc, Flit const *f);

  // ----------------------------------------
  //
  //   Router Power Modellingyes
  //
  // ----------------------------------------

  SwitchMonitor *_switchMonitor;
  BufferMonitor *_bufferMonitor;

public:
  IQRouter(Configuration const &config,
           Module *parent, string const &name, long long int id,
           long long int inputs, long long int outputs);

  virtual ~IQRouter();

  virtual void AddOutputChannel(FlitChannel *channel, CreditChannel *backchannel);

  virtual void ReadInputs();
  virtual void WriteOutputs();

  void Display(ostream &os = cout) const;

  virtual long long int GetUsedCredit(long long int o) const;
  virtual long long int GetBufferOccupancy(long long int i) const;

#ifdef TRACK_BUFFERS
  virtual long long int GetUsedCreditForClass(long long int output, long long int cl) const;
  virtual long long int GetBufferOccupancyForClass(long long int input, long long int cl) const;
#endif

  virtual vector<long long int> UsedCredits() const;
  virtual vector<long long int> FreeCredits() const;
  virtual vector<long long int> MaxCredits() const;

  SwitchMonitor const *const GetSwitchMonitor() const { return _switchMonitor; }
  BufferMonitor const *const GetBufferMonitor() const { return _bufferMonitor; }
};

#endif
