#!/bin/bash

apt-get update -y
apt-get install wget tar -y

CMAKE_VERSION=$1
wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz -P /opt/
tar -zxf /opt/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz -C /opt/
rm -rf /opt/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz
mv /opt/cmake-${CMAKE_VERSION}-linux-x86_64 /opt/cmake
export PATH=/opt/cmake/bin:$PATH