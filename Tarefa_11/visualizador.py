#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
============================================================================
Visualizador 3D — fluido difundindo dentro de um cilindro
============================================================================

Baseado no `sim.c` (grade 3D, laplaciano de 7 pontos, Euler explícito,
ALFA=0.1). Aqui o domínio retangular do C é recortado no formato de um
cilindro: qualquer célula fora do raio escolhido (ou nas "tampas" do topo
e da base) vira parede sólida, mantida fixa em u=0 — exatamente como o C
nunca atualiza a borda da malha (contorno de Dirichlet fixo), só que agora
a "borda" tem o formato de um tubo em vez de uma caixa.

O que você vê na janela:
  - Esquerda:  o cilindro em 3D (parede em wireframe translúcido) com as
               células onde |u| ultrapassa um limiar, coloridas pela
               intensidade — é a "nuvem" da perturbação se espalhando.
  - Direita (cima):   corte transversal (plano XY) numa altura Z ajustável
                       — mostra o círculo do cilindro de frente.
  - Direita (baixo):  corte longitudinal (plano XZ, cortando pelo eixo) —
                       mostra o tubo "aberto ao meio", de lado.

Interatividade:
  - Clique no corte transversal (XY) para injetar uma nova perturbação
    gaussiana bem naquele ponto, na altura Z escolhida no slider.
  - Sliders: ALFA (viscosidade), amplitude, largura (sigma) da perturbação,
    corte Z, limiar de exibição no 3D, passos de física por quadro.
  - Botões: Play/Pause, Reset (parado), Reset (velocidade constante),
    Nova perturbação central.

Uso:
    python3 visualizador_cilindro.py
    python3 visualizador_cilindro.py --nx 60 --ny 60 --nz 140 --raio 26

Dependências: numpy, matplotlib (pip install numpy matplotlib)
============================================================================
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.widgets import Slider, Button
from matplotlib.animation import FuncAnimation
from mpl_toolkits.mplot3d import Axes3D  # noqa: F401 (necessário para projection='3d')

ALFA_PADRAO = 0.1               # mesmo valor do sim.c (limite 3D: alfa <= 1/6)
PASSOS_POR_QUADRO_PADRAO = 2
MAX_PONTOS_DISPERSOS = 4000      # limite de pontos plotados no 3D por quadro


class FluidoCilindro:
    """Campo escalar u(x,y,z) difundindo dentro de um cilindro com paredes
    e tampas sólidas (u fixo em 0 fora do domínio de fluido)."""

    def __init__(self, nx, ny, nz, raio, alfa):
        self.nx, self.ny, self.nz = nx, ny, nz
        self.cx, self.cy = (nx - 1) / 2.0, (ny - 1) / 2.0
        self.raio = raio
        self.alfa = alfa

        i = np.arange(nx).reshape(-1, 1, 1)
        j = np.arange(ny).reshape(1, -1, 1)
        k = np.arange(nz).reshape(1, 1, -1)
        dentro_do_raio = (i - self.cx) ** 2 + (j - self.cy) ** 2 <= raio ** 2
        longe_das_tampas = (k >= 1) & (k <= nz - 2)
        self.mascara = dentro_do_raio & longe_das_tampas  # True = célula de fluido

        self.u = np.zeros((nx, ny, nz))
        self.t = 0
        self.historico_t = []
        self.historico_max = []

    # -------------------- inicializações (fases 1 e 2 do C) -------------
    def inicializar_parado(self):
        self.u[:] = 0.0
        self._reset_historico()

    def inicializar_constante(self, u0=1.0):
        self.u[:] = 0.0
        self.u[self.mascara] = u0
        self._reset_historico()

    def _reset_historico(self):
        self.t = 0
        self.historico_t.clear()
        self.historico_max.clear()

    # -------------------- perturbação (fase 3 do C / cliques do mouse) --
    def adicionar_perturbacao(self, cx, cy, cz, valor, largura):
        i = np.arange(self.nx).reshape(-1, 1, 1)
        j = np.arange(self.ny).reshape(1, -1, 1)
        k = np.arange(self.nz).reshape(1, 1, -1)
        r2 = (i - cx) ** 2 + (j - cy) ** 2 + (k - cz) ** 2
        gauss = valor * np.exp(-r2 / (2.0 * largura ** 2))
        self.u += gauss * self.mascara  # nunca "vaza" para fora do cilindro

    # -------------------- passo de difusão (equivalente ao C) -----------
    def passo(self):
        """Laplaciano de 7 pontos com contorno de Dirichlet fixo em 0
        (mesma ideia do sim.c: bordas nunca são atualizadas)."""
        up = np.pad(self.u, 1, mode="constant", constant_values=0.0)
        lap = (
            up[2:, 1:-1, 1:-1] + up[:-2, 1:-1, 1:-1]
            + up[1:-1, 2:, 1:-1] + up[1:-1, :-2, 1:-1]
            + up[1:-1, 1:-1, 2:] + up[1:-1, 1:-1, :-2]
            - 6.0 * self.u
        )
        u_novo = self.u + self.alfa * lap
        u_novo[~self.mascara] = 0.0  # paredes e tampas do cilindro: sempre 0
        self.u = u_novo
        self.t += 1

    def valor_maximo(self):
        if not self.mascara.any():
            return 0.0
        return float(np.abs(self.u[self.mascara]).max())

    def registrar_estatisticas(self):
        self.historico_t.append(self.t)
        self.historico_max.append(self.valor_maximo())


