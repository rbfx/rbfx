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
