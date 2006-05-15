//____________________________________________________________________________
/*
 Copyright (c) 2003-2006, GENIE Neutrino MC Generator Collaboration
 All rights reserved.
 For the licensing terms see $GENIE/USER_LICENSE.

 Author: Costas Andreopoulos <C.V.Andreopoulos@rl.ac.uk>
         CCLRC, Rutherford Appleton Laboratory - March 11, 2005

 For the class documentation see the corresponding header file.

 Important revisions after version 2.0.0 :

*/
//____________________________________________________________________________

#include <TMath.h>

#include "Algorithm/AlgConfigPool.h"
#include "Conventions/Constants.h"
#include "Conventions/RefFrame.h"
#include "Messenger/Messenger.h"
#include "PDG/PDGCodes.h"
#include "PDG/PDGUtils.h"
#include "ReinSeghal/ReinSeghalCOHPXSec.h"
#include "Utils/HadXSUtils.h"

using namespace genie;
using namespace genie::constants;

//____________________________________________________________________________
ReinSeghalCOHPXSec::ReinSeghalCOHPXSec() :
XSecAlgorithmI("genie::ReinSeghalCOHPXSec")
{

}
//____________________________________________________________________________
ReinSeghalCOHPXSec::ReinSeghalCOHPXSec(string config) :
XSecAlgorithmI("genie::ReinSeghalCOHPXSec", config)
{

}
//____________________________________________________________________________
ReinSeghalCOHPXSec::~ReinSeghalCOHPXSec()
{

}
//____________________________________________________________________________
double ReinSeghalCOHPXSec::XSec(
                 const Interaction * interaction, KinePhaseSpace_t kps) const
{
  if(! this -> ValidProcess    (interaction) ) return 0.;
  if(! this -> ValidKinematics (interaction) ) return 0.;

  const Kinematics &   kinematics = interaction -> GetKinematics();
  const InitialState & init_state = interaction -> GetInitialState();

  //----- Compute the coherent NC pi0 production d2xsec/dxdy
  //      see page 34 in Nucl.Phys.B223:29-144 (1983)
  double E      = init_state.GetProbeE(kRfLab); // neutrino energy
  double x      = kinematics.x(); // bjorken x
  double y      = kinematics.y(); // inelasticity y
  double Q2     = 2.*x*y*kNucleonMass*E;  // momentum transfer Q2>0
  double A      = (double) init_state.GetTarget().A(); // mass number
  double A2     = TMath::Power(A,2.);
  double A_3    = TMath::Power(A,1./3.);
  double Gf     = kGF2 * kNucleonMass / (32 * kPi3);
  double fp     = 0.93 * kPionMass; // pion decay constant
  double fp2    = TMath::Power(fp,2.);
  double Epi    = y*E; // pion energy
  double ma2    = TMath::Power(fMa,2);
  double propg  = TMath::Power(ma2/(ma2+Q2),2.); // propagator term
  double r2     = TMath::Power(fReIm,2.);
  double sTot   = utils::hadxs::TotalPionNucleonXSec(Epi); // tot. pi+N xsec
  double sTot2  = TMath::Power(sTot,2.);
  double sInel  = utils::hadxs::InelasticPionNucleonXSec(Epi); // inel. pi+N xsec
  double Ro2    = TMath::Power(fRo,2.);

  // effect of pion absorption in the nucleus
  double Fabs   = TMath::Exp( -9.*A_3*sInel / (16.*kPi*Ro2) );

  // the xsec in Nucl.Phys.B223:29-144 (1983) is d^3xsec/dxdydt but the only
  // t-dependent factor is an exp(-bt) so it can be integrated analyticaly
  double Epi2   = TMath::Power(Epi,2.);
  double R      = fRo * A_3; // nuclear radius
  double R2     = TMath::Power(R,2.);
  double b      = 0.33333 * R2;
  double MxEpi  = kNucleonMass*x/Epi;
  double mEpi2  = kPionMass2/Epi2;
  double tA     = 1. + MxEpi - 0.5*mEpi2;
  double tB     = TMath::Sqrt(1. + 2*MxEpi) * TMath::Sqrt(1.-mEpi2);
  double tmin   = 2*Epi2 * (tA-tB);
  double tmax   = 2*Epi2 * (tA+tB);
  double tint   = (TMath::Exp(-b*tmin) - TMath::Exp(-b*tmax))/b; // t integral

  double xsec = Gf*fp2 * A2 * E*(1-y) * sTot2 * (1+r2)*propg * Fabs*tint;

  LOG("ReinSeghalCoh", pDEBUG)
      << "\n momentum transfer .............. Q2    = " << Q2
      << "\n mass number .................... A     = " << A
      << "\n pion energy .................... Epi   = " << Epi
      << "\n propagator term ................ propg = " << propg
      << "\n Re/Im of fwd pion scat. ampl. .. Re/Im = " << fRo
      << "\n total pi+N cross section ....... sigT  = " << sTot
      << "\n inelastic pi+N cross section ... sigI  = " << sInel
      << "\n nuclear size scale ............. Ro    = " << fRo
      << "\n pion absorption factor ......... Fabs  = " << Fabs
      << "\n t integration range ............ [" << tmin << "," << tmax << "]"
      << "\n t integration factor ........... tint  =" << tint;

  // (CC COH xsec) = 2 * (NC COH XSEC)
  if(interaction->GetProcessInfo().IsWeakCC()) { xsec *= 2.; }

  LOG("ReinSeghalCoh", pINFO)
                << "d2xsec/dxdy[COH] (x= " << x << ", y="
                                         << y << ", E=" << E << ") = "<< xsec;
  return xsec;
}
//____________________________________________________________________________
bool ReinSeghalCOHPXSec::ValidProcess(const Interaction * interaction) const
{
  if(interaction->TestBit(kISkipProcessChk)) return true;

  const InitialState & init_state = interaction->GetInitialState();
  const ProcessInfo &  proc_info  = interaction->GetProcessInfo();
  const Target &       target     = init_state.GetTarget();

  int nu = init_state.GetProbePDGCode();

  if (!proc_info.IsCoherent())     return false;
  if (!proc_info.IsWeak())         return false;
  if (target.StruckNucleonIsSet()) return false;
  if (!target.A()>1)               return false;
  if (!pdg::IsNeutrino(nu) && !pdg::IsAntiNeutrino(nu)) return false;

  return true;
}
//____________________________________________________________________________
bool ReinSeghalCOHPXSec::ValidKinematics(const Interaction* interaction) const
{
  if(interaction->TestBit(kISkipKinematicChk)) return true;

  const Kinematics &   kinematics = interaction -> GetKinematics();
  const InitialState & init_state = interaction -> GetInitialState();

  double E   = init_state.GetProbeE(kRfLab); // neutrino energy
  double x   = kinematics.x(); // bjorken x
  double y   = kinematics.y(); // inelasticity y

  if (E<=kPionMass || x<=0 || x>=1 || y<=kPionMass/E || y>=1) {

    LOG("ReinSeghalCoh", pINFO)
             << "d2xsec/dxdy[COH] (x= " << x << ", y="
                                             << y << ", E=" << E << ") = 0";
    return false;
  }
  return true;
}
//____________________________________________________________________________
void ReinSeghalCOHPXSec::Configure(const Registry & config)
{
  Algorithm::Configure(config);
  this->LoadConfig();
}
//____________________________________________________________________________
void ReinSeghalCOHPXSec::Configure(string config)
{
  Algorithm::Configure(config);
  this->LoadConfig();
}
//____________________________________________________________________________
void ReinSeghalCOHPXSec::LoadConfig(void)
{
  AlgConfigPool * confp = AlgConfigPool::Instance();
  const Registry * gc = confp->GlobalParameterList();

  fMa   = fConfig->GetDoubleDef("Ma",         gc->GetDouble("COH-Ma"));
  fReIm = fConfig->GetDoubleDef("Re-Im-Ampl", gc->GetDouble("COH-ReImAmpl"));
  fRo   = fConfig->GetDoubleDef("Ro",         gc->GetDouble("COH-Ro"));
}
//____________________________________________________________________________

