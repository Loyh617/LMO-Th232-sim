//==============================================================
// ParticleTimeInformation.C
// 给 LMO_Th232_TotalEdep.root 的两个树添加 Time 分支(单位 ms)
//
// 时间序列规则(每棵树独立):
//   - 先把整棵树的行序用固定种子打乱(Fisher-Yates)
//   - 打乱后: 第 0 行 Time = 0
//   - 之后每一行的时间 = 前一行时间 + 指数分布取样
//     I(t) = r*exp(-r*t), 即到达过程是速率为 r 的泊松过程
//   - r 为该晶体的粒子计数率:
//       r = 树总行数 / 1e8 * 3.0992 Bq
//     其中 1e8 = run.mac 的 beamOn(模拟总衰变数),
//     3.0992 Bq = 实验源活度(次/秒)
//   注意:gNSimDecays 必须与 run.mac 的 beamOn 保持一致,
//   否则时间间隔与总曝光时间会整体差一个倍数。
//   洗牌(2026-08-28 新增):打乱行序后再赋时间, 消除同事件粒子
//   在时间上的关联(原文件行序按事件排列, EventID 随行号单调)。
//   用法:先重跑 EnergyReconstruction.C 生成不含 Time 分支的
//   新文件, 再运行本宏。洗牌与时间序列使用两个独立的
//   TRandom3 实例, 互不干扰。
//
// 单位换算:Bq = 次/秒,指数分布用 ms 为单位时 r 需除以 1000
//
// 用法(在 build 目录):
//   root -l -b -q ../ParticleTimeInformation.C
// 或交互模式:
//   .L ../ParticleTimeInformation.C
//   ParticleTimeInformation()
//==============================================================

#include <TFile.h>
#include <TTree.h>
#include <TRandom.h>
#include <TSystem.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>

using namespace std;

const double gSourceActivity = 3.0992;   // Bq(实验源活度)
const double gNSimDecays     = 1.0e8;     // 模拟总衰变数(run.mac beamOn)
int    gTimeSeed = 12345;                 // 固定种子,保证时间序列可复现
int    gShuffleSeed = 20250828;           // 洗牌固定种子,打乱行序用

// 树的一行数据(与 EnergyReconstruction.C 输出的 schema 一致)
struct RowData
{
    Int_t    EventID;
    Int_t    TrackID;
    char     ParticleName[64];
    Int_t    PDG;
    char     CreatorProcess[64];
    char     VertexVolumeName[64];
    Double_t TotalEdep_keV;
    Double_t MeasuredEdep_keV;
};

