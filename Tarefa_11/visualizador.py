#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
============================================================================
Visualizador interativo — Difusão viscosa (Navier-Stokes sem advecção)
============================================================================

Este script complementa o programa em C `navier_stokes_viscosidade.c`.
Ele tem DOIS modos de uso:

  1) MODO INTERATIVO (padrão)
     Reimplementa a mesma física do C (laplaciano de 5 pontos, malha
     periódica, Euler explícito) usando NumPy vetorizado, para rodar em
     tempo real dentro de uma janela matplotlib. Você pode:
       - Clicar com o botão esquerdo do mouse sobre o campo para "cutucar"
         o fluido: injeta uma nova perturbação gaussiana exatamente onde
         você clicou, com a amplitude/largura escolhidas nos sliders.
       - Ajustar viscosidade (nu), amplitude, largura (sigma) e velocidade
         de reprodução em tempo real, mesmo com a simulação rodando.
       - Play / Pause / Reset (parado ou velocidade constante).
       - Acompanhar max|V| e energia cinética evoluindo em um gráfico ao
         lado, exatamente como o C imprime no terminal.

  2) MODO SNAPSHOTS (--modo snapshots)
     Lê os arquivos CSV que o programa em C efetivamente gera
     (saida/snapshot_XXXX.csv) e permite passear por eles com um slider
     de tempo, sem precisar re-simular nada — é a forma de "ver de verdade"
     os dados que o C produziu.

Uso:
    python3 visualizador_interativo.py                     # modo interativo
    python3 visualizador_interativo.py --modo snapshots \
        --pasta /caminho/para/saida                         # modo snapshots

