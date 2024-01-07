#!/bin/bash
SCRIPT_DIR=$(dirname $0)

GREEN='\e[1;32m'
BLUE='\e[34m'
BOLD_BLUE='\e[1;34m'
RED='\e[1;31m'
RESET='\e[0m'

TIME_FORMAT='%U %S %e %P'

trap ctrl_c INT
function ctrl_c() {
    tester_log "Caught SIGINT, exiting..."
    pkill -f "bin/original"
    pkill -f "bin/solution"
    pkill -f "bin/timechecker"
    exit 1
}

function center() {
    termwidth="$(tput cols)"
    padding="$(printf '%0.1s' ={1..500})"
    printf "$BLUE%*.*s %s %*.*s\n\e[0m" 0 "$(((termwidth - 2 - ${#1}) / 2))" "$padding" "$1" 0 "$(((termwidth - 1 - ${#1}) / 2))" "$padding""$RESET"
}

function tester_log() {
    echo -e "$BOLD_BLUE[TESTER] $1$RESET"
}

function deploy_tester() {
    cd $SCRIPT_DIR
    center "Deploying tester"
    zip -r tester.zip * -x "bin/*" "reports/*" "tmp/*" "solution/*"
    center "Done - tester.zip created"
}

function prepare_cpp_files() {
    cd $SCRIPT_DIR
    center "CPP files preparation"
    rm -rf tmp
    mkdir -p tmp/original/{include,src} tmp/solution/{include,src}
    local original_tmp_dir="tmp/original"
    local solution_tmp_dir="tmp/solution"

    awk -f scripts/script.awk src/Simulation.cpp >"$original_tmp_dir/src/Simulation.cpp"
    find include/ -type f -exec cp {} $original_tmp_dir/include \;
    find src/ -type f -not -name "Simulation.cpp" -exec cp {} $original_tmp_dir/src \;
    tester_log "Original files prepared"

    awk -f scripts/script.awk solution/Simulation.cpp >"$solution_tmp_dir/src/Simulation.cpp"
    find include/ -type f -not -name "Simulation.h" -exec cp {} $solution_tmp_dir/include \;
    find src/ -type f -not -name "Simulation.cpp" -exec cp {} $solution_tmp_dir/src \;
    if [ -f "solution/Simulation.h" ]; then
        cp include/Simulation.h "$solution_tmp_dir/include/Simulation.h"
    fi
    tester_log "Solution files prepared"

    tester_log "Formatting prepared files"
    find tmp/ -type f -name "*.cpp" -name ".h" -exec clang-format -i {} \;
}

function format() {
    cd $SCRIPT_DIR
    tester_log "Formatting..."
    find ./src/ -type f -name "*.cpp" -exec clang-format --verbose -i {} \;
    find ./include/ -type f -name "*.h" -exec clang-format --verbose -i {} \;
    find ./solution/ -type f -name "*.cpp" -exec clang-format --verbose -i {} \;
}

function compile {
    cd $SCRIPT_DIR
    if [ ! -d "bin" ]; then
        mkdir "bin"
    fi

    center "Compiling"
    g++ -std=c++17 -O2 -fopenmp tmp/original/src/*.cpp Tester.cpp -I tmp/original/include -o bin/original
    tester_log "bin/original created"

    g++ -std=c++17 -O2 -fopenmp tmp/solution/src/*.cpp Tester.cpp -I tmp/solution/include -o bin/solution
    tester_log "bin/solution created"

    g++ -std=c++17 helpers/TimeChecker.cpp -o bin/timechecker
    tester_log "bin/timechecker created"
}

function check_files {
    tester_log "Diffing $1 and $2"
    diff $1 $2
    if [ $? -eq 0 ]; then
        tester_log "$GREEN""PASSED""$RESET"
        return 0
    fi
    tester_log "$RED""FAILED""$RESET"
    return 1
}

function run_time_checker() {
    tester_log "Running time checker"
    ./bin/timechecker $1 $2
}

function wrapped_call {
    local cmd=$1
    local conf=$2
    local type=$3
    local time_file=$4
    local log_file=$5
    tester_log "Running $cmd"
    /usr/bin/time -o $time_file -f "$TIME_FORMAT" $cmd $conf $type 2>>$log_file
    cat $time_file >>$log_file
    rm $time_file
}

function run {
    if [[ $1 != "" ]]; then
        conf_files=""
        for conf in $@; do
            conf_files="$conf_files configurations/$conf"
        done
    else
        conf_files=$(find configurations -name "*.conf")
    fi
    center "Configuration files"
    for conf in $conf_files; do
        tester_log "\e[3m$conf"
    done
    if [ -d "reports" ]; then
        rm -rf reports
    fi
    mkdir -p reports/results
    tests_passed=0
    tests_run=0
    summaries=()

    for conf in $conf_files; do
        center "Running $conf"
        filename=$(basename -- "$conf" --suffix=".conf")
        filename="${filename%.*}"
        local original_file="reports/results/"$filename"_original"
        wrapped_call ./bin/original $conf original $original_file"_time.txt" $original_file.txt
        for i in 2 4 6; do
            export OMP_NUM_THREADS=$i
            local solution_file="reports/results/"$filename"_solution_$i"
            center "[$conf] Running solution with $i threads"
            wrapped_call ./bin/solution $conf solution $solution_file"_time.txt" $solution_file.txt
            check_files reports/original/$filename.txt reports/solution/$filename.txt
            if [ $? -eq 0 ]; then
                tests_passed=$((tests_passed + 1))
                run_time_checker $original_file.txt $solution_file.txt
            else
                tester_log "Skipping time checker"
            fi
            tests_run=$((tests_run + 1))
        done
        center "Done $conf"
    done
    center "Summary"
    echo -e "$GREEN""$tests_passed""$RESET"" out of ""$tests_run"" tests passed"
    if [ $tests_passed -eq $tests_run ]; then
        echo -e "$GREEN""PASSED""$RESET"
    else
        echo -e "$RED""FAILED""$RESET"
    fi
}

if [ "$1" == "--deploy" ]; then
    deploy_tester
    exit 0
fi

conf_list=""
for conf in $@; do
    conf_list="$conf_list $conf"
done

format
prepare_cpp_files
compile
run $conf_list
