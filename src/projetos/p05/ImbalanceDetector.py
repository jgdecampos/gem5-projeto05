from m5.params import *
from m5.SimObject import SimObject


class ImbalanceDetector(SimObject):
    type = "ImbalanceDetector"
    cxx_header = "projetos/p05/imbalance_detector.hh"
    cxx_class = "gem5::ImbalanceDetector"

    cpus = VectorParam.BaseCPU("CPUs observadas, na ordem dos contextos")
    period = Param.Latency("10us", "Período de amostragem")
    series_file = Param.String("", "CSV da série temporal; vazio desliga")
    # TODO(aluno): cv_threshold e min_active (tabela de parâmetros).
