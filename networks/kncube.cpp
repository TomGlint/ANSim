// $Id$
/*kn.cpp
 *
 *Meshs, cube, torus
 *
 */

#include "booksim.hpp"
#include <vector>
#include <sstream>
#include <ctime>
#include <cassert>
#include "kncube.hpp"
#include "random_utils.hpp"
#include "misc_utils.hpp"
//#include "iq_router.hpp"

KNCube::KNCube(const Configuration &config, const string &name, bool mesh) : Network(config, name)
{
  _mesh = mesh;

  _ComputeSize(config);
  _Alloc();
  _BuildNet(config);
}

void KNCube::_ComputeSize(const Configuration &config)
{
  _k = config.GetLongInt("k");
  _n = config.GetLongInt("n");

  gK = _k;
  gN = _n;
  _size = powi(_k, _n);
  _channels = 2 * _n * _size;

  _nodes = _size;
}

void KNCube::RegisterRoutingFunctions()
{
}
void KNCube::_BuildNet(const Configuration &config)
{
  long long int left_node;
  long long int right_node;

  long long int right_input;
  long long int left_input;

  long long int right_output;
  long long int left_output;

  ostringstream router_name;

  //latency type, noc or conventional network
  bool use_noc_latency;
  use_noc_latency = (config.GetLongInt("use_noc_latency") == 1);

  for (long long int node = 0; node < _size; ++node)
  {

    router_name << "router";

    if (_k > 1)
    {
      for (long long int dim_offset = _size / _k; dim_offset >= 1; dim_offset /= _k)
      {
        router_name << "_" << (node / dim_offset) % _k;
      }
    }

    _routers[node] = Router::NewRouter(config, this, router_name.str(),
                                       node, 2 * _n + 1, 2 * _n + 1);
    _timed_modules.push_back(_routers[node]);

    router_name.str("");

    for (long long int dim = 0; dim < _n; ++dim)
    {

      //find the neighbor
      left_node = _LeftNode(node, dim);
      right_node = _RightNode(node, dim);

      //
      // Current (N)ode
      // (L)eft node
      // (R)ight node
      //
      //   L--->N<---R
      //   L<---N--->R
      //

      // torus channel is longer due to wrap around
      long long int latency = _mesh ? 1 : 2;

      //get the input channel number
      right_input = _LeftChannel(right_node, dim);
      left_input = _RightChannel(left_node, dim);

      //add the input channel
      _routers[node]->AddInputChannel(_chan[right_input], _chan_cred[right_input]);
      _routers[node]->AddInputChannel(_chan[left_input], _chan_cred[left_input]);

      //set input channel latency
      if (use_noc_latency)
      {
        _chan[right_input]->SetLatency(latency);
        _chan[left_input]->SetLatency(latency);
        _chan_cred[right_input]->SetLatency(latency);
        _chan_cred[left_input]->SetLatency(latency);
      }
      else
      {
        _chan[left_input]->SetLatency(1);
        _chan_cred[right_input]->SetLatency(1);
        _chan_cred[left_input]->SetLatency(1);
        _chan[right_input]->SetLatency(1);
      }
      //get the output channel number
      right_output = _RightChannel(node, dim);
      left_output = _LeftChannel(node, dim);

      //add the output channel
      _routers[node]->AddOutputChannel(_chan[right_output], _chan_cred[right_output]);
      _routers[node]->AddOutputChannel(_chan[left_output], _chan_cred[left_output]);

      //set output channel latency
      if (use_noc_latency)
      {
        _chan[right_output]->SetLatency(latency);
        _chan[left_output]->SetLatency(latency);
        _chan_cred[right_output]->SetLatency(latency);
        _chan_cred[left_output]->SetLatency(latency);
      }
      else
      {
        _chan[right_output]->SetLatency(1);
        _chan[left_output]->SetLatency(1);
        _chan_cred[right_output]->SetLatency(1);
        _chan_cred[left_output]->SetLatency(1);
      }
    }
    //injection and ejection channel, always 1 latency
    _routers[node]->AddInputChannel(_inject[node], _inject_cred[node]);
    _routers[node]->AddOutputChannel(_eject[node], _eject_cred[node]);
    _inject[node]->SetLatency(1);
    _eject[node]->SetLatency(1);
  }
}

