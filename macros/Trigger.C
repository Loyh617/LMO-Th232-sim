//==============================================================
// Trigger.C
// 触发模拟: 对 LMO_Th232_TotalEdep.root 施加 trigger cut
//
// 触发条件: 每个粒子的瞬时幅度 = TotalEdep_keV + N(0, sigma) > 各自阈值
// 才被记录; 记录的值是 MeasuredEdep_keV (重建能量, 不含触发噪声)。
//   Crystal1: threshold = 198.5 keV, sigma = 105.7 keV
//   Crystal2: threshold = 320.3 keV, sigma = 158.7 keV
//
// 输出(2026-08-31 改为全格式透传):
//   LMO_Th232_TotalEdep_trigger.root
//   通过触发的行原样复制全部 9 个分支
//   (EventID, TrackID, ParticleName, PDG, CreatorProcess,
//    VertexVolumeName, TotalEdep_keV, MeasuredEdep_keV, Time),
//   与 LMO_Th232_TotalEdep_PileUp.root 的树格式一致,
//   以便 PileUp.C 直接在其上做堆叠剔除。
//   触发判定不改变任何数值, Time 分支原样透传。
//
// 元数据:
//   NSimEvents            模拟总衰变数
//   comment               文件介绍(参数、归一化)
//   Trigger_Crystal1/2    各树触发通过率统计
//
// 固定随机种子, 结果可复现。
//==============================================================

#include <TFile.h>
#include <TTree.h>
#include <TNamed.h>
#include <TString.h>
#include <iostream>
#include <cstring>

using namespace std;

const double kTrigThresholdC1 = 198.5;
const double kTrigThresholdC2 = 320.3;
const double kTrigSigmaC1  = 105.7;
const double kTrigSigmaC2  = 158.7;
const long   kSeed = 20250825;
const double kSourceActivity = 3.0992;   // Bq(实验源活度)
const long   kNSimEvents = 100000000;    // 模拟总衰变数,须与 run.mac 的 beamOn 一致

// 树的一行数据(与 EnergyReconstruction.C / PileUp.C 输出的 schema 一致)
struct TriggerRow
{
    Int_t    EventID;
    Int_t    TrackID;
    char     ParticleName[64];
    Int_t    PDG;
    char     CreatorProcess[64];
    char     VertexVolumeName[64];
    Double_t TotalEdep_keV;
    Double_t MeasuredEdep_keV;
    Double_t Time;
};