def malha_cilindro(cx, cy, raio, nz, n_aneis=16, n_theta=30):
    """Gera a superfície (wireframe) do cilindro só para referência visual."""
    theta = np.linspace(0, 2 * np.pi, n_theta)
    z = np.linspace(0, nz - 1, n_aneis)
    theta_grade, z_grade = np.meshgrid(theta, z)
    x_grade = cx + raio * np.cos(theta_grade)
    y_grade = cy + raio * np.sin(theta_grade)
    return x_grade, y_grade, z_grade


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                      formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--nx", type=int, default=50, help="pontos da malha em x (padrão 50)")
    parser.add_argument("--ny", type=int, default=50, help="pontos da malha em y (padrão 50)")
    parser.add_argument("--nz", type=int, default=100, help="pontos da malha em z / comprimento do cilindro (padrão 100)")
    parser.add_argument("--raio", type=float, default=None,
                         help="raio do cilindro em pontos de malha (padrão: quase preenche nx,ny)")
    parser.add_argument("--alfa", type=float, default=ALFA_PADRAO,
                         help=f"coeficiente de difusão (padrão {ALFA_PADRAO}; estável até ~0.1667 em 3D)")
    args = parser.parse_args()

    raio = args.raio if args.raio is not None else min(args.nx, args.ny) / 2.0 - 2.0

    campo = FluidoCilindro(args.nx, args.ny, args.nz, raio, args.alfa)
    campo.inicializar_parado()
    campo.adicionar_perturbacao(campo.cx, campo.cy, args.nz / 2.0, valor=5.0, largura=4.0)
    campo.registrar_estatisticas()

    estado = {"rodando": False, "passos_por_quadro": PASSOS_POR_QUADRO_PADRAO,
              "z_corte": args.nz // 2}

    fig = plt.figure(figsize=(13, 6.5))
    fig.suptitle("Difusão viscosa dentro de um cilindro — clique no corte transversal para perturbar",
                 fontsize=11)

    # ---------------- painel 3D (esquerda) ----------------
    ax3d = fig.add_axes([0.03, 0.28, 0.42, 0.64], projection="3d")
    xc, yc, zc = malha_cilindro(campo.cx, campo.cy, campo.raio, campo.nz)
    ax3d.plot_wireframe(xc, yc, zc, color="gray", alpha=0.15, linewidth=0.5)
    dispersos = ax3d.scatter([], [], [], c=[], cmap="inferno", vmin=0, vmax=5, s=8)
    ax3d.set_xlim(0, campo.nx)
    ax3d.set_ylim(0, campo.ny)
    ax3d.set_zlim(0, campo.nz)
    ax3d.set_xlabel("x")
    ax3d.set_ylabel("y")
    ax3d.set_zlabel("z (comprimento do cilindro)")

    # ---------------- corte transversal XY (direita, cima) ----------------
    ax_xy = fig.add_axes([0.50, 0.55, 0.20, 0.37])
    fatia_xy = campo.u[:, :, estado["z_corte"]].copy()
    fatia_xy[~campo.mascara[:, :, estado["z_corte"]]] = np.nan
    im_xy = ax_xy.imshow(fatia_xy.T, origin="lower", cmap="RdBu_r", vmin=-5, vmax=5)
    ax_xy.set_title(f"Corte transversal (z={estado['z_corte']})")
    ax_xy.set_xlabel("x")
    ax_xy.set_ylabel("y")

    # ---------------- corte longitudinal XZ (direita, baixo) ----------------
    ax_xz = fig.add_axes([0.74, 0.55, 0.22, 0.37])
    y_meio = int(round(campo.cy))
    fatia_xz = campo.u[:, y_meio, :].copy()
    fatia_xz[~campo.mascara[:, y_meio, :]] = np.nan
    im_xz = ax_xz.imshow(fatia_xz.T, origin="lower", cmap="RdBu_r", vmin=-5, vmax=5, aspect="auto")
    ax_xz.set_title(f"Corte longitudinal (y={y_meio})")
    ax_xz.set_xlabel("x")
    ax_xz.set_ylabel("z")

    # ---------------- gráfico de max|u| no tempo ----------------
    ax_stats = fig.add_axes([0.50, 0.30, 0.46, 0.16])
    linha_max, = ax_stats.plot([], [], color="crimson")
    ax_stats.set_xlabel("passo")
    ax_stats.set_ylabel("max|u|")
    ax_stats.set_title("Evolução de max|u|", fontsize=9)

    # ---------------- sliders ----------------
    ax_alfa = fig.add_axes([0.06, 0.20, 0.34, 0.03])
    slider_alfa = Slider(ax_alfa, "ALFA", 0.001, 0.166, valinit=campo.alfa)

    ax_amp = fig.add_axes([0.06, 0.16, 0.34, 0.03])
    slider_amp = Slider(ax_amp, "amplitude", 0.5, 10.0, valinit=5.0)

    ax_largura = fig.add_axes([0.06, 0.12, 0.34, 0.03])
    slider_largura = Slider(ax_largura, "largura σ", 1.0, 15.0, valinit=4.0)

    ax_zcorte = fig.add_axes([0.06, 0.08, 0.34, 0.03])
    slider_zcorte = Slider(ax_zcorte, "corte Z", 1, campo.nz - 2,
                            valinit=estado["z_corte"], valstep=1)

    ax_limiar = fig.add_axes([0.50, 0.20, 0.20, 0.03])
    slider_limiar = Slider(ax_limiar, "limiar 3D", 0.01, 5.0, valinit=0.3)

    ax_passos = fig.add_axes([0.76, 0.20, 0.20, 0.03])
    slider_passos = Slider(ax_passos, "passos/quadro", 1, 10,
                            valinit=PASSOS_POR_QUADRO_PADRAO, valstep=1)

    def ao_mudar_alfa(val):
        campo.alfa = val
    slider_alfa.on_changed(ao_mudar_alfa)

    def ao_mudar_zcorte(val):
        estado["z_corte"] = int(val)
    slider_zcorte.on_changed(ao_mudar_zcorte)

    def ao_mudar_passos(val):
        estado["passos_por_quadro"] = int(val)
    slider_passos.on_changed(ao_mudar_passos)

    # ---------------- botões ----------------
    ax_play = fig.add_axes([0.06, 0.02, 0.14, 0.05])
    botao_play = Button(ax_play, "▶ Play / ❚❚ Pause")

    ax_reset_parado = fig.add_axes([0.21, 0.02, 0.16, 0.05])
    botao_reset_parado = Button(ax_reset_parado, "Reset: parado")

    ax_reset_const = fig.add_axes([0.38, 0.02, 0.18, 0.05])
    botao_reset_const = Button(ax_reset_const, "Reset: vel. constante")

    ax_nova_pert = fig.add_axes([0.57, 0.02, 0.20, 0.05])
    botao_nova_pert = Button(ax_nova_pert, "Nova perturbação central")

    def ao_clicar_play(event):
        estado["rodando"] = not estado["rodando"]
    botao_play.on_clicked(ao_clicar_play)

    def ao_clicar_reset_parado(event):
        campo.inicializar_parado()
        campo.registrar_estatisticas()
    botao_reset_parado.on_clicked(ao_clicar_reset_parado)

    def ao_clicar_reset_const(event):
        campo.inicializar_constante(1.0)
        campo.registrar_estatisticas()
    botao_reset_const.on_clicked(ao_clicar_reset_const)

    def ao_clicar_nova_pert(event):
        campo.adicionar_perturbacao(campo.cx, campo.cy, estado["z_corte"],
                                     slider_amp.val, slider_largura.val)
    botao_nova_pert.on_clicked(ao_clicar_nova_pert)

    # ---------------- clique no corte XY = "mexer nas partículas" ----------
    def ao_clicar_no_corte(event):
        if event.inaxes != ax_xy or event.xdata is None:
            return
        x, y = event.xdata, event.ydata
        if (x - campo.cx) ** 2 + (y - campo.cy) ** 2 > campo.raio ** 2:
            return  # clique fora do cilindro: ignora
        campo.adicionar_perturbacao(x, y, estado["z_corte"], slider_amp.val, slider_largura.val)
    fig.canvas.mpl_connect("button_press_event", ao_clicar_no_corte)

    # ---------------- loop de animação ----------------
    def atualizar_quadro(_frame):
        if estado["rodando"]:
            for _ in range(estado["passos_por_quadro"]):
                campo.passo()
            campo.registrar_estatisticas()

        # corte transversal XY
        z = estado["z_corte"]
        fatia_xy = campo.u[:, :, z].copy()
        fatia_xy[~campo.mascara[:, :, z]] = np.nan
        im_xy.set_data(fatia_xy.T)
        ax_xy.set_title(f"Corte transversal (z={z})")

        # corte longitudinal XZ
        fatia_xz = campo.u[:, y_meio, :].copy()
        fatia_xz[~campo.mascara[:, y_meio, :]] = np.nan
        im_xz.set_data(fatia_xz.T)

        # dispersão 3D: só células acima do limiar
        limiar = slider_limiar.val
        indices = np.argwhere(campo.mascara & (np.abs(campo.u) > limiar))
        if len(indices) > MAX_PONTOS_DISPERSOS:
            passo_amostragem = len(indices) // MAX_PONTOS_DISPERSOS + 1
            indices = indices[::passo_amostragem]
        if len(indices) > 0:
            valores = campo.u[indices[:, 0], indices[:, 1], indices[:, 2]]
            dispersos._offsets3d = (indices[:, 0], indices[:, 1], indices[:, 2])
            dispersos.set_array(np.abs(valores))
        else:
            dispersos._offsets3d = ([], [], [])

        # estatísticas
        linha_max.set_data(campo.historico_t, campo.historico_max)
        ax_stats.relim()
        ax_stats.autoscale_view()

        return im_xy, im_xz, dispersos, linha_max

    anim = FuncAnimation(fig, atualizar_quadro, interval=60, blit=False, cache_frame_data=False)
    plt.show()
    return anim


if __name__ == "__main__":
    main()