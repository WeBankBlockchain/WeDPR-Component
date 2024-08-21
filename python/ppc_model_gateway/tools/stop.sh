#!/bin/bash

dirpath="$(cd "$(dirname "$0")" && pwd)"
cd $dirpath

sleep 1

ppc_model_gateway_app_pid=`ps aux |grep ppc_model_gateway_app.py |grep -v grep |awk '{print $2}'`
kill -9 $ppc_model_gateway_app_pid

echo -e "\033[32mServer ppc_model_gateway_app.py killed. \033[0m"
