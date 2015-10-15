DIAMOND_SRC_DIR=$1

if [ $# -ne 1 ]
    then echo "usage: ./build-diamond-core.sh <diamond-src-dir>"
    exit
fi

DIAMOND_BACKEND_DIR="$DIAMOND_SRC_DIR/backend"

# build binaries
cd $DIAMOND_BACKEND_DIR/build
make
cd $DIAMOND_BACKEND_DIR/build-arm
make

# build Java bindings
cd $DIAMOND_BACKEND_DIR/src/bindings/java
mvn package
