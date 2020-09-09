// $Id$

#ifndef _FLIT_HPP_
#define _FLIT_HPP_

#include <iostream>
#include <stack>

#include "booksim.hpp"
#include "outputset.hpp"

class Flit
{

public:
  long long int vc;

  long long int cl;

  bool head;
  bool tail;

  long long int ctime;
  long long int itime;
  long long int atime;

  long long int id;
  long long int pid;

  bool record;

  long long int src;
  long long int dest;

  long long int pri;

  long long int hops;
  bool watch;
  long long int starttime;

  //  long long int ib_time;		//Sneha
  //  long long int rc_time;		//Sneha
  //  long long int vc_time;		//Sneha
  //  long long int sw_time;		//Sneha

  // intermediate destination (if any)
  mutable long long int intm;

  // phase in multi-phase algorithms
  mutable long long int ph;

  // Fields for arbitrary data
  void *data;

  // Lookahead route info
  OutputSet la_route_set;

  void Reset();

  static Flit *New();
  void Free();
  static void FreeAll();

private:
  Flit();
  ~Flit() {}

  static stack<Flit *> _all;
  static stack<Flit *> _free;
};

ostream &operator<<(ostream &os, const Flit &f);

#endif
