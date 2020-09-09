// $Id$

#ifndef _STATS_HPP_
#define _STATS_HPP_

#include "module.hpp"

class Stats : public Module
{
  long long int _num_samples;
  double _sample_sum;
  double _sample_squared_sum;

  //bool _reset;
  double _min;
  double _max;

  long long int _num_bins;
  double _bin_size;

  vector<long long int> _hist;

public:
  Stats(Module *parent, const string &name,
        double bin_size = 1.0, long long int num_bins = 10);

  void Clear();

  double Average() const;
  double Variance() const;
  double Max() const;
  double Min() const;
  double Sum() const;
  double SquaredSum() const;
  long long int NumSamples() const;

  void AddSample(double val);
  inline void AddSample(long long int val)
  {
    AddSample((double)val);
  }

  long long int GetBin(long long int b) { return _hist[b]; }

  void Display(ostream &os = cout) const;

  friend ostream &operator<<(ostream &os, const Stats &s);
};

ostream &operator<<(ostream &os, const Stats &s);

#endif
