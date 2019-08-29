#!/bin/bash

temp_th=$(mktemp)
temp_file_libc=$(mktemp)
temp_file_tc=$(mktemp)
temp_file_je=$(mktemp)

iterations=1000000

echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

echo "ops/sec for 64 byte objects"
for th in {0..12}; do
    echo $th >> $temp_th
    ./proxy_libc --iterations=$iterations --threads=$th --min-size=64 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    ./proxy_tc --iterations=$iterations --threads=$th --min-size=64 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    ./proxy_je --iterations=$iterations --threads=$th --min-size=64 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je 


echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

echo ""
echo "ops/sec for 64-1024 byte objects"
for th in {0..12}; do
    echo $th >> $temp_th
    ./proxy_libc --iterations=$iterations --threads=$th --min-size=64 --max-size=1024 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    ./proxy_tc --iterations=$iterations --threads=$th --min-size=64 --max-size=1024 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    ./proxy_je --iterations=$iterations --threads=$th --min-size=64 --max-size=1024 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je 


echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

for th in {0..12}; do
    echo $th >> $temp_th
    ./proxy_libc --iterations=$iterations --threads=$th --min-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    ./proxy_tc --iterations=$iterations --threads=$th --min-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    ./proxy_je --iterations=$iterations --threads=$th --min-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

echo ""
echo "results for 64 KB objects"
paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je 


echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

for th in {0..12}; do
    echo $th >> $temp_th
    ./proxy_libc --iterations=$iterations --threads=$th --min-size=1024 --max-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    ./proxy_tc --iterations=$iterations --threads=$th --min-size=1024 --max-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    ./proxy_je --iterations=$iterations --threads=$th --min-size=1024 --max-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

echo ""
echo "results for 1-64 KB objects"
paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je 

echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

for th in {0..12}; do
    echo $th >> $temp_th
    ./proxy_libc --iterations=$iterations --threads=$th --min-size=1048576 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    ./proxy_tc --iterations=$iterations --threads=$th --min-size=1048576 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    ./proxy_je --iterations=$iterations --threads=$th --min-size=1048576 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

echo ""
echo "results for 1 MB objects"
paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je 

# cleanup
rm ${temp_th}
rm ${temp_file_libc}
rm ${temp_file_tc}
rm ${temp_file_je}
