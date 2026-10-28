#ifndef __PROJETOS_P05_PROBE_LISTENERS_HH__
#define __PROJETOS_P05_PROBE_LISTENERS_HH__

#include "sim/probe/probe.hh"
#include "projetos/p05/imbalance_detector.hh"

namespace gem5
{

class InstListener : public ProbeListenerArgBase<uint64_t>
{
  public:
    InstListener(ImbalanceDetector *d, unsigned core)
        : ProbeListenerArgBase("RetiredInsts"), det(d), core(core) {}

    void notify(const uint64_t &n) override { det->onInsts(core, n); }

  private:
    ImbalanceDetector *det;
    unsigned core;
};

class SleepListener : public ProbeListenerArgBase<bool>
{
  public:
    SleepListener(ImbalanceDetector *d, unsigned core)
        : ProbeListenerArgBase("Sleeping"), det(d), core(core) {}

    void notify(const bool &sleeping) override { det->onSleep(core, sleeping); }

  private:
    ImbalanceDetector *det;
    unsigned core;
};

} // namespace gem5

#endif // __PROJETOS_P05_PROBE_LISTENERS_HH__