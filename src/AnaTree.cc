#include "AnaTree.hh"

AnaTree::AnaTree(const std::string& file_name, const std::string& tree_name, const std::string& pmt_tree_name)
{
    fChain = new TChain(tree_name.c_str());
    fChain->Add(file_name.c_str());

    std::cout<<"Loading data files: "<<file_name.c_str()<<std::endl;

    std::string single_file_name = fChain->GetFile()->GetName();

    f_pmt = new TFile(single_file_name.c_str());
    t_pmt = (TTree*)f_pmt->Get(pmt_tree_name.c_str());

    std::cout<<"Loading PMT tree from : "<<f_pmt->GetName()<<std::endl;

    m_maskpmt = false;

}

AnaTree::~AnaTree()
{
    if(fChain != nullptr)
        delete fChain->GetCurrentFile();

}

void AnaTree::MaskPMT(int nPMT, bool mPMT, int nPMTpermPMT)
{
    pmt_mask.clear();
    m_maskpmt = false;

    if (nPMT<=0) {
        std::cout << "In AnaTree::MaskPMT(), nPMT = " << nPMT << " <=0\n"
                  << "No PMT is masked"<<std::endl;
    }

    int nPMT_total = t_pmt->GetEntries();
    if (mPMT) nPMT_total = nPMT_total/nPMTpermPMT;

    if (nPMT>=nPMT_total) {
        std::cout << "In AnaTree::MaskPMT(), nPMT = " << nPMT << " >= total nPMT = " << nPMT_total <<"\n"
                  << "No PMT is masked"<<std::endl;
    }

    std::cout<< "Masking "<< nPMT << " out of "<< nPMT_total << "PMTs" << std::endl;
    double PMT_frac = (nPMT+0.)/(nPMT_total);
    int PMT_count = 0;
    for (int i=0;i<nPMT_total;i++)
    {
        if ((PMT_count+0.)/(i+1.)<PMT_frac && PMT_count<nPMT) 
        {
            if (!mPMT) pmt_mask.emplace_back(0);
            else 
            {    
                for (int j=i*nPMTpermPMT;j<(i+1)*nPMTpermPMT;j++) {
                    pmt_mask.emplace_back(0);
                }
            }
            PMT_count++;
        } 
        else 
        {
            if (!mPMT) pmt_mask.emplace_back(1);
            else
            {
                for (int j=i*nPMTpermPMT;j<(i+1)*nPMTpermPMT;j++) {
                    pmt_mask.emplace_back(1);
                }
            }
        }
    }

    m_maskpmt = true;
}

long int AnaTree::GetEntry(long int entry) const
{
    // Read contents of entry.
    if(fChain == nullptr)
        return -1;
    else
        return fChain->GetEntry(entry);
}

void AnaTree::SetDataBranches(std::vector<std::string> binvar, std::vector<std::string> cutvar)
{    
    // Set branch addresses and branch pointers

    fChain->SetBranchAddress("nPE", &nPE);
    fChain->SetBranchAddress("R", &R);
    fChain->SetBranchAddress("costh", &costh);
    fChain->SetBranchAddress("cosths", &cosths);
    fChain->SetBranchAddress("omega", &omega);
    fChain->SetBranchAddress("timetof", &timetof);
    fChain->SetBranchAddress("PMT_id", &PMT_id);

    m_bins = binvar;
    m_cuts = cutvar;
}

void AnaTree::SetLeafs()
{
    leafs_bins.clear();
    leafs_cuts.clear();

    for (int i=0;i<m_bins.size();i++) {
        TLeaf *leaf = fChain->GetLeaf(m_bins[i].c_str());
        if (leaf == nullptr) {
            std::cout<<"[Error] AnaTree: Cannot find bin variable "<<m_bins[i]<<", skipping this variable"<<std::endl;
            continue;
        }
        leafs_bins.push_back(leaf);
    }

    for (int i=0;i<m_cuts.size();i++) {
        TLeaf *leaf = fChain->GetLeaf(m_cuts[i].c_str());
        if (leaf == nullptr) {
            std::cout<<"[Error] AnaTree: Cannot find cut variable "<<m_cuts[i]<<", skipping this variable"<<std::endl;
            continue;
        }
        leafs_cuts.push_back(leaf);
    }
}

