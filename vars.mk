SIPI_REPO := daschswiss/sipi


ifeq ($(BUILD_TAG),)
  BUILD_TAG := $(shell git describe --tag --dirty --abbrev=7)
endif
ifeq ($(BUILD_TAG),)
  BUILD_TAG := $(shell git rev-parse --verify HEAD)
endif

ifeq ($(SIPI_IMAGE),)
  SIPI_IMAGE := $(SIPI_REPO):$(BUILD_TAG)
endif