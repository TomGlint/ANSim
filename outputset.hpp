// $Id$

#ifndef _OUTPUTSET_HPP_
#define _OUTPUTSET_HPP_

#include <set>

class OutputSet
{

public:
  struct sSetElement
  {
    long long int vc_start;
    long long int vc_end;
    long long int pri;
    long long int output_port;
  };

  void Clear();
  void Add(long long int output_port, long long int vc, long long int pri = 0);
  void AddRange(long long int output_port, long long int vc_start, long long int vc_end, long long int pri = 0);

  bool OutputEmpty(long long int output_port) const;
  long long int NumVCs(long long int output_port) const;

  const set<sSetElement> &GetSet() const;

  long long int GetVC(long long int output_port, long long int vc_index, long long int *pri = 0) const;
  bool GetPortVC(long long int *out_port, long long int *out_vc) const;

private:
  set<sSetElement> _outputs;
};

inline bool operator<(const OutputSet::sSetElement &se1,
                      const OutputSet::sSetElement &se2)
{
  return se1.pri > se2.pri; // higher priorities first!
}

#endif
