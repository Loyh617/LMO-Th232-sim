//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file PrimaryGeneratorAction.cc
/// \brief Implementation of the LMOTh232Sim::PrimaryGeneratorAction class

#include "PrimaryGeneratorAction.hh"

#include "G4ParticleGun.hh"
#include "G4Event.hh"
#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <cmath>

namespace LMOTh232Sim
{


PrimaryGeneratorAction::PrimaryGeneratorAction()
{
  G4int n_particle = 1;
  fParticleGun = new G4ParticleGun(n_particle);
}


PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
}


void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event)
{
  // 定义Th-232  
  G4int Z = 90, A = 232; // Th-232
  // G4int Z = 81, A = 208; // Tl-208
  G4double ionCharge = 0. * eplus;
  G4double excitEnergy = 0. * keV;

  G4ParticleDefinition* ion = G4IonTable::GetIonTable()->GetIon(Z, A, excitEnergy);
  fParticleGun->SetParticleDefinition(ion);
  fParticleGun->SetParticleCharge(ionCharge);

  // 静止核
  fParticleGun->SetParticleEnergy(0. * eV);

  // 方向(实际上无意义)
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0.,0.,0.));
  
  // 发射位置:WTh 丝内均匀体抽样
  // 丝:半径 0.8mm、长 10mm,轴沿 x,中心 (0,0,-31.95mm)
  // 截面上按面积均匀(r = R*sqrt(u)),丝长方向均匀
  G4double WireRadius  = 0.8 * mm;      // 半径 = 直径 1.6mm 的一半
  G4double WireHalfLen = 5.0 * mm;
  G4double WireCenterZ = -19.45-12.5 * mm;   // -31.95 mm,LMO(2*2*2)Placement 1

  G4double r   = WireRadius * sqrt(G4UniformRand());
  G4double phi = 2.*M_PI * G4UniformRand();

  G4double x0 = WireHalfLen*(2.*G4UniformRand()-1.);
  G4double y0 = r*sin(phi);
  G4double z0 = WireCenterZ + r*cos(phi);

  fParticleGun->SetParticlePosition(G4ThreeVector(x0, y0, z0));

  //生成初级粒子顶点
  fParticleGun->GeneratePrimaryVertex(event);
}


}  // namespace LMOTh232Sim
