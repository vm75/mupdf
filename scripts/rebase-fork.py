#!/usr/bin/env python3
"""Rebase vm75/mupdf's main patch stack and validate both image extensions."""

import argparse
import datetime
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

BASE_REF = "refs/fork/upstream-base"
ROOT = None


def git(*args, check=True):
    result = subprocess.run(["git", *args], cwd=ROOT, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if check and result.returncode:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip())
    return result


def run(*args):
    print("+ " + shlex.join(args), flush=True)
    subprocess.run(args, cwd=ROOT, check=True)


def commit(ref):
    return git("rev-parse", "--verify", "--end-of-options", ref + "^{commit}").stdout.strip()


def git_path(name):
    return ROOT / git("rev-parse", "--git-path", name).stdout.strip()


def rebasing():
    return git_path("rebase-merge").exists() or git_path("rebase-apply").exists()


def planned_rebase(state):
    directory = git_path("rebase-merge")
    if not directory.exists():
        directory = git_path("rebase-apply")
    expected = {"head-name": "refs/heads/main", "orig-head": state["old_head"],
                "onto": state["new_base"]}
    for name, value in expected.items():
        path = directory / name
        if not path.exists() or path.read_text().strip() != value:
            raise RuntimeError("The active Git rebase does not match the saved plan; inspect it with Git directly.")


def rebase(*args):
    # A user's updateRefs setting must never move the backup with the stack.
    run("git", "-c", "rebase.updateRefs=false", "-c", "submodule.recurse=false", "rebase", *args)


def clean():
    if git("status", "--porcelain").stdout:
        raise RuntimeError("Commit or move uncommitted changes, including submodule changes, before rebasing.")


def main_branch():
    if git("symbolic-ref", "--quiet", "--short", "HEAD", check=False).stdout.strip() != "main":
        raise RuntimeError("Run this helper on main.")


def submodules():
    # Upstream's relative URLs must not resolve against vm75/mupdf's origin.
    run("git", "-c", "branch.main.remote=upstream", "submodule", "sync", "--recursive")
    run("git", "-c", "branch.main.remote=upstream", "submodule", "update", "--init", "--recursive")


def old_base(explicit):
    if explicit:
        base = commit(explicit)
    elif git("show-ref", "--verify", "--quiet", BASE_REF, check=False).returncode == 0:
        base = commit(BASE_REF)
    else:
        base = git("merge-base", "HEAD", "upstream/master").stdout.strip()
        proposed = set(git("rev-list", base + "..HEAD").stdout.splitlines())
        # Also exclude tags: an old stable release may have no surviving branch.
        # Fork-owned tags can make this conservative check require --from-base.
        fork_only = set(git("rev-list", base + "..HEAD", "--not", "--remotes=upstream", "--tags").stdout.splitlines())
        if proposed != fork_only:
            raise RuntimeError("Cannot infer the previous upstream base safely; use --from-base <previous-upstream-ref>.")
    if git("merge-base", "--is-ancestor", base, "HEAD", check=False).returncode:
        raise RuntimeError("The recorded upstream base is not an ancestor of main; use --from-base <previous-upstream-ref>.")
    if git("rev-list", "--merges", base + "..HEAD").stdout.strip():
        raise RuntimeError("The fork stack contains merge commits. Establish a linear fork stack before using this helper.")
    return base


def finish(state, path, skip_tests):
    main_branch()
    if git("merge-base", "--is-ancestor", state["new_base"], "HEAD", check=False).returncode:
        raise RuntimeError("main does not contain the planned upstream base; inspect the saved backup before continuing.")
    if git("status", "--porcelain", "--ignore-submodules=all").stdout:
        raise RuntimeError("Commit worktree changes before retrying the post-rebase checks.")
    submodules()
    clean()
    if state["tests"] and not skip_tests:
        prefix = "build/fork-check-" + commit("HEAD")[:12]
        for webp, jxl in [(1, 1), (0, 1), (1, 0), (0, 0)]:
            run("make", "build=debug", "FZ_ENABLE_WEBP=" + str(webp),
                "FZ_ENABLE_JXL=" + str(jxl),
                "OUT=" + prefix + "-webp" + str(webp) + "-jxl" + str(jxl),
                "test-webp", "test-jxl")
    else:
        print("Tests skipped. Run all codec configurations before publishing.")
    clean()
    run("git", "range-diff", state["old_base"] + ".." + state["old_head"],
        state["new_base"] + "..HEAD")
    run("git", "update-ref", BASE_REF, state["new_base"])
    path.unlink()
    print("Rebase complete. Review the range-diff above; backup: " + state["backup"])
    print("Publish main separately after reviewing the result (see README.md).")


