// $Id$

// ----------------------------------------------------------------------
//
//  File Name: flitchannel.hpp
//
//  The FlitChannel models a flit channel with a multi-cycle
//   transmission delay. The channel latency can be specified as
//   an integer number of simulator cycles.
// ----------------------------------------------------------------------

#ifndef FLITCHANNEL_HPP
#define FLITCHANNEL_HPP

// ----------------------------------------------------------------------
//  $Author: jbalfour $
//  $Date: 2007/06/27 23:10:17 $
//  $Id$
// ----------------------------------------------------------------------

#include "channel.hpp"
#include "flit.hpp"

using namespace std;

class Router;

class FlitChannel : public Channel<Flit>
{
public:
  FlitChannel(Module *parent, string const &name, long long int classes);

  void SetSource(Router const *const router, long long int port);
  inline Router const *const GetSource() const
  {
    return _routerSource;
  }
  inline long long int const &GetSourcePort() const
  {
    return _routerSourcePort;
  }
  void SetSink(Router const *const router, long long int port);
  inline Router const *const GetSink() const
  {
    return _routerSink;
  }
  inline long long int const &GetSinkPort() const
  {
    return _routerSinkPort;
  }
  inline vector<long long int> const &GetActivity() const
  {
    return _active;
  }

  // Send flit
  virtual void Send(Flit *flit);

  virtual void ReadInputs();
  virtual void WriteOutputs();

private:
  ////////////////////////////////////////
  //
  // Power Models OBSOLETE
  //
  ////////////////////////////////////////

  Router const *_routerSource;
  long long int _routerSourcePort;
  Router const *_routerSink;
  long long int _routerSinkPort;

  // Statistics for Activity Factors
  vector<long long int> _active;
  long long int _idle;
};

#endif
