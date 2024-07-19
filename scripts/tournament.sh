set -xe
readonly ELO1350=1350
readonly ELO1750=1750
readonly ELO2000=2000
readonly rounds=10
readonly tc="st=5"
readonly blobfish_engine="-engine cmd=blobfish name=Blobfish2"
readonly elo_limited_engine="-engine cmd=stockfish name=Stockfish${ELO1350} option.UCI_LimitStrength=true option.UCI_Elo=${ELO1350}"
readonly elo_limited_engine1750="-engine cmd=stockfish name=Stockfish${ELO1750} option.UCI_LimitStrength=true option.UCI_Elo=${ELO1750}"
readonly elo_limited_engine2k="-engine cmd=stockfish name=Stockfish${ELO2000} option.UCI_LimitStrength=true option.UCI_Elo=${ELO2000}"
cutechess-cli ${blobfish_engine} ${elo_limited_engine} ${elo_limited_engine1750}  ${elo_limited_engine2k} -each proto=uci ${tc} -rounds ${rounds}

