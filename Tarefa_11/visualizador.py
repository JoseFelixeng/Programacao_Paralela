#!/usr/bin/env python3
# -----------------------------------------------------------------------
# visualizar_simulacao_3d.py
# -----------------------------------------------------------------------
# Le os arquivos CSV (snapshot_u_passo_XXXXX.csv) gerados pelo programa
# navier_stokes_viscoso.c e produz uma visualizacao 3D em que:
#
#   - x e y sao as coordenadas espaciais da malha (fixas, formam o plano
#     horizontal do grafico);
#   - z e a velocidade u(x,y) naquele ponto (a altura da superficie),
#     que varia com o tempo conforme a perturbacao se difunde.
#
# Ou seja, cada snapshot vira uma superficie 3D tipo "montanha": a
# perturbacao aparece como um pico que, com o efeito da viscosidade,
# vai ficando mais baixo e mais largo ao longo do tempo.
#
# Saidas geradas (pasta visualizacao_saida_3d/):
#   1) Uma imagem PNG com a superficie 3D de cada snapshot.
#   2) Um painel comparativo com todas as superficies lado a lado.
#   3) Um GIF animado mostrando a superficie 3D evoluindo no tempo
#      (com leve rotacao de camera para ajudar a percepcao de profundidade).
#
# Uso:
#   1. Gere os snapshots com a simulacao em C:
#        gcc -O2 -o navier_stokes_viscoso navier_stokes_viscoso.c -lm
#        ./navier_stokes_viscoso
#   2. Rode este script na mesma pasta onde os .csv foram gerados:
#        python3 visualizar_simulacao_3d.py
#
# Dependencias: numpy, matplotlib (pip install numpy matplotlib pillow)
# -----------------------------------------------------------------------

import glob
import os
import re
import sys

import numpy as np
import matplotlib

matplotlib.use("Agg")  # backend sem interface grafica
import matplotlib.pyplot as plt
from matplotlib import animation
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401 (necessario para projecao 3D)

PADRAO_ARQUIVO = "saidap/snapshot_u_passo_*.csv"
PASTA_SAIDA = "visualizacao_saida_3d_P"


def encontra_snapshots(pasta="."):
    """Localiza e ordena (pelo numero do passo) todos os arquivos de snapshot."""
    arquivos = glob.glob(os.path.join(pasta, PADRAO_ARQUIVO))
    if not arquivos:
        sys.exit(
            "Nenhum arquivo 'snapshot_u_passo_*.csv' encontrado.\n"
            "Execute antes o programa navier_stokes_viscoso.c para gerar os snapshots."
        )

    def numero_do_passo(caminho):
        m = re.search(r"snapshot_u_passo_(\d+)\.csv", os.path.basename(caminho))
        return int(m.group(1)) if m else -1

    arquivos.sort(key=numero_do_passo)
    passos = [numero_do_passo(a) for a in arquivos]
    return arquivos, passos


def carrega_campos(arquivos):
    """Carrega cada CSV como uma matriz numpy 2D (linhas = y, colunas = x)."""
    return [np.loadtxt(a, delimiter=",") for a in arquivos]


def monta_grade(campo_exemplo):
    """Cria as coordenadas X, Y correspondentes as colunas/linhas do CSV."""
    ny, nx = campo_exemplo.shape
    x = np.linspace(0.0, 1.0, nx)
    y = np.linspace(0.0, 1.0, ny)
    X, Y = np.meshgrid(x, y)
    return X, Y


def desenha_superficie(ax, X, Y, Z, zmin, zmax, titulo, escala_automatica=False):
    """Desenha uma superficie 3D z = u(x,y) em um eixo 3D ja existente.

    Se escala_automatica=True, o eixo z (e a faixa de cores) se ajustam ao
    minimo/maximo do PROPRIO quadro, em vez de usar uma faixa global fixa.
    Isso deixa a forma da superficie (o "morro" se achatando e alargando)
    sempre visivel, mesmo quando a amplitude absoluta ja caiu muito.
    """
    if escala_automatica:
        zmin_local, zmax_local = float(Z.min()), float(Z.max())
        # evita zlim degenerado (min == max) quando o campo esta praticamente uniforme
        if zmax_local - zmin_local < 1e-9:
            zmax_local = zmin_local + 1e-9
    else:
        zmin_local, zmax_local = zmin, zmax

    ax.plot_surface(
        X, Y, Z,
        cmap="viridis",
        vmin=zmin_local,
        vmax=zmax_local,
        linewidth=0,
        antialiased=True,
        rstride=1,
        cstride=1,
    )
    ax.set_zlim(zmin_local, zmax_local)
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_zlabel("velocidade u")
    ax.set_title(titulo)


def gera_imagens_individuais(campos, passos, X, Y, zmin, zmax, pasta_saida):
    """Salva um PNG com a superficie 3D de cada snapshot."""
    for campo, passo in zip(campos, passos):
        fig = plt.figure(figsize=(6, 5))
        ax = fig.add_subplot(111, projection="3d")
        desenha_superficie(ax, X, Y, campo, zmin, zmax, f"Superficie u(x,y) — passo {passo}")
        ax.view_init(elev=35, azim=-60)
        fig.tight_layout()

        caminho_saida = os.path.join(pasta_saida, f"superficie_passo_{passo:05d}.png")
        fig.savefig(caminho_saida, dpi=120)
        plt.close(fig)
        print(f"  -> imagem salva: {caminho_saida}")


