#!/bin/sh
function dependency()
{
    echo "begin make dependency......"
    build_path=$(cd `dirname $0`; pwd)
    echo "build path: "${build_path}
    #需添加下面的环境变量，否则编译的时候有些依赖库会指向lib2而非lib2-64，导致编译通不过
    export MAC=64
    python ${build_path}/codemake/codemake.py -ub
    echo "end make dependency......"
}

function build()
{
    #编译网络库
    echo "begin make......"
    cd ./common-net-app-lib
    make || exit 1
    #编译producer
    cd ../producer
    make || exit 1
    #编译consumer
    cd ../consumer
    make || exit 1
    cd ..
    rm -rf output
    mkdir -p output
    cp -r producer/mqserver_producer output/
    cp -r consumer/mqserver_consumer output/
    cp -r conf output/
    echo "end make......"
}

function clean()
{
    #clean网络库
    echo "begin make clean......"
    cd ./common-net-app-lib
    make clean
    #clean producer
    cd ../producer
    make clean
    #clean consumer
    cd ../consumer
    make clean
    cd ..
    rm -rf output
    echo "end make clean......"
}

if [ "$1" == "all" ]
then
    dependency
    build
elif [ "$1" == "dependency" ]
then
    dependency
elif [ "$1" == "clean" ]
then
    clean
else
    build
fi
