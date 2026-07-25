# SyntropicOS Tools Container Makefile
# Run containerized sanitizers, QEMU ARM emulation, protocol fuzzing, coverage, static analysis, doxygen, and 3rd-party integration.

SHELL := /bin/bash
ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)

.PHONY: help san qemu fuzz cov static dox integration container-build container-san container-qemu container-fuzz container-cov container-static container-dox container-integration

help:
	@echo "SyntropicOS Containerized Execution & Tooling Framework"
	@echo "--------------------------------------------------------"
	@echo "  make san                   Run test suite under ASan + UBSan"
	@echo "  make qemu                  Run QEMU ARM Cortex-M4 bare-metal emulation"
	@echo "  make fuzz                  Run libFuzzer protocol targets"
	@echo "  make cov                   Generate test coverage analysis & HTML report"
	@echo "  make static                Run Cppcheck & Clang scan-build static analysis"
	@echo "  make dox                  Run Doxygen API documentation coverage check"
	@echo "  make integration           Run 3rd-party container integration tests (Mosquitto/Chrony/Modbus)"
	@echo "  make container-build       Build testing container image via Podman/Docker"
	@echo "  make container-san         Execute sanitizer suite inside Podman container"
	@echo "  make container-qemu        Execute QEMU ARM Cortex-M4 inside Podman container"
	@echo "  make container-fuzz        Execute libFuzzer suite inside Podman container"
	@echo "  make container-cov         Execute coverage analysis inside Podman container"
	@echo "  make container-static      Execute static analysis inside Podman container"
	@echo "  make container-dox         Execute doxygen coverage inside Podman container"
	@echo "  make container-integration Execute 3rd-party container integration suite"

test:
	@make -f $(ROOT_DIR)/tests/Makefile.unity test

san:
	@bash $(ROOT_DIR)/tools/containers/run_sanitizers.sh

qemu:
	@bash $(ROOT_DIR)/tests/emulation/run_qemu_arm.sh

renode:
	@bash $(ROOT_DIR)/tests/emulation/renode/run_renode.sh

fuzz:
	@bash $(ROOT_DIR)/tests/fuzz/run_fuzz.sh

cov:
	@bash $(ROOT_DIR)/tools/coverage/run_coverage.sh

static:
	@bash $(ROOT_DIR)/tools/static_analysis/run_static_analysis.sh

dox:
	@bash $(ROOT_DIR)/tools/docs/run_doxygen.sh

integration:
	@bash $(ROOT_DIR)/tests/integration/run_integration.sh

container-build:
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE build -t syntropicos-test -f $(ROOT_DIR)/tools/containers/Containerfile $(ROOT_DIR)

container-san: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers san

container-qemu: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers qemu

container-renode: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers renode

container-fuzz: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers fuzz

container-cov: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers cov

container-static: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers static

container-dox: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers dox

container-integration: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers integration
