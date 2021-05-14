#include "AnaSample.hh"

AnaSample::AnaSample(int sample_id, const std::string& name, const std::string& binning, int pmt_type)
    : m_sample_id(sample_id)
    , m_norm(1.0)
    , m_pmttype(pmt_type)
    , m_pmtmask(0)
    , m_nPMTpermPMT(19)
    , m_name(name)
    , m_binning(binning)
    , m_hpred(nullptr)
    , m_hdata(nullptr)
{
    TH1::SetDefaultSumw2(true);

    std::cout  << "Sample: " << m_name << " (ID: " << m_sample_id << ")" << std::endl;

    m_bm.SetBinning(m_binning);
    m_bm.Print();
    m_nbins = m_bm.GetNbins();

    m_llh = new PoissonLLH;

    MakeHistos(); // with default binning

    std::cout << "MakeHistos called." << std::endl;
}

AnaSample::~AnaSample()
{
    if(m_hpred != nullptr)
        delete m_hpred;

    if(m_hdata != nullptr)
        delete m_hdata;
}

void AnaSample::LoadEventsFromFile(const std::string& file_name, const std::string& tree_name, const std::string& pmt_tree_name)
{
    AnaTree selTree(file_name, tree_name, pmt_tree_name);
    if (m_pmtmask>0) selTree.MaskPMT(m_pmtmask, m_pmttype, m_nPMTpermPMT);

    std::cout  << "Reading events for from "<<file_name<<"...\n";
    std::vector<std::vector<AnaEvent>> event_vec = selTree.GetEvents();

    // Add PMT hits
    std::vector<AnaEvent> hit_vec = event_vec[0];
    for (long int i=0;i<hit_vec.size();i++) {
        bool skip = false;
        for (int j=0;j<m_cutvar.size();j++) {
            double val = hit_vec[i].GetEventVar(m_cutvar[j]);
            if (val<m_cutlow[j] || val>m_cuthigh[j]) {
                skip=true;
                break;
            }
        }
        hit_vec[i].SetSampleType(m_sample_id);
        if (!skip) AddEvent(hit_vec[i]);
    }

    // Add PMT geometry
    std::vector<AnaEvent> pmt_vec = event_vec[1];
    for (long int i=0;i<pmt_vec.size();i++) {
        bool skip = false;
        for (int j=0;j<m_cutvar.size();j++) {
            if (m_cutvar[j]=="nPE"||m_cutvar[j]=="timetof") continue;
            double val = pmt_vec[i].GetEventVar(m_cutvar[j]);
            if (val<m_cutlow[j] || val>m_cuthigh[j]) {
                skip=true;
                break;
            }
        }
        pmt_vec[i].SetSampleType(m_sample_id);
        if (!skip) AddPMT(pmt_vec[i]);
    }

    PrintStats();
}

AnaEvent* AnaSample::GetEvent(const unsigned int evnum)
{
#ifndef NDEBUG
    if(m_events.empty())
    {
        std::cerr << " In AnaSample::GetEvent()" << std::endl;
        std::cerr << " No events are found in " << m_name << " sample." << std::endl;
        return nullptr;
    }
    else if(evnum >= m_events.size())
    {
        std::cerr << " In AnaSample::GetEvent()" << std::endl;
        std::cerr << " Event number out of bounds in " << m_name << " sample." << std::endl;
        return nullptr;
    }
#endif

    return &m_events[evnum];
}

AnaEvent* AnaSample::GetPMT(const unsigned int evnum)
{
#ifndef NDEBUG
    if(m_pmts.empty())
    {
        std::cerr << " In AnaSample::GetPMT()" << std::endl;
        std::cerr << " No PMTs are found in " << m_name << " sample." << std::endl;
        return nullptr;
    }
    else if(evnum >= m_pmts.size())
    {
        std::cerr << " In AnaSample::GetPMT()" << std::endl;
        std::cerr << " PMT number out of bounds in " << m_name << " sample." << std::endl;
        return nullptr;
    }
#endif

    return &m_pmts[evnum];
}

