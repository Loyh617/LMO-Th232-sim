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
/// \file DetectorConstruction.cc
/// \brief Implementation of the B1::DetectorConstruction class

#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4UnionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4Trd.hh"

namespace B1
{

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  // Get nist material manager
  G4NistManager* nist = G4NistManager::Instance();

  // Option to switch on/off checking of volumes overlaps
  //
  G4bool checkOverlaps = true;

  //------------------------------------------------
  // World
  //------------------------------------------------
  G4double world_sizeX = 2*m;
  G4double world_sizeY = 2*m;
  G4double world_sizeZ = 2*m;
  G4Material* world_mat = nist->FindOrBuildMaterial("G4_Galactic");

  auto solidWorld =
    new G4Box("World",  // its name
              0.5 * world_sizeX, 0.5 * world_sizeY, 0.5 * world_sizeZ);  // its size

  auto logicWorld = new G4LogicalVolume(solidWorld,  // its solid
                                        world_mat,  // its material
                                        "World");  // its name

  auto physWorld = new G4PVPlacement(nullptr,  // no rotation
                                     G4ThreeVector(),  // at (0,0,0)
                                     logicWorld,  // its logical volume
                                     "World",  // its name
                                     nullptr,  // its mother  volume
                                     false,  // no boolean operation
                                     0,  // copy number
                                     checkOverlaps);  // overlaps checking

  // //------------------------------------------------
  // // LMO crystal
  // //------------------------------------------------
  G4bool isotopes = false;

  //define LMO
  G4Element*  O = nist->FindOrBuildElement("O" , isotopes);
  G4Element* Li = nist->FindOrBuildElement("Li", isotopes);
  G4Element* Mo = nist->FindOrBuildElement("Mo", isotopes);

  G4Material* LMO = new G4Material("LMO", 3.07 * g / cm3, 3); // 280g 45mm LMO crystal from hallC test
  LMO->AddElement(Li, 2);
  LMO->AddElement(Mo, 1);
  LMO->AddElement(O, 4);

  //Two LMO crystals between 25mm
  //define crystal1
  G4double Crystal_sizeX = 2.*cm;
  G4double Crystal_sizeY = 2.*cm;
  G4double Crystal_sizeZ = 2.*cm;

  auto solidCrystal1 = new G4Box("Crystal1", Crystal_sizeX/2., Crystal_sizeY/2., Crystal_sizeZ/2.);
  auto logicCrystal1 = new G4LogicalVolume(solidCrystal1, LMO, "Crystal1");
  auto Crystal1Position = G4ThreeVector(0, 0, -12.5*mm);
  new G4PVPlacement(nullptr, Crystal1Position, logicCrystal1, "Crystal1", logicWorld, false, 0, checkOverlaps);

  //define crystal2

  auto solidCrystal2 = new G4Box("Crystal2", Crystal_sizeX/2., Crystal_sizeY/2., Crystal_sizeZ/2.);
  auto logicCrystal2 = new G4LogicalVolume(solidCrystal2, LMO, "Crystal2");
  auto Crystal2Position = G4ThreeVector(0, 0, 12.5*mm);
  new G4PVPlacement(nullptr, Crystal2Position, logicCrystal2, "Crystal2", logicWorld, false, 0, checkOverlaps);
  
  // //------------------------------------------------
  // //Cold plates
  // //------------------------------------------------
  // // Mixing chamber (MC)
  G4Material* MC_plate_mat = nist->FindOrBuildMaterial("G4_Cu");

  // G4double MC_plate_diameter = 500*mm;
  // G4double MC_plate_thickness = 8*mm;

  G4RotationMatrix* rot = new G4RotationMatrix();
  rot->rotateX(90.*deg);

  // auto solidMC_plate = new G4Tubs("MCplate", 0.*mm, MC_plate_diameter/2, MC_plate_thickness/2, 0.*deg, 360.*deg);
  // auto logicMC_plate = new G4LogicalVolume(solidMC_plate, MC_plate_mat, "MCplate");

