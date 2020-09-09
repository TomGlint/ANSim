// $Id$

#ifndef _BUFFER_STATE_HPP_
#define _BUFFER_STATE_HPP_

#include <vector>
#include <queue>

#include "module.hpp"
#include "flit.hpp"
#include "credit.hpp"
#include "config_utils.hpp"

class BufferState : public Module
{

  class BufferPolicy : public Module
  {
  protected:
    BufferState const *const _buffer_state;

  public:
    BufferPolicy(Configuration const &config, BufferState *parent,
                 const string &name);
    virtual void SetMinLatency(long long int min_latency) {}
    virtual void TakeBuffer(long long int vc = 0);
    virtual void SendingFlit(Flit const *const f);
    virtual void FreeSlotFor(long long int vc = 0);
    virtual bool IsFullFor(long long int vc = 0) const = 0;
    virtual long long int AvailableFor(long long int vc = 0) const = 0;
    virtual long long int LimitFor(long long int vc = 0) const = 0;

    static BufferPolicy *New(Configuration const &config,
                             BufferState *parent, const string &name);
  };

  class PrivateBufferPolicy : public BufferPolicy
  {
  protected:
    long long int _vc_buf_size;

  public:
    PrivateBufferPolicy(Configuration const &config, BufferState *parent,
                        const string &name);
    virtual void SendingFlit(Flit const *const f);
    virtual bool IsFullFor(long long int vc = 0) const;
    virtual long long int AvailableFor(long long int vc = 0) const;
    virtual long long int LimitFor(long long int vc = 0) const;
  };

  class SharedBufferPolicy : public BufferPolicy
  {
  protected:
    long long int _buf_size;
    vector<long long int> _private_buf_vc_map;
    vector<long long int> _private_buf_size;
    vector<long long int> _private_buf_occupancy;
    long long int _shared_buf_size;
    long long int _shared_buf_occupancy;
    vector<long long int> _reserved_slots;
    void ProcessFreeSlot(long long int vc = 0);

  public:
    SharedBufferPolicy(Configuration const &config, BufferState *parent,
                       const string &name);
    virtual void SendingFlit(Flit const *const f);
    virtual void FreeSlotFor(long long int vc = 0);
    virtual bool IsFullFor(long long int vc = 0) const;
    virtual long long int AvailableFor(long long int vc = 0) const;
    virtual long long int LimitFor(long long int vc = 0) const;
  };

  class LimitedSharedBufferPolicy : public SharedBufferPolicy
  {
  protected:
    long long int _vcs;
    long long int _active_vcs;
    long long int _max_held_slots;

  public:
    LimitedSharedBufferPolicy(Configuration const &config,
                              BufferState *parent,
                              const string &name);
    virtual void TakeBuffer(long long int vc = 0);
    virtual void SendingFlit(Flit const *const f);
    virtual bool IsFullFor(long long int vc = 0) const;
    virtual long long int AvailableFor(long long int vc = 0) const;
    virtual long long int LimitFor(long long int vc = 0) const;
  };

  class DynamicLimitedSharedBufferPolicy : public LimitedSharedBufferPolicy
  {
  public:
    DynamicLimitedSharedBufferPolicy(Configuration const &config,
                                     BufferState *parent,
                                     const string &name);
    virtual void TakeBuffer(long long int vc = 0);
    virtual void SendingFlit(Flit const *const f);
  };

  class ShiftingDynamicLimitedSharedBufferPolicy : public DynamicLimitedSharedBufferPolicy
  {
  public:
    ShiftingDynamicLimitedSharedBufferPolicy(Configuration const &config,
                                             BufferState *parent,
                                             const string &name);
    virtual void TakeBuffer(long long int vc = 0);
    virtual void SendingFlit(Flit const *const f);
  };

  class FeedbackSharedBufferPolicy : public SharedBufferPolicy
  {
  protected:
    long long int _ComputeRTT(long long int vc, long long int last_rtt) const;
    long long int _ComputeLimit(long long int rtt) const;
    long long int _ComputeMaxSlots(long long int vc) const;
    long long int _vcs;
    vector<long long int> _occupancy_limit;
    vector<long long int> _round_trip_time;
    vector<queue<long long int>> _flit_sent_time;
    long long int _min_latency;
    long long int _total_mapped_size;
    long long int _aging_scale;
    long long int _offset;

  public:
    FeedbackSharedBufferPolicy(Configuration const &config,
                               BufferState *parent, const string &name);
    virtual void SetMinLatency(long long int min_latency);
    virtual void SendingFlit(Flit const *const f);
    virtual void FreeSlotFor(long long int vc = 0);
    virtual bool IsFullFor(long long int vc = 0) const;
    virtual long long int AvailableFor(long long int vc = 0) const;
    virtual long long int LimitFor(long long int vc = 0) const;
  };

  class SimpleFeedbackSharedBufferPolicy : public FeedbackSharedBufferPolicy
  {
  protected:
    vector<long long int> _pending_credits;

  public:
    SimpleFeedbackSharedBufferPolicy(Configuration const &config,
                                     BufferState *parent, const string &name);
    virtual void SendingFlit(Flit const *const f);
    virtual void FreeSlotFor(long long int vc = 0);
  };

  bool _wait_for_tail_credit;
  long long int _size;
  long long int _occupancy;
  vector<long long int> _vc_occupancy;
  long long int _vcs;

  BufferPolicy *_buffer_policy;

  vector<long long int> _in_use_by;
  vector<bool> _tail_sent;
  vector<long long int> _last_id;
  vector<long long int> _last_pid;

#ifdef TRACK_BUFFERS
  long long int _classes;
  vector<queue<long long int>> _outstanding_classes;
  vector<long long int> _class_occupancy;
#endif

public:
  BufferState(const Configuration &config,
              Module *parent, const string &name);

  ~BufferState();

  inline void SetMinLatency(long long int min_latency)
  {
    _buffer_policy->SetMinLatency(min_latency);
  }

  void ProcessCredit(Credit const *const c);
  void SendingFlit(Flit const *const f);

  void TakeBuffer(long long int vc = 0, long long int tag = 0);

  inline bool IsFull() const
  {
    assert(_occupancy <= _size);
    return (_occupancy == _size);
  }
  inline bool IsFullFor(long long int vc = 0) const
  {
    return _buffer_policy->IsFullFor(vc);
  }
  inline long long int AvailableFor(long long int vc = 0) const
  {
    return _buffer_policy->AvailableFor(vc);
  }
  inline long long int LimitFor(long long int vc = 0) const
  {
    return _buffer_policy->LimitFor(vc);
  }
  inline bool IsEmptyFor(long long int vc = 0) const
  {
    assert((vc >= 0) && (vc < _vcs));
    return (_vc_occupancy[vc] == 0);
  }
  inline bool IsAvailableFor(long long int vc = 0) const
  {
    assert((vc >= 0) && (vc < _vcs));
    return _in_use_by[vc] < 0;
  }
  inline long long int UsedBy(long long int vc = 0) const
  {
    assert((vc >= 0) && (vc < _vcs));
    return _in_use_by[vc];
  }

  inline long long int Occupancy() const
  {
    return _occupancy;
  }

  inline long long int OccupancyFor(long long int vc = 0) const
  {
    assert((vc >= 0) && (vc < _vcs));
    return _vc_occupancy[vc];
  }

#ifdef TRACK_BUFFERS
  inline long long int OccupancyForClass(long long int c) const
  {
    assert((c >= 0) && (c < _classes));
    return _class_occupancy[c];
  }
#endif

  void Display(ostream &os = cout) const;
};

#endif
