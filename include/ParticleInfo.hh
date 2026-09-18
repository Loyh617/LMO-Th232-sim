#ifndef ParticleInfo_h
#define ParticleInfo_h


#include "globals.hh"
#include "G4ThreeVector.hh"

namespace LMOTh232Sim
{

struct ParticleInfo
{
    G4String name;
    G4int PDG;
    G4int parentID;
    G4String creatorProcess;
    G4ThreeVector vertexPosition;
    G4String vertexVolumeName;
    G4double edep;

    ParticleInfo()
    {
        name="";
        PDG=0;
        parentID=0;
        creatorProcess="";
        vertexPosition=G4ThreeVector(0,0,0);
        vertexVolumeName="";
        edep=0.0;
    }
};

}

#endif