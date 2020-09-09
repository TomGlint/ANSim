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

#ifndef _SWITCH_MONITOR_HPP_
#define _SWITCH_MONITOR_HPP_

#include <vector>
#include <iostream>

using namespace std;

class Flit;

class SwitchMonitor
{
  long long int _cycles;
  long long int _inputs;
  long long int _outputs;
  long long int _classes;
  vector<long long int> _event;
  long long int index(long long int input, long long int output, long long int cl) const;

public:
  SwitchMonitor(long long int inputs, long long int outputs, long long int classes);
  void cycle();
  vector<long long int> const &GetActivity() const
  {
    return _event;
  }
  inline long long int const &NumInputs() const
  {
    return _inputs;
  }
  inline long long int const &NumOutputs() const
  {
    return _outputs;
  }
  inline long long int const &NumClasses() const
  {
    return _classes;
  }
  void traversal(long long int input, long long int output, Flit const *f);
  void display(ostream &os) const;
};

ostream &operator<<(ostream &os, SwitchMonitor const &obj);

#endif
