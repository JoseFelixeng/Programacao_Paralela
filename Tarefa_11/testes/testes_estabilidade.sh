#!/bin/bash
# testes_estabilidade.sh
# Compila e roda os casos de comprovacao (estabilidade) nas versoes sequencial e paralela.
# Uso:  chmod +x testes_estabilidade.sh && ./testes_estabilidade.sh
# Saida completa tambem em saida/estabilidade.txt (util para copiar para o relatorio).

THREADS=${THREADS:-4}          # threads da versao paralela (ex.: THREADS=8 ./testes_estabilidade.sh)
mkdir -p bin saida
LOG=saida/estabilidade.txt
: > "$LOG"

# nome | flags de compilacao | descricao
CASOS=(
  "parado|-DAMPLITUDE=0.0 -DU0=0.0|Fluido parado (u = 0)"
  "constante|-DAMPLITUDE=0.0 -DU0=1.0|Velocidade constante (u = 1, paredes em 0)"
  "perturbacao||Perturbacao gaussiana (caso do relatorio)"
  "instavel|-DFATOR_DT=1.2|Controle: dt acima do limite (deve FALHAR)"
)

roda() {   # $1=seq|par  $2=fonte  $3=flags extras de compilacao
  local tipo=$1 fonte=$2 omp=$3
  for c in "${CASOS[@]}"; do
    IFS='|' read -r nome flags desc <<< "$c"
    bin="bin/${tipo}_${nome}"
    gcc -O2 $omp $flags -o "$bin" "$fonte" -lm || exit 1
    {
      echo "=== [$tipo] $desc"
      if [ "$tipo" = "par" ]; then
        OMP_NUM_THREADS=$THREADS "$bin" 64 64 64 200 /dev/null 2>&1
      else
        "$bin" 64 64 64 200 /dev/null 2>&1
      fi
      echo
    } | tee -a "$LOG"
  done
}

roda seq navier_stokes_seq_teste.c ""
roda par navier_stokes_par_teste.c "-fopenmp"
echo "Resultados salvos em $LOG"
