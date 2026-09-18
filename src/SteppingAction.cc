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
/// \file SteppingAction.cc
/// \brief Implementation of the B1::SteppingAction class

#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4LogicalVolume.hh"
#include "G4RunManager.hh"


namespace B1
{

SteppingAction::SteppingAction(EventAction* event)
: fEventAction(event)
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
    // 获取当前 step 所在逻辑体
    G4LogicalVolume* volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();

    G4String volumeName = volume->GetName();

    // 如果既不是 Crystal1，也不是 Crystal2，则返回
    if (volumeName != "Crystal1" && volumeName != "Crystal2")
        return;

    // 获取当前 step 的能量沉积
    G4double edep = step->GetTotalEnergyDeposit();

    // 获取EventID
    G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();

    // 获取Track
    G4Track* track = step->GetTrack();

    G4int trackID = track->GetTrackID();
    G4int parentID = track->GetParentID();

    // Track产生过程
    G4String creatorProcess = "Primary";
    if(track->GetCreatorProcess())
        creatorProcess = track->GetCreatorProcess()->GetProcessName();

    // Track出生位置
    G4ThreeVector vertexPosition = track->GetVertexPosition();
    G4String vertexVolumeName = track->GetLogicalVolumeAtVertex()->GetName();

    // 获取粒子属性
    auto particle = track->GetDefinition();
    G4String ParticleName = particle->GetParticleName();
    G4int PDG = particle->GetPDGEncoding();

    // 产生key
    std::pair<G4int,G4int> key(eventID,trackID);

    // 判断是否为入射粒子（出生在当前晶体外）
    G4bool bornOutside = (vertexVolumeName != volumeName);

    // 入射粒子无论 edep 是否为 0 都记录身份；
    // 晶体内部产生的粒子仅在 edep > 0 时才记录
    // if (bornOutside || edep > 0.)
    // {
        // 根据晶体分别累积能量
        if (volumeName == "Crystal1")
        {
            fEventAction->AddParticleInfo1(key, ParticleName, PDG, parentID, creatorProcess, vertexPosition, vertexVolumeName, edep);
        }
        else if (volumeName == "Crystal2")
        {
            fEventAction->AddParticleInfo2(key, ParticleName, PDG, parentID, creatorProcess, vertexPosition, vertexVolumeName, edep);
        }
    // }
}

}
