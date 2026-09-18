//==============================================================
// PileUp.C
// 对 LMO_Th232_TotalEdep.root 的时间序列做堆叠(pile-up)剔除
//
// 定义(用户确认 2026-08-29):
//   事件 = 树的一行(一次粒子到达)。
//   以每个事件为中心, 滑动窗口 ±kPileUpWindowMs 内若出现
//   其他事件, 即为堆叠, 参与堆叠的所有事件(整簇)都舍去。
//   等价表述:任意两行时间差 <= 窗口值时, 两行都舍去;
//   聚簇(3 行及以上连续)内每一行都舍去。
//
// 输入(默认): LMO_Th232_TotalEdep_trigger.root
//         即 Trigger.C 的输出(全格式 9 分支, 含 Time 分支,
//         时间随行号单调递增 —— 本宏依赖这个性质)。
//         须先运行 Trigger.C; 也可传参指定其他同格式文件。
// 输出(默认): LMO_Th232_TotalEdep_trigger_PileUp.root
//        树结构与输入相同(8 个数据分支 + Time), 只含幸存行
// 两棵树(Crystal1/Crystal2)独立处理。
//
// 用法(在 build 目录, 默认输入/输出):
//   root -l -b -q ../macros/PileUp.C
// 或交互模式:
//   .L ../macros/PileUp.C
//   PileUp()                        // 默认: trigger 文件 -> trigger_PileUp 文件
//   PileUp("输入.root","输出.root")  // 自定义
//
// 预期拒绝比例(泊松到达, 速率 r): 1 - exp(-2*r*w)
//   r 取输入文件中该晶体的实际计数率(触发后 r 会变小)。
//==============================================================

#include <TFile.h>
#include <TTree.h>
#include <TNamed.h>
#include <TString.h>
#include <iostream>
#include <vector>
#include <cstring>

using namespace std;

const double kPileUpWindowMs = 300.0;   // 堆叠时间窗口(ms)

// 树的一行数据(与 EnergyReconstruction.C 输出的 schema 一致, 外加 Time)
struct PileUpRow
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