def gera_painel_comparativo(campos, passos, X, Y, zmin, zmax, pasta_saida, escala_automatica=False):
    """Cria uma unica figura com todas as superficies 3D lado a lado."""
    n = len(campos)
    n_colunas = min(n, 3)
    n_linhas = int(np.ceil(n / n_colunas))

    fig = plt.figure(figsize=(5 * n_colunas, 4.3 * n_linhas))

    for k, (campo, passo) in enumerate(zip(campos, passos)):
        ax = fig.add_subplot(n_linhas, n_colunas, k + 1, projection="3d")
        desenha_superficie(ax, X, Y, campo, zmin, zmax, f"passo {passo}", escala_automatica)
        ax.view_init(elev=35, azim=-60)

    sufixo_titulo = "(escala de z automatica por quadro)" if escala_automatica else "(escala de z fixa)"
    fig.suptitle(f"Evolucao da superficie de velocidade u(x,y,t) — efeito da viscosidade {sufixo_titulo}")
    fig.tight_layout()

    nome_arquivo = "painel_comparativo_3d_auto.png" if escala_automatica else "painel_comparativo_3d.png"
    caminho_saida = os.path.join(pasta_saida, nome_arquivo)
    fig.savefig(caminho_saida, dpi=130)
    plt.close(fig)
    print(f"  -> painel comparativo salvo: {caminho_saida}")


def gera_animacao_gif(campos, passos, X, Y, zmin, zmax, pasta_saida, escala_automatica=False):
    """Cria um GIF animado com a superficie 3D evoluindo no tempo, com leve rotacao de camera."""
    fig = plt.figure(figsize=(6.5, 5.5))
    ax = fig.add_subplot(111, projection="3d")

    def desenha_quadro(indice):
        ax.clear()
        desenha_superficie(
            ax, X, Y, campos[indice], zmin, zmax,
            f"Superficie u(x,y) — passo {passos[indice]}",
            escala_automatica,
        )
        # leve rotacao da camera a cada quadro, para reforcar a percepcao 3D
        angulo = -60 + 40 * (indice / max(1, len(campos) - 1))
        ax.view_init(elev=35, azim=angulo)
        return ()

    anim = animation.FuncAnimation(
        fig, desenha_quadro, frames=len(campos), interval=700, blit=False
    )

    nome_arquivo = "superficie_animada_3d_auto.gif" if escala_automatica else "superficie_animada_3d.gif"
    caminho_saida = os.path.join(pasta_saida, nome_arquivo)
    anim.save(caminho_saida, writer="pillow", fps=1.5)
    plt.close(fig)
    print(f"  -> animacao 3D salva: {caminho_saida}")


def gera_grafico_decaimento(campos, passos, pasta_saida):
    """Gera um grafico 2D simples (linha) mostrando como o pico de velocidade
    (amplitude maxima da perturbacao) cai ao longo do tempo. Complementa as
    superficies 3D com escala automatica, que mostram a FORMA mas escondem
    a amplitude absoluta."""
    picos = [float(c.max()) for c in campos]

    fig, ax = plt.subplots(figsize=(6, 4))
    ax.plot(passos, picos, marker="o", color="tab:purple")
    ax.set_xlabel("passo de tempo")
    ax.set_ylabel("velocidade maxima (pico)")
    ax.set_title("Decaimento da amplitude da perturbacao ao longo do tempo")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()

    caminho_saida = os.path.join(pasta_saida, "decaimento_amplitude.png")
    fig.savefig(caminho_saida, dpi=130)
    plt.close(fig)
    print(f"  -> grafico de decaimento salvo: {caminho_saida}")


def main():
    os.makedirs(PASTA_SAIDA, exist_ok=True)

    print("Procurando snapshots da simulacao...")
    arquivos, passos = encontra_snapshots(".")
    print(f"Encontrados {len(arquivos)} snapshots: passos {passos}")

    print("Carregando campos...")
    campos = carrega_campos(arquivos)
    X, Y = monta_grade(campos[0])

    # z (velocidade) varia; x e y permanecem como a grade espacial fixa.
    # Escala comum de z para todos os quadros, para que a reducao da
    # amplitude ao longo do tempo fique visivel na comparacao.
    zmin = min(c.min() for c in campos)
    zmax = max(c.max() for c in campos)

    print("\nGerando imagens 3D individuais (uma por snapshot)...")
    gera_imagens_individuais(campos, passos, X, Y, zmin, zmax, PASTA_SAIDA)

    print("\nGerando painel comparativo 3D (escala de z fixa)...")
    gera_painel_comparativo(campos, passos, X, Y, zmin, zmax, PASTA_SAIDA, escala_automatica=False)

    print("\nGerando painel comparativo 3D (escala de z automatica por quadro)...")
    gera_painel_comparativo(campos, passos, X, Y, zmin, zmax, PASTA_SAIDA, escala_automatica=True)

    print("\nGerando animacao 3D (GIF, escala automatica)...")
    gera_animacao_gif(campos, passos, X, Y, zmin, zmax, PASTA_SAIDA, escala_automatica=True)

    print("\nGerando grafico de decaimento da amplitude...")
    gera_grafico_decaimento(campos, passos, PASTA_SAIDA)

    print(f"\nConcluido! Todos os arquivos foram salvos em: {PASTA_SAIDA}/")


if __name__ == "__main__":
    main()