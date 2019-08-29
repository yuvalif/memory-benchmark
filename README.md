# Memory Benchmarking
This allows for benchmarking of: libc, [tcmalloc](http://goog-perftools.sourceforge.net/doc/tcmalloc.html) and [jemalloc](http://jemalloc.net/).
- benchmarking is against a multi-threaded workload, where memeory is allocated in one thread and released in another
- different object sizes may be used to create a more realistic memory management scenario
- number of threads is configurable
- no mutexes are used for thread synchronization, to minimize contention overhead
## Prerequisites
- cmake is used
- tcmalloc and jemalloc needs to be installed on the machine. E.g.
```
sudo dnf install jemalloc-devel
sudo dnf install google-perftools
sudo dnf install numactl
```
- argument parsing is done using [Argh](https://github.com/adishavit/argh), which is cloned as a submodule, so don't forget to update it before building:
```
git submodule update --init
```
## Building:
To build, create a build directory. E.g.
```
mkdir build
```
And run cmake and make there. E.g.
```
cd build && cmake .. && make -j4
```
## Running:
Three binaries are generated: ``proxy_libc``, ``proxy_tc``, ``proxy_je``. The all share the same options:
```
$ ./proxy_libc -h
usage: ./proxy_libc[--iterations=<number>|--threads=<number>|--min-size=<bytes>|--max-size=<bytes>|--overall-iterations=<number>]

	iterations: the number of objects each producer is going to create [default 1M]
	threads: the number of consumer threads and number of producer threads. zero means a single thread [default 0]
	min-size: the minimum size in bytes to allocate per object [default to max-size if set, or to 1K if not]
	max-size: the maximum size in bytes to allocate per object [default to min-size if set, or to 1K if not]
	overall-iterations: number of times the test is executed [default 1]
```
To get a comperative report of operations per second for different setups, you can use the ``run_test.sh`` script. Notre that script should be executed from within the ``build`` directory. E.g.
```
cd build && ../run_tests.sh
```