void PileUp(const char* inputName = "LMO_Th232_TotalEdep_trigger.root",
            const char* outputName = "LMO_Th232_TotalEdep_trigger_PileUp.root")
{
    TFile *f = TFile::Open(inputName,"READ");
    if(!f || f->IsZombie())
    {
        cout<<"Cannot open "<<inputName<<"!"<<endl;
        return;
    }
    TFile *fout = TFile::Open(outputName,"RECREATE");

    // 元数据写入输出文件
    TString metaWStr = TString::Format("%g", kPileUpWindowMs);
    TNamed *metaW = new TNamed("PileUpWindow_ms", metaWStr.Data());
    metaW->Write();
    TString metaCStr = TString::Format(
        "Pile-up rejection applied on %s. Cut: sliding window +-%g ms around "
        "each hit; a hit is rejected if any other hit lies within the window; "
        "all hits in a cluster are rejected. Input rows are per-particle "
        "arrivals with Poisson arrival times (Time branch, ms, monotonically "
        "increasing in row order). Output keeps the full 9-branch schema "
        "unchanged; per-tree statistics in PileUp_Crystal1 / PileUp_Crystal2.",
        inputName, kPileUpWindowMs);
    TNamed *metaC = new TNamed("comment", metaCStr.Data());
    metaC->Write();

    const char* treeNames[2] = {"Crystal1","Crystal2"};

    for (int ic=0; ic<2; ic++)
    {
        TTree *tree = (TTree*)f->Get(treeNames[ic]);
        if(!tree)
        {
            cout<<treeNames[ic]<<" not found!"<<endl;
            continue;
        }
        if(!tree->GetBranch("Time"))
        {
            cout<<treeNames[ic]<<": no Time branch, run ParticleTimeInformation.C first!"<<endl;
            continue;
        }

        Long64_t n = tree->GetEntries();
        if(n <= 0) continue;

        // ---- 第 1 步: 只读 Time 分支(只激活该分支, 加速读取) ----
        Double_t timeVal;
        tree->SetBranchStatus("*",0);
        tree->SetBranchStatus("Time",1);
        tree->SetBranchAddress("Time",&timeVal);
        std::vector<Double_t> t(n);
        for(Long64_t i=0;i<n;i++)
        {
            tree->GetEntry(i);
            t[i] = timeVal;
        }

        // ---- 第 2 步: 标记堆叠行 ----
        // Time 单调递增, 行 i 的最近邻就是 i-1 和 i+1;
        // 与任一邻居间隔 <= 窗口即堆叠。聚簇中每行都会被标记。
        std::vector<unsigned char> rejected(n, 0);
        Long64_t nRejected = 0;
        for(Long64_t i=0;i<n;i++)
        {
            bool r = (i>0   && t[i]-t[i-1] <= kPileUpWindowMs)
                  || (i<n-1 && t[i+1]-t[i] <= kPileUpWindowMs);
            rejected[i] = r ? 1 : 0;
            if(r) nRejected++;
        }
        Long64_t nKept = n - nRejected;

        // ---- 第 3 步: 顺序拷贝幸存行到新树 ----
        Int_t    inEventID, inTrackID, inPDG;
        Double_t inTotal, inMeasured;
        char inParticleName[64], inCreatorProcess[64], inVertexVolumeName[64];
        tree->SetBranchStatus("*",1);   // 重新激活全部输入分支
        tree->SetBranchAddress("EventID",&inEventID);
        tree->SetBranchAddress("TrackID",&inTrackID);
        tree->SetBranchAddress("ParticleName",inParticleName);
        tree->SetBranchAddress("PDG",&inPDG);
        tree->SetBranchAddress("CreatorProcess",inCreatorProcess);
        tree->SetBranchAddress("VertexVolumeName",inVertexVolumeName);
        tree->SetBranchAddress("TotalEdep_keV",&inTotal);
        tree->SetBranchAddress("MeasuredEdep_keV",&inMeasured);

        fout->cd();
        TTree *newtree = new TTree(treeNames[ic], treeNames[ic]);
        newtree->SetAutoSave(0);   // 最后统一写盘

        PileUpRow out;
        newtree->Branch("EventID",&out.EventID,"EventID/I");
        newtree->Branch("TrackID",&out.TrackID,"TrackID/I");
        newtree->Branch("ParticleName",out.ParticleName,"ParticleName/C");
        newtree->Branch("PDG",&out.PDG,"PDG/I");
        newtree->Branch("CreatorProcess",out.CreatorProcess,"CreatorProcess/C");
        newtree->Branch("VertexVolumeName",out.VertexVolumeName,"VertexVolumeName/C");
        newtree->Branch("TotalEdep_keV",&out.TotalEdep_keV,"TotalEdep_keV/D");
        newtree->Branch("MeasuredEdep_keV",&out.MeasuredEdep_keV,"MeasuredEdep_keV/D");
        newtree->Branch("Time",&out.Time,"Time/D");

        for(Long64_t i=0;i<n;i++)
        {
            if(rejected[i]) continue;
            tree->GetEntry(i);
            out.EventID          = inEventID;
            out.TrackID          = inTrackID;
            strcpy(out.ParticleName, inParticleName);
            out.PDG              = inPDG;
            strcpy(out.CreatorProcess, inCreatorProcess);
            strcpy(out.VertexVolumeName, inVertexVolumeName);
            out.TotalEdep_keV    = inTotal;
            out.MeasuredEdep_keV = inMeasured;
            out.Time             = timeVal;   // 第 1 步绑定的地址仍有效
            newtree->Fill();
        }

        newtree->Write("", TObject::kOverwrite);
        delete newtree;

        // 该树元数据
        TString metaStr = TString::Format(
            "PileUp cut: sliding window +-%g ms around each hit; "
            "all hits in a cluster rejected. %s: total=%lld rejected=%lld kept=%lld (%.2f%%)",
            kPileUpWindowMs, treeNames[ic], n, nRejected, nKept,
            100.0*nRejected/(double)n);
        TString metaName = TString::Format("PileUp_%s", treeNames[ic]);
        TNamed *metaC = new TNamed(metaName.Data(), metaStr.Data());
        metaC->Write();

        cout<<treeNames[ic]<<": total="<<n
            <<"  rejected="<<nRejected<<"  kept="<<nKept
            <<"  (拒绝比例 "<<100.0*nRejected/(double)n<<"%)"<<endl;
    }

    f->Close();
    fout->Close();
    cout<<"output: "<<outputName<<endl;
}
