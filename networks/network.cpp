// $Id$

/*
 Copyright (c) 2007-2012, Trustees of The Leland Stanford Junior University
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this 
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice, this
 list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*network.cpp
 *
 *This class is the basis of the entire network, it contains, all the routers
 *channels in the network, and is extended by all the network topologies
 *
 */

#include <cassert>
#include <sstream>

//Orion Power Support
#include <fstream>

#include "booksim.hpp"
#include "network.hpp"

#include "kncube.hpp"
#include "fly.hpp"
#include "cmesh.hpp"
#include "flatfly_onchip.hpp"
#include "qtree.hpp"
#include "tree4.hpp"
#include "fattree.hpp"
#include "anynet.hpp"
#include "dragonfly.hpp"

Network::Network(const Configuration &config, const string &name) : TimedModule(0, name)
{
  _size = -1;
  _nodes = -1;
  _channels = -1;
  _classes = config.GetLongInt("classes");
}

Network::~Network()
{
  for (long long int r = 0; r < _size; ++r)
  {
    if (_routers[r])
      delete _routers[r];
  }
  for (long long int s = 0; s < _nodes; ++s)
  {
    if (_inject[s])
      delete _inject[s];
    if (_inject_cred[s])
      delete _inject_cred[s];
  }
  for (long long int d = 0; d < _nodes; ++d)
  {
    if (_eject[d])
      delete _eject[d];
    if (_eject_cred[d])
      delete _eject_cred[d];
  }
  for (long long int c = 0; c < _channels; ++c)
  {
    if (_chan[c])
      delete _chan[c];
    if (_chan_cred[c])
      delete _chan_cred[c];
  }
}

Network *Network::New(const Configuration &config, const string &name)
{
  const string topo = config.GetStr("topology");
  Network *n = NULL;
  if (topo == "torus")
  {
    KNCube::RegisterRoutingFunctions();
    n = new KNCube(config, name, false);
  }
  else if (topo == "mesh")
  {
    KNCube::RegisterRoutingFunctions();
    n = new KNCube(config, name, true);
  }
  else if (topo == "cmesh")
  {
    CMesh::RegisterRoutingFunctions();
    n = new CMesh(config, name);
  }
  else if (topo == "fly")
  {
    KNFly::RegisterRoutingFunctions();
    n = new KNFly(config, name);
  }
  else if (topo == "qtree")
  {
    QTree::RegisterRoutingFunctions();
    n = new QTree(config, name);
  }
  else if (topo == "tree4")
  {
    Tree4::RegisterRoutingFunctions();
    n = new Tree4(config, name);
  }
  else if (topo == "fattree")
  {
    FatTree::RegisterRoutingFunctions();
    n = new FatTree(config, name);
  }
  else if (topo == "flatfly")
  {
    FlatFlyOnChip::RegisterRoutingFunctions();
    n = new FlatFlyOnChip(config, name);
  }
  else if (topo == "anynet")
  {
    AnyNet::RegisterRoutingFunctions();
    n = new AnyNet(config, name);
  }
  else if (topo == "dragonflynew")
  {
    DragonFlyNew::RegisterRoutingFunctions();
    n = new DragonFlyNew(config, name);
  }
  else
  {
    cerr << "Unknown topology: " << topo << endl;
  }

  /*legacy code that insert random faults in the networks
   *not sure how to use this
   */
  if (n && (config.GetLongInt("link_failures") > 0))
  {
    n->InsertRandomFaults(config);
  }
  return n;
}

void Network::_Alloc()
{
  assert((_size != -1) &&
         (_nodes != -1) &&
         (_channels != -1));

  _routers.resize(_size);
  gNodes = _nodes;

  /*booksim used arrays of flits as the channels which makes have capacity of
   *one. To simulate channel latency, flitchannel class has been added
   *which are fifos with depth = channel latency and each cycle the channel
   *shifts by one
   *credit channels are the necessary counter part
   */
  _inject.resize(_nodes);
  _inject_cred.resize(_nodes);
  for (long long int s = 0; s < _nodes; ++s)
  {
    ostringstream name;
    name << Name() << "_fchan_ingress" << s;
    _inject[s] = new FlitChannel(this, name.str(), _classes);
    _inject[s]->SetSource(NULL, s);
    _timed_modules.push_back(_inject[s]);
    name.str("");
    name << Name() << "_cchan_ingress" << s;
    _inject_cred[s] = new CreditChannel(this, name.str());
    _timed_modules.push_back(_inject_cred[s]);
  }
  _eject.resize(_nodes);
  _eject_cred.resize(_nodes);
  for (long long int d = 0; d < _nodes; ++d)
  {
    ostringstream name;
    name << Name() << "_fchan_egress" << d;
    _eject[d] = new FlitChannel(this, name.str(), _classes);
    _eject[d]->SetSink(NULL, d);
    _timed_modules.push_back(_eject[d]);
    name.str("");
    name << Name() << "_cchan_egress" << d;
    _eject_cred[d] = new CreditChannel(this, name.str());
    _timed_modules.push_back(_eject_cred[d]);
  }
  _chan.resize(_channels);
  _chan_cred.resize(_channels);
  for (long long int c = 0; c < _channels; ++c)
  {
    ostringstream name;
    name << Name() << "_fchan_" << c;
    _chan[c] = new FlitChannel(this, name.str(), _classes);
    _timed_modules.push_back(_chan[c]);
    name.str("");
    name << Name() << "_cchan_" << c;
    _chan_cred[c] = new CreditChannel(this, name.str());
    _timed_modules.push_back(_chan_cred[c]);
  }
}

