# SyntropicOS Toolchain Testing Makefile
# Run containerized sanitizers, QEMU ARM emulation, and protocol fuzzing.

SHELL := /bin/bash
ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)

.PHONY: help san qemu fuzz container-build container-san container-qemu container-fuzz

help:
	@echo "SyntropicOS Containerized Testing Framework"
	@echo "------------------------------------------"
	@echo "  make san              Run test suite under ASan + UBSan"
	@echo "  make qemu             Run QEMU ARM Cortex-M4 bare-metal emulation"
	@echo "  make fuzz             Run libFuzzer protocol targets"
	@echo "  make container-build  Build testing container image via Podman/Docker"
	@echo "  make container-san    Execute sanitizer suite inside Podman container"
	@echo "  make container-qemu   Execute QEMU ARM Cortex-M4 inside Podman container"
	@echo "  make container-fuzz   Execute libFuzzer suite inside Podman container"

san:
	@bash $(ROOT_DIR)/toolchain/testing/scripts/run_sanitizers.sh

qemu:
	@bash $(ROOT_DIR)/toolchain/testing/scripts/run_qemu_arm.sh

fuzz:
	@bash $(ROOT_DIR)/toolchain/testing/scripts/run_fuzz.sh

container-build:
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE build -t syntropicos-test -f $(ROOT_DIR)/toolchain/testing/Containerfile $(ROOT_DIR)

container-san: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C toolchain/testing san

container-qemu: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C toolchain/testing qemu

container-fuzz: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C toolchain/testing fuzz
