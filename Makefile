container="rokups/crossbuildurho:latest"
docker_params=-it --rm -v $(shell pwd):/workdir -v /tmp/Urho3D-crossbuild:/tmp/Urho3D-crossbuild $(container) ./.crossbuild

cross: cross-linux cross-linux-static cross-osx cross-osx-static cross-win32 cross-win32-static cross-win64 cross-win64-static

prepare:
	mkdir -p /tmp/Urho3D-crossbuild

cross-linux: prepare
	docker run -e CROSS_TRIPLE=linux -e STATIC=1 $(docker_params)

cross-linux-static: prepare
	docker run -e CROSS_TRIPLE=linux -e SHARED=1 $(docker_params)

cross-osx: prepare
	docker run -e CROSS_TRIPLE=osx   -e STATIC=1 $(docker_params)

cross-osx-static: prepare
	docker run -e CROSS_TRIPLE=osx   -e SHARED=1 $(docker_params)

cross-win32: prepare
	docker run -e CROSS_TRIPLE=win32 -e STATIC=1 $(docker_params)

cross-win32-static: prepare
	docker run -e CROSS_TRIPLE=win32 -e SHARED=1 $(docker_params)

cross-win64: prepare
	docker run -e CROSS_TRIPLE=win64 -e STATIC=1 $(docker_params)

cross-win64-static: prepare
	docker run -e CROSS_TRIPLE=win64 -e SHARED=1 $(docker_params)

cross-win: prepare cross-win32 cross-win32-static cross-win64 cross-win64-static

clean:
	rm -rf /tmp/Urho3D-crossbuild

.PHONY: conf_project_ios
conf_project_ios:
	mkdir -p buildios && \
	cd buildios && \
	cmake -G Xcode \
	-T buildsystem=12 \
	-DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchains/IOS.cmake \
	-DENABLE_BITCODE=OFF \
	-DPLATFORM=OS64COMBINED \
	-DDEPLOYMENT_TARGET=11.0 \
	-DURHO3D_COMPUTE=OFF \
	-DURHO3D_GRAPHICS_API=GLES2 \
	-DURHO3D_GLOW=OFF \
	-DURHO3D_FEATURES="SYSTEMUI" \
	-DURHO3D_PROFILING=OFF \
	-DURHO3D_PLAYER=OFF \
	-DURHO3D_SAMPLES=ON \
	-DURHO3D_EXTRAS=OFF \
	-DURHO3D_TOOLS=OFF \
	-DURHO3D_RMLUI=ON \
	..
