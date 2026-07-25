# SyntropicOS Tools Container Makefile
# Run containerized sanitizers, QEMU ARM emulation, protocol fuzzing, coverage, and static analysis.

SHELL := /bin/bash
ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)

.PHONY: help san qemu fuzz cov static container-build container-san container-qemu container-fuzz container-cov container-static

help:
	@echo "SyntropicOS Containerized Execution & Tooling Framework"
	@echo "--------------------------------------------------------"
	@echo "  make san              Run test suite under ASan + UBSan"
	@echo "  make qemu             Run QEMU ARM Cortex-M4 bare-metal emulation"
	@echo "  make fuzz             Run libFuzzer protocol targets"
	@echo "  make cov              Generate test coverage analysis & HTML report"
	@echo "  make static           Run Cppcheck & Clang scan-build static analysis"
	@echo "  make container-build  Build testing container image via Podman/Docker"
	@echo "  make container-san    Execute sanitizer suite inside Podman container"
	@echo "  make container-qemu   Execute QEMU ARM Cortex-M4 inside Podman container"
	@echo "  make container-fuzz   Execute libFuzzer suite inside Podman container"
	@echo "  make container-cov    Execute coverage analysis inside Podman container"
	@echo "  make container-static Execute static analysis inside Podman container"

san:
	@bash $(ROOT_DIR)/tools/containers/run_sanitizers.sh

qemu:
	@bash $(ROOT_DIR)/tests/emulation/run_qemu_arm.sh

fuzz:
	@bash $(ROOT_DIR)/tests/fuzz/run_fuzz.sh

cov:
	@bash $(ROOT_DIR)/tools/coverage/run_coverage.sh

static:
	@bash $(ROOT_DIR)/tools/static_analysis/run_static_analysis.sh

container-build:
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE build -t syntropicos-test -f $(ROOT_DIR)/tools/containers/Containerfile $(ROOT_DIR)

container-san: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers san

container-qemu: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers qemu

container-fuzz: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers fuzz

container-cov: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers cov

container-static: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers static
