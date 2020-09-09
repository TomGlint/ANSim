// $Id$

#ifndef _INJECTION_HPP_
#define _INJECTION_HPP_

#include "config_utils.hpp"

using namespace std;

class InjectionProcess
{
protected:
  long long int _nodes;
  double _rate;
  InjectionProcess(long long int nodes, double rate);

public:
  virtual ~InjectionProcess() {}
  virtual bool test(long long int source) = 0;
  virtual void reset();
  static InjectionProcess *New(string const &inject, long long int nodes, double load,
                               Configuration const *const config = NULL);
};

class BernoulliInjectionProcess : public InjectionProcess
{
public:
  BernoulliInjectionProcess(long long int nodes, double rate);
  virtual bool test(long long int source);
};

class OnOffInjectionProcess : public InjectionProcess
{
private:
  double _alpha;
  double _beta;
  double _r1;
  vector<long long int> _initial;
  vector<long long int> _state;

public:
  OnOffInjectionProcess(long long int nodes, double rate, double alpha, double beta,
                        double r1, vector<long long int> initial);
  virtual void reset();
  virtual bool test(long long int source);
};

#endif
