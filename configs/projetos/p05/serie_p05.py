import sys
import pandas as pd


def carrega(caminho):
    s = pd.read_csv(caminho)
    # TODO(aluno): descartar ou marcar os intervalos parciais.
    return s


def cv_por_intervalo(s):
    # TODO(aluno): agrupar por intervalo e calcular o CV populacional
    #              (ddof=0) de ipc_ef entre as CPUs.
    ...


def confere_totais(s, stats):
    # TODO(aluno): soma de insts por cpu == stats "insts::cpuN";
    #              soma de ativos por cpu == "activeTicks::cpuN".
    ...


def graficos(s, saida):
    # TODO(aluno): mapa de calor cpu x intervalo (ipc_ef);
    #              CV(k) no tempo; ocupação integrada x amostrada.
    ...
