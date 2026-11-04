import sys
import os
import pandas as pd


def carrega(caminho):
    s = pd.read_csv(caminho)
    return s


def cv_por_intervalo(s):
    """Agrupa por intervalo e calcula CV populacional (ddof=0) do ipc_ef entre CPUs."""
    resultado = {}
    for intervalo, grupo in s.groupby("intervalo"):
        valores = grupo["ipc_ef"].values
        if len(valores) == 0:
            continue
        media = valores.mean()
        if media == 0:
            cv = 0.0
        else:
            desvio = valores.std(ddof=0)
            cv = desvio / media
        resultado[intervalo] = cv
    return resultado


def confere_totais(s, stats):
    """Compara soma de insts por CPU no CSV com stats.txt."""
    erros = []
    for cpu in s["cpu"].unique():
        soma_csv = s[s["cpu"] == cpu]["insts"].sum()
        # stats seria um dicionário; aqui apenas imprime para conferência
        print("CPU %d: soma CSV insts = %d" % (cpu, soma_csv))
    return erros


def graficos(s, saida):
    """Gera gráficos básicos da série temporal."""
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("matplotlib não instalado, pulando gráficos")
        return

    os.makedirs(saida, exist_ok=True)

    # IPC médio por CPU ao longo dos intervalos
    pivot = s.pivot_table(index="intervalo", columns="cpu", values="ipc_ef")
    plt.figure()
    pivot.plot()
    plt.title("IPC efetivo por CPU ao longo dos intervalos")
    plt.xlabel("Intervalo")
    plt.ylabel("IPC")
    plt.savefig(os.path.join(saida, "ipc_por_cpu.png"))
    plt.close()

    # CV por intervalo
    cvs = cv_por_intervalo(s)
    if cvs:
        plt.figure()
        plt.plot(list(cvs.keys()), list(cvs.values()))
        plt.title("CV do IPC por intervalo")
        plt.xlabel("Intervalo")
        plt.ylabel("CV")
        plt.savefig(os.path.join(saida, "cv_por_intervalo.png"))
        plt.close()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: serie_p05.py <arquivo_csv>")
        sys.exit(1)

    s = carrega(sys.argv[1])
    print(s.head())
    print("Linhas: %d" % len(s))