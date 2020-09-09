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

#ifndef _EVENT_ROUTER_HPP_
#define _EVENT_ROUTER_HPP_

#include <string>
#include <queue>
#include <vector>

#include "module.hpp"
#include "router.hpp"
#include "buffer.hpp"
#include "vc.hpp"
#include "prio_arb.hpp"
#include "routefunc.hpp"
#include "outputset.hpp"
#include "pipefifo.hpp"

class EventNextVCState : public Module
{
public:
  enum eNextVCState
  {
    idle,
    busy,
    tail_pending
  };

  struct tWaiting
  {
    long long int input;
    long long int vc;
    long long int id;
    long long int pres;
    bool watch;
  };

private:
  long long int _buf_size;
  long long int _vcs;

  vector<long long int> _credits;
  vector<long long int> _presence;
  vector<long long int> _input;
  vector<long long int> _inputVC;

  vector<list<tWaiting *>> _waiting;

  vector<eNextVCState> _state;

public:
  EventNextVCState(const Configuration &config,
                   Module *parent, const string &name);

  eNextVCState GetState(long long int vc) const;
  long long int GetPresence(long long int vc) const;
  long long int GetCredits(long long int vc) const;
  long long int GetInput(long long int vc) const;
  long long int GetInputVC(long long int vc) const;

  bool IsWaiting(long long int vc) const;
  bool IsInputWaiting(long long int vc, long long int w_input, long long int w_vc) const;

  void PushWaiting(long long int vc, tWaiting *w);
  void IncrWaiting(long long int vc, long long int w_input, long long int w_vc);
  tWaiting *PopWaiting(long long int vc);

  void SetState(long long int vc, eNextVCState state);
  void SetCredits(long long int vc, long long int value);
  void SetPresence(long long int vc, long long int value);
  void SetInput(long long int vc, long long int input);
  void SetInputVC(long long int vc, long long int in_vc);
};

class EventRouter : public Router
{
  long long int _vcs;

  long long int _vct;

  vector<Buffer *> _buf;
  vector<vector<bool>> _active;

  tRoutingFunction _rf;

  vector<EventNextVCState *> _output_state;

  PipelineFIFO<Flit> *_crossbar_pipe;
  PipelineFIFO<Credit> *_credit_pipe;

  vector<queue<Flit *>> _input_buffer;
  vector<queue<Flit *>> _output_buffer;

  vector<queue<Credit *>> _in_cred_buffer;
  vector<queue<Credit *>> _out_cred_buffer;

  struct tArrivalEvent
  {
    long long int input;
    long long int output;
    long long int src_vc;
    long long int dst_vc;
    bool head;
    bool tail;

    long long int id; // debug
    bool watch;       // debug
  };

  PipelineFIFO<tArrivalEvent> *_arrival_pipe;
  vector<queue<tArrivalEvent *>> _arrival_queue;
  vector<PriorityArbiter *> _arrival_arbiter;

  struct tTransportEvent
  {
    long long int input;
    long long int src_vc;
    long long int dst_vc;

    long long int id; // debug
    bool watch;       // debug
  };

  vector<queue<tTransportEvent *>> _transport_queue;
  vector<PriorityArbiter *> _transport_arbiter;

  vector<bool> _transport_free;
  vector<long long int> _transport_match;

  void _ReceiveFlits();
  void _ReceiveCredits();

  void _IncomingFlits();
  void _ArrivalRequests(long long int input);
  void _ArrivalArb(long long int output);
  void _SendTransport(long long int input, long long int output, tArrivalEvent *aevt);
  void _ProcessWaiting(long long int output, long long int out_vc);
  void _TransportRequests(long long int output);
  void _TransportArb(long long int input);
  void _OutputQueuing();

  void _SendFlits();
  void _SendCredits();

  virtual void _InternalStep();

public:
  EventRouter(const Configuration &config,
              Module *parent, const string &name, long long int id,
              long long int inputs, long long int outputs);
  virtual ~EventRouter();

  virtual void ReadInputs();
  virtual void WriteOutputs();

  virtual long long int GetUsedCredit(long long int o) const { return 0; }
  virtual long long int GetBufferOccupancy(long long int i) const { return 0; }

#ifdef TRACK_BUFFERS
  virtual long long int GetUsedCreditForClass(long long int output, long long int cl) const
  {
    return 0;
  }
  virtual long long int GetBufferOccupancyForClass(long long int input, long long int cl) const { return 0; }
#endif

  virtual vector<long long int> UsedCredits() const
  {
    return vector<long long int>();
  }
  virtual vector<long long int> FreeCredits() const { return vector<long long int>(); }
  virtual vector<long long int> MaxCredits() const { return vector<long long int>(); }

  void Display(ostream &os = cout) const;
};

#endif
