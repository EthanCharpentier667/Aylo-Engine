---
name: open-pr
description: Open a draft pull request, or a stack of them, for the Aylo-Engine repository with the template, labels, assignee, milestone and project fields set. Use when asked to "open a PR", "create a pull request", "submit", "push this for review", or when a branch is ready for review.
---

# Open a pull request

Always a draft (`--draft`); never merge it, never push to `main`. Run the `verify` skill first and put its result in the
description. If the change touches what scripts can see and `manual/` is not in the diff, run `write-manual` first. Follow
this whole skill, the template, labels, assignee, milestone and project fields included, even for a one-line PR.

## Rules

- **One pull request per issue.** Changes that depend on each other are a stack, one branch and one PR per link, each
  based on the previous one, built with `gh stack` (v0.3+).
- **Title** in the commit norm: `<type>(<scope>): <lowercase summary>`.
- **Body** from `.github/pull_request_template.md`: Summary, Why, Changes, Stack, Test plan with the checklist, Closes.
  Write the assumptions you made in it. No AI attribution line.
- **`Closes #n`** for each issue the PR completes (one line per issue). In a stack, the PR that completes an issue closes
  it, not the others.
- **Stack position**: in the Stack section write "Part n of N" and the list `#a -> #b -> #c`.
- **Assignee**: the person running the command (`--assignee @me`); never a hard-coded login.
- **Labels**: the area labels of the issue (`ecs`, `luau`, `scripting`, `network`, `physics`, `Vulkan`...), plus
  `documentation`, `bug` or `enhancement` as fits. Priority and size are not labels.
- **Milestone**: the issue's, when it has one (`gh issue view <n> --json milestone`).
- **Issues**: the ones a pull request closes live in this repository (`Closes #n`).
- **Project**: add it to the project "Aylo Engine" of the repository owner with Status "In review". Find its number by
  its title, never by a remembered number. If the owner has no such project, skip this step and say so in your report.

## Single pull request

Get the owner and the project once:

```sh
OWNER=$(gh repo view --json owner --jq .owner.login)
NUMBER=$(gh project list --owner $OWNER --format json --jq '.projects[] | select(.title == "Aylo Engine") | .number')
```

```sh
git push -u origin <branch>
gh pr create --draft --base main --title "<title>" --body-file <body.md> \
  --assignee @me --label ecs --label enhancement [--milestone "<name>"]
```

Fill the body in a file (the template is the skeleton) rather than inline.

## Stack

```sh
gh stack init <branch1> <branch2> <branch3>   # or: gh stack init, then gh stack add <next-branch> for each link
gh stack view                                 # check the order
gh stack submit                               # pushes the branches and creates the draft PRs, each on the previous
```

If `gh stack` is unavailable or fails, push each branch and create each PR with `--base <previous branch>` (the first on
`main`); retarget the bases by hand with `gh pr edit <n> --base <branch>`. Only the top pull request of a stack runs the
CI checks, on the whole stack; the others skip them, so say in each description what you ran locally. After `submit`,
edit each description to fill in "Part n of N" and the list of PR numbers (`gh pr edit <n> --body-file ...`).
`gh stack sync` and `gh stack rebase` keep the stack up to date after a change; never force-push a branch you did not
create.

## After creating each pull request

Set the Status by ids: `item-add` returns the item's id, and this works whoever owns the project
(`gh project item-edit --url` fails with "is not an item in project" when the project and the repository have different
owners).

```sh
gh pr edit <n> --add-assignee @me --add-label documentation
ITEM=$(gh project item-add $NUMBER --owner $OWNER --url <pr url> --format json --jq .id)
PROJECT=$(gh project view $NUMBER --owner $OWNER --format json --jq .id)
STATUS=$(gh project field-list $NUMBER --owner $OWNER --format json --jq '.fields[] | select(.name == "Status") | .id')
IN_REVIEW=$(gh project field-list $NUMBER --owner $OWNER --format json \
  --jq '.fields[] | select(.name == "Status") | .options[] | select(.name == "In review") | .id')
gh project item-edit --id $ITEM --project-id $PROJECT --field-id $STATUS --single-select-option-id $IN_REVIEW
```

`item-add` on a pull request already in the project returns its item, so running it again is harmless.

## Check

`gh pr view <n> --json isDraft,assignees,labels,milestone,body` shows a draft, an assignee, the labels and the
milestone. Check the project item with `gh project item-list $NUMBER --owner $OWNER --format json --limit 1000 --jq
'.items[] | select(.content.type == "PullRequest" and .content.number == <n>) | .status'`, which prints `In review`
(`gh pr view --json projectItems` misses the projects of another owner). Fix what is missing.