Dependências: numpy, matplotlib (pip install numpy matplotlib)
============================================================================
"""

import argparse
import glob
import os
import sys

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider, Button, RadioButtons

# ---------------------------------------------------------------------------
# Parâmetros padrão (espelham as macros do programa em C)
# ---------------------------------------------------------------------------
NX_PADRAO, NY_PADRAO = 100, 100
DX, DY = 1.0, 1.0
NU_PADRAO = 0.1
FATOR_CFL = 0.5
PASSOS_POR_QUADRO_PADRAO = 4  # quantos passos de física por frame de animação


def dt_estavel(nu, dx=DX, dy=DY, fator_cfl=FATOR_CFL):
    """Mesmo critério de estabilidade usado no C: nu*dt*(1/dx^2+1/dy^2) <= 0.5."""
    limite = 0.5 / (nu * (1.0 / dx ** 2 + 1.0 / dy ** 2))
    return fator_cfl * limite


class CampoFluido:
    """Campo de velocidade (u, v) em malha periódica, com difusão viscosa
    vetorizada via np.roll (equivalente ao laplaciano de 5 pontos do C)."""

    def __init__(self, nx=NX_PADRAO, ny=NY_PADRAO, nu=NU_PADRAO):
        self.nx, self.ny = nx, ny
        self.nu = nu
        self.u = np.zeros((nx, ny))
        self.v = np.zeros((nx, ny))
        self.t = 0.0
        self.historico_t = []
        self.historico_vmax = []
        self.historico_energia = []

    def inicializar_parado(self):
        self.u[:] = 0.0
        self.v[:] = 0.0
        self._reset_historico()

    def inicializar_constante(self, u0=1.0, v0=0.5):
        self.u[:] = u0
        self.v[:] = v0
        self._reset_historico()

    def _reset_historico(self):
        self.t = 0.0
        self.historico_t.clear()
        self.historico_vmax.clear()
        self.historico_energia.clear()

    def adicionar_perturbacao(self, cx, cy, amplitude, largura):
        """Injeta uma gaussiana centrada em (cx, cy) — usado tanto na
        inicialização quanto nos cliques do mouse ("mexer nas partículas")."""
        i = np.arange(self.nx).reshape(-1, 1)
        j = np.arange(self.ny).reshape(1, -1)
        r2 = (i - cx) ** 2 + (j - cy) ** 2
        self.u += amplitude * np.exp(-r2 / (2.0 * largura ** 2))

    def laplaciano(self, campo):
        """Laplaciano 2D com contorno periódico (idêntico ao idx_periodico do C)."""
        return (
            (np.roll(campo, -1, axis=0) - 2 * campo + np.roll(campo, 1, axis=0)) / DX ** 2
            + (np.roll(campo, -1, axis=1) - 2 * campo + np.roll(campo, 1, axis=1)) / DY ** 2
        )

    def passo(self, dt):
        """Um passo de Euler explícito — mesma fórmula de passo_difusao() no C."""
        u_novo = self.u + self.nu * dt * self.laplaciano(self.u)
        v_novo = self.v + self.nu * dt * self.laplaciano(self.v)
        self.u, self.v = u_novo, v_novo
        self.t += dt

    def velocidade_maxima(self):
        return float(np.sqrt(self.u ** 2 + self.v ** 2).max())

    def energia_cinetica(self):
        return float(0.5 * np.sum(self.u ** 2 + self.v ** 2))

    def registrar_estatisticas(self):
        self.historico_t.append(self.t)
        self.historico_vmax.append(self.velocidade_maxima())
        self.historico_energia.append(self.energia_cinetica())


# ---------------------------------------------------------------------------
# MODO 1: interface interativa (sliders + cliques do mouse)
# ---------------------------------------------------------------------------
def rodar_modo_interativo():
    campo = CampoFluido()
    campo.inicializar_parado()
    campo.adicionar_perturbacao(campo.nx // 2, campo.ny // 2, amplitude=5.0, largura=3.0)
    campo.registrar_estatisticas()

    estado = {"rodando": False, "passos_por_quadro": PASSOS_POR_QUADRO_PADRAO}

    fig = plt.figure(figsize=(11, 6))
    fig.suptitle("Difusão viscosa interativa — clique no campo para perturbar o fluido",
                 fontsize=11)

    ax_campo = fig.add_axes([0.06, 0.32, 0.55, 0.58])
    imagem = ax_campo.imshow(campo.u.T, origin="lower", cmap="RdBu_r",
                              vmin=-5, vmax=5, animated=True)
    ax_campo.set_title("Campo u(x, y)")
    ax_campo.set_xlabel("x")
    ax_campo.set_ylabel("y")
    barra_cor = fig.colorbar(imagem, ax=ax_campo, fraction=0.046, pad=0.04)
    barra_cor.set_label("u")

    ax_stats = fig.add_axes([0.68, 0.55, 0.28, 0.35])
    linha_vmax, = ax_stats.plot([], [], color="crimson", label="max|V|")
    ax_stats.set_xlabel("t")
    ax_stats.set_ylabel("max|V|", color="crimson")
    ax_stats.tick_params(axis="y", labelcolor="crimson")

    ax_energia = ax_stats.twinx()
    linha_energia, = ax_energia.plot([], [], color="steelblue", label="energia")
    ax_energia.set_ylabel("energia cinética", color="steelblue")
    ax_energia.tick_params(axis="y", labelcolor="steelblue")
    ax_stats.set_title("Evolução no tempo", fontsize=10)

    # --- Sliders -----------------------------------------------------------
    ax_nu = fig.add_axes([0.68, 0.42, 0.28, 0.03])
    slider_nu = Slider(ax_nu, "viscosidade ν", 0.001, 0.3, valinit=campo.nu)

    ax_amp = fig.add_axes([0.68, 0.36, 0.28, 0.03])
    slider_amp = Slider(ax_amp, "amplitude", 0.5, 10.0, valinit=5.0)

    ax_largura = fig.add_axes([0.68, 0.30, 0.28, 0.03])
    slider_largura = Slider(ax_largura, "largura σ", 1.0, 15.0, valinit=3.0)

    ax_velocidade = fig.add_axes([0.68, 0.24, 0.28, 0.03])
    slider_velocidade = Slider(ax_velocidade, "passos/quadro", 1, 20,
                                valinit=PASSOS_POR_QUADRO_PADRAO, valstep=1)

    def ao_mudar_nu(val):
        campo.nu = val
    slider_nu.on_changed(ao_mudar_nu)

    def ao_mudar_velocidade(val):
        estado["passos_por_quadro"] = int(val)
    slider_velocidade.on_changed(ao_mudar_velocidade)

    # --- Botões --------------------------------------------------------
    ax_play = fig.add_axes([0.06, 0.14, 0.12, 0.06])
    botao_play = Button(ax_play, "▶ Play / ❚❚ Pause")

    ax_reset_parado = fig.add_axes([0.20, 0.14, 0.18, 0.06])
    botao_reset_parado = Button(ax_reset_parado, "Reset: fluido parado")

    ax_reset_const = fig.add_axes([0.40, 0.14, 0.20, 0.06])
    botao_reset_const = Button(ax_reset_const, "Reset: vel. constante")

    ax_limpar = fig.add_axes([0.62, 0.14, 0.12, 0.06])
    botao_limpar = Button(ax_limpar, "Limpar histórico")

    def ao_clicar_play(event):
        estado["rodando"] = not estado["rodando"]
    botao_play.on_clicked(ao_clicar_play)

    def ao_clicar_reset_parado(event):
        campo.inicializar_parado()
        campo.registrar_estatisticas()
    botao_reset_parado.on_clicked(ao_clicar_reset_parado)

    def ao_clicar_reset_const(event):
        campo.inicializar_constante(1.0, 0.5)
        campo.registrar_estatisticas()
    botao_reset_const.on_clicked(ao_clicar_reset_const)

    def ao_clicar_limpar(event):
        campo._reset_historico()
        campo.registrar_estatisticas()
    botao_limpar.on_clicked(ao_clicar_limpar)

    # --- Clique do mouse = "mexer nas partículas" -----------------------
    def ao_clicar_no_campo(event):
        if event.inaxes != ax_campo or event.xdata is None:
            return
        cx, cy = event.xdata, event.ydata
        campo.adicionar_perturbacao(cx, cy, slider_amp.val, slider_largura.val)
    fig.canvas.mpl_connect("button_press_event", ao_clicar_no_campo)

    texto_info = fig.text(0.06, 0.06,
                           "Dica: clique em qualquer ponto do campo para injetar uma nova "
                           "perturbação ali, mesmo com a simulação rodando.",
                           fontsize=9, style="italic")

    def atualizar_quadro(_frame):
        if estado["rodando"]:
            dt = dt_estavel(campo.nu)
            for _ in range(estado["passos_por_quadro"]):
                campo.passo(dt)
            campo.registrar_estatisticas()

        imagem.set_data(campo.u.T)
        limite = max(1.0, np.abs(campo.u).max())
        imagem.set_clim(-limite, limite)

        linha_vmax.set_data(campo.historico_t, campo.historico_vmax)
        linha_energia.set_data(campo.historico_t, campo.historico_energia)
        for eixo, linha in ((ax_stats, linha_vmax), (ax_energia, linha_energia)):
            eixo.relim()
            eixo.autoscale_view()

        titulo = f"Campo u(x, y)  |  t = {campo.t:6.2f}  |  ν = {campo.nu:.3f}"
        ax_campo.set_title(titulo)
        return imagem, linha_vmax, linha_energia

    from matplotlib.animation import FuncAnimation
    anim = FuncAnimation(fig, atualizar_quadro, interval=50, blit=False)
    plt.show()
    return anim  # mantém referência viva enquanto a janela está aberta


# ---------------------------------------------------------------------------
# MODO 2: navegar pelos snapshots CSV gerados pelo programa em C
# ---------------------------------------------------------------------------
def rodar_modo_snapshots(pasta):
    arquivos = sorted(glob.glob(os.path.join(pasta, "snapshot_*.csv")))
    if not arquivos:
        print(f"Nenhum arquivo 'snapshot_*.csv' encontrado em: {pasta}")
        print("Rode o programa em C primeiro (ele salva em saida/) ou aponte "
              "--pasta para o diretório correto.")
        sys.exit(1)

    quadros = [np.loadtxt(arq, delimiter=",") for arq in arquivos]
    passos = [int(os.path.basename(a).split("_")[1].split(".")[0]) for a in arquivos]

    fig, ax = plt.subplots(figsize=(7, 6.5))
    plt.subplots_adjust(bottom=0.2)
    limite = max(np.abs(q).max() for q in quadros)
    imagem = ax.imshow(quadros[0].T, origin="lower", cmap="RdBu_r",
                        vmin=-limite, vmax=limite)
    ax.set_title(f"Snapshot do C — passo {passos[0]}")
    fig.colorbar(imagem, ax=ax, label="u")

    ax_slider = fig.add_axes([0.2, 0.06, 0.6, 0.04])
    slider_quadro = Slider(ax_slider, "quadro", 0, len(quadros) - 1,
                            valinit=0, valstep=1)

    def ao_mudar_quadro(val):
        idx = int(val)
        imagem.set_data(quadros[idx].T)
        ax.set_title(f"Snapshot do C — passo {passos[idx]}")
        fig.canvas.draw_idle()
    slider_quadro.on_changed(ao_mudar_quadro)

    plt.show()


# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--modo", choices=["interativo", "snapshots"],
                         default="interativo",
                         help="'interativo' roda a simulação em Python ao vivo; "
                              "'snapshots' navega pelos CSVs gerados pelo C.")
    parser.add_argument("--pasta", default="saida",
                         help="Pasta com os snapshot_*.csv do programa em C "
                              "(usado apenas no modo 'snapshots').")
    args = parser.parse_args()

    if args.modo == "interativo":
        rodar_modo_interativo()
    else:
        rodar_modo_snapshots(args.pasta)


if __name__ == "__main__":
    main()