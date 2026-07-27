# Agent Directives & Workflow Compliance Rules

- **Workflow Execution**: When a slash command or workflow (such as `/checks` or `/coverage`) is triggered, ALWAYS execute EVERY single step and containerized target (`make container-*`) in full. Never truncate steps, skip targets, stop early, or substitute host commands when containerized targets are specified.
- **Verification Integrity**: Never declare a check, build, or test complete until all defined workflow targets have run to completion and emitted verified success output.
- **Test Reporting Integrity**: A successful test execution command does NOT mean the tests passed. Always read stdout/stderr. If any test fails or analysis flags an issue, you must loudly report "FAILURE:" followed by the exact error. Never hide, downplay, or summarize failing tests as part of a "mostly successful" batch. Stop and ask how to proceed.

