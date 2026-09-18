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
/// \file RunAction.cc
/// \brief Implementation of the LMOTh232Sim::RunAction class

#include "RunAction.hh"

#include "G4AnalysisManager.hh"

namespace LMOTh232Sim
{

RunAction::RunAction()
{
    auto am = G4AnalysisManager::Instance();

    am->SetFileName("LMO_Th232.root");
    am->SetNtupleMerging(true);

    // ================= EVENT TREE =================
    // Crystal 1
    fCrystal1Id = am->CreateNtuple("Crystal1", "Crystal1");
    am->CreateNtupleIColumn("EventID");
    am->CreateNtupleIColumn("TrackID");
    am->CreateNtupleSColumn("ParticleName");
    am->CreateNtupleIColumn("PDG");
    am->CreateNtupleIColumn("ParentID");
    am->CreateNtupleSColumn("CreatorProcess");
    // am->CreateNtupleDColumn("VertexX_mm");
    // am->CreateNtupleDColumn("VertexY_mm");
    // am->CreateNtupleDColumn("VertexZ_mm");
    am->CreateNtupleSColumn("VertexVolumeName");
    am->CreateNtupleDColumn("edep_keV");
    am->FinishNtuple();

    // Crystal 2
    fCrystal2Id = am->CreateNtuple("Crystal2", "Crystal2");
    am->CreateNtupleIColumn("EventID");
    am->CreateNtupleIColumn("TrackID");
    am->CreateNtupleSColumn("ParticleName");
    am->CreateNtupleIColumn("PDG");
    am->CreateNtupleIColumn("ParentID");
    am->CreateNtupleSColumn("CreatorProcess");
    // am->CreateNtupleDColumn("VertexX_mm");
    // am->CreateNtupleDColumn("VertexY_mm");
    // am->CreateNtupleDColumn("VertexZ_mm");
    am->CreateNtupleSColumn("VertexVolumeName");
    am->CreateNtupleDColumn("edep_keV");
    am->FinishNtuple();

}

RunAction::~RunAction() {}

void RunAction::BeginOfRunAction(const G4Run*)
{
    G4AnalysisManager::Instance()->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run*)
{
    auto am = G4AnalysisManager::Instance();
    am->Write();
    am->CloseFile();
}

}