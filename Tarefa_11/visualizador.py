#!/usr/bin/env python3
"""
Animacao 3D do CSV gerado por navier_stokes_3d_seq.c / navier_stokes_3d_omp.c.

Colunas do CSV: passo,t,i,j,k,x,y,z,u   (cada 'passo' e um quadro da animacao)

Saidas (use uma ou mais):
  --saida anim.gif     GIF animado (matplotlib + Pillow)
  --saida anim.mp4     video (precisa do ffmpeg instalado)
  --html  anim.html    pagina interativa: gira com o mouse, tem botao play e
                       barra de tempo (precisa de: pip install plotly)

Modos do GIF/MP4:
  nuvem       nuvem de pontos 3D (so u >= limiar * u_max do quadro)
  superficie  superficie z = u(x,y) no plano central
  ambos       (padrao) os dois lado a lado

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

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib import cm, colors
from matplotlib.animation import FFMpegWriter, FuncAnimation, PillowWriter

CMAP = "inferno"


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


def desenha_nuvem(ax, q, esc, limiar):
    g = q["g"]
    sel = g[g["u"] >= limiar * q["umax"]]
    v = (sel["u"] / esc).clip(0, 1)
    ax.scatter(sel["x"], sel["y"], sel["z"], c=v, cmap=CMAP, vmin=0, vmax=1,
               s=6 + 30 * v, alpha=0.6, linewidths=0)
    ax.set_xlim(0, 1); ax.set_ylim(0, 1); ax.set_zlim(0, 1)
    ax.set_xlabel("x"); ax.set_ylabel("y"); ax.set_zlabel("z")


def desenha_superficie(ax, q, esc):
    ax.plot_surface(q["X"], q["Y"], q["Z"] / esc, cmap=CMAP, vmin=0, vmax=1,
                    linewidth=0, antialiased=True)
    ax.set_xlim(0, 1); ax.set_ylim(0, 1); ax.set_zlim(0, 1)
    ax.set_xlabel("x"); ax.set_ylabel("y"); ax.set_zlabel("u (normalizado)")


def anima_matplotlib(quadros, args):
    modos = ["nuvem", "superficie"] if args.modo == "ambos" else [args.modo]
    fig = plt.figure(figsize=(5.2 * len(modos) + 0.8, 5.2))
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
                desenha_nuvem(ax, q, esc, args.limiar)
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
        g = q["g"]
        sel = g[g["u"] >= args.limiar * q["umax"]]
        v = (sel["u"] / esc).clip(0, 1)
        return go.Scatter3d(x=sel["x"], y=sel["y"], z=sel["z"], mode="markers",
                            marker=dict(size=3 + 3 * v, color=v, colorscale="Inferno", cmin=0, cmax=1,
                                        opacity=0.6, colorbar=dict(title="u / u_max")),
                            hovertemplate="x=%{x:.2f} y=%{y:.2f} z=%{z:.2f}<extra></extra>")

    frames = [go.Frame(data=[traco(q)], name=str(q["passo"]),
                       layout=dict(title_text=f"passo {q['passo']}  t = {q['t']:.3g}  u_max = {q['umax']:.3g}"))
              for q in quadros]
    fig = go.Figure(data=[traco(quadros[0])], frames=frames)
    passo_ms = int(1000 / args.fps)
    fig.update_layout(
        title_text=f"passo {quadros[0]['passo']}  t = {quadros[0]['t']:.3g}  u_max = {quadros[0]['umax']:.3g}",
        scene=dict(xaxis=dict(range=[0, 1], title="x"), yaxis=dict(range=[0, 1], title="y"),
                   zaxis=dict(range=[0, 1], title="z"), aspectmode="cube"),
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


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("csv")
    ap.add_argument("--saida", default=None, help="arquivo .gif ou .mp4")
    ap.add_argument("--html", default=None, help="pagina HTML interativa (plotly)")
    ap.add_argument("--modo", choices=["nuvem", "superficie", "ambos"], default="ambos")
    ap.add_argument("--escala", choices=["quadro", "global"], default="quadro")
    ap.add_argument("--limiar", type=float, default=0.05, help="fracao de u_max abaixo da qual o ponto some (nuvem)")
    ap.add_argument("--fps", type=int, default=10)
    ap.add_argument("--dpi", type=int, default=80)
    ap.add_argument("--elev", type=float, default=22, help="elevacao da camera")
    ap.add_argument("--azim", type=float, default=35, help="azimute inicial da camera")
    ap.add_argument("--giro", type=float, default=1.0, help="graus de rotacao da camera por quadro (0 = camera fixa)")
    args = ap.parse_args()

    if not args.saida and not args.html:
        args.saida = args.csv.rsplit(".", 1)[0] + "_animp.gif"

    quadros = carrega(args.csv)
    if len(quadros) < 2:
        raise SystemExit("O CSV so tem 1 snapshot. Gere mais (ex.: ./ns3d_seq ... arq.csv 24 60).")
    if args.saida:
        anima_matplotlib(quadros, args)
    if args.html:
        anima_html(quadros, args)


if __name__ == "__main__":
    main()