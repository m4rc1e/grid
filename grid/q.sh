set -e

sh build.sh src/main.cpp
./a.out $@
open output3.pdf
