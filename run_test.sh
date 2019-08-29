#!/bin/bash

temp_th=$(mktemp)
temp_file_libc=$(mktemp)
temp_file_tc=$(mktemp)
temp_file_je=$(mktemp)

iterations=1000000
NPROC=$(($(nproc)/4))

echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

echo "ops/sec for 64 byte objects"
for th in $(eval echo "{0..$NPROC}"); do
    echo $th >> $temp_th
    numactl --cpunodebind=0 ./proxy_libc --iterations=$iterations --threads=$th --min-size=64 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    numactl --cpunodebind=0 ./proxy_tc --iterations=$iterations --threads=$th --min-size=64 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    numactl --cpunodebind=0 ./proxy_je --iterations=$iterations --threads=$th --min-size=64 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je | column -t -R 1,2,3,4


echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

echo ""
echo "ops/sec for 64-1024 byte objects"
for th in $(eval echo "{0..$NPROC}"); do
    echo $th >> $temp_th
    numactl --cpunodebind=0 ./proxy_libc --iterations=$iterations --threads=$th --min-size=64 --max-size=1024 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    numactl --cpunodebind=0 ./proxy_tc --iterations=$iterations --threads=$th --min-size=64 --max-size=1024 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    numactl --cpunodebind=0 ./proxy_je --iterations=$iterations --threads=$th --min-size=64 --max-size=1024 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je | column -t -R 1,2,3,4


echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

for th in $(eval echo "{0..$NPROC}"); do
    echo $th >> $temp_th
    numactl --cpunodebind=0 ./proxy_libc --iterations=$iterations --threads=$th --min-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    numactl --cpunodebind=0 ./proxy_tc --iterations=$iterations --threads=$th --min-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    numactl --cpunodebind=0 ./proxy_je --iterations=$iterations --threads=$th --min-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

echo ""
echo "results for 64 KB objects"
paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je | column -t -R 1,2,3,4


echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

for th in $(eval echo "{0..$NPROC}"); do
    echo $th >> $temp_th
    numactl --cpunodebind=0 ./proxy_libc --iterations=$iterations --threads=$th --min-size=1024 --max-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    numactl --cpunodebind=0 ./proxy_tc --iterations=$iterations --threads=$th --min-size=1024 --max-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    numactl --cpunodebind=0 ./proxy_je --iterations=$iterations --threads=$th --min-size=1024 --max-size=65536 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

echo ""
echo "results for 1-64 KB objects"
paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je | column -t -R 1,2,3,4

echo "#th" > $temp_th
echo "libc" > $temp_file_libc
echo "tcmalloc" > $temp_file_tc
echo "jemalloc" > $temp_file_je

for th in $(eval echo "{0..$NPROC}"); do
    echo $th >> $temp_th
    numactl --cpunodebind=0 ./proxy_libc --iterations=$iterations --threads=$th --min-size=1048576 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_libc
    numactl --cpunodebind=0 ./proxy_tc --iterations=$iterations --threads=$th --min-size=1048576 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_tc
    numactl --cpunodebind=0 ./proxy_je --iterations=$iterations --threads=$th --min-size=1048576 | grep "ops per second" | cut -d ':' -f 2 >> $temp_file_je
done

echo ""
echo "results for 1 MB objects"
paste $temp_th $temp_file_libc $temp_file_tc $temp_file_je | column -t -R 1,2,3,4

# cleanup
rm ${temp_th}
rm ${temp_file_libc}
rm ${temp_file_tc}
rm ${temp_file_je}
