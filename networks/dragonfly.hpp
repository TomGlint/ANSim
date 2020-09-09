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

#ifndef _DragonFly_HPP_
#define _DragonFly_HPP_

#include "network.hpp"
#include "routefunc.hpp"

class DragonFlyNew : public Network
{

  long long int _m;
  long long int _n;
  long long int _r;
  long long int _k;
  long long int _p, _a, _g;
  long long int _radix;
  long long int _net_size;
  long long int _stageout;
  long long int _numinput;
  long long int _stages;
  long long int _num_of_switch;
  long long int _grp_num_routers;
  long long int _grp_num_nodes;

  void _ComputeSize(const Configuration &config);
  void _BuildNet(const Configuration &config);

public:
  DragonFlyNew(const Configuration &config, const string &name);

  long long int GetN() const;
  long long int GetK() const;

  double Capacity() const;
  static void RegisterRoutingFunctions();
  void InsertRandomFaults(const Configuration &config);
};
long long int dragonfly_port(long long int rID, long long int source, long long int dest);

void ugal_dragonflynew(const Router *r, const Flit *f, long long int in_channel,
                       OutputSet *outputs, bool inject);
void min_dragonflynew(const Router *r, const Flit *f, long long int in_channel,
                      OutputSet *outputs, bool inject);

#endif
