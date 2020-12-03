all: build

build:
	jpm build

test:
	janet ./test.janet

.PHONY: build
