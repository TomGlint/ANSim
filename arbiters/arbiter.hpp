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

// ----------------------------------------------------------------------
//
//  Arbiter: Base class for Matrix and Round Robin Arbiter
//
// ----------------------------------------------------------------------

#ifndef _ARBITER_HPP_
#define _ARBITER_HPP_

#include <vector>

#include "module.hpp"

class Arbiter : public Module
{

protected:
  typedef struct
  {
    bool valid;
    long long int id;
    long long int pri;
  } entry_t;

  vector<entry_t> _request;
  long long int _size;

  long long int _selected;
  long long int _highest_pri;
  long long int _best_input;

public:
  long long int _num_reqs;
  // Constructors
  Arbiter(Module *parent, const string &name, long long int size);

  // Print priority matrix to standard output
  virtual void PrintState() const = 0;

  // Register request with arbiter
  virtual void AddRequest(long long int input, long long int id, long long int pri);

  // Update priority matrix based on last aribtration result
  virtual void UpdateState() = 0;

  // Arbitrate amongst requests. Returns winning input and
  // updates pointers to metadata when valid pointers are passed
  virtual long long int Arbitrate(long long int *id = 0, long long int *pri = 0);

  virtual void Clear();

  inline long long int LastWinner() const
  {
    return _selected;
  }

  static Arbiter *NewArbiter(Module *parent, const string &name,
                             const string &arb_type, long long int size);
};

#endif
