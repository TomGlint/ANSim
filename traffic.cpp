// $Id$

#include <iostream>
#include <sstream>
#include <ctime>
#include "random_utils.hpp"
#include "traffic.hpp"

TrafficPattern::TrafficPattern(long long int nodes)
    : _nodes(nodes)
{
  if (nodes <= 0)
  {
    cout << "Error: Traffic patterns require at least one node." << endl;
    exit(-1);
  }
}

void TrafficPattern::reset()
{
}

TrafficPattern *TrafficPattern::New(string const &pattern, long long int nodes,
                                    Configuration const *const config)
{
  string pattern_name;
  string param_str;
  size_t left = pattern.find_first_of('(');
  if (left == string::npos)
  {
    pattern_name = pattern;
  }
  else
  {
    pattern_name = pattern.substr(0, left);
    size_t right = pattern.find_last_of(')');
    if (right == string::npos)
    {
      param_str = pattern.substr(left + 1);
    }
    else
    {
      param_str = pattern.substr(left + 1, right - left - 1);
    }
  }
  vector<string> params = tokenize_str(param_str);

  TrafficPattern *result = NULL;
  if (pattern_name == "bitcomp")
  {
    result = new BitCompTrafficPattern(nodes);
  }
  else if (pattern_name == "transpose")
  {
    result = new TransposeTrafficPattern(nodes);
  }
  else if (pattern_name == "bitrev")
  {
    result = new BitRevTrafficPattern(nodes);
  }
  else if (pattern_name == "shuffle")
  {
    result = new ShuffleTrafficPattern(nodes);
  }
  else if (pattern_name == "randperm")
  {
    long long int perm_seed = -1;
    if (params.empty())
    {
      if (config)
      {
        if (config->GetStr("perm_seed") == "time")
        {
          perm_seed = time(NULL);
          cout << "SEED: perm_seed=" << perm_seed << endl;
        }
        else
        {
          perm_seed = config->GetLongInt("perm_seed");
        }
      }
      else
      {
        cout << "Error: Missing parameter for random permutation traffic pattern: " << pattern << endl;
        exit(-1);
      }
    }
    else
    {
      perm_seed = atoll(params[0].c_str());
    }
    result = new RandomPermutationTrafficPattern(nodes, perm_seed);
  }
  else if (pattern_name == "uniform")
  {
    result = new UniformRandomTrafficPattern(nodes);
  }
  else if (pattern_name == "background")
  {
    vector<long long int> excludes = tokenize_int(params[0]);
    result = new UniformBackgroundTrafficPattern(nodes, excludes);
  }
  else if (pattern_name == "diagonal")
  {
    result = new DiagonalTrafficPattern(nodes);
  }
  else if (pattern_name == "asymmetric")
  {
    result = new AsymmetricTrafficPattern(nodes);
  }
  else if (pattern_name == "taper64")
  {
    result = new Taper64TrafficPattern(nodes);
  }
  else if (pattern_name == "bad_dragon")
  {
    bool missing_params = false;
    long long int k = -1;
    if (params.size() < 1)
    {
      if (config)
      {
        k = config->GetLongInt("k");
      }
      else
      {
        missing_params = true;
      }
    }
    else
    {
      k = atoll(params[0].c_str());
    }
    long long int n = -1;
    if (params.size() < 2)
    {
      if (config)
      {
        n = config->GetLongInt("n");
      }
      else
      {
        missing_params = true;
      }
    }
    else
    {
      n = atoll(params[1].c_str());
    }
    if (missing_params)
    {
      cout << "Error: Missing parameters for dragonfly bad permutation traffic pattern: " << pattern << endl;
      exit(-1);
    }
    result = new BadPermDFlyTrafficPattern(nodes, k, n);
  }
  else if ((pattern_name == "tornado") || (pattern_name == "neighbor") ||
           (pattern_name == "badperm_yarc"))
  {
    bool missing_params = false;
    long long int k = -1;
    if (params.size() < 1)
    {
      if (config)
      {
        k = config->GetLongInt("k");
      }
      else
      {
        missing_params = true;
      }
    }
    else
    {
      k = atoll(params[0].c_str());
    }
    long long int n = -1;
    if (params.size() < 2)
    {
      if (config)
      {
        n = config->GetLongInt("n");
      }
      else
      {
        missing_params = true;
      }
    }
    else
    {
      n = atoll(params[1].c_str());
    }
    long long int xr = -1;
    if (params.size() < 3)
    {
      if (config)
      {
        xr = config->GetLongInt("xr");
      }
      else
      {
        missing_params = true;
      }
    }
    else
    {
      xr = atoll(params[2].c_str());
    }
    if (missing_params)
    {
      cout << "Error: Missing parameters for digit permutation traffic pattern: " << pattern << endl;
      exit(-1);
    }
    if (pattern_name == "tornado")
    {
      result = new TornadoTrafficPattern(nodes, k, n, xr);
    }
    else if (pattern_name == "neighbor")
    {
      result = new NeighborTrafficPattern(nodes, k, n, xr);
    }
    else if (pattern_name == "badperm_yarc")
    {
      result = new BadPermYarcTrafficPattern(nodes, k, n, xr);
    }
  }
  else if (pattern_name == "hotspot")
  {
    if (params.empty())
    {
      params.push_back("-1");
    }
    vector<long long int> hotspots = tokenize_int(params[0]);
    for (size_t i = 0; i < hotspots.size(); ++i)
    {
      if (hotspots[i] < 0)
      {
        hotspots[i] = RandomInt(nodes - 1);
      }
    }
    vector<long long int> rates;
    if (params.size() >= 2)
    {
      rates = tokenize_int(params[1]);
      rates.resize(hotspots.size(), rates.back());
    }
    else
    {
      rates.resize(hotspots.size(), 1);
    }
    result = new HotSpotTrafficPattern(nodes, hotspots, rates);
  }
  else
  {
    cout << "Error: Unknown traffic pattern: " << pattern << endl;
    exit(-1);
  }
  return result;
}

