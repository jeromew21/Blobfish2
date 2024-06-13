# TODO: copy executable into the current dir
set -xe
readonly ELO=1350
readonly rounds=10
readonly tc="40/60"
readonly blobfish_engine="-engine cmd=blobfish name=Blobfish2"
readonly elo_limited_engine="-engine cmd=stockfish name=Stockfish${ELO} option.UCI_LimitStrength=true option.UCI_Elo=${ELO}"
cutechess-cli ${blobfish_engine} ${elo_limited_engine} -each proto=uci tc=${tc} -rounds ${rounds}