void AnaSample::PrintStats() const
{
    const double mem_kb = sizeof(m_events) * m_events.size() / 1000.0;
    std::cout << "Sample " << m_name << " ID = " << m_sample_id << std::endl;
    std::cout << "Num of events = " << m_events.size() << std::endl;
    std::cout << "Memory used   = " << mem_kb << " kB." << std::endl;
    std::cout << "Num of PMTs   = " << m_pmts.size() << std::endl;
}

void AnaSample::MakeHistos()
{
    if(m_hpred != nullptr)
        delete m_hpred;
    m_hpred = new TH1D(Form("%s_pred", m_name.c_str()), Form("%s_pred", m_name.c_str()), m_nbins, 0, m_nbins);
    m_hpred->SetDirectory(0);

    if(m_hdata != nullptr)
        delete m_hdata;
    m_hdata = new TH1D(Form("%s_data", m_name.c_str()), Form("%s_data", m_name.c_str()), m_nbins, 0, m_nbins);
    m_hdata->SetDirectory(0);
}

void AnaSample::InitEventMap()
{
    for(auto& e : m_events)
    {
        std::vector<double> binvar;
        for (auto t : m_binvar)
            binvar.push_back(e.GetEventVar(t));
        const int b = m_bm.GetBinIndex(binvar);
#ifndef NDEBUG
        if(b < 0)
        {
            std::cout << "In AnaSample::InitEventMap()\n"
                      << "No bin for current event." << std::endl;
            std::cout << "Event kinematics: " << std::endl;
            for(const auto val : e.GetRecoVar())
                std::cout << "\t" << val << std::endl;
        }
#endif
        e.SetSampleBin(b);
    }

    std::sort(m_events.begin(), m_events.end(), [](const AnaEvent a, const AnaEvent b){ return a.GetSampleBin() < b.GetSampleBin(); });

    for(auto& e : m_pmts)
    {
        std::vector<double> binvar;
        for (auto t : m_binvar)
            binvar.push_back(e.GetEventVar(t));
        const int b = m_bm.GetBinIndex(binvar);
#ifndef NDEBUG
        if(b < 0)
        {
            std::cout << "In AnaSample::InitEventMap()\n"
                      << "No bin for current PMT." << std::endl;
            std::cout << "PMT Var: " << std::endl;
            for(const auto val : e.GetRecoVar())
                std::cout << "\t" << val << std::endl;
        }
#endif
        e.SetSampleBin(b);
    }
}

void AnaSample::FillEventHist(bool reset_weights)
{
#ifndef NDEBUG
    if(m_hpred == nullptr)
    {
        std::cerr << "In AnaSample::FillEventHist() h_pred is a nullptr!"
                  << "Returning from function." << std::endl;
        return;
    }
#endif
    m_hpred->Reset();

    for(const auto& e : m_pmts)
    {
        const double weight = reset_weights ? e.GetEvWghtMC() : e.GetEvWght();
        const int reco_bin  = e.GetSampleBin();
        m_hpred->Fill(reco_bin + 0.5, weight);
    }

    return;
}

void AnaSample::FillDataHist(bool stat_fluc)
{
#ifndef NDEBUG
    if(m_hdata == nullptr)
    {
        std::cerr << "In AnaSample::FillDataHist() m_hdata is a nullptr!"
                  << "Returning from function." << std::endl;
        return;
    }
#endif
    m_hdata->Reset();

    for(const auto& e : m_events)
    {
        const double weight = e.GetEvWght()*e.GetPE();
        const int reco_bin  = e.GetSampleBin();
        m_hdata->Fill(reco_bin + 0.5, weight);
    }

    m_hdata->Scale(m_norm);

    if(stat_fluc) 
    {
        std::cout << "Applying statistical fluctuations..." << std::endl;

        for(int j = 1; j <= m_hdata->GetNbinsX(); ++j)
        {
            double val = m_hdata->GetBinContent(j);
            val = gRandom->Poisson(val);
#ifndef NDEBUG
            if(val <= 0.0)
            {
                std::cout   << "In AnaSample::FillEventHist()\n"
                            << "In Sample " <<  m_name << ", bin " << j
                            << " has 0 (or negative) entries. This may cause a problem with chi2 computations."
                            << std::endl;
            }
#endif
            m_hdata->SetBinContent(j, val);
        }
    }

    return;
}

