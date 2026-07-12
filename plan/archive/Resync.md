# Plan: resync fork history with upstream/master

## Status (2026-07-10): DONE - pushed

Executed: folded `spi` into `reverse` (clean merge, no conflicts,
482/482 tests), squashed to a single commit, rebased that commit onto
`upstream/master` (2 real conflicts resolved: `doc/CHANGELOG.rst` content
merge, and `native/python/pyjp_array.cpp` where upstream's #845 slice-clone
fix collided with this branch's `JPArray(frame, ...)`/`setRange(frame, ...)`
signature change — combined both, plus a follow-up fix for a third call
site using the bare `PyJPArray_Type` symbol instead of `st->PyJPArray_Type`,
caught by a real native rebuild + runtime check, not just `mvn test`).
Squashed again to a single commit on `upstream/master` (`47f97604`),
verified 489/489 passing, force-pushed to `origin/reverse`. Separately,
`origin/master` was fast-forwarded to `upstream/master` (`9b0a4a39`) and
pushed cleanly (strict ancestor, no rewrite needed). Side quest along the
way: reviewed the `overloads` PR (`SIDETRACK.md`/`pr/pr1385`) — see that
branch's commit `6fc84547`, pushed to `origin/overloads` for review.

## Context

Correction from an earlier framing this session: `review` is **not** this
fork's mainline — it's a frozen snapshot branch (`eeefa8dae9`) cut for an
LLNL code review and hasn't moved since. The real fork mainline is
`origin/master` (Thrameos/jpype:master), which is only **13 commits**
behind `upstream/master` (jpype-project/jpype:master). That's the actual
resync target, not `review`.

Branch topology (as of 2026-07-10):

| branch | base commit | own commits | behind origin/master |
|---|---|---|---|
| `origin/master` | — (mainline) | — | 0 (13 behind upstream/master) |
| `reverse` | `ce0566f9` (ancestor of origin/master) | 57 | 10 |
| `spi` | `f519ef37` (ancestor of origin/master) | 120 | 9 |
| `wip` (current HEAD `1f2dabc2`) | same as `spi`'s base | 97 (ancestor of `spi`; `spi` has 23 more) | 9 |
| `review` | `eeefa8da` | 0 (frozen LLNL-review snapshot) | n/a, not part of resync |

So `reverse`, `spi`, and `wip` are three different branches off three
different points on `origin/master`'s own history, each carrying its own
slice of the reverse-bridge (`python.lang`/`python.io`/subinterpreter/SPI)
work. `wip` is a strict ancestor of `spi` — `spi` is currently the most
complete branch.

## What upstream/master has that origin/master doesn't (13 commits)

`git log --oneline origin/master..upstream/master`:

Mostly noise: 2 merge commits, a CI coredump-collection/faulthandler pair
(`b6c4ccec`, `084476df`), a `trigger crash`/`Revert "trigger crash"` pair
(CI test scaffolding, `e23a5e79`/`9b0a4a39`), `cd95751c fix agent.os access
pattern` (CI), `8ea9863a Wont fix #981` (doc-only, `doc/userguide.rst`).

Real code fixes, 3 total:

1. **`a3b1287e` "One line mismatch on calling doc fixed"** —
   `jpype/_jclass.py` (1 line) + changelog. No overlap with any local
   branch's touched files. Safe, trivial.
