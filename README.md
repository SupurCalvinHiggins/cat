# cat
🐱‍🏍 A blazingly fast POSIX-ish `cat`!

# Implementation
`cat` is a wrapper around Linux's `sendfile` syscall which performs a file to stdout copy in kernel space. 

# Performance
After 30 warmup iterations, `cat` reads a 100M byte file in ~90ms while `/bin/cat` takes ~150ms on average.

# Portablity
The interface is POSIX-ish and runs correctly on the latest versions of Linux and MacOS although MacOS does not have a `sendfile` equivalent. Windows is not supported.

# Acknowledgements 
This project was inspired by [fcat](https://github.com/mre/fcat/tree/master) which used the `splice` syscall to accelerate `cat`.