void Trigger()
{
    gRandom->SetSeed(kSeed);

    TFile *fin = TFile::Open("LMO_Th232_TotalEdep.root", "READ");
    TFile *fout = TFile::Open("LMO_Th232_TotalEdep_trigger.root", "RECREATE");
    fout->SetCompressionSettings(106);   // ZLIB level 6

    // 归一化与文件介绍写入输出文件
    TString metaNStr = TString::Format("%ld", kNSimEvents);
    TNamed *metaN = new TNamed("NSimEvents", metaNStr.Data());
    metaN->Write();
    TString metaCStr = TString::Format(
        "Th-232 decay chain simulation in LMO crystals, with trigger simulation applied. "
        "Trigger: TotalEdep_keV + Gaussian(0, sigma) > threshold; recorded value is "
        "MeasuredEdep_keV (reconstructed energy, no trigger noise added). "
        "threshold = %g keV (Crystal1), %g keV (Crystal2); "
        "sigma = %g keV (Crystal1), %g keV (Crystal2); fixed seed %ld. "
        "All 9 branches (incl. Time) passed through unchanged for passing rows; "
        "tree schema identical to LMO_Th232_TotalEdep_PileUp.root so PileUp.C "
        "can run on this file directly. "
        "One entry = one particle (track). Normalization: %g Bq x 86400 s / %ld "
        "= %g counts/day per entry. Crystal1 <-> exp LMO2, Crystal2 <-> exp LMO1. "
        "Entries with MeasuredEdep_keV <= 0 are pure noise triggers (zero-deposition).",
        kTrigThresholdC1, kTrigThresholdC2, kTrigSigmaC1, kTrigSigmaC2, kSeed,
        kSourceActivity, kNSimEvents, kSourceActivity*86400.0/kNSimEvents);
    TNamed *metaC = new TNamed("comment", metaCStr.Data());
    metaC->Write();

    for (auto tn : {"Crystal1", "Crystal2"}) {
        double sig  = (string(tn) == "Crystal1") ? kTrigSigmaC1 : kTrigSigmaC2;
        double thr  = (string(tn) == "Crystal1") ? kTrigThresholdC1 : kTrigThresholdC2;

        TTree *t = (TTree *)fin->Get(tn);

        // 绑定全部 9 个输入分支(只激活需要的, 跳过其余以加快读取)
        Int_t    inEventID, inTrackID, inPDG;
        Double_t inTotal, inMeasured, inTime;
        char inParticleName[64], inCreatorProcess[64], inVertexVolumeName[64];
        t->SetBranchStatus("*", 0);
        t->SetBranchStatus("EventID", 1);
        t->SetBranchStatus("TrackID", 1);
        t->SetBranchStatus("ParticleName", 1);
        t->SetBranchStatus("PDG", 1);
        t->SetBranchStatus("CreatorProcess", 1);
        t->SetBranchStatus("VertexVolumeName", 1);
        t->SetBranchStatus("TotalEdep_keV", 1);
        t->SetBranchStatus("MeasuredEdep_keV", 1);
        t->SetBranchStatus("Time", 1);
        t->SetBranchAddress("EventID",&inEventID);
        t->SetBranchAddress("TrackID",&inTrackID);
        t->SetBranchAddress("ParticleName",inParticleName);
        t->SetBranchAddress("PDG",&inPDG);
        t->SetBranchAddress("CreatorProcess",inCreatorProcess);
        t->SetBranchAddress("VertexVolumeName",inVertexVolumeName);
        t->SetBranchAddress("TotalEdep_keV",&inTotal);
        t->SetBranchAddress("MeasuredEdep_keV",&inMeasured);
        t->SetBranchAddress("Time",&inTime);

        TTree *tout = new TTree(tn, tn);
        tout->SetAutoFlush(-100000000);   // 100 MB 内存缓冲, 加快写入

        TriggerRow out;
        tout->Branch("EventID",&out.EventID,"EventID/I");
        tout->Branch("TrackID",&out.TrackID,"TrackID/I");
        tout->Branch("ParticleName",out.ParticleName,"ParticleName/C");
        tout->Branch("PDG",&out.PDG,"PDG/I");
        tout->Branch("CreatorProcess",out.CreatorProcess,"CreatorProcess/C");
        tout->Branch("VertexVolumeName",out.VertexVolumeName,"VertexVolumeName/C");
        tout->Branch("TotalEdep_keV",&out.TotalEdep_keV,"TotalEdep_keV/D");
        tout->Branch("MeasuredEdep_keV",&out.MeasuredEdep_keV,"MeasuredEdep_keV/D");
        tout->Branch("Time",&out.Time,"Time/D");

        Long64_t n = t->GetEntries(), nPass = 0, nNoise = 0;
        for (Long64_t i = 0; i < n; i++) {
            t->GetEntry(i);
            // 触发判定: TotalEdep + 触发噪声 > 该晶体阈值; 记录 MeasuredEdep 本身
            if (inTotal + gRandom->Gaus(0.0, sig) > thr) {
                out.EventID          = inEventID;
                out.TrackID          = inTrackID;
                strcpy(out.ParticleName, inParticleName);
                out.PDG              = inPDG;
                strcpy(out.CreatorProcess, inCreatorProcess);
                strcpy(out.VertexVolumeName, inVertexVolumeName);
                out.TotalEdep_keV    = inTotal;
                out.MeasuredEdep_keV = inMeasured;
                out.Time             = inTime;
                tout->Fill();
                nPass++;
                if (inMeasured <= 0.0) nNoise++;   // 纯噪声触发(零沉积条目)
            }
        }
        tout->Write();
        // 写完与文件目录解绑, 防止 Close 时重复写入产生多余 cycle
        tout->SetDirectory(nullptr);

        // 该树触发通过率统计元数据
        TString metaTreeStr = TString::Format(
            "Trigger pass-rate stats: threshold=%g keV, sigma=%g keV. "
            "%s: total=%lld passed=%lld passRate=%.2f%% noiseTriggers=%lld",
            thr, sig, tn, n, nPass, 100.0*nPass/(double)n, nNoise);
        TString metaTreeName = TString::Format("Trigger_%s", tn);
        TNamed *metaT = new TNamed(metaTreeName.Data(), metaTreeStr.Data());
        metaT->Write();

        cout << tn << ": " << n << " -> " << nPass << " entries passed"
             << "  (通过率 " << 100.0*nPass/(double)n << "%, 其中纯噪声触发 "
             << nNoise << " 条)" << endl;
    }

    // fin 不显式 Close (本机 ROOT JIT 下只读文件 Close 会段错误);
    // fout 是写文件, Close 正常。
    fout->Close();
    cout << "output: LMO_Th232_TotalEdep_trigger.root" << endl;
}
