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

ppc_model_app_pid=`ps aux |grep ppc_model_app.py |grep -v grep |awk '{print $2}'`
kill -9 $ppc_model_app_pid

echo -e "\033[32mServer ppc_model_app.py killed. \033[0m"
