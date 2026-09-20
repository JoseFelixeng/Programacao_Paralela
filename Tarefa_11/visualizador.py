#!/usr/bin/env python3
"""
Animacao 3D do CSV gerado por navier_stokes_3d_seq.c / navier_stokes_3d_omp.c.

Colunas do CSV: passo,t,i,j,k,x,y,z,u   (cada 'passo' e um quadro da animacao)

Saidas (use uma ou mais):
  --saida anim.gif     GIF animado (matplotlib + Pillow)
  --saida anim.mp4     video (precisa do ffmpeg instalado)
  --html  anim.html    pagina interativa: gira com o mouse, tem botao play e
                       barra de tempo (precisa de: pip install plotly)

Pastas de saida (--pasta, padrao "saida"):
  saida/gifs/    GIFs
  saida/videos/  MP4
  saida/html/    paginas interativas
  Se voce passar um caminho com diretorio (--saida out/x.gif), ele e respeitado.

Modos do GIF/MP4:
  nuvem       (padrao) nuvem de pontos 3D (so u >= limiar * u_max do quadro)
  superficie  superficie z = u(x,y) no plano central
  ambos       os dois lado a lado

Cano:
  --cano / --no-cano   (padrao: ligado) desenha a parede de um cano ao longo do
                       eixo x (centrado em y = z = 0.5, raio 0.5) e mostra so os
                       pontos que estao dentro dele. Vale para a nuvem e para o HTML.
  ATENCAO: o CSV atual e uma difusao viscosa parada, entao o fluido NAO se
  desloca ao longo do cano: ele so se espalha a partir do centro. Para ver a
  mancha andando, o solver precisa de um termo de adveccao (u_t + U u_x = nu lap u).

Escala de cor/altura:
  quadro      (padrao) cada quadro dividido pelo seu proprio u_max: mostra a
              FORMA se espalhando (o u_max real aparece no titulo)
  global      tudo dividido pelo u_max do passo 0: mostra o DECAIMENTO
              (a partir de certo ponto o quadro fica quase apagado)

Dica: para ficar suave, gere MUITOS snapshots no CSV, por exemplo:
  ./ns3d_seq 48 48 48 240 anim.csv 24 60      # 60 quadros, 24 pontos por eixo
  python3 animar_3d.py anim.csv --saida anim.gif --html anim.html
"""
import argparse
import os

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib import cm, colors
from matplotlib.animation import FFMpegWriter, FuncAnimation, PillowWriter

CMAP = "inferno"

# Geometria do cano: eixo ao longo de x, secao circular centrada em (y, z) = (C, C)
CENTRO = 0.5
RAIO = 0.5
ALONGAMENTO = 2.5  # so visual: alonga o eixo x para o cano parecer um cano


