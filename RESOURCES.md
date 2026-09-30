# Recursos de estudo (DCA3703)

## Livros e referências

- [Livro: *An Introduction to Parallel Programming*, Peter S. Pacheco](https://www.cs.usfca.edu/~peter/ipp/)
  Cobre modelos de memória compartilhada (OpenMP, Pthreads), memória distribuída (MPI) e sincronização.
- [Livro: *Computer Architecture: A Quantitative Approach*, John L. Hennessy e David A. Patterson](https://www.elsevier.com/books/computer-architecture/hennessy/978-0-12-811905-1)
  Cobre pipelining, ILP, coerência de cache e arquiteturas multicore/NUMA.
- [Livro: *Computer Systems: A Programmer's Perspective (CS:APP)*, Randal E. Bryant e David R. O'Hallaron](https://csapp.cs.cmu.edu/)
  Cobre hierarquia de memória, localidade espacial e temporal e otimização de laços.
- [Documentação oficial da especificação OpenMP](https://www.openmp.org/specifications/)
  Referência de diretivas, cláusulas e funções de runtime.

## Fonte primária da Tarefa 11

- [Slides do professor, transcrição local](assets/index.md), slides 33–36: particionamento, `schedule`, `chunksize`, `collapse` e enunciado de difusão viscosa.
- Especificação oficial OpenMP 5.1, consultada para revisar o relatório: [sections, §2.10.1](https://www.openmp.org/spec-html/5.1/openmpsu42.html), [worksharing-loop, schedule e collapse, §2.11.4](https://www.openmp.org/spec-html/5.1/openmpsu48.html) e [SIMD, §2.11.5](https://www.openmp.org/spec-html/5.1/openmpsu49.html). Fundamenta a distinção entre blocos, iterações, threads e lanes vetoriais; defaults e granularidade das políticas.

## Comunidades

- [Stack Overflow (tags: openmp, c, cpu-cache, posix)](https://stackoverflow.com/questions/tagged/openmp)
  Exemplos práticos de compilação, falsos compartilhamentos e diagnósticos de concorrência.
- Fórum da disciplina DCA3703 (Sigaa / DIMAp / UFRN)
  Esclarecimentos de enunciados, prazos e critérios do professor.
