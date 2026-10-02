# InkPy agent workflow

## Scope and authority
InkPy targets only Xteink X4 Pro. Read README.md for product scope and docs/CHECKPOINT.md for current state before work. Implement one bounded stage per user continuation, then commit progress and pause. Do not implement the whole roadmap in one run. Planning, research, and unsuccessful experiments with useful findings are valid checkpoints.

## Efficient execution
- Start with git status and the checkpoint; inspect only files relevant to the current stage.
- Reuse recorded findings; re-read sources only for missing detail, changed versions, or verification needed by the current decision.
- Batch independent reads, searches, and checks. Keep dependent edits and mutations sequential.
- Prefer targeted searches and compact output over whole-repository dumps.
- Avoid speculative scaffolding, broad refactors, repeated verification, and unrequested features.
- Keep one concise current checkpoint, not duplicated plans in many files.
- Do not delegate to subagents unless the user explicitly requests it.
- Optimize for reliable completion per stage, not simply the fewest lines of code.

## Prototype pace
Prioritize a working main path and a version the user can explore. Keep stages
small, roughly the Stage 5 effort. Do basic compile/run/visual checks; postpone
broad edge-case suites, polish, exhaustive measurements and optimization unless
a concrete failure blocks use. Preserve bounded-file memory and source files.

## Implementation direction
C-first application on pinned ESP-IDF; small C++ dependencies remain acceptable where they materially reduce porting work. Avoid inheritance-heavy frameworks, hidden state, and unnecessary generic abstraction. Use explicit memory ownership and small cohesive interfaces.
Reuse proven components, strip unused application/device functionality, and preserve required licence notices. Do not rewrite mature internals solely for stylistic consistency.
Use bounded memory for document processing, editing, caches, and Python output. File size must not directly determine RAM consumption.

## Checkpoint protocol
At each stage boundary, update docs/CHECKPOINT.md with:
- completed deliverables and relevant paths;
- decisions and concise rationale, with proposals distinguished from confirmed requirements;
- source URLs and pinned revisions when inspected;
- validation performed and its limits;
- unresolved risks and the exact next bounded task.
Save useful conclusions and implementation notes, not private deliberation transcripts.
Commit the scoped changes and push to GitHub when authorized (the user has requested progress commits). Do not force-push or overwrite unrelated changes. Report the actual commit and pause; if pushing fails, state that clearly.
Physical-device results must be supplied or observed; never equate a desktop preview or successful build with hardware validation.
