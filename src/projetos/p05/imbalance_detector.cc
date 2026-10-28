#include "projetos/p05/imbalance_detector.hh"

#include "base/logging.hh"
#include "base/trace.hh"
#include "debug/Imbalance.hh"
#include "projetos/p05/probe_listeners.hh"

namespace gem5
{

// Função auxiliar: média, desvio padrão populacional e CV.
struct Resumo { double media, max, min, cv; unsigned n; };
Resumo resume(const std::vector<double> &x)
{
    Resumo r{0, 0, 0, 0, 0};
    if (x.empty()) return r;

    r.n = x.size();
    double soma = 0;
    for (double v : x) soma += v;
    r.media = soma / r.n;

    r.max = x[0];
    r.min = x[0];
    for (double v : x) {
        if (v > r.max) r.max = v;
        if (v < r.min) r.min = v;
    }

    // Desvio padrão POPULACIONAL (divisor P, não P-1)
    double var = 0;
    for (double v : x) var += (v - r.media) * (v - r.media);
    var /= r.n;
    double sigma = std::sqrt(var);

    r.cv = (r.media != 0) ? sigma / r.media : 0;
    return r;
}


ImbalanceDetector::ImbalanceDetector(const Params &p)
    : SimObject(p),
      cpus(p.cpus.begin(), p.cpus.end()),
      period(p.period),
      cvThreshold(p.cv_threshold),
      minActive(p.min_active),
      cores(p.cpus.size()),
      sampleEvent([this] { closeInterval(false); },
                  name() + ".sampleEvent"),
      stats(this, p.cpus.size())
{
    // Validações
    fatal_if(cpus.empty(), "%s: A lista de CPUs observadas nao pode estar vazia", name());
    fatal_if(period == 0, "%s: O periodo de amostragem deve ser maior que zero", name());
}

ImbalanceDetector::~ImbalanceDetector() = default;


void
ImbalanceDetector::startup()
{
    intervalStart = roiStart = curTick();

    // Estado inicial 'awake' de cada núcleo
    for (unsigned i = 0; i < cpus.size(); i++) {
        ThreadContext *tc = cpus[i]->getContext(0);
        cores[i].awake = (tc->status() == ThreadContext::Active);
        if (cores[i].awake)
            cores[i].awakeSince = curTick();
    }

    // Agenda o primeiro evento para daqui a um período
    schedule(sampleEvent, curTick() + period);
}

void
ImbalanceDetector::resetStats()
{
    SimObject::resetStats();

    // O programa chamou m5_reset_stats(): reinicia a região de interesse
    roiStart = intervalStart = curTick();
    intervalIndex = 0;

    // Zera os acumuladores de cada núcleo
    for (unsigned i = 0; i < cores.size(); i++) {
        cores[i].instsInInterval = 0;
        cores[i].activeInInterval = 0;
        if (cores[i].awake)
            cores[i].awakeSince = curTick();
    }

    // Reagenda o evento para exatamente um período depois do reset
    reschedule(sampleEvent, curTick() + period, true);
}

void
ImbalanceDetector::regProbeListeners()
{
    // Etapa 3 - será implementado depois
    for (unsigned i = 0; i < cpus.size(); i++) {
        listeners.push_back(cpus[i]->getProbeManager()->connect<InstListener>(this, i));
        listeners.push_back(cpus[i]->getProbeManager()->connect<SleepListener>(this, i));
    }
}

void
ImbalanceDetector::onInsts(unsigned core, uint64_t n)
{
    // Etapa 3 - será implementado depois
    cores[core].instsInInterval += n;
}

void
ImbalanceDetector::onSleep(unsigned core, bool sleeping)
{
    CoreState &c = cores[core];
    // Etapa 3 - será implementado depois
    const Tick now = curTick();
    if (sleeping && c.awake) {
        c.activeInInterval += now - c.awakeSince;
        c.awake = false;
    } else if (!sleeping && !c.awake) {
        c.awake = true;
        c.awakeSince = now;
    }
}

void
ImbalanceDetector::closeInterval(bool partial)
{
    const Tick now = curTick();
    DPRINTF(Imbalance, "intervalo %llu [%llu, %llu)%s\n",
            intervalIndex, intervalStart, now, partial ? " parcial" : "");

    // Fecha o treço acordado em andamento (contagem de ticks ativos)
    std::vector<double> ipc(cores.size());
    for (unsigned i = 0; i < cores.size(); i++) {
        CoreState &c = cores[i];

        // Se o núcleo está acordado no fim do intervalo,
        // soma o tempo desde awakeSince até agora
        if (c.awake) {
            c.activeInInterval += now - c.awakeSince;
            c.awakeSince = now; // reinicia para o próximo intervalo
        }

        // Ticks totais do intervalo para este núcleo
        Tick ticks_intervalo = now - intervalStart;
        unsigned long long ciclos = ticks_intervalo / cpus[i]->clockPeriod();

        // Acumula nos vetores globais
        stats.insts[i] += c.instsInInterval;
        stats.activeTicks[i] += c.activeInInterval;

        // Amostra instantânea de ocupação
        if (c.awake)
            stats.activeSamples[i]++;

        // IPC efetivo do intervalo (instruções / ciclos do intervalo)
        ipc[i] = (ciclos > 0) ? (double)c.instsInInterval / ciclos : 0;

        // Zera acumuladores para o próximo intervalo
        c.instsInInterval = 0;
        c.activeInInterval = 0;
    }

    // CV do intervalo (se min_active núcleos têm instruções)
    unsigned n_ativos = 0;
    for (unsigned i = 0; i < cores.size(); i++) {
        if (stats.insts[i] > 0) n_ativos++;
    }
    if (n_ativos >= minActive) {
        double cv_intervalo = resume(ipc).cv;
        stats.intervalCV.sample(cv_intervalo);
        if (cv_intervalo > cvThreshold)
            stats.imbalancedIntervals++;
    }

    stats.intervals++;
    intervalIndex++;
    intervalStart = now;

    // Verifica se todos os contextos estão Halted
    bool todosHalted = true;
    for (unsigned i = 0; i < cpus.size(); i++) {
        ThreadContext *tc = cpus[i]->getContext(0);
        if (tc->status() != ThreadContext::Halted) {
            todosHalted = false;
            break;
        }
    }

    if (!partial && !todosHalted)
        schedule(sampleEvent, now + period);
}

void
ImbalanceDetector::writeSeriesRow(unsigned core, bool partial,
                                  Tick ini, Tick fim, double ipc,
                                  bool amostraAtiva)
{
    if (!series)
        return;
    std::ostream &os = *series->stream();
    os << core << "," << ini << "," << fim << "," << (partial ? 1 : 0)
       << "," << (amostraAtiva ? 1 : 0) << "," << ipc << "\n";
}


ImbalanceDetector::DetectorStats::DetectorStats(ImbalanceDetector *parent,
                                                unsigned n)
    : statistics::Group(parent),
      parent(parent)
      ADD_STAT(insts, statistics::units::Count::get(), "Instrucoes completadas na regiao, por nucleo"),
      ADD_STAT(activeTicks, statistics::units::Tick::get(), "Ticks com a CPU acordada na regiao, por nucleo"),
      ADD_STAT(activeSamples, statistics::units::Count::get(), "Amostras instantaneas em que o contexto estava ativo"),
      ADD_STAT(intervals, statistics::units::Count::get(), "Intervalos fechados (completos e parciais)"),
      ADD_STAT(occupancy, statistics::units::Ratio::get(), "activeTicks / ticks da regiao, por nucleo"),
      ADD_STAT(ipcEff, statistics::units::Rate<statistics::units::Count, statistics::units::Cycle>::get(), "IPC efetivo da regiao, por nucleo"),
      ADD_STAT(ipcActive, statistics::units::Rate<statistics::units::Count, statistics::units::Cycle>::get(), "IPC ativo da regiao, por nucleo"),
      ADD_STAT(ipcMean, statistics::units::Rate<statistics::units::Count, statistics::units::Cycle>::get(), "Media do IPC efetivo"),
      ADD_STAT(ipcMax, statistics::units::Rate<statistics::units::Count, statistics::units::Cycle>::get(), "Maximo do IPC efetivo"),
      ADD_STAT(ipcMin, statistics::units::Rate<statistics::units::Count, statistics::units::Cycle>::get(), "Minimo do IPC efetivo"),
      ADD_STAT(ipcCV, statistics::units::Ratio::get(), "CV do IPC efetivo entre nucleos"),
      ADD_STAT(ipcActiveCV, statistics::units::Ratio::get(), "CV do IPC ativo entre nucleos"),
      ADD_STAT(instsCV, statistics::units::Ratio::get(), "CV das instrucoes entre nucleos"),
      ADD_STAT(activeCV, statistics::units::Ratio::get(), "CV dos ticks ativos entre nucleos"),
      ADD_STAT(imbalance, statistics::units::Ratio::get(), "Maximo / Media de activeTicks"),
      ADD_STAT(intervalCV, statistics::units::Ratio::get(), "Distribuicao do CV de IPC efetivo por intervalo"),
      ADD_STAT(imbalancedIntervals, statistics::units::Count::get(), "Intervalos com CV acima de cv_threshold")
{
    insts.init(n);
    activeTicks.init(n);
    activeSamples.init(n);
    occupancy.init(n);
    ipcEff.init(n);
    ipcActive.init(n);

    for (unsigned i = 0; i < n; i++) {
        std::string sub = csprintf("cpu%d", i);
        insts.subname(i, sub);
        activeTicks.subname(i, sub);
        activeSamples.subname(i, sub);
        occupancy.subname(i, sub);
        ipcEff.subname(i, sub);
        ipcActive.subname(i, sub);
    }

    double max_cv = std::sqrt(n - 1.0);
    intervalCV.init(0.0, max_cv, 0.05);
}

void
ImbalanceDetector::DetectorStats::preDumpStats()
{
    statistics::Group::preDumpStats();

    unsigned n = parent->cpus.size();

    // occupancy, ipcEff e ipcActive por núcleo
    Tick ticks_roi = curTick() - parent->roiStart;
    for (unsigned i = 0; i < n; i++) {
        double oticks = ticks_roi / cpus[i]->clockPeriod();
        occupancy[i] = (oticks > 0) ? (double)activeTicks[i] / oticks : 0;
        ipcEff[i] = (oticks > 0) ? (double)insts[i] / oticks : 0;
        ipcActive[i] = (activeTicks[i] > 0) ? (double)insts[i] / (activeTicks[i] / cpus[i]->clockPeriod()) : 0;
    }

    // Resumos de ipcEff
    std::vector<double> v_ipcEff(n);
    for (unsigned i = 0; i < n; i++) v_ipcEff[i] = ipcEff[i];
    Resumo r = resume(v_ipcEff);
    ipcMean = r.media;
    ipcMax = r.max;
    ipcMin = r.min;
    ipcCV = r.cv;

    // CV de ipcActive
    std::vector<double> v_ipcActive(n);
    for (unsigned i = 0; i < n; i++) v_ipcActive[i] = ipcActive[i];
    ipcActiveCV = resume(v_ipcActive).cv;

    // CV de insts e activeTicks
    std::vector<double> v_insts(n), v_act(n);
    for (unsigned i = 0; i < n; i++) {
        v_insts[i] = insts[i];
        v_act[i] = activeTicks[i];
    }
    instsCV = resume(v_insts).cv;
    activeCV = resume(v_act).cv;

    // imbalance = max / media de activeTicks
    double max_at = 0, soma_at = 0;
    for (unsigned i = 0; i < n; i++) {
        if (v_act[i] > max_at) max_at = v_act[i];
        soma_at += v_act[i];
    }
    double media_at = soma_at / n;
    imbalance = (media_at > 0) ? max_at / media_at : 0;
}

} // namespace gem5