void AnaTree::SetPMTBranches()
{
    // Set branch addresses and branch pointers

    t_pmt->SetBranchAddress("R", &R);
    t_pmt->SetBranchAddress("costh", &costh);
    t_pmt->SetBranchAddress("cosths", &cosths);
    t_pmt->SetBranchAddress("omega", &omega);
    t_pmt->SetBranchAddress("PMT_id", &PMT_id);
    
}

std::vector<AnaEvent> AnaTree::GetPMTs()
{
    std::vector<AnaEvent> pmt_vec;

    if(t_pmt == nullptr)
    {
        std::cout<<"[Error] Reading no PMTs in AnaTree::GetPMTs()"<<std::endl;
        return pmt_vec;
    }
    unsigned long nentries = fChain->GetEntries();

    SetPMTBranches();
    for(long int jentry = 0; jentry < t_pmt->GetEntries(); jentry++)
    {
        t_pmt->GetEntry(jentry);

        if (m_maskpmt) if (pmt_mask.at(PMT_id)) continue;

        AnaEvent ev(jentry);
        ev.SetR(R);
        ev.SetCosth(costh);
        ev.SetCosths(cosths);
        ev.SetOmega(omega); 
        ev.SetPMTID(PMT_id);
        
        std::vector<double> reco_var;
        reco_var.emplace_back(costh); reco_var.emplace_back(R);
        ev.SetRecoVar(reco_var);

        pmt_vec.push_back(ev);

    }

    return pmt_vec;
}

/*void AnaTree::GetData(std::vector<std::vector<double>>& data_vec, std::vector<std::vector<double>>& cut_vec, std::vector<double>& weight_vec)
{
    std::cout<<"Reading PMT hit data..."<<std::endl;
    if(fChain == nullptr)
    {
        std::cout<<"[Error] Reading no evenets in AnaTree::GetData()"<<std::endl;
        return;
    }
    unsigned long nentries = fChain->GetEntries();

    int currentTreeNumber = -1;
    for (unsigned long entry=0;entry<nentries;entry++) 
    {
        fChain->GetEntry(entry);

        if (currentTreeNumber != fChain->GetTreeNumber()) 
        {
            SetLeafs();
            currentTreeNumber = fChain->GetTreeNumber();
        }

        if (m_maskpmt) if (pmt_mask.at(PMT_id)) continue;

        std::vector<double> vec;
        for (int i=0;i<leafs_bins.size();i++)
        {
            vec.push_back(leafs_bins[i]->GetValue(0));
        }
        data_vec.emplace_back(vec);

        vec.clear();
        for (int i=0;i<leafs_cuts.size();i++)
        {
            vec.push_back(leafs_cuts[i]->GetValue(0));
        }
        cut_vec.emplace_back(vec);

        weight_vec.push_back(nPE);
    }
}*/

void AnaTree::GetData(std::vector<std::vector<double>>& data_vec, std::vector<std::vector<double>>& cut_vec, std::vector<double>& weight_vec)
{
    std::cout<<"Reading PMT hit data..."<<std::endl;
    if(fChain == nullptr)
    {
        std::cout<<"[Error] Reading no evenets in AnaTree::GetData()"<<std::endl;
        return;
    }
    unsigned long nentries = fChain->GetEntries();

    for (unsigned long entry=0;entry<nentries;entry++) 
    {
        fChain->GetEntry(entry);

        if (m_maskpmt) if (pmt_mask.at(PMT_id)) continue;

        std::vector<double> vec;
        for (int i=0;i<m_bins.size();i++)
        {
            vec.push_back(GetEventVar(m_bins[i]));
        }
        data_vec.emplace_back(vec);

        vec.clear();
        for (int i=0;i<m_cuts.size();i++)
        {
            vec.push_back(GetEventVar(m_cuts[i]));
        }
        cut_vec.emplace_back(vec);

        weight_vec.push_back(nPE);
    }
}

bool AnaTree::GetDataEntry(unsigned long entry, std::vector<double>& data_vec, std::vector<double>& cut_vec, double& weight)
{
    fChain->GetEntry(entry);

    if (m_maskpmt) if (pmt_mask.at(PMT_id)) return false;

    for (int i=0;i<m_bins.size();i++)
    {
        data_vec.push_back(GetEventVar(m_bins[i]));
    }

    for (int i=0;i<m_cuts.size();i++)
    {
        cut_vec.push_back(GetEventVar(m_cuts[i]));
    }

    weight = nPE;

    return true;
}