  // new G4PVPlacement(rot, G4ThreeVector(0, 306.*mm, 0), logicMC_plate, "MCplate", logicWorld, false, 0, checkOverlaps);

  // //------------------------------------------------
  // //Shielding buckets
  // //------------------------------------------------
  G4Material* CuShielding_mat = nist->FindOrBuildMaterial("G4_Cu");
  G4Material* Al = nist->FindOrBuildMaterial("G4_Al");

  // //Layer 1
  // G4double Layer1_BucketInnerRadius = 260.*mm;
  // G4double Layer1_BucketInnerHeight = 646.7*mm;
  // G4double Layer1_BucketOuterRadius = 260.5*mm;
  // G4double Layer1_BucketOuterHeight = 648.2*mm;

  // G4double Layer1_UpperFixedRingRadius = 270.*mm;
  // G4double Layer1_UpperFixedRingHeight = 24.*mm;

  // G4double Layer1_UnderFixedRingRadius = 270.*mm;
  // G4double Layer1_UnderFixedRingHeight = 13.5*mm;

  // G4double Layer1_BottomnThickness = Layer1_BucketOuterHeight-Layer1_BucketInnerHeight;

  // auto solidLayer1_Bucket = new G4Tubs("Layer1 Bucket", Layer1_BucketInnerRadius, Layer1_BucketOuterRadius, Layer1_BucketOuterHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer1_UpperFixedRing = new G4Tubs("Layer1 Upper Fixed Ring", Layer1_BucketOuterRadius, Layer1_UpperFixedRingRadius, Layer1_UpperFixedRingHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer1_UnderFixedRing = new G4Tubs("Layer1 Under Fixed Ring", Layer1_BucketOuterRadius, Layer1_UnderFixedRingRadius, Layer1_UnderFixedRingHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer1_Bottom = new G4Tubs("Layer1 Bottom", 0.*mm, Layer1_BucketInnerRadius, Layer1_BottomnThickness/2, 0.*deg, 360.*deg);
  // auto solidLayer1_Union12 = new G4UnionSolid("Layer1 Union12", solidLayer1_Bucket, solidLayer1_UpperFixedRing, nullptr, G4ThreeVector(0.*mm, 0.*mm, 289.4*mm));
  // auto solidLayer1_Union123 = new G4UnionSolid("Layer1 Union123", solidLayer1_Union12, solidLayer1_UnderFixedRing, nullptr, G4ThreeVector(0.*mm, 0.*mm, -317.35*mm));
  // auto solidLayer1 = new G4UnionSolid("Layer1", solidLayer1_Union123, solidLayer1_Bottom, nullptr, G4ThreeVector(0.*mm, 0.*mm, -323.35*mm));

  // auto logicLayer1 = new G4LogicalVolume(solidLayer1, CuShielding_mat, "Layer1");

  // new G4PVPlacement(rot, G4ThreeVector(0.*mm, 0.*mm, 0.*mm), logicLayer1, "Layer1", logicWorld, false, 0, checkOverlaps);

  // //Layer 2
  // G4double Layer2_BucketInnerRadius = 275.5*mm;
  // G4double Layer2_BucketInnerHeight = 662.7*mm;
  // G4double Layer2_BucketOuterRadius = 276.5*mm;
  // G4double Layer2_BucketOuterHeight = 665.7*mm;

  // G4double Layer2_UpperFixedRingRadius = 295.*mm;
  // G4double Layer2_UpperFixedRingHeight = 17.5*mm;

  // G4double Layer2_UnderFixedRingRadius = 287.5*mm;
  // G4double Layer2_UnderFixedRingHeight = 10.*mm;

  // G4double Layer2_BottomnThickness = Layer2_BucketOuterHeight - Layer2_BucketInnerHeight;

