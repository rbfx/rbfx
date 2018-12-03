
rm -r -f build

cmake -Bbuild -H. -DURHO3D_WIN32_CONSOLE=ON \
-DURHO3D_RENDERER=OpenGL \
-DURHO3D_ENABLE_ALL=ON \
-DURHO3D_SSE=ON \
-DURHO3D_PROFILING=OFF \
-DBUILD_SHARED_LIBS=ON \
-G "Visual Studio 15 2017 Win64" \
\
\
\
-DURHO3D_CSHARP=OFF #\
# \
# -DLLVM_VERSION_EXPLICIT=5.0.1 \
# -DLIBCLANG_LIBRARY="C:/Program Files/LLVM/lib/libclang.lib" \
# -DLIBCLANG_INCLUDE_DIR="C:/Program Files/LLVM/include" \
# -DLIBCLANG_SYSTEM_INCLUDE_DIR="C:/Program Files/LLVM/lib/clang/5.0.1/include" \
# -DCLANG_BINARY="C:/Program Files/LLVM/bin/clang++.exe"