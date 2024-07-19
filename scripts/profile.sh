set -xe
readonly outfile=callgrind.out
valgrind --tool=callgrind --instr-atstart=no --collect-atstart=no --callgrind-out-file=${outfile} blobfish
gprof2dot -f callgrind callgrind.out | dot -Tsvg -o output.svg
rm ${outfile}
