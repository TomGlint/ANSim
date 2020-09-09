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

#ifndef _ALLOCATOR_HPP_
#define _ALLOCATOR_HPP_

#include <string>
#include <map>
#include <set>
#include <vector>

#include "module.hpp"
#include "config_utils.hpp"

class Allocator : public Module
{
protected:
  const long long int _inputs;
  const long long int _outputs;

  bool _dirty;

  vector<long long int> _inmatch;
  vector<long long int> _outmatch;

public:
  struct sRequest
  {
    long long int port;
    long long int label;
    long long int in_pri;
    long long int out_pri;
  };

  Allocator(Module *parent, const string &name,
            long long int inputs, long long int outputs);

  virtual void Clear();

  virtual long long int ReadRequest(long long int in, long long int out) const = 0;
  virtual bool ReadRequest(sRequest &req, long long int in, long long int out) const = 0;

  virtual void AddRequest(long long int in, long long int out, long long int label = 1,
                          long long int in_pri = 0, long long int out_pri = 0);
  virtual void RemoveRequest(long long int in, long long int out, long long int label = 1) = 0;

  virtual void Allocate() = 0;

  long long int OutputAssigned(long long int in) const;
  long long int InputAssigned(long long int out) const;

  virtual bool OutputHasRequests(long long int out) const = 0;
  virtual bool InputHasRequests(long long int in) const = 0;

  virtual long long int NumOutputRequests(long long int out) const = 0;
  virtual long long int NumInputRequests(long long int in) const = 0;

  virtual void PrintRequests(ostream *os = NULL) const = 0;
  void PrintGrants(ostream *os = NULL) const;

  static Allocator *NewAllocator(Module *parent, const string &name,
                                 const string &alloc_type,
                                 long long int inputs, long long int outputs,
                                 Configuration const *const config = NULL);
};

//==================================================
// A dense allocator stores the entire request
// matrix.
//==================================================

class DenseAllocator : public Allocator
{
protected:
  vector<vector<sRequest>> _request;

public:
  DenseAllocator(Module *parent, const string &name,
                 long long int inputs, long long int outputs);

  void Clear();

  long long int ReadRequest(long long int in, long long int out) const;
  bool ReadRequest(sRequest &req, long long int in, long long int out) const;

  void AddRequest(long long int in, long long int out, long long int label = 1,
                  long long int in_pri = 0, long long int out_pri = 0);
  void RemoveRequest(long long int in, long long int out, long long int label = 1);

  bool OutputHasRequests(long long int out) const;
  bool InputHasRequests(long long int in) const;

  long long int NumOutputRequests(long long int out) const;
  long long int NumInputRequests(long long int in) const;

  void PrintRequests(ostream *os = NULL) const;
};

//==================================================
// A sparse allocator only stores the requests
// (allows for a more efficient implementation).
//==================================================

class SparseAllocator : public Allocator
{
protected:
  set<long long int> _in_occ;
  set<long long int> _out_occ;

  vector<map<long long int, sRequest>> _in_req;
  vector<map<long long int, sRequest>> _out_req;

public:
  SparseAllocator(Module *parent, const string &name,
                  long long int inputs, long long int outputs);

  void Clear();

  long long int ReadRequest(long long int in, long long int out) const;
  bool ReadRequest(sRequest &req, long long int in, long long int out) const;

  void AddRequest(long long int in, long long int out, long long int label = 1,
                  long long int in_pri = 0, long long int out_pri = 0);
  void RemoveRequest(long long int in, long long int out, long long int label = 1);

  bool OutputHasRequests(long long int out) const;
  bool InputHasRequests(long long int in) const;

  long long int NumOutputRequests(long long int out) const;
  long long int NumInputRequests(long long int in) const;

  void PrintRequests(ostream *os = NULL) const;
};

#endif