PermutationTrafficPattern::PermutationTrafficPattern(long long int nodes)
    : TrafficPattern(nodes)
{
}

BitPermutationTrafficPattern::BitPermutationTrafficPattern(long long int nodes)
    : PermutationTrafficPattern(nodes)
{
  if ((nodes & -nodes) != nodes)
  {
    cout << "Error: Bit permutation traffic patterns require the number of "
         << "nodes to be a power of two." << endl;
    exit(-1);
  }
}

BitCompTrafficPattern::BitCompTrafficPattern(long long int nodes)
    : BitPermutationTrafficPattern(nodes)
{
}

long long int BitCompTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  long long int const mask = _nodes - 1;
  return ~source & mask;
}

TransposeTrafficPattern::TransposeTrafficPattern(long long int nodes)
    : BitPermutationTrafficPattern(nodes), _shift(0)
{
  while (nodes >>= 1)
  {
    ++_shift;
  }
  if (_shift % 2)
  {
    cout << "Error: Transpose traffic pattern requires the number of nodes to "
         << "be an even power of two." << endl;
    exit(-1);
  }
  _shift >>= 1;
}

long long int TransposeTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  long long int const mask_lo = (1 << _shift) - 1;
  long long int const mask_hi = mask_lo << _shift;
  return (((source >> _shift) & mask_lo) | ((source << _shift) & mask_hi));
}

BitRevTrafficPattern::BitRevTrafficPattern(long long int nodes)
    : BitPermutationTrafficPattern(nodes)
{
}

long long int BitRevTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  long long int result = 0;
  for (long long int n = _nodes; n > 1; n >>= 1)
  {
    result = (result << 1) | (source % 2);
    source >>= 1;
  }
  return result;
}

ShuffleTrafficPattern::ShuffleTrafficPattern(long long int nodes)
    : BitPermutationTrafficPattern(nodes)
{
}

long long int ShuffleTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  long long int const shifted = source << 1;
  return ((shifted & (_nodes - 1)) | bool(shifted & _nodes));
}

DigitPermutationTrafficPattern::DigitPermutationTrafficPattern(long long int nodes, long long int k,
                                                               long long int n, long long int xr)
    : PermutationTrafficPattern(nodes), _k(k), _n(n), _xr(xr)
{
}

TornadoTrafficPattern::TornadoTrafficPattern(long long int nodes, long long int k, long long int n, long long int xr)
    : DigitPermutationTrafficPattern(nodes, k, n, xr)
{
}

long long int TornadoTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));

  long long int offset = 1;
  long long int result = 0;

  for (long long int n = 0; n < _n; ++n)
  {
    result += offset *
              (((source / offset) % (_xr * _k) + ((_xr * _k + 1) / 2 - 1)) % (_xr * _k));
    offset *= (_xr * _k);
  }
  return result;
}

NeighborTrafficPattern::NeighborTrafficPattern(long long int nodes, long long int k, long long int n, long long int xr)
    : DigitPermutationTrafficPattern(nodes, k, n, xr)
{
}

long long int NeighborTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));

  long long int offset = 1;
  long long int result = 0;

  for (long long int n = 0; n < _n; ++n)
  {
    result += offset *
              (((source / offset) % (_xr * _k) + 1) % (_xr * _k));
    offset *= (_xr * _k);
  }
  return result;
}

RandomPermutationTrafficPattern::RandomPermutationTrafficPattern(long long int nodes,
                                                                 long long int seed)
    : TrafficPattern(nodes)
{
  _dest.resize(nodes);
  randomize(seed);
}

