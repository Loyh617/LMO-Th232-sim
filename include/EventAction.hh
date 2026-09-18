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
/// \file EventAction.hh
/// \brief Definition of the LMOTh232Sim::EventAction class

#ifndef EventAction_h
#define EventAction_h 1

#include <map>
#include <utility>

#include "ParticleInfo.hh"
#include "G4UserEventAction.hh"
#include "globals.hh"

namespace LMOTh232Sim
{

class RunAction;

/// Event action class

class EventAction : public G4UserEventAction
{
  public:
    EventAction(RunAction* runAction);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event* event) override;
    void EndOfEventAction(const G4Event* event) override;

    void AddParticleInfo1(std::pair<G4int,G4int> key, G4String ParticleName, G4int PDG, G4int parentID, G4String creatorProcess, G4ThreeVector vertexPosition, G4String vertexVolumeName, G4double edep);
    void AddParticleInfo2(std::pair<G4int,G4int> key, G4String ParticleName, G4int PDG, G4int parentID, G4String creatorProcess, G4ThreeVector vertexPosition, G4String vertexVolumeName, G4double edep);

  private:
    std::map<std::pair<G4int,G4int>,ParticleInfo> particleMap1;
    std::map<std::pair<G4int,G4int>,ParticleInfo> particleMap2;
    RunAction* fRunAction = nullptr;
};

}

#endif
