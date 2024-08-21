#!/bin/bash
dirpath="$(cd "$(dirname "$0")" && pwd)"
cd "${dirpath}"

dirs=($(ls -l ${dirpath} | awk '/^d/ {print $NF}'))
for dir in ${dirs[*]}
do
    if [[ -f "${dirpath}/${dir}/start.sh" ]];then
        echo "try to start ${dirpath}/${dir}"
        bash ${dirpath}/${dir}/start.sh &
    fi
done
wait