  // auto solidLayer2_Bucket = new G4Tubs("Layer2 Bucket", Layer2_BucketInnerRadius, Layer2_BucketOuterRadius, Layer2_BucketOuterHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer2_UpperFixedRing = new G4Tubs("Layer2 Upper Fixed Ring", Layer2_BucketOuterRadius, Layer2_UpperFixedRingRadius, Layer2_UpperFixedRingHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer2_UnderFixedRing = new G4Tubs("Layer2 Under Fixed Ring", Layer2_BucketOuterRadius, Layer2_UnderFixedRingRadius, Layer2_UnderFixedRingHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer2_Bottom = new G4Tubs("Layer2 Bottom", 0.*mm, Layer2_BucketInnerRadius, Layer2_BottomnThickness/2, 0.*deg, 360.*deg);

  // auto solidLayer2_Union12 = new G4UnionSolid("Layer2 Union12", solidLayer2_Bucket, solidLayer2_UpperFixedRing, nullptr, G4ThreeVector(0.*mm, 0.*mm, 307.9*mm));
  // auto solidLayer2_Union123 = new G4UnionSolid("Layer2 Union123", solidLayer2_Union12, solidLayer2_UnderFixedRing, nullptr, G4ThreeVector(0.*mm, 0.*mm, -327.85*mm));
  // auto solidLayer2 = new G4UnionSolid("Layer2", solidLayer2_Union123, solidLayer2_Bottom, nullptr, G4ThreeVector(0.*mm, 0.*mm, -331.35*mm));

  // auto logicLayer2 = new G4LogicalVolume(solidLayer2, CuShielding_mat, "Layer2");

  // new G4PVPlacement(rot, G4ThreeVector(0.*mm, -8.75*mm, 0.*mm), logicLayer2, "Layer2", logicWorld, false, 0, checkOverlaps);

  // //Layer 3
  // G4double Layer3_BucketInnerRadius = 310.*mm;
  // G4double Layer3_BucketInnerHeight = 683.2*mm;
  // G4double Layer3_BucketOuterRadius = 310.5*mm;
  // G4double Layer3_BucketOuterHeight = 683.2*mm;

  // G4double Layer3_UpperFixedRingRadius = 330.*mm;
  // G4double Layer3_UpperFixedRingHeight = 17.5*mm;

  // G4double Layer3_UnderFixedRingInnerRadius = 298.*mm;
  // G4double Layer3_UnderFixedRingHeight = 10.*mm;

  // G4double Layer3_BottomnThickness = 3.*mm;

  // auto solidLayer3_Bucket = new G4Tubs("Layer3 Bucket", Layer3_BucketInnerRadius, Layer3_BucketOuterRadius, Layer3_BucketOuterHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer3_UpperFixedRing = new G4Tubs("Layer3 Upper Fixed Ring", Layer3_BucketOuterRadius, Layer3_UpperFixedRingRadius, Layer3_UpperFixedRingHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer3_UnderFixedRing = new G4Tubs("Layer3 Under Fixed Ring", Layer3_UnderFixedRingInnerRadius, Layer3_BucketInnerRadius, Layer3_UnderFixedRingHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer3_Bottom = new G4Tubs("Layer3 Bottom", 0.*mm, Layer3_BucketInnerRadius, Layer3_BottomnThickness/2, 0.*deg, 360.*deg);

  // auto solidLayer3_Union12 = new G4UnionSolid("Layer3 Union12", solidLayer3_Bucket, solidLayer3_UpperFixedRing, nullptr, G4ThreeVector(0.*mm, 0.*mm, 323.15*mm));
  // auto solidLayer3_Union123 = new G4UnionSolid("Layer3 Union123", solidLayer3_Union12, solidLayer3_UnderFixedRing, nullptr, G4ThreeVector(0.*mm, 0.*mm, -336.6*mm));
  // auto solidLayer3 = new G4UnionSolid("Layer3", solidLayer3_Union123, solidLayer3_Bottom, nullptr, G4ThreeVector(0.*mm, 0.*mm, -343.1*mm));

