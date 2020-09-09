// $Id$

#ifndef _CREDIT_HPP_
#define _CREDIT_HPP_

#include <set>
#include <stack>

class Credit
{

public:
  set<long long int> vc;

  // these are only used by the event router
  bool head, tail;
  long long int id;

  void Reset();

  static Credit *New();
  void Free();
  static void FreeAll();
  static long long int OutStanding();

private:
  static stack<Credit *> _all;
  static stack<Credit *> _free;

  Credit();
  ~Credit() {}
};

#endif
