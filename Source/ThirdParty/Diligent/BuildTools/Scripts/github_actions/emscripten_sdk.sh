# $1 - runner workspace

cd "$1"

EMSCRIPTEN_SDK_VERSION="2.0.30"

git clone https://github.com/emscripten-core/emsdk.git
./emsdk/emsdk install $EMSCRIPTEN_SDK_VERSION
./emsdk/emsdk activate $EMSCRIPTEN_SDK_VERSION
source ./emsdk/emsdk_env.sh

echo "EMSCRIPTEN_SDK_VERSION=$EMSCRIPTEN_SDK_VERSION" >> $GITHUB_ENV
echo "PATH=$PATH" >> $GITHUB_ENV
echo "EMSDK=$EMSDK" >> $GITHUB_ENV
echo "EM_CONFIG=$EM_CONFIG" >> $GITHUB_ENV
echo "EMSDK_NODE=$EMSDK_NODE" >> $GITHUB_ENV
echo "EMSDK_PYTHON=$EMSDK_PYTHON" >> $GITHUB_ENV
echo "JAVA_HOME=$JAVA_HOME" >> $GITHUB_ENV