def carrega(csv):
    df = pd.read_csv(csv)
    quadros = []
    for passo, g in df.groupby("passo"):
        zs = np.sort(g["z"].unique())
        plano = g[g["z"] == zs[len(zs) // 2]].pivot(index="y", columns="x", values="u")
        X, Y = np.meshgrid(plano.columns.values, plano.index.values)
        quadros.append(dict(passo=int(passo), t=float(g["t"].iloc[0]), g=g,
                            umax=float(g["u"].max()), X=X, Y=Y, Z=plano.values))
    return quadros


def escala_do_quadro(q, quadros, modo_escala):
    return q["umax"] if modo_escala == "quadro" else quadros[0]["umax"]


def seleciona_pontos(q, limiar, cano):
    """Pontos visiveis do quadro: acima do limiar e, se cano, dentro do cilindro."""
    g = q["g"]
    sel = g[g["u"] >= limiar * q["umax"]]
    if cano:
        dentro = (sel["y"] - CENTRO) ** 2 + (sel["z"] - CENTRO) ** 2 <= RAIO ** 2
        sel = sel[dentro]
    return sel


def malha_cilindro(n=60):
    th = np.linspace(0, 2 * np.pi, n)
    T, Xc = np.meshgrid(th, [0.0, 1.0])
    return th, T, Xc


def desenha_cano(ax):
    th, T, Xc = malha_cilindro()
    ax.plot_surface(Xc, CENTRO + RAIO * np.cos(T), CENTRO + RAIO * np.sin(T),
                    color="lightblue", alpha=0.10, linewidth=0, shade=False)
    for x0 in (0.0, 1.0):  # aros nas extremidades
        ax.plot(np.full_like(th, x0), CENTRO + RAIO * np.cos(th), CENTRO + RAIO * np.sin(th),
                color="gray", lw=1)


def formata_eixos(ax, zlabel="z", cano=False):
    ax.set_xlim(0, 1); ax.set_ylim(0, 1); ax.set_zlim(0, 1)
    ax.set_xlabel("x"); ax.set_ylabel("y"); ax.set_zlabel(zlabel)
    if cano:
        ax.set_box_aspect((ALONGAMENTO, 1, 1))


def desenha_nuvem(ax, q, esc, limiar, cano=False):
    sel = seleciona_pontos(q, limiar, cano)
    v = (sel["u"] / esc).clip(0, 1)
    ax.scatter(sel["x"], sel["y"], sel["z"], c=v, cmap=CMAP, vmin=0, vmax=1,
               s=6 + 30 * v, alpha=0.6, linewidths=0)
    if cano:
        desenha_cano(ax)
    formata_eixos(ax, "z", cano)


def desenha_superficie(ax, q, esc):
    ax.plot_surface(q["X"], q["Y"], q["Z"] / esc, cmap=CMAP, vmin=0, vmax=1,
                    linewidth=0, antialiased=True)
    formata_eixos(ax, "u (normalizado)")


def anima_matplotlib(quadros, args):
    modos = ["nuvem", "superficie"] if args.modo == "ambos" else [args.modo]
    largura = 5.2 * len(modos) + 0.8
    if args.cano and modos == ["nuvem"]:
        largura = 9.0  # cano e comprido: precisa de mais largura
    fig = plt.figure(figsize=(largura, 5.2))
    eixos = [fig.add_subplot(1, len(modos), n + 1, projection="3d") for n in range(len(modos))]
    rotulo = "u / u_max do quadro" if args.escala == "quadro" else "u / u_max do passo 0"
    fig.colorbar(cm.ScalarMappable(norm=colors.Normalize(0, 1), cmap=CMAP),
                 ax=eixos, shrink=0.6, pad=0.06, label=rotulo)

    def quadro(n):
        q = quadros[n]
        esc = escala_do_quadro(q, quadros, args.escala)
        for ax, modo in zip(eixos, modos):
            ax.clear()
            if modo == "nuvem":
                desenha_nuvem(ax, q, esc, args.limiar, args.cano)
            else:
                desenha_superficie(ax, q, esc)
            ax.set_title(modo, fontsize=10)
            ax.view_init(elev=args.elev, azim=args.azim + args.giro * n)
        fig.suptitle(f"Difusao viscosa 3D  |  passo {q['passo']}  t = {q['t']:.3g}  "
                     f"u_max = {q['umax']:.3g}", fontsize=11)

    anim = FuncAnimation(fig, quadro, frames=len(quadros), interval=1000 / args.fps)
    if args.saida.lower().endswith(".mp4"):
        writer = FFMpegWriter(fps=args.fps, bitrate=3000)
    else:
        writer = PillowWriter(fps=args.fps)
    anim.save(args.saida, writer=writer, dpi=args.dpi)
    print("Animacao:", args.saida, f"({len(quadros)} quadros)")


def anima_html(quadros, args):
    try:
        import plotly.graph_objects as go
    except ImportError:
        raise SystemExit("Para --html instale o plotly:  pip install plotly")

    def traco(q):
        esc = escala_do_quadro(q, quadros, args.escala)
        sel = seleciona_pontos(q, args.limiar, args.cano)
        v = (sel["u"] / esc).clip(0, 1)
        return go.Scatter3d(x=sel["x"], y=sel["y"], z=sel["z"], mode="markers",
                            marker=dict(size=3 + 3 * v, color=v, colorscale="Inferno", cmin=0, cmax=1,
                                        opacity=0.6, colorbar=dict(title="u / u_max")),
                            hovertemplate="x=%{x:.2f} y=%{y:.2f} z=%{z:.2f}<extra></extra>")

    def titulo(q):
        return f"passo {q['passo']}  t = {q['t']:.3g}  u_max = {q['umax']:.3g}"

    if args.cano:
        # traco 0 = parede fixa do cano; traco 1 = nuvem (so ela muda nos frames)
        _, T, Xc = malha_cilindro()
        parede = go.Surface(x=Xc, y=CENTRO + RAIO * np.cos(T), z=CENTRO + RAIO * np.sin(T),
                            opacity=0.12, showscale=False,
                            colorscale=[[0, "lightblue"], [1, "lightblue"]], hoverinfo="skip")
        dados_iniciais = [parede, traco(quadros[0])]
        frames = [go.Frame(data=[traco(q)], traces=[1], name=str(q["passo"]),
                           layout=dict(title_text=titulo(q)))
                  for q in quadros]
        cena = dict(xaxis=dict(range=[0, 1], title="x"), yaxis=dict(range=[0, 1], title="y"),
                    zaxis=dict(range=[0, 1], title="z"),
                    aspectmode="manual", aspectratio=dict(x=ALONGAMENTO, y=1, z=1))
    else:
        dados_iniciais = [traco(quadros[0])]
        frames = [go.Frame(data=[traco(q)], name=str(q["passo"]),
                           layout=dict(title_text=titulo(q)))
                  for q in quadros]
        cena = dict(xaxis=dict(range=[0, 1], title="x"), yaxis=dict(range=[0, 1], title="y"),
                    zaxis=dict(range=[0, 1], title="z"), aspectmode="cube")

    fig = go.Figure(data=dados_iniciais, frames=frames)
    passo_ms = int(1000 / args.fps)
    fig.update_layout(
        title_text=titulo(quadros[0]),
        scene=cena,
        updatemenus=[dict(type="buttons", showactive=False, x=0.05, y=0.05, buttons=[
            dict(label="Play", method="animate",
                 args=[None, dict(frame=dict(duration=passo_ms, redraw=True), fromcurrent=True)]),
            dict(label="Pause", method="animate",
                 args=[[None], dict(frame=dict(duration=0, redraw=False), mode="immediate")])])],
        sliders=[dict(steps=[dict(method="animate", label=str(q["passo"]),
                                  args=[[str(q["passo"])], dict(mode="immediate",
                                                                frame=dict(duration=0, redraw=True))])
                             for q in quadros])])
    fig.write_html(args.html, include_plotlyjs="cdn")
    print("HTML:", args.html)


SUBPASTAS = {".gif": "gifs", ".mp4": "videos", ".html": "html"}


def caminho_saida(arquivo, pasta_base):
    """Coloca o arquivo em <pasta_base>/<gifs|videos|html>/ e cria a pasta.
    Se o usuario ja passou um caminho com diretorio (ex.: out/x.gif), respeita."""
    if os.path.dirname(arquivo):
        destino = arquivo
    else:
        ext = os.path.splitext(arquivo)[1].lower()
        destino = os.path.join(pasta_base, SUBPASTAS.get(ext, "outros"), arquivo)
    os.makedirs(os.path.dirname(destino) or ".", exist_ok=True)
    return destino


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv")
    ap.add_argument("--saida", default=None, help="arquivo .gif ou .mp4")
    ap.add_argument("--html", default=None, help="pagina HTML interativa (plotly)")
    ap.add_argument("--pasta", default="saida",
                    help="pasta base das saidas; cria saida/gifs, saida/videos e saida/html (padrao: saida)")
    ap.add_argument("--modo", choices=["nuvem", "superficie", "ambos"], default="nuvem")
    ap.add_argument("--escala", choices=["quadro", "global"], default="quadro")
    ap.add_argument("--cano", action=argparse.BooleanOptionalAction, default=True,
                    help="desenha a parede de um cano e recorta a nuvem dentro dele (use --no-cano para desligar)")
    ap.add_argument("--limiar", type=float, default=0.05, help="fracao de u_max abaixo da qual o ponto some (nuvem)")
    ap.add_argument("--fps", type=int, default=4)
    ap.add_argument("--dpi", type=int, default=80)
    ap.add_argument("--elev", type=float, default=15, help="elevacao da camera")
    ap.add_argument("--azim", type=float, default=-60, help="azimute inicial da camera")
    ap.add_argument("--giro", type=float, default=0.0, help="graus de rotacao da camera por quadro (0 = camera fixa)")
    args = ap.parse_args()

    if not args.saida and not args.html:
        nome = os.path.splitext(os.path.basename(args.csv))[0]
        args.saida = nome + "_anim_static4.gif"
    if args.saida:
        args.saida = caminho_saida(args.saida, args.pasta)
    if args.html:
        args.html = caminho_saida(args.html, args.pasta)

    quadros = carrega(args.csv)
    if len(quadros) < 2:
        raise SystemExit("O CSV so tem 1 snapshot. Gere mais (ex.: ./ns3d_seq ... arq.csv 24 60).")
    if args.saida:
        anima_matplotlib(quadros, args)
    if args.html:
        anima_html(quadros, args)


if __name__ == "__main__":
    main()