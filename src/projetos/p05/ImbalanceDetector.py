from m5.params import *
from m5.SimObject import SimObject


class ImbalanceDetector(SimObject):
    type = "ImbalanceDetector"
    cxx_header = "projetos/p05/imbalance_detector.hh"
    cxx_class = "gem5::ImbalanceDetector"

    cpus = VectorParam.BaseCPU("CPUs observadas, na ordem dos contextos")
    period = Param.Latency("10us", "Período de amostragem")
    series_file = Param.String("", "CSV da série temporal; vazio desliga")
    cv_threshold = Param.Float(0.2, "CV acima do qual um intervalo conta como desequilibrado")
    min_active = Param.Unsigned(2, "Mínimo de nucleos com instruções para calcular CV")
