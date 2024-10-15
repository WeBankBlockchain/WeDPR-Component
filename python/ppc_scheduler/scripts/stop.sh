#!/usr/bin/env bash

dirpath="$(cd "$(dirname "$0")" && pwd)"
cd $dirpath

LANG=zh_CN.UTF-8
##############################################################################
##
##  Wedpr scheduler service start up script for UN*X
##
##############################################################################

# @function: output log with red color (error log)
# @param: content: error message

function LOG_ERROR() {
    local content=${1}
    echo -e "\033[31m"${content}"\033[0m"
}

# @function: output information log
# @param: content: information message
function LOG_INFO() {
    local content=${1}
    echo -e "\033[32m"${content}"\033[0m"
}

function Usage() {
    LOG_INFO "Usage:start the console"
    LOG_INFO "./start_scheduler.sh"
}

pid=$(ps aux | grep $dirpath/ppc_scheduler/ppc_scheduler_app.py | grep -v grep | awk '{print $2}')

if [ ! -z ${pid} ]; then
    kill -9 ${pid}
    echo " scheduler is running, pid is ${pid}, kill it."
    exit 0
else
    echo " scheduler is not running."
    exit 1
fi
