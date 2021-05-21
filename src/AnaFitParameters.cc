#include "AnaFitParameters.hh"

AnaFitParameters::AnaFitParameters(const std::string& par_name, const int pmttype)
    : m_name(par_name)
    , m_pmttype(pmttype)
    , Npar(0)
    , m_rng_start(false)
    , m_do_cap_weights(false)
    , m_weight_cap(1000)
    , m_info_frac(1.00)
{
    m_func = new Identity;
    std::cout<<"Setting up parameter "<<m_name<<std::endl;
}

AnaFitParameters::~AnaFitParameters()
{
}


bool AnaFitParameters::CheckDims(const std::vector<double>& params) const
{
    bool vector_size = false;

    if(params.size() == pars_prior.size())
    {
        vector_size = true;
    }

    else
    {
        std::cerr << "Dimension of parameter vector does not match priors.\n"
                  << "Prams size is: " << params.size() << std::endl
                  << "Prior size is: " << pars_prior.size() << std::endl;
        vector_size = false;
    }

    return vector_size;
}


void AnaFitParameters::SetParameterFunction(const std::string& func_name)
{
    if(m_func != nullptr)
        delete m_func;

    if(func_name.empty())
    {
        std::cout << "Parameter function name empty. Setting to identity by default." << std::endl;
        m_func = new Identity;
    }
    else if(func_name == "Identity")
    {
        std::cout << "Setting function to Identity." << std::endl;
        m_func = new Identity;
    }
    else if(func_name == "Attenuation")
    {
        std::cout << "Setting function to Attenuation." << std::endl;
        m_func = new Attenuation;
    }
}

void AnaFitParameters::InitParameters(std::vector<std::string> names, std::vector<double> priors, std::vector<double> steps, 
                                      std::vector<double> lows, std::vector<double> highs, std::vector<bool> fixed)
{
    SetParNames(names);
    SetParPriors(priors);
    SetParSteps(steps);
    SetParLimits(lows,highs);
    SetParFixed(fixed);

    Npar = pars_name.size();
    pars_original = pars_prior;

    std::cout<<"Number of parameters = "<<Npar<<std::endl;
}

void AnaFitParameters::InitEventMap(std::vector<AnaSample*> &sample)
{
    m_evmap.clear();

    for(std::size_t s=0; s < sample.size(); s++)
    {
        std::vector<int> sample_map;
        for(int i=0; i < sample[s] -> GetNPMTs(); i++)
        {
            AnaEvent* ev = sample[s] -> GetPMT(i);

            std::vector<double> binvar;
            for (auto t : m_binvar)
                binvar.push_back(ev->GetEventVar(t));

            const int bin = m_bm.GetBinIndex(binvar);

            if (m_pmttype == -1 || sample[s]->GetPMTType() == m_pmttype)
                sample_map.push_back(bin);
            else sample_map.push_back(PASSEVENT);
        }
        std::cout<<"In AnaFitParameters::InitEventMap, built event map for sample "<< sample[s]->GetName() << " of total "<< sample[s] -> GetNPMTs() << "PMTs"<<std::endl;
        m_evmap.push_back(sample_map);
    }
}

void AnaFitParameters::ReWeight(AnaEvent* event, int pmttype, int nsample, int nevent, std::vector<double>& params)
{
#ifndef NDEBUG
    if(m_evmap.empty()) //need to build an event map first
    {
        std::cerr  << "In AnaFitParameters::ReWeight()\n"
                   << "Need to build event map index for " << m_name << std::endl;
        return;
    }
#endif

    if (m_pmttype >=0 && pmttype != m_pmttype) return;

    const int bin = m_evmap[nsample][nevent];
    if(bin == PASSEVENT || bin == BADBIN)
        return;
    else
    {
#ifndef NDEBUG
        if(bin > params.size())
        {
            std::cout  << "In AnaFitParameters::ReWeight()\n"
                        << "Number of bins in " << m_name << " does not match num of parameters.\n"
                        << "Setting event weight to zero." << std::endl;
            event -> AddEvWght(0.0);
        }
#endif
        double wgt = (*m_func)(params[bin],*event);
        event -> AddEvWght(wgt);
    }
}