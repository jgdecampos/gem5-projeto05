#ifndef __PROJETOS_P05_IMBALANCE_DETECTOR_HH__
#define __PROJETOS_P05_IMBALANCE_DETECTOR_HH__

#include <memory>
#include <vector>

#include "base/output.hh"
#include "base/statistics.hh"
#include "cpu/base.hh"
#include "params/ImbalanceDetector.hh"
#include "sim/eventq.hh"
#include "sim/probe/probe.hh"
#include "sim/sim_object.hh"

namespace gem5
{

class ImbalanceDetector : public SimObject
{
  public:
    PARAMS(ImbalanceDetector);
    ImbalanceDetector(const Params &p);
    ~ImbalanceDetector() override;

    void startup() override;
    void regProbeListeners() override;
    void resetStats() override;

    /** Chamados pelos ouvintes (etapa 3). */
    void onInsts(unsigned core, uint64_t n);
    void onSleep(unsigned core, bool sleeping);

  private:
    /* Estado de um núcleo pode ser identificado por:
       - instsInInterval: um acumulador das instruções que o núcleo
         completou a cada intervalo do tipo int;
       - activeInInterval: quantidade de tempo que o núcleo esteve
         ativo a cada intervalo do tipo Tick;
       - awake: identifica se o núcleo está executando (true) ou bloqueado/dormindo
	 naquele momento (false), representado por um tipo bool;
       - awakeSince: guarda o tick que o núcleo acordou pela última vez.
       */
    struct CoreState
    {
        uint64_t instsInInterval = 0;
        Tick activeInInterval = 0;
        bool awake = false;
        Tick awakeSince = 0;
    };

    const std::vector<BaseCPU *> cpus;
    const Tick period;
    const double cvThreshold;
    const unsigned minActive;
    std::vector<CoreState> cores;
    Tick intervalStart = 0; // início do intervalo corrente
    Tick roiStart = 0;      // tick do último reset
    uint64_t intervalIndex = 0;

    EventFunctionWrapper sampleEvent;
    void closeInterval(bool partial);
    void writeSeriesRow(unsigned core, bool partial, Tick ini, Tick fim,
                        double ipc, bool amostraAtiva);

    std::vector<ProbeListenerPtr<>> listeners;
    OutputStream *series = nullptr;

    struct DetectorStats : public statistics::Group
    {
        DetectorStats(ImbalanceDetector *parent, unsigned n);
        void preDumpStats() override;

        statistics::Vector insts;
        statistics::Vector activeTicks;
        statistics::Vector activeSamples;
        statistics::Scalar intervals;
        statistics::Vector occupancy;
        statistics::Vector ipcEff;
        statistics::Vector ipcActive;
        statistics::Scalar ipcMean, ipcMax, ipcMin, ipcCV;
        statistics::Scalar ipcActiveCV, instsCV, activeCV, imbalance;
        statistics::Distribution intervalCV;
        statistics::Scalar imbalancedIntervals;
    } stats;
};

} // namespace gem5

#endif // __PROJETOS_P05_IMBALANCE_DETECTOR_HH__
