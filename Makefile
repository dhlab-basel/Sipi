.PHONY: docs-build
docs-build: ## build docs into the local 'site' folder
	mkdocs build

.PHONY: docs-serve
docs-serve: ## serve docs for local viewing
	mkdocs serve

.PHONY: docs-publish
docs-publish: ## build and publish docs to Github Pages
	mkdocs gh-deploy

.PHONY: docs-install-requirements
docs-install-requirements: ## install requirements for documentation
	pip3 install -r requirements.txt

.PHONY: compile
compile: ## compile SIPI inside Docker
	docker run -it --rm -v ${PWD}:/sipi dhlabbasel/sipi-base:18.04 /bin/sh -c "mkdir -p /sipi/build-linux && cd /sipi/build-linux && cmake .. && make"

.PHONY: test
test: ## compile and run tests inside Docker
	@mkdir -p ${PWD}/images
	docker run -it --rm -v ${PWD}:/sipi dhlabbasel/sipi-base:18.04 /bin/sh -c "mkdir -p /sipi/build-linux && cd /sipi/build-linux && cmake .. && make && ctest --verbose"

.PHONY: run
run: ## run SIPI inside Docker (does not compile)
	@mkdir -p ${PWD}/images
	docker run -it --rm -v ${PWD}:/sipi --workdir "/sipi" --expose "1024" --expose "1025" -p 1024:1024 -p 1025:1025 dhlabbasel/sipi-base:18.04 /bin/sh -c "/sipi/build-linux/sipi --config=/sipi/config/sipi.config.lua"

.PHONY: clean
clean: ## cleans the project directory
	@rm -rf build-linux/
	@rm -rf site/

.PHONY: help
help: ## this help
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST) | sort

.DEFAULT_GOAL := help
