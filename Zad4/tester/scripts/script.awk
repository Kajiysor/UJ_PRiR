#!/usr/bin/awk -f

# AUTHOR: @ffffffffffffffff0

BEGIN {
    CHECK=0
    RETURN=0
    CHRONO=0    
    FUNCTION=""
}

{
    if ( $0 ~ /#include .*/ ) {
        print $0
        if (CHRONO == 0) {
            print "#include <chrono>"
            print "#include <omp.h>"
            CHRONO=1
        }
    }
    else if ( $0 ~ /(void|double) Simulation::.*/ ) {
        CHECK=1
        RETURN=1
        FUNCTION=$0
        print $0
    }
    else if ( $0 ~ /^\{$/ ) {
        if (CHECK == 1) {
            print $0
            print "\tconst auto start = std::chrono::system_clock::now();"
        }
        else {
            print $0
        }
    }
    else if ( $0 ~ /^\}$/ || $0 ~ /return .*;/ ) {
        if (CHECK == 1 && RETURN == 1) {
            print "#pragma omp barrier"
            print "\tconst auto end = std::chrono::system_clock::now();"
            print "\tconst std::chrono::duration<double> elapsed_seconds = end - start;"
            print "\tfprintf(stderr, \""FUNCTION": %.10fs\\n\", elapsed_seconds.count());"
            print "\tstd::cerr.flush();"
            RETURN=0
        }
        print $0
    }
    else {
        print $0
    }
}

END {

}