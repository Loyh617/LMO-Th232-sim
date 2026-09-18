//使用方法：.x spectrum.C
void spectrum()
{
    gStyle->SetOptStat(0);   // 关闭 ROOT 自动 stats box

    //==================== 基本参数 ====================
    double E0 = 2614.5;   // Tl-208 peak
    double binWidth = 1.0;   // keV
    double Emin = 0.0;     // keV, 直方图最小能量
    double Emax = 8000.0;     // keV, 直方图最大能量
    // 画哪个能量:true = MeasuredEdep_keV(含探测器分辨率), false = TotalEdep_keV(真值)
    // 注意:选 true 前必须先重新运行 EnergyReconstruction.C+ 生成新分支
    bool useMeasured = true;
    const char *energyBranch = useMeasured ? "MeasuredEdep_keV" : "TotalEdep_keV";
    bool excludeAlphas = false;   // 去掉入射 α 粒子

    // 实验分辨率模型(与 EnergyReconstruction.C 中 Sigma1/Sigma2 一致,单位 keV):
    //   LMO1: sigma = 0.00735*E + 6.24
    //   LMO2: sigma = 0.01039*E + 3.69
    // 晶体映射(用户确认 2026-08-26):Crystal1 = LMO2, Crystal2 = LMO1
    double sigC1 = 0.01039*E0 + 3.69;   // Crystal1 ← LMO2 模型
    double sigC2 = 0.00735*E0 + 6.24;   // Crystal2 ← LMO1 模型

    //==================== 创建 TChain ====================
    TChain chain1("Crystal1");
    TChain chain2("Crystal2");

    TString file = Form("LMO_Th232_TotalEdep.root");
    chain1.Add(file);
    chain2.Add(file);

    //==================== 直方图 ====================
    if (gROOT->FindObject("h1")) delete gROOT->FindObject("h1");
    if (gROOT->FindObject("h2")) delete gROOT->FindObject("h2");

    TH1D *h1 = new TH1D("h1",";Energy (keV);Counts",
                        (Emax-Emin)/binWidth, Emin, Emax);

    TH1D *h2 = new TH1D("h2",";Energy (keV);Counts",
                        (Emax-Emin)/binWidth, Emin, Emax);

    //==================== 自动填充 ====================
    TString var1 = Form("%s>>h1", energyBranch);
    TString var2 = Form("%s>>h2", energyBranch);
    TString cut  = Form("%s>0",  energyBranch);
    if (excludeAlphas) cut += " && PDG != 1000020040";
    chain1.Draw(var1, cut, "goff");
    chain2.Draw(var2, cut, "goff");

    //==================== 样式 ====================
    h1->SetLineColor(kRed);
    h1->SetLineWidth(2);

    h2->SetLineColor(kBlue);
    h2->SetLineWidth(2);

    //==================== 统计事件数 ====================
    int n1 = h1->GetEntries();
    int n2 = h2->GetEntries();

    //==================== 高斯拟合 ====================

    // Crystal1(峰下连续本底 ~760/bin,用高斯+线性本底;窗口按实验 sigma 取 ±3σ)
    TF1 *fit1 = new TF1("fit1","[0]*exp(-0.5*((x-[1])/[2])^2)+[3]+[4]*(x-2614.5)",
                        E0-3.0*sigC1,E0+3.0*sigC1);

    double amp1 = h1->GetBinContent(h1->FindBin(E0));
    if (amp1 < 1) amp1 = 1;
    double bgl1 = h1->GetBinContent(h1->FindBin(E0-3.0*sigC1-10));
    double bgr1 = h1->GetBinContent(h1->FindBin(E0+3.0*sigC1+10));

    fit1->SetParameters(amp1, E0, sigC1, 0.5*(bgl1+bgr1),
                        (bgr1-bgl1)/(2.0*(3.0*sigC1+10)));

    h1->Fit(fit1,"RQ0");

    double mean1  = fit1->GetParameter(1);
    double sigma1 = fit1->GetParameter(2);

    double roiLow1  = mean1 - 3.0*sigma1;
    double roiHigh1 = mean1 + 3.0*sigma1;

    int peak1 = h1->Integral(
        h1->FindBin(roiLow1),

        h1->FindBin(roiHigh1)
    );

    // Crystal2(峰被实验分辨率抹宽后与本底同量级,也改用高斯+线性本底)
    TF1 *fit2 = new TF1("fit2","[0]*exp(-0.5*((x-[1])/[2])^2)+[3]+[4]*(x-2614.5)",
                        E0-3.0*sigC2,E0+3.0*sigC2);

    double amp2 = h2->GetBinContent(h2->FindBin(E0));
    if (amp2 < 1) amp2 = 1;
    double bgl2 = h2->GetBinContent(h2->FindBin(E0-3.0*sigC2-10));
    double bgr2 = h2->GetBinContent(h2->FindBin(E0+3.0*sigC2+10));

    fit2->SetParameters(amp2, E0, sigC2, 0.5*(bgl2+bgr2),
                        (bgr2-bgl2)/(2.0*(3.0*sigC2+10)));

    h2->Fit(fit2,"RQ0");

    double mean2  = fit2->GetParameter(1);
    double sigma2 = fit2->GetParameter(2);

    double roiLow2  = mean2 - 3.0*sigma2;
    double roiHigh2 = mean2 + 3.0*sigma2;

    int peak2 = h2->Integral(
        h2->FindBin(roiLow2),
        h2->FindBin(roiHigh2)
    );
    //==================== 画图 ====================
    TCanvas *c = new TCanvas("c","Spectrum",900,700);
    c->SetLogy();

    h1->SetTitle("");   // 清掉 ROOT 默认标题
    h2->SetTitle("");

    h1->Draw("HIST");
    h2->Draw("HIST SAME");

    // 顶部标题
    TLatex latex;
    latex.SetNDC();
    latex.SetTextSize(0.04);
    latex.DrawLatex(0.25, 0.92, "LMO Energy Spectrum of ^{232}Th (1.6cm WTh wire)");

    // fit1->SetLineColor(kRed);
    // fit2->SetLineColor(kBlue);

    // fit1->Draw("same");
    // fit2->Draw("same");

    // ==================== 图例 ====================
    TLegend *leg = new TLegend(0.65,0.75,0.88,0.88);
    leg->SetTextSize(0.035);
    leg->SetBorderSize(0);     // 去边框
    leg->SetFillStyle(0);      // 透明背景
    leg->SetMargin(0.2);       // 内边距
    leg->AddEntry(h1,"Crystal 1","l");
    leg->AddEntry(h2,"Crystal 2","l");
    leg->Draw();

    //==================== Th-232 衰变链特征γ谱线标记 ====================
    // struct GammaLine { double E; const char *iso; };
    // GammaLine gLines[] = {
    //     {238.63, "Pb-212"},
    //     {240.99, "Ra-224"},
    //     {277.37, "Tl-208"},
    //     {300.09, "Pb-212"},
    //     {338.32, "Ac-228"},
    //     {463.00, "Ac-228"},
    //     {510.77, "Tl-208"},
    //     {583.19, "Tl-208"},
    //     {727.33, "Bi-212"},
    //     {794.95, "Ac-228"},
    //     {860.56, "Tl-208"},
    //     {911.20, "Ac-228"},
    //     {964.77, "Ac-228"},
    //     {968.97, "Ac-228"},
    //     {2614.51, "Tl-208"}
    // };
    // int nGLines = sizeof(gLines)/sizeof(GammaLine);

    // // 坐标说明:此前用数据坐标在 log-y 画布上画线反复踩坑
    // // (pad 范围是 log10 值、YtoPad 内部再取 log10、缩放还会改变范围)。
    // // 现改用 NDC 坐标:TLine::SetNDC() + TLatex::SetNDC(),
    // // 绕开数据坐标语义,不受缩放与 log 契约影响。
    // // x:数据[0,3000] → NDC[0.10,0.90](ROOT 默认边距 0.1)
    // // y:线从画框底(0.10)到画框顶(0.90),正好停在 x 轴上

    // for (int i = 0; i < nGLines; i++)
    // {
    //     double xndc = 0.10 + (gLines[i].E/3000.0) * 0.80;

    //     TLine *gl = new TLine(xndc, 0.10, xndc, 0.90);
    //     gl->SetNDC();              // 关键:按 NDC 解释坐标
    //     gl->SetLineStyle(2);       // 虚线
    //     gl->SetLineColor(kGray+2);
    //     gl->SetLineWidth(1);
    //     if (gLines[i].E > 2000)    // 2614.5 主峰突出显示
    //     {
    //         gl->SetLineColor(kRed+1);
    //         gl->SetLineWidth(2);
    //     }
    //     gl->Draw();

    //     // 标签:NDC 固定高度(画框 95%/78%),旋转90°,奇偶交替防重叠
    //     TLatex *lab = new TLatex();
    //     lab->SetNDC();
    //     lab->SetTextSize(0.02);
    //     lab->SetTextAngle(90);
    //     lab->SetTextAlign(12);
    //     lab->DrawLatex(xndc, (i%2==0) ? 0.86 : 0.72,
    //                    Form("%.1f %s", gLines[i].E, gLines[i].iso));
    // }

    // ==================== 写数值信息 ====================
    // latex.SetNDC();
    // latex.SetTextSize(0.03);

    double counts1 = h1->GetBinContent(h1->FindBin(E0));
    double counts2 = h2->GetBinContent(h2->FindBin(E0));

    printf("Crystal1: Entries = %d\n", n1);
    printf("Crystal1 Peak(2614.5 keV) = %.0f\n", counts1);
    // printf("Crystal1 ROI(3sigma) Counts = %d\n", peak1);
    // printf("Crystal1: μ = %.2f keV   sigma = %.2f keV\n", mean1, sigma1);
    printf("Crystal2: Entries = %d\n", n2);
    printf("Crystal2 Peak(2614.5 keV) = %.0f\n", counts2);
    // printf("Crystal2 ROI(3sigma) Counts = %d\n", peak2);
    // printf("Crystal2: μ = %.2f keV   sigma = %.2f keV\n", mean2, sigma2);

    // latex.SetTextColor(kRed);
    // latex.DrawLatex(0.15, 0.50,
    //     Form("Crystal1: Entries = %d", n1));

    // latex.DrawLatex(0.15, 0.45,
    //     Form("Crystal1 ROI(3#sigma) Counts = %d", peak1));
    //     // Form("Crystal1 Peak(2614.5 keV) = %.0f", counts1));

    // latex.DrawLatex(0.15,0.76,
    //     Form("#mu = %.2f keV   #sigma = %.2f keV", mean1,sigma1));

    // latex.SetTextColor(kBlue);
    // latex.DrawLatex(0.15, 0.40,
    //     Form("Crystal2: Entries = %d", n2));

    // latex.DrawLatex(0.15,0.35,
    //     Form("Crystal2 ROI(3#sigma) Counts = %d", peak2));
    //     // Form("Crystal2 Peak(2614.5 keV) = %.0f", counts2));

    // latex.DrawLatex(0.15,0.63,
    //     Form("#mu = %.2f keV   #sigma = %.2f keV", mean2,sigma2));

    //==================== 保存 ====================
    c->SaveAs("LMO_spectrum.png");

    // TFile fout("LMO_spectrum_compare.root","RECREATE");
    // h1->Write();
    // h2->Write();
    // c->Write();
    // fout.Close();
}