  // auto logicLayer3 = new G4LogicalVolume(solidLayer3, CuShielding_mat, "Layer3");

  // new G4PVPlacement(rot, G4ThreeVector(0.*mm, -17.5*mm, 0.*mm), logicLayer3, "Layer3", logicWorld, false, 0, checkOverlaps);

  // //Outermost Layer
  // G4double Layer4_BucketInnerRadius = 340.*mm;
  // G4double Layer4_BucketInnerHeight = 778.8*mm;
  // G4double Layer4_BucketOuterRadius = 344.2*mm;
  // G4double Layer4_BucketOuterHeight = 783.*mm;

  // G4double Layer4_BottomnThickness = Layer2_BucketOuterHeight - Layer2_BucketInnerHeight;

  // auto solidLayer4_Bucket = new G4Tubs("Layer4 Bucket", Layer4_BucketInnerRadius, Layer4_BucketOuterRadius, Layer4_BucketOuterHeight/2, 0.*deg, 360.*deg);
  // auto solidLayer4_Bottom = new G4Tubs("Layer4 Bottom", 0.*mm, Layer4_BucketInnerRadius, Layer4_BottomnThickness/2, 0.*deg, 360.*deg);
  // auto solidLayer4 = new G4UnionSolid("Layer4", solidLayer4_Bucket, solidLayer4_Bottom, nullptr, G4ThreeVector(0.*mm, 0.*mm, -389.4*mm));

  // auto logicLayer4 = new G4LogicalVolume(solidLayer4, Al, "Layer4");

  // new G4PVPlacement(rot, G4ThreeVector(0.*mm, -67.4*mm, 0.*mm), logicLayer4, "Layer4", logicWorld, false, 0, checkOverlaps);

  // //------------------------------------------------
  // //Copper Shielding
  // //------------------------------------------------
  // G4double TopHoleDiameter = 30.*mm;
  // G4double TopHeight = 20.*mm;
  // G4double MainHoleDiameter = 120.*mm;
  // G4double MainHeight = 100.*mm;
  // G4double CopperShieldingDiameter = 320.*mm;
  // G4double CopperShieldingHeight = TopHeight + MainHeight;

  // auto solidCuShieldingTop = new G4Tubs("CuShieldingTop", TopHoleDiameter/2, CopperShieldingDiameter/2, TopHeight/2, 0.*deg, 360.*deg);
  // auto solidCuShieldingMain = new G4Tubs("CuShieldingMain", MainHoleDiameter/2, CopperShieldingDiameter/2, MainHeight/2, 0.*deg, 360.*deg);
  // auto solidCuShielding = new G4UnionSolid("CuShielding", solidCuShieldingMain, solidCuShieldingTop, nullptr, G4ThreeVector(0.*mm, 0.*mm, MainHeight/2+TopHeight/2));
  // auto logicCuShielding = new G4LogicalVolume(solidCuShielding, CuShielding_mat, "CuShielding");
  // new G4PVPlacement(rot, G4ThreeVector(0.*mm, 0.*mm, 0.*mm), logicCuShielding, "CuShielding", logicWorld, false, 0, checkOverlaps);

  //------------------------------------------------
  //WTh Wire(放射源:圆柱形钨钍电极,2% ThO2 + 98% W,质量比)
  //------------------------------------------------
  G4Material* W_mat = nist->FindOrBuildMaterial("G4_W");

  G4Material* ThO2_mat = new G4Material("ThO2", 10.0*g/cm3, 2);
  ThO2_mat->AddElement(nist->FindOrBuildElement("Th"), 1);
  ThO2_mat->AddElement(nist->FindOrBuildElement("O"), 2);

