#!/bin/bash

dirpath="$(cd "$(dirname "$0")" && pwd)"
cd $dirpath

# kill crypto process
crypto_pro_num=`ps -ef  | grep /ppc/scripts | grep j- | grep -v 'grep' | awk '{print $2}' | wc -l`
for i in $( seq 1 $crypto_pro_num )
do
  crypto_pid=`ps -ef  | grep /ppc/scripts | grep j- | grep -v 'grep' | awk '{print $2}' | awk 'NR==1{print}'`
  kill -9 $crypto_pid
done

sleep 1

nohup python ppc_model_app.py > start.out 2>&1 &

check_service() {
    try_times=5
    i=0
    while [ -z `ps -ef | grep ${1} | grep python | grep -v grep | awk '{print $2}'` ]; do
        sleep 1
        ((i = i + 1))
        if [ $i -lt ${try_times} ]; then
            echo -e "\033[32m.\033[0m\c"
        else
            echo -e "\033[31m\nServer ${1} isn't running. \033[0m"
            return
        fi
    done

    echo -e "\033[32mServer ${1} started \033[0m"
}

sleep 5
check_service ppc_model_app.py