def main():
    global ROOT
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--continue", dest="resume", action="store_true", help="continue after resolving conflicts or retry failed checks")
    mode.add_argument("--abort", action="store_true", help="abort an active Git rebase and restore its original main")
    parser.add_argument("--base", default="upstream/master", help="new upstream branch/tag/commit (default: upstream/master)")
    parser.add_argument("--from-base", help="previous upstream base, for initial setup or repairing a stale local base marker")
    parser.add_argument("--dry-run", action="store_true", help="inspect cached refs without fetching or changing anything")
    parser.add_argument("--skip-tests", action="store_true", help="leave codec checks to a separate manual run")
    args = parser.parse_args()
    ROOT = Path(git("rev-parse", "--show-toplevel").stdout.strip())
    path = git_path("fork-rebase.json")

    if args.dry_run and (args.resume or args.abort):
        parser.error("--dry-run cannot be combined with --continue or --abort")
    if args.resume or args.abort:
        if not path.exists():
            raise RuntimeError("No saved fork rebase; use Git directly if this rebase was started elsewhere.")
        state = json.loads(path.read_text())
        if rebasing():
            planned_rebase(state)
        if args.abort:
            if not rebasing():
                if commit("HEAD") == state["old_head"]:
                    main_branch()
                    clean()
                    path.unlink()
                    print("Pending plan cancelled; main has not moved. Backup: " + state["backup"])
                    return
                raise RuntimeError("Git has already finished the rebase. Retry checks with --continue, or inspect backup " + state["backup"])
            rebase("--abort")
            path.unlink()
            submodules()
            print("Rebase aborted; original main restored. Backup: " + state["backup"])
        else:
            if rebasing():
                rebase("--continue")
            finish(state, path, args.skip_tests)
        return

    if path.exists():
        raise RuntimeError("A fork rebase is pending; use --continue or --abort.")
    main_branch()
    clean()
    for name in ["rebase-merge", "rebase-apply", "MERGE_HEAD", "CHERRY_PICK_HEAD", "REVERT_HEAD"]:
        if git_path(name).exists():
            raise RuntimeError("Finish the existing Git operation before starting a fork rebase.")
    git("remote", "get-url", "upstream")
    if not args.dry_run:
        run("git", "fetch", "--tags", "upstream")
    base = old_base(args.from_base)
    head = commit("HEAD")
    target = commit(args.base)
    count = git("rev-list", "--count", base + "..HEAD").stdout.strip()
    if count == "0":
        raise RuntimeError("No committed fork patches to replay; commit the extensions on main first.")
    print("Fork stack: " + count + " commits; previous base: " + base + "; new base: " + target)
    if args.dry_run:
        print(git("log", "--oneline", "--reverse", base + "..HEAD").stdout, end="")
        print("Dry run: cached refs only; no fetch, backup, rebase, submodule updates, or tests performed.")
        return
    stamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    backup = "backup/main-" + stamp
    run("git", "branch", backup, head)
    state = {"old_head": head, "old_base": base, "new_base": target,
             "backup": backup, "tests": not args.skip_tests}
    # Exclusive creation prevents another helper from replacing an active plan.
    with path.open("x") as stream:
        json.dump(state, stream)
        stream.flush()
        os.fsync(stream.fileno())
    print("Saved backup: " + backup, flush=True)
    rebase("--onto", target, base, "main")
    finish(state, path, args.skip_tests)


if __name__ == "__main__":
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError, OSError, ValueError) as error:
        print("fork rebase: " + str(error), file=sys.stderr)
        print("If a fork rebase is pending, resolve conflicts and run --continue; see README.md for recovery.", file=sys.stderr)
        sys.exit(1)