  G4Material* WThWire_mat = new G4Material("WThWire", 18.95*g/cm3, 2);
  WThWire_mat->AddMaterial(ThO2_mat, 2.0*perCent);
  WThWire_mat->AddMaterial(W_mat, 98.0*perCent);

  G4double WireDiameter = 1.6*mm;
  G4double WireLength = 10.*mm;

  auto solidWThWire = new G4Tubs("WThWire", 0.*mm, WireDiameter/2,
                                 WireLength/2, 0.*deg, 360.*deg);
  auto logicWThWire = new G4LogicalVolume(solidWThWire, WThWire_mat, "WThWire");

  G4RotationMatrix* rotWire = new G4RotationMatrix();
  rotWire->rotateY(90.*deg);   // 轴从 z 转到 x,水平放置(与 PrimaryGeneratorAction 一致)

  // 放在源杯空腔中心(z = -31.95mm),与 PrimaryGeneratorAction 的抽样中心一致
  new G4PVPlacement(rotWire, Crystal1Position + G4ThreeVector(0.*mm, 0.*mm, -19.45*mm),
                    logicWThWire, "WThWire", logicWorld, false, 0, checkOverlaps);

  //------------------------------------------------
  //Source Cup
  //------------------------------------------------
  G4double CupDistance = 7.5*mm;

  G4double CupHeight = 4.9*mm;
  G4double CupDiameter = 18.*mm;
  G4double CupLidHeight = 2.*mm;
  G4double CupLidDiameter = 14.6*mm;
  G4double CupHoleDiameter = 2.*mm;
  G4double CupHoleHeight = 1.*mm;

  auto solidCupTop = new G4Tubs("Cup Top", 0.*mm, CupDiameter/2, CupLidHeight/2, 0.*deg, 360.*deg);
  auto solidCupMiddle = new G4Tubs("Cup Middle", CupLidDiameter/2, CupDiameter/2, (CupHeight-CupLidHeight-CupHoleHeight)/2, 0.*deg, 360.*deg);
  auto solidCupBottom = new G4Tubs("Cup Bottom", CupHoleDiameter/2, CupDiameter/2, CupHoleHeight/2, 0.*deg, 360.*deg);

  auto solidCupTopMiddle = new G4UnionSolid("CupTopMiddle", solidCupTop, solidCupMiddle, nullptr, G4ThreeVector(0.*mm, 0.*mm, (CupHeight-CupHoleHeight)/2));
  auto solidCup = new G4UnionSolid("Cup", solidCupTopMiddle, solidCupBottom, nullptr, G4ThreeVector(0.*mm, 0.*mm, (CupHeight-CupLidHeight/2-CupHoleHeight/2)));
  auto logicCup = new G4LogicalVolume(solidCup, CuShielding_mat, "Cup");
  new G4PVPlacement(nullptr, Crystal1Position + G4ThreeVector(0.*mm, 0.*mm, -(CupHeight-CupLidHeight/2+Crystal_sizeZ/2+CupDistance)*mm), logicCup, "Cup", logicWorld, false, 0, checkOverlaps);

  //------------------------------------------------
  //Cooper Tape
  //------------------------------------------------
  G4double thickness = 0.005*mm;

  auto solidCooperTape = new G4Tubs("CooperTape", 0.*mm, 1.5*mm, thickness/2, 0.*deg, 360.*deg);
  auto logicCooperTape = new G4LogicalVolume(solidCooperTape, CuShielding_mat, "CooperTape");
  new G4PVPlacement(nullptr, Crystal1Position + G4ThreeVector(0.*mm, 0.*mm, -(Crystal_sizeZ/2+CupDistance-thickness/2)*mm), logicCooperTape, "CooperTape", logicWorld, false, 0, checkOverlaps);


  //Set Crystal as scoring volume
  
  // fScoringVolume = logicCrystal;

  //
  // always return the physical World
  //
  return physWorld;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

}  // namespace B1
