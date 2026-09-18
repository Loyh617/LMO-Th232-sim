//==============================================================
//使用方法(在 build 目录):
//   root -l -b -q ../macros/EnergyReconstruction.C+
//   (ACLiC 编译后自动调用与文件名同名的 EnergyReconstruction())
//或交互模式:
//   .L ../macros/EnergyReconstruction.C+
//   EnergyReconstruction()
//==============================================================


#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>        // sqrt
#include "TRandom.h"    // gRandom

using namespace std;

//==============================================================
// 保存单个Track的信息
//==============================================================

struct TrackInfo
{

    int EventID;
    int TrackID;
    int ParentID;
    string ParticleName;
    int PDG;
    
    string CreatorProcess;
    string VertexVolumeName;

    // Geant4中已经累计好的该Track沉积能量
    double edep_keV;

    TrackInfo()
    {
        EventID = -1;
        TrackID = -1;
        ParentID = -1;
        PDG = 0;
        edep_keV = 0.0;
    }

};

//==============================================================
// 当前Event的Track缓存
//
// 注意:
// TrackID只在一个Event内唯一
// Event结束后必须清空
//
//==============================================================

unordered_map<int, TrackInfo> eventTrackMap;

//==============================================================
// 当前Event的粒子树
//
// ParentID ---> 子Track列表
// 例如:
// 73
// |
// |--75
// |--76
// |--77
//
//==============================================================

unordered_map<int, vector<int>> eventChildMap;

//==============================================================
// 当前Event的入射粒子列表
//
// 通过:
//
// VertexVolumeName != Crystal
//
// 得到
//
//==============================================================

vector<int> incidentTrackList;

//==============================================================
// 递归计算某个入射粒子的总沉积能量
//
// TotalEdep(track)
// = 当前track
// + 所有daughter
// + granddaughter
// + ...
//
//==============================================================

double CalculateTotalEdep(int trackID)
{

    double totalEnergy = 0.0;

    //----------------------------------------------------------
    // 加入当前track自身沉积
    //----------------------------------------------------------

    auto trackIt = eventTrackMap.find(trackID);

    if(trackIt == eventTrackMap.end())
    {
        return 0.0;
    }


    
    totalEnergy += trackIt->second.edep_keV;

    //----------------------------------------------------------
    // 查找所有次级粒子
    //----------------------------------------------------------

    auto childIt = eventChildMap.find(trackID);

    if(childIt != eventChildMap.end())
    {
        for(int childID : childIt->second)
        {
            totalEnergy += CalculateTotalEdep(childID);
        }

    }

    return totalEnergy;

}

//==============================================================
// 清空当前Event所有缓存
//==============================================================

void ClearEventCache()
{
    eventTrackMap.clear();
    eventChildMap.clear();
    incidentTrackList.clear();
}

//==============================================================
// 探测器能量分辨率模型(2026-08 新增)
// gSmearOn=false 时 MeasuredEdep_keV == TotalEdep_keV(复现旧行为)
//==============================================================

bool   gSmearOn = true;
int    gSeed    = 12345; // 固定种子,保证展宽可复现

double Sigma1(double E)
{
    return 0.00735*E+6.24;
}

double Sigma2(double E)
{
    return 0.01039*E+3.69;
}

double ApplyEnergyResolutionLMO1(double Etrue)
{
    if (Etrue <= 0.) return 0.;
    return gRandom->Gaus(Etrue, Sigma1(Etrue));
}

double ApplyEnergyResolutionLMO2(double Etrue)
{
    if (Etrue <= 0.) return 0.;
    return gRandom->Gaus(Etrue, Sigma2(Etrue));
}

//==============================================================
// Output ROOT file
//==============================================================

TFile *outFile = nullptr;

TTree *outCrystal1 = nullptr;
TTree *outCrystal2 = nullptr;


//==============================================================
// Output branches
//==============================================================

Int_t outEventID;
Int_t outTrackID;
Int_t outPDG;
Double_t outTotalEdep;
Double_t outMeasuredEdep;

char outParticleName[64];
char outCreatorProcess[64];
char outVertexVolumeName[64];


//==============================================================
// Create output file
//==============================================================

void CreateOutputFile(const char *filename)
{
    outFile = new TFile(filename,"RECREATE");

    outCrystal1 = new TTree("Crystal1","Crystal1");
    outCrystal2 = new TTree("Crystal2","Crystal2");
    outCrystal1->SetAutoSave(0);   // 禁止填充期自动保存,避免产生备份 cycle
    outCrystal2->SetAutoSave(0);

    auto BuildTree = [](TTree *tree)
    {
        tree->Branch("EventID",&outEventID,"EventID/I");
        tree->Branch("TrackID",&outTrackID,"TrackID/I");
        tree->Branch("ParticleName",outParticleName,"ParticleName/C");
        tree->Branch("PDG",&outPDG,"PDG/I");
        tree->Branch("CreatorProcess",outCreatorProcess,"CreatorProcess/C");
        tree->Branch("VertexVolumeName",outVertexVolumeName,"VertexVolumeName/C");
        tree->Branch("TotalEdep_keV",&outTotalEdep,"TotalEdep_keV/D");
        tree->Branch("MeasuredEdep_keV",&outMeasuredEdep,"MeasuredEdep_keV/D");
    };

    BuildTree(outCrystal1);
    BuildTree(outCrystal2);
}


//==============================================================
// Fill one incident particle
//==============================================================

