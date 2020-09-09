// $Id$
#ifndef _KNCUBE_HPP_
#define _KNCUBE_HPP_

#include "network.hpp"

class KNCube : public Network
{

  bool _mesh;

  long long int _k;
  long long int _n;

  void _ComputeSize(const Configuration &config);
  void _BuildNet(const Configuration &config);

  long long int _LeftChannel(long long int node, long long int dim);
  long long int _RightChannel(long long int node, long long int dim);

  long long int _LeftNode(long long int node, long long int dim);
  long long int _RightNode(long long int node, long long int dim);

public:
  KNCube(const Configuration &config, const string &name, bool mesh);
  static void RegisterRoutingFunctions();

  long long int GetN() const;
  long long int GetK() const;

  double Capacity() const;

  void InsertRandomFaults(const Configuration &config);
};

#endif
