#!/bin/bash

DIAMOND_SRC="/home/nl35/research/diamond-src"
PROJECT_DIR="$DIAMOND_SRC/apps/chat/DesktopChat"
JAVA_BINDINGS_DIR="$DIAMOND_SRC/backend/src/bindings/java"

if [ ! -d $DIAMOND_SRC ]
then
    echo "Error: DIAMOND_SRC directory does not exist"
    exit
fi

if [ ! -e "$PROJECT_DIR/bin/Main.class" ]
then
    echo "Error: DesktopChat has not been compiled"
    exit
fi

classpath="$PROJECT_DIR/bin:$JAVA_BINDINGS_DIR/libs/javacpp.jar:$JAVA_BINDINGS_DIR/target/diamond-1.0-SNAPSHOT.jar"
nativePath="$JAVA_BINDINGS_DIR/target/classes/x86-lib:$DIAMOND_SRC/backend/build"

export LD_LIBRARY_PATH=$nativePath
java -cp $classpath -Djava.library.path=$nativePath Main $1 $2 $3 $4 $5