set -xe
valgrind --tool=callgrind --instr-atstart=no --collect-atstart=no --callgrind-out-file=callgrind.out ./build/blobfish
gprof2dot -f callgrind callgrind.out | dot -Tsvg -o output.svg

