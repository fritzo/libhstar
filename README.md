# libhstar

[![Build Status](https://travis-ci.org/fritzo/libhstar.svg?branch=master)](https://travis-ci.org/fritzo/libhstar)

Pure untyped extensional non-deterministic memoized combinator machine.

## TODO

- [ ] Test infrastructure
  - [ ] Collect benchmark problems
  - [ ] Specify behavior
  - [ ] Build test jig
- [ ] Basic reduction
  - [ ] Eager linear reduction
  - [ ] Memoization
  - [ ] Eta reduction as in - [ ]
- [ ] Nondeterminism
  - [ ] Reduction rules for JOIN
  - [ ] Sampling from joins over flat domains
- [ ] Todd-Coxeter
  - [ ] Forward-chaining merge logic
- [ ] Types
  - [ ] Reduction rules for basic datatypes
  - [ ] Optimizations for basic datatypes (byte operations, integers)
  - [ ] Json datatypes: booleans, numbers, strings, lists, dicts
  - [ ] Serialization via protobuf, json, etc. (use upb)
- [ ] Fast C implementation
  - [ ] C implementation
  - [ ] Python bindings for c version
  - [ ] Define performance metrics
  - [ ] Build performance benchmarking jig

## References

- [Scott76] <a name="Scott76"/>
  Dana Scott (1976)
  [Datatypes as Lattices](http://www.cs.ox.ac.uk/files/3287/PRG05.pdf)
- [Hindley08] <a name="Hindley2008"/>
  J. Roger Hindley, J.P. Seldin (2008)
  "Lambda calculus and combinatory logic: an introduction"
- [Koopman91] <a name="Koopman91"/>
  Philip Koopman, Peter Lee (1991)
  [Architectural considerations for graph reduction](http://users.ece.cmu.edu/~koopman/tigre/lee_book_ch15.pdf)
- [Boulifa03] <a name="Boulifa03"/>
  Rabea Boulifa, Mohamed Mezghiche (2003)
  [Another Implementation Technique for Functional Languages](http://jfla.inria.fr/2003/actes/PS/04-boulifa.ps)
- [Obermeyer09] <a name="Obermeyer09"/>
  Fritz Obermeyer (2009)
  [Automated Equational Reasoning in Nondeterministic &lambda;-Calculi Modulo Theories H*](http://fritzo.org/thesis.pdf)
- [Fischer11] <a name="Fischer11"/>
  Sebastian Fischer, Oleg Kiselyov, Chung-Chieh Shan (2011)
  [Purely functional lazy nondeterministic programming](http://okmij.org/ftp/Haskell/FLP/lazy-nondet.pdf)

