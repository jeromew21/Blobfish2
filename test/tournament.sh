# TODO: copy executable into the current dir
set -xe
readonly ELO1350=1350
readonly ELO2000=2000
readonly rounds=10
readonly tc="40/10"
readonly blobfish_engine="-engine cmd=blobfish name=Blobfish2"
readonly elo_limited_engine="-engine cmd=stockfish name=Stockfish${ELO1350} option.UCI_LimitStrength=true option.UCI_Elo=${ELO1350}"
readonly elo_limited_engine2k="-engine cmd=stockfish name=Stockfish${ELO2000} option.UCI_LimitStrength=true option.UCI_Elo=${ELO2000}"
readonly stockfish_default_engine="-engine cmd=stockfish name=StockfishDefault cmd=stockfish"
cutechess-cli ${blobfish_engine} ${stockfish_default_engine}  ${elo_limited_engine} ${elo_limited_engine2k} -each proto=uci tc=${tc} -rounds ${rounds}

