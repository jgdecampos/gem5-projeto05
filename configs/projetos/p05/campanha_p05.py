MODOS = ["uniforme", "desbalanceado", "corrigido"]
NUCLEOS = [2, 4, 8]
BARREIRAS = {"futex": "bin/triang", "ativa": "bin/triang_ativa"}
CENARIOS = {"estavel": "...", "alternado": "...", "fases": "..."}
PERIODOS = ["100ns", "300ns", "1us", "10us", "100us"]
SEMENTES = [1, 2, 3]


def combinacoes():
    # TODO(aluno): parte A (produto cartesiano + O3 em 4 núcleos),
    #              parte B e parte C, cada combinação com o campo
    #              "parte" para compor o diretório.
    ...


def comando(comb):
    # TODO(aluno): gem5.opt -d <dir> sistema_p05.py --cores ...
    #              --period ... [--no-detector] -- <programa> <args>
    ...
