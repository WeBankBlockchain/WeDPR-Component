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

function check_python() {
    # check python
    if ! command -v python >/dev/null 2>&1; then
        echo "Python is not installed."
        exit 1
    fi

    # check python version, 3.9 support
    python_version=$(python --version)
    if [[ $python_version =~ Python\ 3.9.* ]]; then
        echo "Python version is 3.9, as required."
    else
        echo "Python version is not 3.9. Installed version is: $python_version"
        exit 2
    fi
}

# source venv/bin/activate

pid=$(ps aux | grep $dirpath/ppc_scheduler/ppc_scheduler_app.py | grep -v grep | awk '{print $2}')
if [ ! -z ${pid} ]; then
    echo " the scheduler has started, pid is ${pid}."
    exit 0
fi

nohup python $dirpath/ppc_scheduler/ppc_scheduler_app.py --config $dirpath/conf/application.yml --log_config $dirpath/conf/logging.conf >/dev/null 2>&1 &

sleep 3

pid=$(ps aux | grep $dirpath/ppc_scheduler/ppc_scheduler_app.py | grep -v grep | awk '{print $2}')
if [ ! -z ${pid} ]; then
    echo " start scheduler successfully, pid is ${pid}."
    exit 0
else
    echo " start scheduler failed."
    exit 1
fi
