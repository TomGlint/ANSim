// $Id$

#ifndef _ROUTEFUNC_HPP_
#define _ROUTEFUNC_HPP_

#include <vector>
#include <map>

#include "flit.hpp"
#include "router.hpp"
#include "outputset.hpp"
#include "config_utils.hpp"

typedef void (*tRoutingFunction)(const Router *, const Flit *, long long int in_channel, OutputSet *, bool);

void InitializeRoutingMap(const Configuration &config);

extern map<string, tRoutingFunction> gRoutingFunctionMap;

extern long long int gNumVCs;
extern long long int gNumClasses;
extern vector<long long int> gBeginVCs;
extern vector<long long int> gEndVCs;

#endif