2. **`21f7b8c8` "Fix for array slice issue"** (closes #845) —
   `native/python/pyjp_array.cpp`, ~28 lines: `JArray(sliced_array)`
   constructor was ignoring slice bounds and copying the full backing
   array instead of just the sliced range. Touches the **forward**
   direction (Python code holding a Java array). Confirmed no overlap
   with our reverse-bridge `332dcb07 Fix subList() slicing bug` (that
   commit touches `jpype/_jbridge.py` and
   `PyTupleNGTest.java` — different direction, different files). Safe to
   merge as-is.
3. **`c3af9487` "Fixes #1379"** — `native/python/pyjp_number.cpp`, 3 call
   sites in `PyJPNumber_create` wrapping `PyLong_FromLongLong`/
   `PyFloat_FromDouble` results in `JPPyObject::call(...)` before packing
   into a tuple (refcount-leak fix — the raw `PyObject*` was being handed
   to `JPPyTuple_Pack` without ownership tracking).
   **Real conflict**: our `8b8a1a3b` (globals-removal work, `spi`/`wip`)
   touches the *same three call sites* in the same function, changing
   `value.getValue().l != nullptr` → `!value.isJavaNull()` and
   `frame.CallBooleanMethodA(value.getJavaObject(), ...)` →
   `frame.CallBooleanMethodA(value.getJavaObject(frame), ...)`, but leaves
   the `JPPyObject args = JPPyTuple_Pack(PyLong_FromLongLong(l));` line
   itself untouched. Both fixes are real and independent (one fixes null
   handling, one fixes refcounting) — resolution is to keep both: apply
   upstream's `JPPyObject::call(...)` wrapping on top of our
   `isJavaNull()`/`getJavaObject(frame)` version. Not a textual
   auto-mergeable hunk (same lines, both sides changed) — needs a manual
   3-way resolve at this one spot.

Also confirmed: `7964495c` ("Fixes #1380", `native/common/jp_context.cpp`,
Windows-only `getShared()` UTF-8 path fix) touches a file our
`plan/Globals.md` work also modified, but the changed regions don't
overlap (isolated Windows-specific function vs. our `std::call_once`
immortals/reference-queue changes elsewhere in the file) — expected to
merge cleanly.

## Plan (per existing TODO.md framing, corrected for origin/master)

1. **Fold `spi` (most complete) into `reverse`.** `reverse` is the branch
   directly off `origin/master`'s history per the existing TODO note.
   Since `spi` is a strict superset of `wip`, only `spi` needs folding in
   (not `wip` separately). Decide merge vs. rebase: given `reverse` (57
   commits) and `spi` (120 commits) diverged from *different* points on
   `origin/master`, a straight rebase of one onto the other will replay
   through unrelated history — likely cleaner to treat this as a merge
   (`git merge spi` into `reverse`, or vice versa) and resolve at the
   merge commit, rather than rebasing 100+ commits individually.
2. **Squash.** Per TODO.md: "Squash merge so that the massive number of
   commits are more consistent with a direct transfer" — once folded and
   verified (tests green), squash the combined history into a small
   number of reviewable commits before this becomes the new mainline
   candidate. Decide squash granularity (one commit? by feature area —
   subinterpreters / SPI / globals-removal / reverse-bridge tests?) —
   recommend by feature area, matching how `TODO.md`'s "[DONE]"/"[SCOPED]"
   entries already group the work, so the squashed history stays legible.
3. **Push.** Push the folded+squashed `reverse` branch to `origin`.
   Confirm with user before this step — rewriting `reverse`'s history
   (via squash) and pushing is a shared-state, hard-to-reverse operation
   per the standing git safety protocol, even though `reverse` is not
   `origin/master` itself.
4. **Pull upstream/master fixes and resolve.** Merge (or cherry-pick) the
   3 real fixes above onto the resynced branch:
   - `a3b1287e` — trivial, no conflict expected.
   - `21f7b8c8` — no conflict expected (different file region/direction).
   - `c3af9487` — manual resolve required at the 3 call sites in
     `PyJPNumber_create` (`native/python/pyjp_number.cpp`), per above.
   The remaining 10 upstream commits (CI/doc/merge noise) can be skipped
   — no code content to pull.

## Verification

- Full `mvn -q test -Dpython.executable=python3.10` (and `python3.12-dbg`
  per established practice) after each fold/merge step, not just at the
  end — expect 475/475 to remain green throughout.
- After resolving `c3af9487`'s conflict specifically: a targeted manual
  check that `PyJPNumber_create`'s three branches (`Boolean`/`Long`/
  `Float` wrapper construction) both (a) correctly treat Java-null boxed
  values via `isJavaNull()` and (b) don't leak the `PyLong_FromLongLong`/
  `PyFloat_FromDouble` result — ideally a small refcount-under-load test
  if one doesn't already exist for boxed-null number construction.
- Confirm `git diff` between the final resynced branch and `spi` (pre-fold)
  touches only expected files (the 3 upstream commits' files) — nothing
  else should move.

## Open questions (need user decision before executing)

1. Merge direction for step 1: fold `spi` into `reverse`, or make
   `reverse` the thing merged into `spi` and rename `spi`→`reverse`
   afterward? Functionally similar, but affects which branch name
   survives and which commit hashes get squashed away.
2. Squash granularity for step 2 (single commit vs. by feature area).
3. Confirmed go-ahead needed before the push in step 3 (shared remote
   state, matches standing git safety protocol for this session).
