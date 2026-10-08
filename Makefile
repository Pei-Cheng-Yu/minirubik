CC ?= cc
CFLAGS ?= -O3 -std=c99 -Wall -Wextra -Wpedantic
FRAMA_C ?= frama-c
CLANG_FORMAT := $(shell command -v clang-format-20 2>/dev/null || \
	command -v clang-format 2>/dev/null)
C_SOURCES := $(wildcard *.c *.h)
SAMPLE_STATE := 21345671111111
SAMPLE_SOLUTION := B' R' D2 R' B R B' R D2 B R'
VECTORS := tests/solutions.txt
IDA_VECTORS := tests/solutions_ida.txt
# One per rejection path: short, long, cubie digit low, cubie digit high,
# orientation digit low, orientation digit high, non-digit, duplicate, parity.
INVALID_STATES := 1234567111111 123456711111111 02345671111111 82345671111111 \
	12345671111110 12345671111114 1234567111111a 11345671111111 12345671111112

.PHONY: all check check-pdb prove clean indent
# The generator runs on the host, not in Ripes.
gen_pdb: gen_pdb.c
	$(CC) $(CFLAGS) -UNDEBUG $< -o $@

pdb4_data.h: gen_pdb
	./gen_pdb > $@.tmp && mv $@.tmp $@

check-pdb: gen_pdb pdb4_data.h
	./gen_pdb --self-test

all: solver mini

solver: solver.c
	$(CC) $(CFLAGS) $< -o $@

mini: mini.c
	$(CC) $(CFLAGS) $< -o $@

check: solver mini $(VECTORS)
	./solver --self-test
	@expected=$$(mktemp); actual=$$(mktemp); \
	trap 'rm -f "$$expected" "$$actual"' 0 1 2 15; \
	for binary in ./solver ./mini; do \
		case "$$binary" in \
			./solver) vectors="$(IDA_VECTORS)" ;; \
			./mini) vectors="$(VECTORS)" ;; \
		esac; \
		count=0; \
		while IFS='|' read -r state solution; do \
			case "$$state" in ""|\#*) continue ;; esac; \
			solution=$$(printf '%s' "$$solution" | tr -d '\r'); \
			printf '%s\n' "$$solution" >"$$expected"; \
			$$binary "$$state" >"$$actual"; \
			status=$$?; \
			test $$status -eq 0 || { \
				echo "$$binary $$state: exit status $$status"; exit 1; }; \
			cmp -s "$$actual" "$$expected" || { \
				echo "$$binary $$state: output mismatch"; \
				echo "  expected: $$solution"; \
				printf '  got:      '; cat "$$actual"; exit 1; }; \
			count=$$((count + 1)); \
		done <"$$vectors"; \
		echo "$$count solution vectors matched by $$binary"; \
	done
	@for binary in ./solver ./mini; do \
		for bad in $(INVALID_STATES); do \
			$$binary "$$bad" >/dev/null 2>&1; \
			status=$$?; \
			test $$status -eq 2 || { \
				echo "$$binary $$bad: expected status 2, got $$status"; exit 1; }; \
		done; \
		$$binary >/dev/null 2>&1; \
		status=$$?; \
		test $$status -eq 2 || { \
			echo "$$binary with no argument: expected status 2, got $$status"; \
			exit 1; }; \
		$$binary $(SAMPLE_STATE) $(SAMPLE_STATE) >/dev/null 2>&1; \
		status=$$?; \
		test $$status -eq 2 || { \
			echo "$$binary with two arguments: expected status 2, got $$status"; \
			exit 1; }; \
		$$binary $(SAMPLE_STATE) >&- 2>/dev/null; \
		status=$$?; \
		test $$status -eq 1 || { \
			echo "$$binary with stdout closed: expected status 1, got $$status"; \
			exit 1; }; \
	done
	@./solver --self-test >&- 2>/dev/null; \
		status=$$?; \
		test $$status -eq 1 || { \
			echo "solver --self-test with stdout closed: expected 1, got $$status"; \
			exit 1; }
	@echo "invalid input rejected with status 2, unwritable stdout with status 1"

prove: solver.c
	@log=$$(mktemp); trap 'rm -f "$$log"' 0 1 2 15; \
		$(FRAMA_C) -wp -wp-fct quarter_turn,rank_state,valid,parse_state \
		-wp-rte -rte-verbose 0 -wp-prover alt-ergo -wp-timeout 20 \
		-wp-cache none solver.c >"$$log" 2>&1; rc=$$?; \
		grep -Fvx -e '[wp] Warning: Skipped RTE guards: unaligned pointers (\aligned not supported)' \
		-e '[wp] Warning: Skipped RTE guards: invalid function pointer calls (\valid_function not supported)' "$$log"; \
		test $$rc -eq 0 && awk '$$1 == "[wp]" && $$2 == "Proved" && $$3 == "goals:" && $$4 > 0 && $$4 == $$6 { ok = 1 } END { exit !ok }' "$$log" && \
		! grep -Eq '(^|[[:space:]])(Timeout|Unknown|Failed):' "$$log"

indent:
ifeq ($(CLANG_FORMAT),)
	$(error clang-format 20 not found)
endif
	@$(CLANG_FORMAT) --version | grep -q 'version 20' || \
		{ echo "error: clang-format version 20 required"; exit 1; }
	$(CLANG_FORMAT) -i $(C_SOURCES)

clean:
	$(RM) solver mini gen_pdb pdb4_data.h pdb4_data.h.tmp
