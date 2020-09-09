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

#ifndef _CHAOS_ROUTER_HPP_
#define _CHAOS_ROUTER_HPP_

#include <string>
#include <queue>
#include <vector>

#include "module.hpp"
#include "router.hpp"
#include "allocator.hpp"
#include "routefunc.hpp"
#include "outputset.hpp"
#include "buffer_state.hpp"
#include "pipefifo.hpp"
#include "vc.hpp"

class ChaosRouter : public Router
{

  tRoutingFunction _rf;

  vector<OutputSet *> _input_route;
  vector<OutputSet *> _mq_route;

  enum eQState
  {
    empty,       //            input avail
    filling,     //    >**H    ready to send
    full,        //  T****H    ready to send
    leaving,     //    T***>   input avail
    cut_through, //    >***>
    shared       // >**HT**>
  };

  PipelineFIFO<Flit> *_crossbar_pipe;

  long long int _multi_queue_size;
  long long int _buffer_size;

  vector<queue<Flit *>> _input_frame;
  vector<queue<Flit *>> _output_frame;
  vector<queue<Flit *>> _multi_queue;

  vector<long long int> _next_queue_cnt;

  vector<queue<Credit *>> _credit_queue;

  vector<eQState> _input_state;
  vector<eQState> _multi_state;

  vector<long long int> _input_output_match;
  vector<long long int> _input_mq_match;
  vector<long long int> _multi_match;

  vector<long long int> _mq_age;

  vector<bool> _output_matched;
  vector<bool> _mq_matched;

  long long int _cur_channel;
  long long int _read_stall;

  bool _IsInjectionChan(long long int chan) const;
  bool _IsEjectionChan(long long int chan) const;

  bool _InputReady(long long int input) const;
  bool _OutputFull(long long int out) const;
  bool _OutputAvail(long long int out) const;
  bool _MultiQueueFull(long long int mq) const;

  long long int _InputForOutput(long long int output) const;
  long long int _MultiQueueForOutput(long long int output) const;
  long long int _FindAvailMultiQueue() const;

  void _NextInterestingChannel();
  void _OutputAdvance();
  void _SendFlits();
  void _SendCredits();

  virtual void _InternalStep();

public:
  ChaosRouter(const Configuration &config,
              Module *parent, const string &name, long long int id,
              long long int inputs, long long int outputs);

  virtual ~ChaosRouter();

  virtual void ReadInputs();
  virtual void WriteOutputs();

  virtual long long int GetUsedCredit(long long int out) const { return 0; }
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
