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

#ifndef _FlatFlyOnChip_HPP_
#define _FlatFlyOnChip_HPP_

#include "network.hpp"

#include "routefunc.hpp"
#include <cassert>

class FlatFlyOnChip : public Network
{

  long long int _m;
  long long int _n;
  long long int _r;
  long long int _k;
  long long int _c;
  long long int _radix;
  long long int _net_size;
  long long int _stageout;
  long long int _numinput;
  long long int _stages;
  long long int _num_of_switch;

  void _ComputeSize(const Configuration &config);
  void _BuildNet(const Configuration &config);

  long long int _OutChannel(long long int stage, long long int addr, long long int port, long long int outputs) const;
  long long int _InChannel(long long int stage, long long int addr, long long int port) const;

public:
  FlatFlyOnChip(const Configuration &config, const string &name);

  long long int GetN() const;
  long long int GetK() const;

  static void RegisterRoutingFunctions();
  double Capacity() const;
  void InsertRandomFaults(const Configuration &config);
};
void adaptive_xyyx_flatfly(const Router *r, const Flit *f, long long int in_channel,
                           OutputSet *outputs, bool inject);
void xyyx_flatfly(const Router *r, const Flit *f, long long int in_channel,
                  OutputSet *outputs, bool inject);
void min_flatfly(const Router *r, const Flit *f, long long int in_channel,
                 OutputSet *outputs, bool inject);
void ugal_xyyx_flatfly_onchip(const Router *r, const Flit *f, long long int in_channel,
                              OutputSet *outputs, bool inject);
void ugal_flatfly_onchip(const Router *r, const Flit *f, long long int in_channel,
                         OutputSet *outputs, bool inject);
void ugal_pni_flatfly_onchip(const Router *r, const Flit *f, long long int in_channel,
                             OutputSet *outputs, bool inject);
void valiant_flatfly(const Router *r, const Flit *f, long long int in_channel,
                     OutputSet *outputs, bool inject);

long long int find_distance(long long int src, long long int dest);
long long int find_ran_intm(long long int src, long long int dest);
long long int flatfly_outport(long long int dest, long long int rID);
long long int flatfly_transformation(long long int dest);
long long int flatfly_outport_yx(long long int dest, long long int rID);

#endif