void RandomPermutationTrafficPattern::randomize(long long int seed)
{
  vector<long> save_x;
  vector<double> save_u;
  SaveRandomState(save_x, save_u);
  RandomSeed(seed);

  _dest.assign(_nodes, -1);

  for (long long int i = 0; i < _nodes; ++i)
  {
    long long int ind = RandomInt(_nodes - 1 - i);

    long long int j = 0;
    long long int cnt = 0;
    while ((cnt < ind) || (_dest[j] != -1))
    {
      if (_dest[j] == -1)
      {
        ++cnt;
      }
      ++j;
      assert(j < _nodes);
    }

    _dest[j] = i;
  }

  RestoreRandomState(save_x, save_u);
}

long long int RandomPermutationTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  assert((_dest[source] >= 0) && (_dest[source] < _nodes));
  return _dest[source];
}

RandomTrafficPattern::RandomTrafficPattern(long long int nodes)
    : TrafficPattern(nodes)
{
}

UniformRandomTrafficPattern::UniformRandomTrafficPattern(long long int nodes)
    : RandomTrafficPattern(nodes)
{
}

long long int UniformRandomTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  return RandomInt(_nodes - 1);
}

UniformBackgroundTrafficPattern::UniformBackgroundTrafficPattern(long long int nodes, vector<long long int> excluded_nodes)
    : RandomTrafficPattern(nodes)
{
  for (size_t i = 0; i < excluded_nodes.size(); ++i)
  {
    long long int const node = excluded_nodes[i];
    assert((node >= 0) && (node < _nodes));
    _excluded.insert(node);
  }
}

long long int UniformBackgroundTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));

  long long int result;

  do
  {
    result = RandomInt(_nodes - 1);
  } while (_excluded.count(result) > 0);

  return result;
}

DiagonalTrafficPattern::DiagonalTrafficPattern(long long int nodes)
    : RandomTrafficPattern(nodes)
{
}

long long int DiagonalTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  return ((RandomInt(2) == 0) ? ((source + 1) % _nodes) : source);
}

AsymmetricTrafficPattern::AsymmetricTrafficPattern(long long int nodes)
    : RandomTrafficPattern(nodes)
{
}

long long int AsymmetricTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  long long int const half = _nodes / 2;
  return (source % half) + (RandomInt(1) ? half : 0);
}

Taper64TrafficPattern::Taper64TrafficPattern(long long int nodes)
    : RandomTrafficPattern(nodes)
{
  if (nodes != 64)
  {
    cout << "Error: Tthe Taper64 traffic pattern requires the number of nodes "
         << "to be exactly 64." << endl;
    exit(-1);
  }
}

long long int Taper64TrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  if (RandomInt(1))
  {
    return ((64 + source + 8 * (RandomInt(2) - 1) + (RandomInt(2) - 1)) % 64);
  }
  else
  {
    return RandomInt(_nodes - 1);
  }
}

BadPermDFlyTrafficPattern::BadPermDFlyTrafficPattern(long long int nodes, long long int k, long long int n)
    : DigitPermutationTrafficPattern(nodes, k, n, 1)
{
}

long long int BadPermDFlyTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));

  long long int const grp_size_routers = 2 * _k;
  long long int const grp_size_nodes = grp_size_routers * _k;

  return ((RandomInt(grp_size_nodes - 1) + ((source / grp_size_nodes) + 1) * grp_size_nodes) % _nodes);
}

BadPermYarcTrafficPattern::BadPermYarcTrafficPattern(long long int nodes, long long int k, long long int n,
                                                     long long int xr)
    : DigitPermutationTrafficPattern(nodes, k, n, xr)
{
}

long long int BadPermYarcTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  long long int const row = source / (_xr * _k);
  return RandomInt((_xr * _k) - 1) * (_xr * _k) + row;
}

HotSpotTrafficPattern::HotSpotTrafficPattern(long long int nodes, vector<long long int> hotspots,
                                             vector<long long int> rates)
    : TrafficPattern(nodes), _hotspots(hotspots), _rates(rates), _max_val(-1)
{
  assert(!_hotspots.empty());
  size_t const size = _hotspots.size();
  _rates.resize(size, _rates.empty() ? 1 : _rates.back());
  for (size_t i = 0; i < size; ++i)
  {
    long long int const hotspot = _hotspots[i];
    assert((hotspot >= 0) && (hotspot < _nodes));
    long long int const rate = _rates[i];
    assert(rate >= 0);
    _max_val += rate;
  }
}

long long int HotSpotTrafficPattern::dest(long long int source)
{
  assert((source >= 0) && (source < _nodes));

  if (_hotspots.size() == 1)
  {
    return _hotspots[0];
  }

  long long int pct = RandomInt(_max_val);

  for (size_t i = 0; i < (_hotspots.size() - 1); ++i)
  {
    long long int const limit = _rates[i];
    if (limit > pct)
    {
      return _hotspots[i];
    }
    else
    {
      pct -= limit;
    }
  }
  assert(_rates.back() > pct);
  return _hotspots.back();
}