void FillIncidentParticle(TTree *tree,const TrackInfo &track,double totalEdep,bool isLMO2)
{
    // 过滤:TotalEdep <= 0 的入射粒子不写入输出
    // (在能量重建之后、分辨率展宽之前)
    if (totalEdep <= 0.) return;

    outEventID = track.EventID;
    outTrackID = track.TrackID;
    outPDG = track.PDG;
    outTotalEdep = totalEdep;

    // 探测器能量分辨率展宽(真值分支保持不变);
    // 两个晶体使用各自的实验分辨率模型
    // 晶体映射(用户确认 2026-08-26):Crystal1 = LMO2, Crystal2 = LMO1
    outMeasuredEdep = gSmearOn
        ? (isLMO2 ? ApplyEnergyResolutionLMO2(totalEdep)
                  : ApplyEnergyResolutionLMO1(totalEdep))
        : totalEdep;

    strncpy(outParticleName,
            track.ParticleName.c_str(),
            sizeof(outParticleName)-1);
    outParticleName[sizeof(outParticleName)-1]='\0';

    strncpy(outCreatorProcess,
            track.CreatorProcess.c_str(),
            sizeof(outCreatorProcess)-1);
    outCreatorProcess[sizeof(outCreatorProcess)-1]='\0';

    strncpy(outVertexVolumeName,
            track.VertexVolumeName.c_str(),
            sizeof(outVertexVolumeName)-1);
    outVertexVolumeName[sizeof(outVertexVolumeName)-1]='\0';

    tree->Fill();
}


//==============================================================
// Write output ROOT file
//==============================================================

void WriteOutputFile()
{
    outFile->cd();

    outCrystal1->Write();
    outCrystal2->Write();

    outFile->Close();
}

//==============================================================
// Process one event
//==============================================================

void ProcessEvent(TTree *outTree,const string &crystalName)
{
    incidentTrackList.clear();

    // 找到所有从晶体外产生的入射粒子
    for(const auto &item : eventTrackMap)
    {
        const TrackInfo &track = item.second;

        if(track.VertexVolumeName != crystalName)
            incidentTrackList.push_back(track.TrackID);
    }

    // 计算每个入射粒子的总沉积能量并写入输出Tree
    for(int trackID : incidentTrackList)
    {
        auto it = eventTrackMap.find(trackID);

        if(it == eventTrackMap.end())
            continue;

        double totalEdep = CalculateTotalEdep(trackID);

        FillIncidentParticle(outTree,it->second,totalEdep, crystalName == "Crystal1");   // Crystal1 = LMO2
    }
}

//==============================================================
// Process one input TTree
//==============================================================

void ProcessTree(TTree *inputTree,TTree *outputTree,const string &crystalName)
{
    Int_t EventID;
    Int_t TrackID;
    Int_t PDG;
    Int_t ParentID;
    Double_t edep_keV;

    Char_t ParticleName[64];
    Char_t CreatorProcess[64];
    Char_t VertexVolumeName[64];


    inputTree->SetBranchAddress("EventID",&EventID);
    inputTree->SetBranchAddress("TrackID",&TrackID);
    inputTree->SetBranchAddress("PDG",&PDG);
    inputTree->SetBranchAddress("ParentID",&ParentID);
    inputTree->SetBranchAddress("edep_keV",&edep_keV);

    inputTree->SetBranchAddress("ParticleName",ParticleName);
    inputTree->SetBranchAddress("CreatorProcess",CreatorProcess);
    inputTree->SetBranchAddress("VertexVolumeName",VertexVolumeName);


    Long64_t nEntries = inputTree->GetEntries();


    int currentEvent = -1;


    for(Long64_t i=0;i<nEntries;i++)
    {
        inputTree->GetEntry(i);


        // Event发生变化
        if(currentEvent!=-1 && EventID!=currentEvent)
        {
            ProcessEvent(outputTree,crystalName);

            ClearEventCache();
        }


        currentEvent = EventID;


        TrackInfo track;

        track.EventID = EventID;
        track.TrackID = TrackID;
        track.ParentID = ParentID;

        track.PDG = PDG;

        track.ParticleName = ParticleName;
        track.CreatorProcess = CreatorProcess;
        track.VertexVolumeName = VertexVolumeName;

        track.edep_keV = edep_keV;


        // 保存当前Event中的Track信息

        eventTrackMap[TrackID] = track;


        // 建立Parent->Child关系

        eventChildMap[ParentID].push_back(TrackID);

    }


    // 处理最后一个Event

    if(currentEvent!=-1)
    {
        ProcessEvent(outputTree,crystalName);

        ClearEventCache();
    }
}




//==============================================================
// Main function
//==============================================================

void EnergyReconstruction()
{

    // 固定随机种子,保证展宽结果可复现
    gRandom->SetSeed(gSeed);

    TFile *inputFile = new TFile("LMO_Th232.root","READ");

    if(!inputFile || inputFile->IsZombie())
    {
        cout<<"Cannot open input ROOT file!"<<endl;
        return;
    }

    TTree *crystal1 = (TTree*)inputFile->Get("Crystal1");
    TTree *crystal2 = (TTree*)inputFile->Get("Crystal2");

    if(!crystal1 || !crystal2)
    {
        cout<<"Cannot find Crystal trees!"<<endl;
        return;
    }

    CreateOutputFile("LMO_Th232_TotalEdep.root");

    cout<<"Processing Crystal1..."<<endl;

    ProcessTree(crystal1,outCrystal1,"Crystal1");

    cout<<"Processing Crystal2..."<<endl;

    ProcessTree(crystal2,outCrystal2,"Crystal2");

    WriteOutputFile();

    inputFile->Close();

    cout<<"Finished!"<<endl;

}