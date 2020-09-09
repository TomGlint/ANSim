// $Id$

#ifndef _PIPEFIFO_HPP_
#define _PIPEFIFO_HPP_

#include <vector>

#include "module.hpp"

template <class T>
class PipelineFIFO : public Module
{
  long long int _lanes;
  long long int _depth;

  long long int _pipe_len;
  long long int _pipe_ptr;

  vector<vector<T *>> _data;

public:
  PipelineFIFO(Module *parent, const string &name, long long int lanes, long long int depth);
  ~PipelineFIFO();

  void Write(T *val, long long int lane = 0);
  void WriteAll(T *val);

  T *Read(long long int lane = 0);

  void Advance();
};

template <class T>
PipelineFIFO<T>::PipelineFIFO(Module *parent,
                              const string &name,
                              long long int lanes, long long int depth) : Module(parent, name),
                                                                          _lanes(lanes), _depth(depth)
{
  _pipe_len = depth + 1;
  _pipe_ptr = 0;

  _data.resize(_lanes);
  for (long long int l = 0; l < _lanes; ++l)
  {
    _data[l].resize(_pipe_len, 0);
  }
}

template <class T>
PipelineFIFO<T>::~PipelineFIFO()
{
}

template <class T>
void PipelineFIFO<T>::Write(T *val, long long int lane)
{
  _data[lane][_pipe_ptr] = val;
}

template <class T>
void PipelineFIFO<T>::WriteAll(T *val)
{
  for (long long int l = 0; l < _lanes; ++l)
  {
    _data[l][_pipe_ptr] = val;
  }
}

template <class T>
T *PipelineFIFO<T>::Read(long long int lane)
{
  return _data[lane][_pipe_ptr];
}

template <class T>
void PipelineFIFO<T>::Advance()
{
  _pipe_ptr = (_pipe_ptr + 1) % _pipe_len;
}

#endif
