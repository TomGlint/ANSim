/*-------------------------------------------------------------------------
 *                             ORION 2.0 
 *
 *         					Copyright 2009 
 *  	Princeton University, and Regents of the University of California 
 *                         All Rights Reserved
 *
 *                         
 *  ORION 2.0 was developed by Bin Li at Princeton University and Kambiz Samadi at
 *  University of California, San Diego. ORION 2.0 was built on top of ORION 1.0. 
 *  ORION 1.0 was developed by Hangsheng Wang, Xinping Zhu and Xuning Chen at 
 *  Princeton University.
 *
 *  If your use of this software contributes to a published paper, we
 *  request that you cite our paper that appears on our website 
 *  http://www.princeton.edu/~peh/orion.html
 *
 *  Permission to use, copy, and modify this software and its documentation is
 *  granted only under the following terms and conditions.  Both the
 *  above copyright notice and this permission notice must appear in all copies
 *  of the software, derivative works or modified versions, and any portions
 *  thereof, and both notices must appear in supporting documentation.
 *
 *  This software may be distributed (but not offered for sale or transferred
 *  for compensation) to third parties, provided such third parties agree to
 *  abide by the terms and conditions of this notice.
 *
 *  This software is distributed in the hope that it will be useful to the
 *  community, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 *
 *-----------------------------------------------------------------------*/

#ifndef _SIM_POWER_LINK_H
#define _SIM_POWER_LINK_H

/* added by a.mazloumi */
#include "router.hpp"
#include "SIM_port.h"
/* end of a.mazloumi codes */

double computeResistance(double Length);
double computeGroundCapacitance(double Length);
double computeCouplingCapacitance(double Length);
void getOptBuffering(int *k, double *h, double Length);
double LinkDynamicEnergyPerBitPerMeter(double Length, double vdd);
double LinkLeakagePowerPerMeter(double Length, double vdd);
double LinkArea(double Length, unsigned NumBits);

/* added by a.mazloumi */
extern double SIM_link_power_report(Router *current_router);
extern double SIM_link_power_report_static(Router *current_router);
/* end of a.mazloumi */

#endif /* _SIM_POWER_LINK_H */
