#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gerar_graficos_benchmark.py — le resultados_benchmark.csv e gera:
  1) grafico_speedup.png       -> speedup vs numero de threads, uma curva
                                   por schedule, um painel por collapse
  2) grafico_schedule_collapse.png -> barras comparando schedule x collapse
                                       no maior numero de threads testado

Uso:
    python3 gerar_graficos_benchmark.py --csv resultados_benchmark.csv
"""

import argparse
import csv
from collections import defaultdict

import matplotlib.pyplot as plt


def carregar_csv(caminho):
    linhas = []
    with open(caminho) as f:
        leitor = csv.DictReader(f)
        for linha in leitor:
            linha["collapse"] = int(linha["collapse"])
            linha["threads"] = int(linha["threads"])
            linha["tempo_s"] = float(linha["tempo_s"])
            linhas.append(linha)
    return linhas


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--csv", default="resultados_benchmark.csv")
    parser.add_argument("--prefixo-saida", default="")
    args = parser.parse_args()

    linhas = carregar_csv(args.csv)
    collapses = sorted(set(l["collapse"] for l in linhas))
    schedules = sorted(set(l["schedule"] for l in linhas),
                        key=lambda s: list(l["schedule"] for l in linhas).index(s))
    threads_disponiveis = sorted(set(l["threads"] for l in linhas))

    # tempo de referencia (1 thread) por collapse+schedule, para calcular speedup
    tempo_base = {}
    for l in linhas:
        if l["threads"] == 1:
            tempo_base[(l["collapse"], l["schedule"])] = l["tempo_s"]

    # ---------------- grafico 1: speedup vs threads ----------------
    fig, eixos = plt.subplots(1, len(collapses), figsize=(5 * len(collapses), 4.5), sharey=True)
    if len(collapses) == 1:
        eixos = [eixos]

    for eixo, collapse in zip(eixos, collapses):
        for schedule in schedules:
            pontos = sorted(
                [(l["threads"], l["tempo_s"]) for l in linhas
                 if l["collapse"] == collapse and l["schedule"] == schedule],
                key=lambda p: p[0])
            if not pontos:
                continue
            base = tempo_base.get((collapse, schedule))
            if not base:
                continue
            xs = [p[0] for p in pontos]
            speedups = [base / p[1] for p in pontos]
            eixo.plot(xs, speedups, marker="o", label=schedule)

        eixo.plot(threads_disponiveis, threads_disponiveis, "--", color="gray",
                  alpha=0.5, label="speedup ideal")
        eixo.set_title(f"collapse={collapse}")
        eixo.set_xlabel("threads (OMP_NUM_THREADS)")
        eixo.grid(alpha=0.3)
    eixos[0].set_ylabel("speedup (T1 / Tn)")
    eixos[-1].legend(fontsize=8)
    fig.suptitle("Speedup por schedule e por nível de collapse")
    fig.tight_layout()
    fig.savefig(f"{args.prefixo_saida}grafico_speedup.png", dpi=140)
    print(f"Salvo: {args.prefixo_saida}grafico_speedup.png")

    # ---------------- grafico 2: barras schedule x collapse no maior thread count ----
    threads_max = max(threads_disponiveis)
    fig2, ax2 = plt.subplots(figsize=(7, 4.5))
    largura = 0.8 / len(collapses)
    for idx, collapse in enumerate(collapses):
        tempos = []
        for schedule in schedules:
            achados = [l["tempo_s"] for l in linhas
                       if l["collapse"] == collapse and l["schedule"] == schedule
                       and l["threads"] == threads_max]
            tempos.append(achados[0] if achados else float("nan"))
        posicoes = [i + idx * largura for i in range(len(schedules))]
        ax2.bar(posicoes, tempos, width=largura, label=f"collapse={collapse}")

    ax2.set_xticks([i + largura * (len(collapses) - 1) / 2 for i in range(len(schedules))])
    ax2.set_xticklabels(schedules, rotation=20, ha="right")
    ax2.set_ylabel("tempo (s)")
    ax2.set_title(f"Tempo por schedule e collapse ({threads_max} threads)")
    ax2.legend()
    ax2.grid(axis="y", alpha=0.3)
    fig2.tight_layout()
    fig2.savefig(f"{args.prefixo_saida}grafico_schedule_collapse.png", dpi=140)
    print(f"Salvo: {args.prefixo_saida}grafico_schedule_collapse.png")


if __name__ == "__main__":
    main()