#!/bin/sh
# Teste la vraie lib avec des programmes reels

LIB="$(pwd)/libmalloc.so"
FAIL=0

run()
{
    printf "%-35s" "$*"
    if LD_PRELOAD="$LIB" "$@" > /dev/null 2>&1; then
        echo "[ OK ]"
    else
        echo "[FAIL]"
        FAIL=1
    fi
}

[ -f "$LIB" ] || { echo "libmalloc.so introuvable (make library)"; exit 1; }

run ls
run ls -la /usr
run find libmalloc.so
run tree
run od libmalloc.so
run git status
run less Makefile
run factor 20 30 40 50 60 70 80 90
run sort /etc/passwd
run cat Makefile
run ip a
run sh -c 'echo hello | tr a-z A-Z'
run find /usr/include -maxdepth 1

if [ -x ./tests/speed ]; then
    run ./tests/speed
fi
if [ -x ./tests/corruptionproof ]; then
    run ./tests/corruptionproof
fi
if [ -x ./tests/memoryfootprint ]; then
    run ./tests/memoryfootprint
fi

exit $FAIL
