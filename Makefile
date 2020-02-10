# Determine this makefile's path.
# Be sure to place this BEFORE `include` directives, if any.
THIS_FILE := $(lastword $(MAKEFILE_LIST))
CURRENT_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

include vars.mk

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

.PHONY: build-sipi-image
build-sipi-image: ## build sipi docker image
	docker build -t $(SIPI_IMAGE) -t $(SIPI_REPO):latest .

.PHONY: publish-sipi-image
publish-sipi-image: build-sipi-image ## publish sipi docker image to Dockerhub
	docker push $(SIPI_REPO)

.PHONY: test
test: ## compile and run tests inside Docker
	docker run -it --rm -v $(CURRENT_DIR):/sipi dhlabbasel/sipi-base:18.04 /bin/sh -c "mkdir -p /sipi/build-linux && cd /sipi/build-linux && cmake .. && make && ctest --verbose"

.PHONY: clean
clean: ## cleans the project directory
	@rm -rf site/
	@rm -rf build-linux/

.PHONY: help
help: ## this help
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z_-]+:.*?## / {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}' $(MAKEFILE_LIST) | sort

.DEFAULT_GOAL := help
