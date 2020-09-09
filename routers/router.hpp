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

#ifndef _ROUTER_HPP_
#define _ROUTER_HPP_

#include <string>
#include <vector>

#include "timed_module.hpp"
#include "flit.hpp"
#include "credit.hpp"
#include "flitchannel.hpp"
#include "channel.hpp"
#include "config_utils.hpp"
#include "asyncConfig.hpp"

// Orion Power Support
#include "SIM_router.h"

typedef Channel<Credit> CreditChannel;

class Router : public TimedModule
{

protected:
  static long long int const STALL_BUFFER_BUSY;
  static long long int const STALL_BUFFER_CONFLICT;
  static long long int const STALL_BUFFER_FULL;
  static long long int const STALL_BUFFER_RESERVED;
  static long long int const STALL_CROSSBAR_CONFLICT;

  long long int _id;

  long long int _inputs;
  long long int _outputs;

  long long int _classes;

  long long int _input_speedup;
  long long int _output_speedup;

  double _internal_speedup;
  double _partial_internal_cycles;

  long long int _crossbar_delay;
  long long int _credit_delay;

  vector<FlitChannel *> _input_channels;
  vector<CreditChannel *> _input_credits;
  vector<FlitChannel *> _output_channels;
  vector<CreditChannel *> _output_credits;
  vector<bool> _channel_faults;

  //#ifdef TRACK_FLOWS
  //  vector<vector<long long int> > _received_flits;
  //  vector<vector<long long int> > _stored_flits;
  //  vector<vector<long long int> > _sent_flits;
  //  vector<vector<long long int> > _outstanding_credits;
  //  vector<vector<long long int> > _active_packets;
  //#endif

  //#ifdef TRACK_STALLS
  //  vector<long long int> _buffer_busy_stalls;
  //  vector<long long int> _buffer_conflict_stalls;
  //  vector<long long int> _buffer_full_stalls;
  //  vector<long long int> _buffer_reserved_stalls;
  //  vector<long long int> _crossbar_conflict_stalls;
  //#endif

  virtual void _InternalStep() = 0;

public:
  // Orion Power Support

  /*
  * add power calculation from Orion 
  */

  SIM_router_area_t _orion_router_area;
  SIM_router_power_t _orion_router_power;
  SIM_router_info_t _orion_router_info;

  //crossbar power
  int *_orion_crosbar_last_match;

  //switch arbitor power
  unsigned int *_orion_last_sw_request; // bit vector _ [outputs]
  int *_orion_last_sw_grant;            // [outputs]

  //VC arbitor power
  unsigned int *_orion_last_vc_reguest; //bit vector _ [outputs * VCs]
  int *_orion_last_vc_grant;            // [outputs * VCs]

  //link power
  int *_orion_link_output_counters;
  double *_orion_link_length;

  //temporary counters; TODO: delete this variables
  int _number_of_crossed_flits;
  int _number_of_crossed_headerFlits;
  int _number_of_calls_of_power_functions;

  Router(const Configuration &config,
         Module *parent, const string &name, long long int id,
         long long int inputs, long long int outputs);

  // Orion Power Support
  virtual ~Router();

  static Router *NewRouter(const Configuration &config,
                           Module *parent, const string &name, long long int id,
                           long long int inputs, long long int outputs);

  virtual void AddInputChannel(FlitChannel *channel, CreditChannel *backchannel);
  virtual void AddOutputChannel(FlitChannel *channel, CreditChannel *backchannel);

  inline FlitChannel *GetInputChannel(long long int input) const
  {
    assert((input >= 0) && (input < _inputs));
    return _input_channels[input];
  }
  inline FlitChannel *GetOutputChannel(long long int output) const
  {
    assert((output >= 0) && (output < _outputs));
    return _output_channels[output];
  }

  virtual void ReadInputs() = 0;
  virtual void Evaluate();
  virtual void WriteOutputs() = 0;

  void OutChannelFault(long long int c, bool fault = true);
  bool IsFaultyOutput(long long int c) const;

  inline long long int GetID() const { return _id; }

  virtual long long int GetUsedCredit(long long int o) const = 0;
  virtual long long int GetBufferOccupancy(long long int i) const = 0;

#ifdef TRACK_BUFFERS
  virtual long long int GetUsedCreditForClass(long long int output, long long int cl) const = 0;
  virtual long long int GetBufferOccupancyForClass(long long int input, long long int cl) const = 0;
#endif

#ifdef TRACK_FLOWS
  inline vector<long long int> const &GetReceivedFlits(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _received_flits[c];
  }
  inline vector<long long int> const &GetStoredFlits(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _stored_flits[c];
  }
  inline vector<long long int> const &GetSentFlits(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _sent_flits[c];
  }
  inline vector<long long int> const &GetOutstandingCredits(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _outstanding_credits[c];
  }

  inline vector<long long int> const &GetActivePackets(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _active_packets[c];
  }

  inline void ResetFlowStats(long long int c)
  {
    assert((c >= 0) && (c < _classes));
    _received_flits[c].assign(_received_flits[c].size(), 0);
    _sent_flits[c].assign(_sent_flits[c].size(), 0);
  }
#endif

  virtual vector<long long int> UsedCredits() const = 0;
  virtual vector<long long int> FreeCredits() const = 0;
  virtual vector<long long int> MaxCredits() const = 0;

#ifdef TRACK_STALLS
  inline long long int GetBufferBusyStalls(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _buffer_busy_stalls[c];
  }
  inline long long int GetBufferConflictStalls(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _buffer_conflict_stalls[c];
  }
  inline long long int GetBufferFullStalls(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _buffer_full_stalls[c];
  }
  inline long long int GetBufferReservedStalls(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _buffer_reserved_stalls[c];
  }
  inline long long int GetCrossbarConflictStalls(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _crossbar_conflict_stalls[c];
  }

  inline void ResetStallStats(long long int c)
  {
    assert((c >= 0) && (c < _classes));
    _buffer_busy_stalls[c] = 0;
    _buffer_conflict_stalls[c] = 0;
    _buffer_full_stalls[c] = 0;
    _buffer_reserved_stalls[c] = 0;
    _crossbar_conflict_stalls[c] = 0;
  }
#endif

  inline long long int NumInputs() const
  {
    return _inputs;
  }
  inline long long int NumOutputs() const { return _outputs; }
};

#endif
