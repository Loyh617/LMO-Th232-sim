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
/// \file EventAction.cc
/// \brief Implementation of the B1::EventAction class

#include "EventAction.hh"
#include "RunAction.hh"

#include "G4AnalysisManager.hh"
#include "G4Event.hh"
#include "G4SystemOfUnits.hh"

namespace B1
{

EventAction::EventAction(RunAction* runAction)
: fRunAction(runAction)
{}

void EventAction::BeginOfEventAction(const G4Event*)
{
}

void EventAction::EndOfEventAction(const G4Event* evt)
{
    auto am = G4AnalysisManager::Instance();

    for(auto &p:particleMap1)
    {
        am->FillNtupleIColumn(fRunAction->fCrystal1Id, 0, p.first.first);
        am->FillNtupleIColumn(fRunAction->fCrystal1Id, 1, p.first.second);
        am->FillNtupleSColumn(fRunAction->fCrystal1Id, 2, p.second.name);
        am->FillNtupleIColumn(fRunAction->fCrystal1Id, 3, p.second.PDG);
        am->FillNtupleIColumn(fRunAction->fCrystal1Id, 4, p.second.parentID);
        am->FillNtupleSColumn(fRunAction->fCrystal1Id, 5, p.second.creatorProcess);
        // am->FillNtupleDColumn(fRunAction->fCrystal1Id, 6, p.second.vertexPosition.x()/mm);
        // am->FillNtupleDColumn(fRunAction->fCrystal1Id, 7, p.second.vertexPosition.y()/mm);
        // am->FillNtupleDColumn(fRunAction->fCrystal1Id, 8, p.second.vertexPosition.z()/mm);
        am->FillNtupleSColumn(fRunAction->fCrystal1Id, 6, p.second.vertexVolumeName);
        am->FillNtupleDColumn(fRunAction->fCrystal1Id, 7, p.second.edep);
        am->AddNtupleRow(fRunAction->fCrystal1Id);
    }

    for(auto &p:particleMap2)
    {
        am->FillNtupleIColumn(fRunAction->fCrystal2Id, 0, p.first.first);
        am->FillNtupleIColumn(fRunAction->fCrystal2Id, 1, p.first.second);
        am->FillNtupleSColumn(fRunAction->fCrystal2Id, 2, p.second.name);
        am->FillNtupleIColumn(fRunAction->fCrystal2Id, 3, p.second.PDG);
        am->FillNtupleIColumn(fRunAction->fCrystal2Id, 4, p.second.parentID);
        am->FillNtupleSColumn(fRunAction->fCrystal2Id, 5, p.second.creatorProcess);
        // am->FillNtupleDColumn(fRunAction->fCrystal2Id, 6, p.second.vertexPosition.x()/mm);
        // am->FillNtupleDColumn(fRunAction->fCrystal2Id, 7, p.second.vertexPosition.y()/mm);
        // am->FillNtupleDColumn(fRunAction->fCrystal2Id, 8, p.second.vertexPosition.z()/mm);
        am->FillNtupleSColumn(fRunAction->fCrystal2Id, 6, p.second.vertexVolumeName);
        am->FillNtupleDColumn(fRunAction->fCrystal2Id, 7, p.second.edep);
        am->AddNtupleRow(fRunAction->fCrystal2Id);
    }

    particleMap1.clear();
    particleMap2.clear();
}

void EventAction::AddParticleInfo1(std::pair<G4int,G4int> key, G4String ParticleName, G4int PDG, G4int parentID, G4String creatorProcess, G4ThreeVector vertexPosition, G4String vertexVolumeName, G4double edep)
{
    // 判断是否为第一次遇到这个粒子
    if(particleMap1.find(key)==particleMap1.end())
    {
        ParticleInfo info;
        info.name = ParticleName;
        info.PDG = PDG;
        info.parentID = parentID;
        info.creatorProcess = creatorProcess;
        info.vertexPosition = vertexPosition;
        info.vertexVolumeName = vertexVolumeName;
        info.edep = edep/keV;
        particleMap1[key] = info;
    }
    else
    {
        particleMap1[key].edep += edep/keV;
    }
}

void EventAction::AddParticleInfo2(std::pair<G4int,G4int> key, G4String ParticleName, G4int PDG, G4int parentID, G4String creatorProcess, G4ThreeVector vertexPosition, G4String vertexVolumeName, G4double edep)
{
    // 判断是否为第一次遇到这个粒子
    if(particleMap2.find(key)==particleMap2.end())
    {
        ParticleInfo info;
        info.name = ParticleName;
        info.PDG = PDG;
        info.parentID = parentID;
        info.creatorProcess = creatorProcess;
        info.vertexPosition = vertexPosition;
        info.vertexVolumeName = vertexVolumeName;
        info.edep = edep/keV;
        particleMap2[key] = info;
    }
    else
    {
        particleMap2[key].edep += edep/keV;
    }
}

}