void AnaSample::SetLLHFunction(const std::string& func_name)
{
    if(m_llh != nullptr)
        delete m_llh;

    if(func_name.empty())
    {
        std::cout << "Likelihood function name empty. Setting to Poisson by default." << std::endl;
        m_llh = new PoissonLLH;
    }
    else if(func_name == "Poisson")
    {
        std::cout << "Setting likelihood function to Poisson." << std::endl;
        m_llh = new PoissonLLH;
    }
    else if(func_name == "Effective")
    {
        std::cout << "Setting likelihood function to Tianlu's effective likelihood." << std::endl;
        m_llh = new EffLLH;
    }
    else if(func_name == "Barlow")
    {
        std::cout << "Setting likelihood function to Barlow-Beeston." << std::endl;
        m_llh = new BarlowLLH;
    }
}

double AnaSample::CalcLLH() const
{
    const unsigned int nbins = m_hpred->GetNbinsX();
    double* exp_w  = m_hpred->GetArray();
    double* exp_w2 = m_hpred->GetSumw2()->GetArray();
    double* data   = m_hdata->GetArray();

    double chi2 = 0.0;
    for(unsigned int i = 1; i <= nbins; ++i)
        chi2 += (*m_llh)(exp_w[i], exp_w2[i], data[i]);

    return chi2;
}

double AnaSample::CalcChi2() const
{
    if(m_hdata == nullptr)
    {
        std::cerr << "[ERROR]: In AnaSample::CalcChi2()\n"
                  << "[ERROR]: Need to define data histogram." << std::endl;
        return 0.0;
    }

    int nbins = m_hpred->GetNbinsX();
    if(nbins != m_hdata->GetNbinsX())
    {
        std::cerr << "[ERROR]: In AnaSample::CalcChi2()\n"
                  << "[ERROR]: Binning mismatch between data and mc.\n"
                  << "[ERROR]: MC bins: " << nbins << ", Data bins: " << m_hdata->GetNbinsX()
                  << std::endl;
        return 0.0;
    }

    double chi2 = 0.0;
    for(int j = 1; j <= nbins; ++j)
    {
        double obs = m_hdata->GetBinContent(j);
        double exp = m_hpred->GetBinContent(j);
        if(exp > 0.0)
        {
            // added when external fake datasets (you cannot reweight when simply 0)
            // this didn't happen when all from same MC since if exp=0 then obs =0

            chi2 += 2 * (exp - obs);
            if(obs > 0.0)
                chi2 += 2 * obs * TMath::Log(obs / exp);

            if(chi2 < 0.0)
            {
#ifndef NDEBUG
                std::cerr << "[WARNING]: In AnaSample::CalcChi2()\n"
                          << "[WARNING]: Stat chi2 is less than 0: " << chi2 << ", setting to 0."
                          << std::endl;
                std::cerr << "[WARNING]: exp and obs is: " << exp << " and " << obs << "."
                          << std::endl;
#endif
                chi2 = 0.0;
            }
        }
    }

    if(chi2 != chi2)
    {
        std::cerr << "[WARNING]: In AnaSample::CalcChi2()\n"
                  << "[WARNING]: Stat chi2 is nan, setting to 0." << std::endl;
        chi2 = 0.0;
    }

    return chi2;
}

void AnaSample::WriteEventHist(TDirectory* dirout, const std::string& bsname)
{
    dirout->cd();
    if(m_hpred != nullptr)
        m_hpred->Write(Form("evhist_sam%d_pred%s", m_sample_id, bsname.c_str()));
}

void AnaSample::WriteDataHist(TDirectory* dirout, const std::string& bsname)
{
    dirout->cd();
    if(m_hdata != nullptr)
        m_hdata->Write(Form("evhist_sam%d_data%s", m_sample_id, bsname.c_str()));
}