void Network::ReadInputs()
{
  for (deque<TimedModule *>::const_iterator iter = _timed_modules.begin();
       iter != _timed_modules.end();
       ++iter)
  {
    (*iter)->ReadInputs();
  }
}

void Network::Evaluate()
{
  for (deque<TimedModule *>::const_iterator iter = _timed_modules.begin();
       iter != _timed_modules.end();
       ++iter)
  {
    (*iter)->Evaluate();
  }
}

void Network::WriteOutputs()
{
  for (deque<TimedModule *>::const_iterator iter = _timed_modules.begin();
       iter != _timed_modules.end();
       ++iter)
  {
    (*iter)->WriteOutputs();
  }
}

void Network::WriteFlit(Flit *f, long long int source)
{
  assert((source >= 0) && (source < _nodes));
  _inject[source]->Send(f);
}

Flit *Network::ReadFlit(long long int dest)
{
  assert((dest >= 0) && (dest < _nodes));
  return _eject[dest]->Receive();
}

void Network::WriteCredit(Credit *c, long long int dest)
{
  assert((dest >= 0) && (dest < _nodes));
  _eject_cred[dest]->Send(c);
}

Credit *Network::ReadCredit(long long int source)
{
  assert((source >= 0) && (source < _nodes));
  return _inject_cred[source]->Receive();
}

void Network::InsertRandomFaults(const Configuration &config)
{
  Error("InsertRandomFaults not implemented for this topology!");
}

void Network::OutChannelFault(long long int r, long long int c, bool fault)
{
  assert((r >= 0) && (r < _size));
  _routers[r]->OutChannelFault(c, fault);
}

double Network::Capacity() const
{
  return 1.0;
}

/* this function can be heavily modified to display any information
 * neceesary of the network, by default, call display on each router
 * and display the channel utilization rate
 */
void Network::Display(ostream &os) const
{
  for (long long int r = 0; r < _size; ++r)
  {
    _routers[r]->Display(os);
  }
}

void Network::DumpChannelMap(ostream &os, string const &prefix) const
{
  os << prefix << "source_router,source_port,dest_router,dest_port" << endl;
  for (long long int c = 0; c < _nodes; ++c)
  {
    //  cout << "\n_Begin\n";
    //  cout << prefix
    //       << "-1,"
    //       << _inject[c]->GetSourcePort() << ','
    //       << _inject[c]->GetSink()->GetID() << ','
    //       << _inject[c]->GetSinkPort() << endl;
    os << prefix
       << "-1,"
       << _inject[c]->GetSourcePort() << ','
       << _inject[c]->GetSink()->GetID() << ','
       << _inject[c]->GetSinkPort() << endl;
  }
  for (long long int c = 0; c < _channels; ++c)
  {
    cout << "\n_channels\n";
    cout << prefix
         << _chan[c]->GetSource()->GetID() << ','
         << _chan[c]->GetSourcePort() << ','
         << _chan[c]->GetSink()->GetID() << ','
         << _chan[c]->GetSinkPort() << endl;
    os << prefix
       << _chan[c]->GetSource()->GetID() << ','
       << _chan[c]->GetSourcePort() << ','
       << _chan[c]->GetSink()->GetID() << ','
       << _chan[c]->GetSinkPort() << endl;
  }
  for (long long int c = 0; c < _nodes; ++c)
  {
    //  cout << "\n_nodes\n";
    //  cout << prefix
    //       << _eject[c]->GetSource()->GetID() << ','
    //       << _eject[c]->GetSourcePort() << ','
    //       << "-1,"
    //       << _eject[c]->GetSinkPort() << endl;
    os << prefix
       << _eject[c]->GetSource()->GetID() << ','
       << _eject[c]->GetSourcePort() << ','
       << "-1,"
       << _eject[c]->GetSinkPort() << endl;
  }
}

