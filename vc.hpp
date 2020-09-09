// $Id$

#ifndef _VC_HPP_
#define _VC_HPP_

#include <deque>

#include "flit.hpp"
#include "outputset.hpp"
#include "routefunc.hpp"
#include "config_utils.hpp"

class VC : public Module
{
public:
  enum eVCState
  {
    state_min = 0,
    idle = state_min,
    routing,
    vc_alloc,
    active,
    state_max = active
  };
  struct state_info_t
  {
    long long int cycles;
  };
  static const char *const VCSTATE[];

private:
  deque<Flit *> _buffer;

  eVCState _state;

  OutputSet *_route_set;
  long long int _out_port, _out_vc;

  enum ePrioType
  {
    local_age_based,
    queue_length_based,
    hop_count_based,
    none,
    other
  };

  ePrioType _pri_type;

  long long int _pri;

  long long int _priority_donation;

  bool _watched;

  long long int _expected_pid;

  long long int _last_id;
  long long int _last_pid;

  bool _lookahead_routing;

public:
  VC(const Configuration &config, long long int outputs,
     Module *parent, const string &name);
  ~VC();

  void AddFlit(Flit *f);
  inline Flit *FrontFlit() const
  {
    return _buffer.empty() ? NULL : _buffer.front();
  }

  Flit *RemoveFlit();

  inline bool Empty() const
  {
    return _buffer.empty();
  }

  inline VC::eVCState GetState() const
  {
    return _state;
  }

  void SetState(eVCState s);

  const OutputSet *GetRouteSet() const;
  void SetRouteSet(OutputSet *output_set);

  void SetOutput(long long int port, long long int vc);

  inline long long int GetOutputPort() const
  {
    return _out_port;
  }

  inline long long int GetOutputVC() const
  {
    return _out_vc;
  }

  void UpdatePriority();

  inline long long int GetPriority() const
  {
    return _pri;
  }
  void Route(tRoutingFunction rf, const Router *router, const Flit *f, long long int in_channel);

  inline long long int GetOccupancy() const
  {
    return (long long int)_buffer.size();
  }

  // ==== Debug functions ====

  void SetWatch(bool watch = true);
  bool IsWatched() const;
  void Display(ostream &os = cout) const;
};

#endif