long long int KNCube::_LeftChannel(long long int node, long long int dim)
{
  // The base channel for a node is 2*_n*node
  long long int base = 2 * _n * node;
  // The offset for a left channel is 2*dim + 1
  long long int off = 2 * dim + 1;

  return (base + off);
}

long long int KNCube::_RightChannel(long long int node, long long int dim)
{
  // The base channel for a node is 2*_n*node
  long long int base = 2 * _n * node;
  // The offset for a right channel is 2*dim
  long long int off = 2 * dim;
  return (base + off);
}

long long int KNCube::_LeftNode(long long int node, long long int dim)
{
  long long int k_to_dim = powi(_k, dim);
  long long int loc_in_dim = (node / k_to_dim) % _k;
  long long int left_node;
  // if at the left edge of the dimension, wraparound
  if (loc_in_dim == 0)
  {
    left_node = node + (_k - 1) * k_to_dim;
  }
  else
  {
    left_node = node - k_to_dim;
  }

  return left_node;
}

long long int KNCube::_RightNode(long long int node, long long int dim)
{
  long long int k_to_dim = powi(_k, dim);
  long long int loc_in_dim = (node / k_to_dim) % _k;
  long long int right_node;
  // if at the right edge of the dimension, wraparound
  if (loc_in_dim == (_k - 1))
  {
    right_node = node - (_k - 1) * k_to_dim;
  }
  else
  {
    right_node = node + k_to_dim;
  }

  return right_node;
}

long long int KNCube::GetN() const
{
  return _n;
}

long long int KNCube::GetK() const
{
  return _k;
}

/*legacy, not sure how this fits into the new scheme of things*/
void KNCube::InsertRandomFaults(const Configuration &config)
{
  long long int num_fails = config.GetLongInt("link_failures");

  if (_size && num_fails)
  {
    vector<long> save_x;
    vector<double> save_u;
    SaveRandomState(save_x, save_u);
    long long int fail_seed;
    if (config.GetStr("fail_seed") == "time")
    {
      fail_seed = time(NULL);
      cout << "SEED: fail_seed=" << fail_seed << endl;
    }
    else
    {
      fail_seed = config.GetLongInt("fail_seed");
    }
    RandomSeed(fail_seed);

    vector<bool> fail_nodes(_size);

    for (long long int i = 0; i < _size; ++i)
    {
      long long int node = i;

      // edge test
      bool edge = false;
      for (long long int n = 0; n < _n; ++n)
      {
        if (((node % _k) == 0) ||
            ((node % _k) == _k - 1))
        {
          edge = true;
        }
        node /= _k;
      }

      if (edge)
      {
        fail_nodes[i] = true;
      }
      else
      {
        fail_nodes[i] = false;
      }
    }

    for (long long int i = 0; i < num_fails; ++i)
    {
      long long int j = RandomInt(_size - 1);
      bool available = false;
      long long int node = -1;
      long long int chan = -1;
      long long int t;

      for (t = 0; (t < _size) && (!available); ++t)
      {
        long long int node = (j + t) % _size;

        if (!fail_nodes[node])
        {
          // check neighbors
          long long int c = RandomInt(2 * _n - 1);

          for (long long int n = 0; (n < 2 * _n) && (!available); ++n)
          {
            chan = (n + c) % 2 * _n;

            if (chan % 1)
            {
              available = fail_nodes[_LeftNode(node, chan / 2)];
            }
            else
            {
              available = fail_nodes[_RightNode(node, chan / 2)];
            }
          }
        }

        if (!available)
        {
          cout << "skipping " << node << endl;
        }
      }

      if (t == _size)
      {
        Error("Could not find another possible fault channel");
      }

      assert(node != -1);
      assert(chan != -1);
      OutChannelFault(node, chan);
      fail_nodes[node] = true;

      for (long long int n = 0; (n < _n) && available; ++n)
      {
        fail_nodes[_LeftNode(node, n)] = true;
        fail_nodes[_RightNode(node, n)] = true;
      }

      cout << "failure at node " << node << ", channel "
           << chan << endl;
    }

    RestoreRandomState(save_x, save_u);
  }
}

double KNCube::Capacity() const
{
  return (double)_k / (_mesh ? 8.0 : 4.0);
}
