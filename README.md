# st_coroutine

将srs的协程框架剥离出来单独使用。

## state-thread
linux-x86_64
> make linux-debug

linux-aarch64
> make linux-debug EXTRA_CFLAGS="-D__aarch64__"

qnx-aarch64
> make qnx-debug EXTRA_CFLAGS="-D__aarch64__"

## eventWork
linux
> cmake -DBUILD_OS_TYPE=linux  .. && make

qnx-aarch64
> cmake -DBUILD_OS_TYPE=qnx  .. && make
