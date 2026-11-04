import subprocess
import os
import itertools

MODOS = ["uniforme", "desbalanceado", "corrigido"]
NUCLEOS = [2, 4, 8]
BARREIRAS = {"futex": "bin/triang", "ativa": "bin/triang_ativa"}
PERIODOS = ["100ns", "300ns", "1us", "10us", "100us"]
SEMENTES = [1, 2, 3]

GEM5 = "./build/X86/gem5.opt"
SISTEMA = "configs/projetos/p05/sistema_p05.py"
SAIDA = "resultados"


def combinacoes():
    combs = []
    # Parte A: 3 modos x nucleos x barreiras x sementes
    for modo in MODOS:
        for n in NUCLEOS:
            for b in BARREIRAS:
                for s in SEMENTES:
                    combs.append({
                        "parte": "A",
                        "modo": modo,
                        "cores": n,
                        "barreira": b,
                        "seed": s,
                        "period": "10us",
                        "detector": True,
                    })
    # Parte B: sensibilidade ao período (apenas desbalanceado, 4 núcleos)
    for p in PERIODOS:
        combs.append({
            "parte": "B",
            "modo": "desbalanceado",
            "cores": 4,
            "barreira": "futex",
            "seed": 1,
            "period": p,
            "detector": True,
        })
    # Parte C: custo com/sem detector (4 núcleos, desbalanceado)
    combs.append({
        "parte": "C",
        "modo": "desbalanceado",
        "cores": 4,
        "barreira": "futex",
        "seed": 1,
        "period": "10us",
        "detector": True,
    })
    combs.append({
        "parte": "C",
        "modo": "desbalanceado",
        "cores": 4,
        "barreira": "futex",
        "seed": 1,
        "period": "10us",
        "detector": False,
    })
    return combs


def comando(comb):
    modo_num = {"uniforme": 0, "desbalanceado": 1, "corrigido": 2}[comb["modo"]]
    bar_num = 0 if comb["barreira"] == "futex" else 1
    prog = BARREIRAS[comb["barreira"]]

    direcao = os.path.join(SAIDA, comb["parte"],
                           comb["modo"], "c%d" % comb["cores"],
                           comb["barreira"], "p%s" % comb["period"],
                           "s%d" % comb["seed"])
    os.makedirs(direcao, exist_ok=True)

    cmd = [
        GEM5,
        "-d", direcao,
        SISTEMA,
        "--cores", str(comb["cores"]),
        "--period", comb["period"],
        "--series", os.path.join(direcao, "serie.csv"),
    ]
    if not comb["detector"]:
        cmd.append("--no-detector")
    if comb["parte"] == "C":
        cmd += ["--max-ticks", "1000000000"]
    cmd += ["--", prog, str(comb["cores"]), "10000", "1", str(modo_num)]

    return cmd, direcao


if __name__ == "__main__":
    combs = combinacoes()
    print("Total de combinacoes: %d" % len(combs))
    for c in combs[:5]:
        cmd, d = comando(c)
        print(" ".join(cmd))