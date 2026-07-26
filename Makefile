# SyntropicOS Top-Level Convenience Makefile
# Forwards targets to tools/containers/Makefile

.PHONY: help clean test format lint misra stack size complexity san qemu renode fuzz cov static dox integration container-build container-test container-format container-lint container-misra container-stack container-size container-complexity container-san container-qemu container-renode container-fuzz container-cov container-static container-dox container-integration

clean:
	@rm -rf build/ doxygen_output/ coverage_html/ coverage.info coverage_src.info doxygen_warnings.txt
	@find . \( -name "*.gcda" -o -name "*.gcno" -o -name "*.o" -o -name "*.su" \) -delete

help:
	@make -C tools/containers help

test:
	@make -C tools/containers test

format:
	@make -C tools/containers format

lint:
	@make -C tools/containers lint

misra:
	@make -C tools/containers misra

stack:
	@make -C tools/containers stack

size:
	@make -C tools/containers size

complexity:
	@make -C tools/containers complexity

san:
	@make -C tools/containers san

qemu:
	@make -C tools/containers qemu

renode:
	@make -C tools/containers renode

fuzz:
	@make -C tools/containers fuzz

cov:
	@make -C tools/containers cov

static:
	@make -C tools/containers static

dox:
	@make -C tools/containers dox

integration:
	@make -C tools/containers integration

container-build:
	@make -C tools/containers container-build

container-test:
	@make -C tools/containers container-test

container-format:
	@make -C tools/containers container-format

container-lint:
	@make -C tools/containers container-lint

container-misra:
	@make -C tools/containers container-misra

container-stack:
	@make -C tools/containers container-stack

container-size:
	@make -C tools/containers container-size

container-complexity:
	@make -C tools/containers container-complexity

container-san:
	@make -C tools/containers container-san

container-qemu:
	@make -C tools/containers container-qemu

container-renode:
	@make -C tools/containers container-renode

container-fuzz:
	@make -C tools/containers container-fuzz

container-cov:
	@make -C tools/containers container-cov

container-static:
	@make -C tools/containers container-static

container-dox:
	@make -C tools/containers container-dox

container-pio:
	@make -C tools/containers container-pio

container-integration:
	@make -C tools/containers container-integration
