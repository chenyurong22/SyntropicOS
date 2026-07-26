# SyntropicOS Tools Container Makefile
# Run containerized sanitizers, QEMU ARM emulation, protocol fuzzing, coverage, static analysis, doxygen, and 3rd-party integration.

SHELL := /bin/bash
ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)

.PHONY: help san qemu renode fuzz cov static dox integration format lint misra container-build container-san container-qemu container-renode container-fuzz container-cov container-static container-dox container-integration container-format container-lint container-misra

help:
	@echo "SyntropicOS Containerized Execution & Tooling Framework"
	@echo "--------------------------------------------------------"
	@echo "  make format                Check code formatting via clang-format"
	@echo "  make lint                  Run AST code quality linter via clang-tidy"
	@echo "  make misra                 Run MISRA C:2023 safety compliance scan"
	@echo "  make san                   Run test suite under ASan + UBSan"
	@echo "  make qemu                  Run QEMU ARM Cortex-M4 bare-metal emulation"
	@echo "  make renode                Run Renode STM32F4 board hardware emulation"
	@echo "  make fuzz                  Run libFuzzer protocol targets"
	@echo "  make cov                   Generate test coverage analysis & HTML report"
	@echo "  make static                Run Cppcheck & Clang scan-build static analysis"
	@echo "  make dox                   Run Doxygen API documentation coverage check"
	@echo "  make integration           Run 3rd-party container integration tests"
	@echo "  make container-build       Build testing container image via Podman/Docker"
	@echo "  make container-format      Run clang-format formatting check in container"
	@echo "  make container-lint        Run clang-tidy linter inside container"
	@echo "  make container-misra       Run MISRA C:2023 compliance scan in container"
	@echo "  make container-san         Execute sanitizer suite inside Podman container"
	@echo "  make container-qemu        Execute QEMU ARM Cortex-M4 inside Podman container"
	@echo "  make container-renode      Execute Renode hardware emulation inside container"
	@echo "  make container-fuzz        Execute libFuzzer suite inside Podman container"
	@echo "  make container-cov         Execute coverage analysis inside Podman container"
	@echo "  make container-static      Execute static analysis inside Podman container"
	@echo "  make container-dox         Execute doxygen coverage inside Podman container"
	@echo "  make container-integration Execute 3rd-party container integration suite"

test:
	@make -C $(ROOT_DIR) -f tests/Makefile.unity test

format:
	@bash $(ROOT_DIR)/tools/lint/run_format.sh

lint:
	@bash $(ROOT_DIR)/tools/lint/run_tidy.sh

misra:
	@bash $(ROOT_DIR)/tools/lint/run_misra.sh

stack:
	@bash $(ROOT_DIR)/tools/stack/run_stack_analysis.sh

size:
	@bash $(ROOT_DIR)/tools/size/run_size_analysis.sh

complexity:
	@bash $(ROOT_DIR)/tools/complexity/run_complexity_analysis.sh

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

pio:
	@bash $(ROOT_DIR)/tools/platformio/run_platformio.sh

integration:
	@bash $(ROOT_DIR)/tests/integration/run_integration.sh

container-build:
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE build -t syntropicos-test -f $(ROOT_DIR)/tools/containers/Containerfile $(ROOT_DIR)

container-format: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers format

container-lint: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers lint

container-misra: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers misra

container-stack: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers stack

container-size: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers size

container-complexity: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers complexity

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

container-test: container-cov

container-cov: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers cov

container-static: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers static

container-dox: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers dox

container-pio: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers pio

valgrind:
	@bash $(ROOT_DIR)/tools/valgrind/run_valgrind.sh

container-valgrind: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers valgrind

analyzer:
	@bash $(ROOT_DIR)/tools/analyzer/run_analyzer.sh

container-analyzer: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers analyzer

container-integration: container-build
	@CONTAINER_ENGINE=$$(command -v podman || command -v docker) && \
	$$CONTAINER_ENGINE run --rm -v $(ROOT_DIR):/workspace:z syntropicos-test make -C tools/containers integration
