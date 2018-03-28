[![Build Status](https://travis-ci.org/bobrofon/kenobi.svg?branch=master)](https://travis-ci.org/bobrofon/kenobi)
[![license](https://img.shields.io/github/license/mashape/apistatus.svg?maxAge=2592000)](https://github.com/bobrofon/kenobi/blob/master/LICENSE)
# kenobi
Kenobi is a PoC of the processes invisible for system administration tools.

## Caveat about `ptrace()`

* On many Linux distributions, the kernel is configured by default to prevent any process from calling `ptrace()` on another process that it did not create (e.g. via `fork()`).

* This is a security feature meant to prevent exactly the kind of mischief that this tool causes.

* You can temporarily disable it until the next reboot using the following command:

        echo 0 | sudo tee /proc/sys/kernel/yama/ptrace_scope
        
## Usage
Just run `kenobi` to make all processes with name 'kenobi' invisible for system administration tools.

## Compiling
    mkdir -p build
    cd build
    cmake ..
    make

## Notes
* only x86_64 support
