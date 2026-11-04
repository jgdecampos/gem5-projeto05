import sys
import os
import glob


def extrai_stats(caminho_stats):
    """Lê stats.txt e retorna dicionário com valores do detector."""
    dados = {}
    with open(caminho_stats) as f:
        for linha in f:
            linha = linha.strip()
            if not linha or linha.startswith("#"):
                continue
            partes = linha.split()
            if len(partes) < 2:
                continue
            chave = partes[0]
            valor = partes[1]
            if chave.startswith("system.detector"):
                dados[chave] = valor
    return dados


def extrai_serie(caminho_csv):
    """Lê o CSV da série temporal."""
    linhas = []
    with open(caminho_csv) as f:
        cabecalho = f.readline()
        for linha in f:
            linhas.append(linha.strip())
    return cabecalho, linhas


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Uso: extrai_p05.py <diretorio_de_resultados>")
        sys.exit(1)

    raiz = sys.argv[1]
    for stats in glob.glob(os.path.join(raiz, "**", "stats.txt"), recursive=True):
        dados = extrai_stats(stats)
        print(stats)
        for k, v in dados.items():
            print("  %s = %s" % (k, v))