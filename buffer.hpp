// $Id$

#ifndef _BUFFER_HPP_
#define _BUFFER_HPP_

#include <vector>

#include "vc.hpp"
#include "flit.hpp"
#include "outputset.hpp"
#include "routefunc.hpp"
#include "config_utils.hpp"

class Buffer : public Module
{

  long long int _occupancy;
  long long int _size;

  vector<VC *> _vc;

#ifdef TRACK_BUFFERS
  vector<long long int> _class_occupancy;
#endif

public:
  Buffer(const Configuration &config, long long int outputs,
         Module *parent, const string &name);
  ~Buffer();

  void AddFlit(long long int vc, Flit *f);

  inline Flit *RemoveFlit(long long int vc)
  {
    --_occupancy;
#ifdef TRACK_BUFFERS
    long long int cl = _vc[vc]->FrontFlit()->cl;
    assert(_class_occupancy[cl] > 0);
    --_class_occupancy[cl];
#endif
    return _vc[vc]->RemoveFlit();
  }

  inline Flit *FrontFlit(long long int vc) const
  {
    return _vc[vc]->FrontFlit();
  }

  inline bool Empty(long long int vc) const
  {
    return _vc[vc]->Empty();
  }

  inline bool Full() const
  {
    return _occupancy >= _size;
  }

  inline VC::eVCState GetState(long long int vc) const
  {
    return _vc[vc]->GetState();
  }

  inline void SetState(long long int vc, VC::eVCState s)
  {
    _vc[vc]->SetState(s);
  }

  inline const OutputSet *GetRouteSet(long long int vc) const
  {
    return _vc[vc]->GetRouteSet();
  }

  inline void SetRouteSet(long long int vc, OutputSet *output_set)
  {
    _vc[vc]->SetRouteSet(output_set);
  }

  inline void SetOutput(long long int vc, long long int out_port, long long int out_vc)
  {
    _vc[vc]->SetOutput(out_port, out_vc);
  }

  inline long long int GetOutputPort(long long int vc) const
  {
    return _vc[vc]->GetOutputPort();
  }

  inline long long int GetOutputVC(long long int vc) const
  {
    return _vc[vc]->GetOutputVC();
  }

  inline long long int GetPriority(long long int vc) const
  {
    return _vc[vc]->GetPriority();
  }

  inline void Route(long long int vc, tRoutingFunction rf, const Router *router, const Flit *f, long long int in_channel)
  {
    _vc[vc]->Route(rf, router, f, in_channel);
  }

  // ==== Debug functions ====

  inline void SetWatch(long long int vc, bool watch = true)
  {
    _vc[vc]->SetWatch(watch);
  }

  inline bool IsWatched(long long int vc) const
  {
    return _vc[vc]->IsWatched();
  }

  inline long long int GetOccupancy() const
  {
    return _occupancy;
  }

  inline long long int GetOccupancy(long long int vc) const
  {
    return _vc[vc]->GetOccupancy();
  }

#ifdef TRACK_BUFFERS
  inline long long int GetOccupancyForClass(long long int c) const
  {
    return _class_occupancy[c];
  }
#endif

  void Display(ostream &os = cout) const;
};

#endif
