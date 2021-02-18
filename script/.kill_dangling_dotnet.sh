#!/usr/bin/env sh
# "dotnet msbuild" has a bad habit of leaving stale processes running, which prevents
# cmake from completing build. Kill them after build is done and get on with our lives.
pgid=$(ps -o pgid= -p $$)
dotnet=$(realpath $1)
for pid in $(pgrep -g $pgid);
do
    exe_cmd=$(ps awx | awk "\$1 == $pid { print \$5 }")
    if [ "$exe_cmd" != "" ];
    then
        exe_cmd=$(realpath $exe_cmd)
        if [ "$exe_cmd" = "$dotnet" ];
        then
            kill $pid
        fi
    fi
done
