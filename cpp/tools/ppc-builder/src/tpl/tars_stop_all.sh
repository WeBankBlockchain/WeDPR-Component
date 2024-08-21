#!/bin/bash
dirpath="$(cd "$(dirname "$0")" && pwd)"
cd "${dirpath}"

dirs=($(ls -l ${dirpath} | awk '/^d/ {print $NF}'))
for dir in ${dirs[*]}
do
    if [[ -f "${dirpath}/${dir}/stop.sh" ]];then
        echo "try to stop ${dirpath}/${dir}"
        bash ${dirpath}/${dir}/stop.sh
    fi
done
wait