void Network::DumpNodeMap(ostream &os, string const &prefix) const
{
  os << prefix << "source_router,dest_router" << endl;
  cout << prefix << "source_router,dest_router" << endl;
  for (long long int s = 0; s < _nodes; ++s)
  {
    cout << "\n Hello hello I am back!\n";
    cout << prefix
         << _eject[s]->GetSource()->GetID() << ','
         << _inject[s]->GetSink()->GetID() << endl;
    os << prefix
       << _eject[s]->GetSource()->GetID() << ','
       << _inject[s]->GetSink()->GetID() << endl;
  }
}

//orion power support
void Network::_PowerReports(int report_mode, string fileout, long long power_time)
{

  ofstream PowerOut;
  PowerOut.open(fileout.c_str(), std::ofstream::app);

  double total_power_crossbar = 0.0;
  double total_power_buffer = 0.0;
  double total_power_link = 0.0;
  double total_power_SW_arbiter = 0.0;
  double total_power_VC_arbiter = 0.0;
  double total_power_routing = 0.0;

  double static_power_crossbar = 0.0;
  double static_power_buffer = 0.0;
  double static_power_link = 0.0;
  double static_power_SW_arbiter = 0.0;
  double static_power_VC_arbiter = 0.0;
 // double static_power_routing = 0.0;

  double total_power_CSRegister = 0.0;
  int total_number_of_crossed_flits = 0;
  int total_number_of_crossed_headerFlits = 0;
  int total_number_of_calls_of_power_functions = 0;

  if (report_mode == 0)
  {
    for (int r = 0; r < _size; r++)
    {

      total_number_of_crossed_flits += _routers[r]->_number_of_crossed_flits;
      total_number_of_crossed_headerFlits += _routers[r]->_number_of_crossed_headerFlits;
      total_number_of_calls_of_power_functions += _routers[r]->_number_of_calls_of_power_functions;

      //double SIM_crossbar_report(SIM_crossbar_t *crsbar)
      total_power_crossbar += SIM_crossbar_report(&(_routers[r]->_orion_router_power.crossbar));
      //double SIM_array_power_report(SIM_array_info_t *info, SIM_array_t *arr );
      total_power_buffer += SIM_array_power_report(&(_routers[r]->_orion_router_info.in_buf_info), &(_routers[r]->_orion_router_power.in_buf));
      //double SIM_link_power_report(Router * current_router );
      total_power_link += SIM_link_power_report(_routers[r]);
      static_power_link += SIM_link_power_report_static(_routers[r]);
      //double SIM_arbiter_report(SIM_arbiter_t *arb);
      total_power_SW_arbiter += SIM_arbiter_report(&(_routers[r]->_orion_router_power.sw_out_arb));
      if (!(PARM_v_channel < 2 && PARM_v_class < 2))
        total_power_VC_arbiter += SIM_arbiter_report(&(_routers[r]->_orion_router_power.vc_out_arb));
    }
    total_power_crossbar *= bitwidth64x;
    total_power_buffer *= bitwidth64x;
    total_power_link *= bitwidth64x;

    total_power_crossbar *= (PARM_Freq / power_time);
    total_power_buffer *= (PARM_Freq / power_time);
    total_power_link *= (PARM_Freq / power_time);
    total_power_SW_arbiter *= (PARM_Freq / power_time);
    total_power_VC_arbiter *= (PARM_Freq / power_time);

    static_power_crossbar = _size * _routers[0]->_orion_router_power.I_crossbar_static * Vdd * SCALE_S * bitwidth64x; //* Period;// * power_time;
    static_power_buffer = _size * _routers[0]->_orion_router_power.I_buf_static * Vdd * SCALE_S * bitwidth64x / 2.0;  //* Period;// * power_time;
    static_power_SW_arbiter = _size * _routers[0]->_orion_router_power.I_sw_arbiter_static * Vdd * SCALE_S;           // * Period;// * power_time;//* bitwidth64x;
    static_power_VC_arbiter = _size * _routers[0]->_orion_router_power.I_vc_arbiter_static * Vdd * SCALE_S;           // * Period;// * power_time;//* bitwidth64x;
    static_power_link *= bitwidth64x;
    /*    cout<<"\n==i static= "<<_routers[0]->_orion_router_power.I_crossbar_static<<endl;
    cout<<"\n==vdd= "<<Vdd<<endl;
    cout<<"\n==SCALE_S= "<<SCALE_S<<endl;
    cout<<"\n==tg= "<<Period<<endl;
    cout<<"\n==time= "<<power_time<<endl;*/

    total_power_routing = total_power_VC_arbiter;

    total_power_CSRegister = (((double)g_total_cs_register_writes) * (677e-11));

    PowerOut << "Crossbar Power  :: Dynamic: " << total_power_crossbar << "  Static: " << static_power_crossbar << "  Total: " << static_power_crossbar + total_power_crossbar << "\n";
    PowerOut << "Buffer Power    :: Dynamic: " << total_power_buffer << "  Static: " << static_power_buffer << "  Total: " << static_power_buffer + total_power_buffer << "\n";
    PowerOut << "SW_arbiter Power:: Dynamic: " << total_power_SW_arbiter << "  Static: " << static_power_SW_arbiter << "  Total: " << static_power_SW_arbiter + total_power_SW_arbiter << "\n";
    PowerOut << "VC_arbiter Power:: Dynamic: " << total_power_VC_arbiter << "  Static: " << static_power_VC_arbiter << "  Total: " << static_power_VC_arbiter + total_power_VC_arbiter << "\n";
    PowerOut << "Link Power      :: Dynamic: " << total_power_link << "  Static: " << static_power_link << "  Total: " << static_power_VC_arbiter + total_power_link << "\n";
    //PowerOut << "Total Routing     Energy Consumption:   \t" << total_power_routing << "\n";
    PowerOut << "Circuit-switching Power (CS mode only):\t" << total_power_CSRegister << "\n";

    double total_power = total_power_crossbar + total_power_buffer + total_power_link + total_power_SW_arbiter + total_power_VC_arbiter + total_power_routing + total_power_CSRegister;
    double static_power = static_power_crossbar + static_power_buffer + static_power_link + static_power_SW_arbiter + static_power_VC_arbiter;

    total_power += static_power;

    PowerOut << "\nTotal Average Power Consumption (W)    =\t" << total_power << endl;
    PowerOut << "Total Average Energy per Flit =\t" << (double)((total_power * Period * power_time) / g_number_of_retired_flits) << "\n\n";

    PowerOut << "Power Breakdown:\n"
             << "\tCrossbars:\t" << (double)(total_power_crossbar + static_power_crossbar) / total_power * 100 << " %\n"
             << "\tBuffers:\t" << (double)(total_power_buffer + static_power_buffer) / total_power * 100 << " %\n"
             << "\tLinks:\t\t" << (double)(total_power_link + static_power_link) / total_power * 100 << " %\n"
             << "\tSW_arbiters:\t" << (double)(total_power_SW_arbiter + static_power_SW_arbiter) / total_power * 100 << " %\n"
             << "\tVC_arbiters:\t" << (double)(total_power_VC_arbiter + static_power_VC_arbiter) / total_power * 100 << " %\n";
    //<< "\tRouting:\t" << (double) total_power_routing/total_power*100 << " %\n";

    PowerOut << "\nInternal Reports:\nTotal Number of Detected Flits:\t\t" << total_number_of_crossed_flits << " ( Header: " << total_number_of_crossed_headerFlits << " )" << endl;
    PowerOut << "Total Number of Orion Function Calls:\t" << total_number_of_calls_of_power_functions << endl;

    cout << "\nPower Report Header,"
         << " Frequency, Time, StaticIQ, DynamicIQ, StaticVC, DynamicVC, StaticSA, DynamicSA, StaticST, DynamicST, StaticLink, DynamicLink, NoFlits, ePerFlit" << endl;
    cout << "Power Report,"
         << PARM_Freq << ","
         << power_time << ","
         << static_power_buffer << ","
         << total_power_buffer << ","
         << static_power_VC_arbiter << ","
         << total_power_VC_arbiter << ","
         << static_power_SW_arbiter << ","
         << total_power_SW_arbiter << ","
         << static_power_crossbar << ","
         << total_power_crossbar << ","
         << static_power_link << ","
         << total_power_link << ","
         << g_number_of_retired_flits << ","
         << (double)((total_power * Period * power_time) / g_number_of_retired_flits) << endl;
  }
  else
  {
    PowerOut << "Power Consumption for :\n";
    for (int r = 0; r < _size; r++)
    {
      PowerOut << "Router " << r << " :\n\tCrossbar:\t" << SIM_crossbar_report(&(_routers[r]->_orion_router_power.crossbar))
               << "\n\tBuffers:\t" << SIM_array_power_report(&(_routers[r]->_orion_router_info.in_buf_info), &(_routers[r]->_orion_router_power.in_buf))
               << "\n\tLinks:\t" << SIM_link_power_report(_routers[r])
               << "\n\tSW_arbiter:\t" << SIM_arbiter_report(&(_routers[r]->_orion_router_power.sw_out_arb))
               << "\n\tVC_arbiter:\t" << SIM_arbiter_report(&(_routers[r]->_orion_router_power.vc_out_arb)) << "\n";
    }
  }

  PowerOut.close();
}