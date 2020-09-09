// $Id$

#ifndef _TRAFFIC_HPP_
#define _TRAFFIC_HPP_

#include <vector>
#include <set>
#include "config_utils.hpp"

using namespace std;

class TrafficPattern
{
protected:
  long long int _nodes;
  TrafficPattern(long long int nodes);

public:
  virtual ~TrafficPattern() {}
  virtual void reset();
  virtual long long int dest(long long int source) = 0;
  static TrafficPattern *New(string const &pattern, long long int nodes,
                             Configuration const *const config = NULL);
};

class PermutationTrafficPattern : public TrafficPattern
{
protected:
  PermutationTrafficPattern(long long int nodes);
};

class BitPermutationTrafficPattern : public PermutationTrafficPattern
{
protected:
  BitPermutationTrafficPattern(long long int nodes);
};

class BitCompTrafficPattern : public BitPermutationTrafficPattern
{
public:
  BitCompTrafficPattern(long long int nodes);
  virtual long long int dest(long long int source);
};

class TransposeTrafficPattern : public BitPermutationTrafficPattern
{
protected:
  long long int _shift;

public:
  TransposeTrafficPattern(long long int nodes);
  virtual long long int dest(long long int source);
};

class BitRevTrafficPattern : public BitPermutationTrafficPattern
{
public:
  BitRevTrafficPattern(long long int nodes);
  virtual long long int dest(long long int source);
};

class ShuffleTrafficPattern : public BitPermutationTrafficPattern
{
public:
  ShuffleTrafficPattern(long long int nodes);
  virtual long long int dest(long long int source);
};

class DigitPermutationTrafficPattern : public PermutationTrafficPattern
{
protected:
  long long int _k;
  long long int _n;
  long long int _xr;
  DigitPermutationTrafficPattern(long long int nodes, long long int k, long long int n, long long int xr = 1);
};

class TornadoTrafficPattern : public DigitPermutationTrafficPattern
{
public:
  TornadoTrafficPattern(long long int nodes, long long int k, long long int n, long long int xr = 1);
  virtual long long int dest(long long int source);
};

class NeighborTrafficPattern : public DigitPermutationTrafficPattern
{
public:
  NeighborTrafficPattern(long long int nodes, long long int k, long long int n, long long int xr = 1);
  virtual long long int dest(long long int source);
};

class RandomPermutationTrafficPattern : public TrafficPattern
{
private:
  vector<long long int> _dest;
  inline void randomize(long long int seed);

public:
  RandomPermutationTrafficPattern(long long int nodes, long long int seed);
  virtual long long int dest(long long int source);
};

class RandomTrafficPattern : public TrafficPattern
{
protected:
  RandomTrafficPattern(long long int nodes);
};

class UniformRandomTrafficPattern : public RandomTrafficPattern
{
public:
  UniformRandomTrafficPattern(long long int nodes);
  virtual long long int dest(long long int source);
};

class UniformBackgroundTrafficPattern : public RandomTrafficPattern
{
private:
  set<long long int> _excluded;

public:
  UniformBackgroundTrafficPattern(long long int nodes, vector<long long int> excluded_nodes);
  virtual long long int dest(long long int source);
};

class DiagonalTrafficPattern : public RandomTrafficPattern
{
public:
  DiagonalTrafficPattern(long long int nodes);
  virtual long long int dest(long long int source);
};

class AsymmetricTrafficPattern : public RandomTrafficPattern
{
public:
  AsymmetricTrafficPattern(long long int nodes);
  virtual long long int dest(long long int source);
};

class Taper64TrafficPattern : public RandomTrafficPattern
{
public:
  Taper64TrafficPattern(long long int nodes);
  virtual long long int dest(long long int source);
};

class BadPermDFlyTrafficPattern : public DigitPermutationTrafficPattern
{
public:
  BadPermDFlyTrafficPattern(long long int nodes, long long int k, long long int n);
  virtual long long int dest(long long int source);
};

class BadPermYarcTrafficPattern : public DigitPermutationTrafficPattern
{
public:
  BadPermYarcTrafficPattern(long long int nodes, long long int k, long long int n, long long int xr = 1);
  virtual long long int dest(long long int source);
};

class HotSpotTrafficPattern : public TrafficPattern
{
private:
  vector<long long int> _hotspots;
  vector<long long int> _rates;
  long long int _max_val;

public:
  HotSpotTrafficPattern(long long int nodes, vector<long long int> hotspots,
                        vector<long long int> rates = vector<long long int>());
  virtual long long int dest(long long int source);
};

#endif
