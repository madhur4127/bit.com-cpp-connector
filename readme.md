# Introduction

# Build

```shell
git clone https://github.com/madhur4127/bit.com-cpp-connector.git
cd bit.com-cpp-connector
git submodule update --init
mkdir build
cd build
CC=gcc-11 CXX=g++-11 BOOST_ROOT=/opt/boost/1.78.0 cmake  ../
make -j 4
./assignment BTC-USDT
```