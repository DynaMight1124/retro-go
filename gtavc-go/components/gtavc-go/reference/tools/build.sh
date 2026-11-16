#!/bin/bash
# Build pipeline: compile -> CLDC preverify -> (package is done by package.py)
set -e
P="$(cd "$(dirname "$0")/.." && pwd)"
JAVAC="java --module jdk.compiler/com.sun.tools.javac.Main"
EXP="--add-exports java.base/jdk.internal.org.objectweb.asm=ALL-UNNAMED"
cd $P
mkdir -p build/tools build/shim build/raw build/cldc
if [ ! -f build/tools/Preverify.class ] || [ tools/Preverify.java -nt build/tools/Preverify.class ]; then
  $JAVAC $EXP -Xlint:-options -d build/tools tools/Preverify.java
fi
if [ ! -f build/shim/.done ]; then
  $JAVAC -Xlint:-options -d build/shim $(find tools/shim -name '*.java') && touch build/shim/.done
fi
rm -rf build/raw build/cldc; mkdir -p build/raw build/cldc
$JAVAC -source 8 -target 8 -Xlint:-options -nowarn -cp build/shim -d build/raw $(find src -name '*.java')
java $EXP -cp build/tools Preverify build/raw build/cldc build/shim
