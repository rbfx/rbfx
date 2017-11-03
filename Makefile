container="rokups/crossbuildurho:latest"

all: cross-osx cross-linux

prepare:
	mkdir -p /tmp/Urho3D-crossbuild

cross-osx: prepare
	docker run -it --rm -v $(shell pwd):/workdir -v /tmp/Urho3D-crossbuild:/tmp/Urho3D-crossbuild -e CROSS_TRIPLE=osx   $(container) ./.crossbuild

cross-linux: prepare
	docker run -it --rm -v $(shell pwd):/workdir -v /tmp/Urho3D-crossbuild:/tmp/Urho3D-crossbuild -e CROSS_TRIPLE=linux $(container) ./.crossbuild

clean:
	rm -rf /tmp/Urho3D-crossbuild