void ParticleTimeInformation()
{
    // 读原文件(只读); 洗牌+Time 结果写临时文件, 有新树写入时才替换
    TFile *f = TFile::Open("LMO_Th232_TotalEdep.root","READ");
    if(!f || f->IsZombie())
    {
        cout<<"Cannot open LMO_Th232_TotalEdep.root!"<<endl;
        return;
    }
    TFile *fout = TFile::Open("LMO_Th232_TotalEdep_shuffled_tmp.root","RECREATE");

    // 洗牌与时间序列使用独立的随机源, 互不干扰
    TRandom3 rndShuffle;
    TRandom3 rndTime(gTimeSeed);

    const char* treeNames[2] = {"Crystal1","Crystal2"};
    bool anyWritten = false;

    for (int ic=0; ic<2; ic++)
    {
        TTree *tree = (TTree*)f->Get(treeNames[ic]);
        if(!tree)
        {
            cout<<treeNames[ic]<<" not found!"<<endl;
            continue;
        }

        Long64_t n = tree->GetEntries();
        if(n <= 0) continue;

        // 计数率:Bq → 次/ms
        double ratePerMs = (double)n / gNSimDecays * gSourceActivity / 1000.0;

        if(tree->GetBranch("Time"))
        {
            cout<<treeNames[ic]<<": Time branch already exists, skipped."<<endl;
            continue;
        }

        // ---- 第 1 步: 顺序读入全部行到内存(单一大 basket 下顺序读最快) ----
        Int_t    inEventID, inTrackID, inPDG;
        Double_t inTotal, inMeasured;
        char inParticleName[64], inCreatorProcess[64], inVertexVolumeName[64];
        tree->SetBranchAddress("EventID",&inEventID);
        tree->SetBranchAddress("TrackID",&inTrackID);
        tree->SetBranchAddress("ParticleName",inParticleName);
        tree->SetBranchAddress("PDG",&inPDG);
        tree->SetBranchAddress("CreatorProcess",inCreatorProcess);
        tree->SetBranchAddress("VertexVolumeName",inVertexVolumeName);
        tree->SetBranchAddress("TotalEdep_keV",&inTotal);
        tree->SetBranchAddress("MeasuredEdep_keV",&inMeasured);

        std::vector<RowData> rows;
        rows.reserve(n);
        for (Long64_t i=0; i<n; i++)
        {
            tree->GetEntry(i);
            RowData r;
            r.EventID = inEventID;
            r.TrackID = inTrackID;
            strcpy(r.ParticleName, inParticleName);
            r.PDG = inPDG;
            strcpy(r.CreatorProcess, inCreatorProcess);
            strcpy(r.VertexVolumeName, inVertexVolumeName);
            r.TotalEdep_keV    = inTotal;
            r.MeasuredEdep_keV = inMeasured;
            rows.push_back(r);
        }
        tree->SetBranchStatus("*", 0);   // 解绑, 释放输入树资源

        // ---- 第 2 步: 固定种子的行序排列(Fisher-Yates, 两棵树排列不同但固定) ----
        // perm[i] = 输出第 i 行对应的原始行号。
        rndShuffle.SetSeed(gShuffleSeed + ic);
        std::vector<Long64_t> perm(n);
        for(Long64_t i=0;i<n;i++) perm[i]=i;
        for(Long64_t i=n-1;i>0;i--)
        {
            Long64_t j = (Long64_t)(rndShuffle.Rndm()*(i+1));
            std::swap(perm[i], perm[j]);
        }

        // ---- 第 3 步: 按洗牌后的顺序写新树, 时间随行序单调累加 ----
        Double_t timeVal = 0.;
        fout->cd();
        TTree *newtree = new TTree(treeNames[ic], treeNames[ic]);
        newtree->SetAutoSave(0);   // 最后统一写盘

        RowData out;
        newtree->Branch("EventID",&out.EventID,"EventID/I");
        newtree->Branch("TrackID",&out.TrackID,"TrackID/I");
        newtree->Branch("ParticleName",out.ParticleName,"ParticleName/C");
        newtree->Branch("PDG",&out.PDG,"PDG/I");
        newtree->Branch("CreatorProcess",out.CreatorProcess,"CreatorProcess/C");
        newtree->Branch("VertexVolumeName",out.VertexVolumeName,"VertexVolumeName/C");
        newtree->Branch("TotalEdep_keV",&out.TotalEdep_keV,"TotalEdep_keV/D");
        newtree->Branch("MeasuredEdep_keV",&out.MeasuredEdep_keV,"MeasuredEdep_keV/D");
        newtree->Branch("Time",&timeVal,"Time/D");

        double t = 0.;   // 打乱后第 0 行 Time = 0
        for (Long64_t i=0; i<n; i++)
        {
            out = rows[perm[i]];         // 洗牌后的第 i 行
            timeVal = t;
            newtree->Fill();
            // 指数间隔:期望 1/ratePerMs 毫秒
            t += rndTime.Exp(1.0/ratePerMs);
        }

        newtree->Write("", TObject::kOverwrite);
        delete newtree;                  // 释放该树的缓冲
        std::vector<RowData>().swap(rows);   // 释放本树内存, 再处理下一棵树
        std::vector<Long64_t>().swap(perm);
        anyWritten = true;

        cout<<treeNames[ic]<<": entries="<<n
            <<"  rate="<<ratePerMs<<" /ms  ("<<ratePerMs*1000.<<" Bq)"
            <<"  mean interval="<<1.0/ratePerMs<<" ms"
            <<"  last Time="<<t<<" ms"<<endl;
    }

    f->Close();
    fout->Close();

    // 只有真的写了新树才替换; 否则(全部被 Time 检查跳过)保留原文件
    if(anyWritten)
        gSystem->Rename("LMO_Th232_TotalEdep_shuffled_tmp.root",
                        "LMO_Th232_TotalEdep.root");
    cout<<"Done."<<endl;
}
