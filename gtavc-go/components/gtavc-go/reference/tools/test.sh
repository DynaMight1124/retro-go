#!/bin/bash
# usage: tools/test.sh HarnessName [args...]   (compiles harness against raw classes + shim and runs it)
set -e
P="$(cd "$(dirname "$0")/.." && pwd)"
JAVAC="java --module jdk.compiler/com.sun.tools.javac.Main"
H=$1; shift
mkdir -p $P/build/test
$JAVAC -Xlint:-options -nowarn -cp $P/build/cldc:$P/build/shim -d $P/build/test $P/tools/test/$H.java
java -Xverify:all -cp $P/build/test:$P/build/cldc:$P/build/shim:$P/res $H "$@"
