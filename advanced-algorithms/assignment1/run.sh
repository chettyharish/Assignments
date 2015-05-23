gcc -fopenmp -o linearSearch linearSearch.c -O3
gcc -fopenmp -o linearSearchNew linearSearchNew.c -O3
python pyrat.py
python queryCreator.py
./linearSearch input.txt query.txt 32
./linearSearchNew input.txt query.txt 32
