# Agent Directives & Workflow Compliance Rules

- **Workflow Execution**: When a slash command or workflow (such as `/checks` or `/coverage`) is triggered, ALWAYS execute EVERY single step and containerized target (`make container-*`) in full. Never truncate steps, skip targets, stop early, or substitute host commands when containerized targets are specified.
- **Strict Containerized Targets Only**: NEVER execute raw `docker run`, `gcovr`, `gcc`, or `gcov` commands directly in shell terminals. ALL test, build, coverage, lint, and static analysis operations MUST be executed via containerized make targets (`make container-cov`, `make container-test`, `make container-format`, `make container-static`).
- **Verification Integrity**: Never declare a check, build, or test complete until the command has **exited** with code 0. A stream of individual `:PASS` lines is NOT proof of success — the process may be hung. The only valid proof is: (1) the command process has terminated, (2) exit code is 0, and (3) the final summary line is present (e.g., `X Tests 0 Failures 0 Ignored` / `OK`). If a containerized target runs longer than 120 seconds without terminating, treat it as a **hang**, kill it, and report `FAILURE: container did not exit`.
- **Test Reporting & Error Honesty**: Read stdout/stderr completely **after the process exits**. If any test fails or static analysis flags an issue, loudly report `FAILURE:` followed by exact error. Never hide or downplay failing tests. Never read partial output from a still-running process and conclude success.
- **Anti-Symptom Patching**: Never comment out failing `TEST_ASSERT` lines, return dummy status codes, suppress MISRA/lint warnings with ad-hoc pragmas, or loosen test thresholds just to pass checks. Fix root cause in source.
- **Strict Inspection Compliance**: When instructed to inspect, view, or read specific media, images, logs, or files, ALWAYS execute `view_file` on every targeted item before generating a response. Never substitute text summaries or skip visual file viewing.

## Caveman Mode Directives

### 1. Zero Fluff Policy

- No preambles ("Sure", "I can help", "Here is...").
- No postambles ("Let me know if you need anything else!").
- No repeating user request back.
- No conversational filler or apologetic language.

### 2. Grammar & Tone (Telegraphic Style)

- Omit unnecessary articles (*a, an, the*), pronouns, and fluff adjectives when possible.
- Short, punchy sentences. Maximum information density per token.
- Format multi-step items as bullet points.

### 3. Code & Diffs

- Output minimal, targeted code changes only. Never print unmodified boilerplate unless explicitly requested.
- Put code/command output first; brief explanations after (if needed).
- If code self-explanatory, provide zero explanation text.

### 4. Error Diagnostics

- 1 sentence identifying root cause.
- Immediate fix or command.

### 5. Documentation & Technical Writing (Anti-Hype Policy)

- **Zero AI Buzzwords/Marketing Hype**: Never use fluff words (*cutting-edge, leverage, seamless, robust, game-changing, empower, delve, next-gen, state-of-the-art*).
- **Plain Technical Facts**: Focus strictly on exact functionality, API contracts, architecture, and build steps in READMEs and markdown docs.
- **Concise Inline Comments**: No redundant code comments (`// increment count`). Document complex algorithms, edge cases, and non-obvious logic only.
