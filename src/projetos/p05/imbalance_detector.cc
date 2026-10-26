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
    // TODO(aluno)
    return {};
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
    // TODO(aluno): validações e abertura do arquivo da série.
    // if (!p.series_file.empty()) series = simout.create(p.series_file);
    // TODO(aluno): escrever o cabeçalho do CSV.
    // Validacao
    fatal_if(cpus.empty(), "%s: A lista de CPUs observadas nao pode estar vazia", name());
    fatal_if(period == 0, "%s: O periodo de amostragem deve ser maior que zero", name());
}

ImbalanceDetector::~ImbalanceDetector() = default;

// TODO(aluno): fechar o arquivo com simout.close(series).


void
ImbalanceDetector::startup()
{
    intervalStart = roiStart = curTick();
    // TODO(aluno): estado inicial 'awake' de cada núcleo a partir de
    //              getContext(0)->status(); agendar o primeiro evento.
}

void
ImbalanceDetector::resetStats()
{
    SimObject::resetStats();
    // TODO(aluno): roiStart = intervalStart = curTick(); zerar os
    //              acumuladores; awakeSince = curTick() nos núcleos
    //              acordados; intervalIndex = 0;
    //              reschedule(sampleEvent, curTick() + period, true).
}

void
ImbalanceDetector::regProbeListeners()
{
    // TODO(aluno): para cada CPU i, guardar em listeners o resultado
    //              de cpus[i]->getProbeManager()->connect<L>(this, i)
    //              para L = InstListener e L = SleepListener.
}

void
ImbalanceDetector::onInsts(unsigned core, uint64_t n)
{
    // TODO(aluno): somar n ao acumulador de instruções do núcleo no intervalo.
}

void
ImbalanceDetector::onSleep(unsigned core, bool sleeping)
{
    CoreState &c = cores[core];
    // TODO(aluno): regra da figura de contabilidade. Uma notificação
    //              repetida (dormir já dormindo) não deve somar nada;
    //              conte-a numa estatística de diagnóstico.
}

void
ImbalanceDetector::closeInterval(bool partial)
{
    const Tick now = curTick();
    DPRINTF(Imbalance, "intervalo %llu [%llu, %llu)%s\n",
            intervalIndex, intervalStart, now, partial ? " parcial" : "");

    std::vector<double> ipc(cores.size());
    for (unsigned i = 0; i < cores.size(); i++) {
        CoreState &c = cores[i];
        // TODO(aluno): fechar o trecho acordado em andamento
        //              (figura de contabilidade).
        // TODO(aluno): somar c.instsInInterval e c.activeInInterval
        //              aos vetores; amostra instantânea de status().
        // TODO(aluno): ipc[i] = instruções / ciclos do intervalo,
        //              com os ciclos do relógio de cpus[i].
    }
    // TODO(aluno): se ao menos min_active núcleos têm instruções,
    //              amostrar o CV em intervalCV e comparar com
    //              cv_threshold; senão, não amostrar.
    // TODO(aluno): série temporal (etapa 5); zerar acumuladores.

    stats.intervals++;
    intervalIndex++;
    intervalStart = now;
    if (!partial /* TODO(aluno): e nem todos os contextos Halted */)
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
    // TODO(aluno): uma linha com as colunas do formato da Listagem 14.11:
    // intervalo,tick_ini,tick_fim,parcial,cpu,insts,ativos,amostra,ipc_ef
}

ImbalanceDetector::DetectorStats::DetectorStats(ImbalanceDetector *parent,
                                                unsigned n)
    : statistics::Group(parent), // Inicializacao das estatisticas
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
      // TODO(aluno): inicializar todas as estatísticas da Tabela 14.2 com ADD_STAT
{
    // TODO(aluno): dimensionar os vetores com init(n), nomear posições com
    //              subname(i, ...) e inicializar limites de intervalCV.
    // Dimensionamento dos vetores  
    insts.init(n);
    activeTicks.init(n);
    activeSamples.init(n);
    occupancy.init(n);
    ipcEff.init(n);
    ipcActive.init(n);

    // Nomenclatura das posições
    for (unsigned i = 0; i < n; i++) {
        std::string sub = csprintf("cpu%d", i);
        insts.subname(i, sub);
        activeTicks.subname(i, sub);
        activeSamples.subname(i, sub);
        occupancy.subname(i, sub);
        ipcEff.subname(i, sub);
        ipcActive.subname(i, sub);
    }

    // Limites da distribuição
    double max_cv = std::sqrt(n - 1.0);
    intervalCV.init(0.0, max_cv, 0.05);

}

void
ImbalanceDetector::DetectorStats::preDumpStats()
{
    statistics::Group::preDumpStats();
    // TODO(aluno): o detector fecha o intervalo parcial ANTES deste
    //              cálculo (cuidado com a ordem das chamadas).
    // TODO(aluno): occupancy, ipcEff e ipcActive por núcleo; resumos
    //              de ipcEff (média, máx, mín, CV), CV de ipcActive,
    //              de insts e de activeTicks; imbalance.
}

} // namespace gem5
