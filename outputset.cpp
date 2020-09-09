// $Id$

/*outputset.cpp
 *
 *output set assigns a flit which output to go to in a router
 *used by the VC class
 *the output assignment is done by the routing algorithms..
 *
 */

#include <cassert>

#include "booksim.hpp"
#include "outputset.hpp"

void OutputSet::Clear()
{
  _outputs.clear();
}

void OutputSet::Add(long long int output_port, long long int vc, long long int pri)
{
  AddRange(output_port, vc, vc, pri);
}

void OutputSet::AddRange(long long int output_port, long long int vc_start, long long int vc_end, long long int pri)
{

  sSetElement s;

  s.vc_start = vc_start;
  s.vc_end = vc_end;
  s.pri = pri;
  s.output_port = output_port;
  _outputs.insert(s);
}

//legacy support, for performance, just use GetSet()
long long int OutputSet::NumVCs(long long int output_port) const
{
  long long int total = 0;
  set<sSetElement>::const_iterator i = _outputs.begin();
  while (i != _outputs.end())
  {
    if (i->output_port == output_port)
    {
      total += (i->vc_end - i->vc_start + 1);
    }
    i++;
  }
  return total;
}

bool OutputSet::OutputEmpty(long long int output_port) const
{
  set<sSetElement>::const_iterator i = _outputs.begin();
  while (i != _outputs.end())
  {
    if (i->output_port == output_port)
    {
      return false;
    }
    i++;
  }
  return true;
}

const set<OutputSet::sSetElement> &OutputSet::GetSet() const
{
  return _outputs;
}

//legacy support, for performance, just use GetSet()
long long int OutputSet::GetVC(long long int output_port, long long int vc_index, long long int *pri) const
{

  long long int range;
  long long int remaining = vc_index;
  long long int vc = -1;

  if (pri)
  {
    *pri = -1;
  }

  set<sSetElement>::const_iterator i = _outputs.begin();
  while (i != _outputs.end())
  {
    if (i->output_port == output_port)
    {
      range = i->vc_end - i->vc_start + 1;
      if (remaining >= range)
      {
        remaining -= range;
      }
      else
      {
        vc = i->vc_start + remaining;
        if (pri)
        {
          *pri = i->pri;
        }
        break;
      }
    }
    i++;
  }
  return vc;
}

//legacy support, for performance, just use GetSet()
bool OutputSet::GetPortVC(long long int *out_port, long long int *out_vc) const
{

  bool single_output = false;
  long long int used_outputs = 0;

  set<sSetElement>::const_iterator i = _outputs.begin();
  if (i != _outputs.end())
  {
    used_outputs = i->output_port;
  }
  while (i != _outputs.end())
  {

    if (i->vc_start == i->vc_end)
    {
      *out_vc = i->vc_start;
      *out_port = i->output_port;
      single_output = true;
    }
    else
    {
      // multiple vc's selected
      break;
    }
    if (used_outputs != i->output_port)
    {
      // multiple outputs selected
      single_output = false;
      break;
    }
    i++;
  }
  return single_output;
}
