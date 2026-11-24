# Contributing to Smart Spot

These rules let four people work in parallel without breaking each other's work. They follow the review and completion rules in our Software Project Management Plan.

## 1. Setup

- Use **VS Code**. When you open the project, accept the prompt to install the recommended extensions.
- Clone the repository into a normal folder such as `C:\Users\<name>\Projects`. **Never inside OneDrive, Google Drive or Dropbox**, because sync tools can corrupt the `.git` folder.

## 2. Branches

- `main` is protected: all changes go through a pull request.
- Before starting a task, update `main` and create a branch from it:

  ```
  git checkout main
  git pull
  git checkout -b feature/backend-login
  ```

- Branch names: `<type>/<area>-<short-name>`
  - type: `feature`, `fix`, `docs`, `test`, `chore`
  - area: `backend`, `frontend`, `firmware`, `docs`, `tools`
  - examples: `feature/frontend-login-page`, `fix/firmware-echo-timeout`
- One task per branch. Keep pull requests small enough to review in under 30 minutes.

## 3. Commits

- Describe what changed, in the imperative: `Add reservation conflict check (FR-18)`.
- Mention requirement IDs (FR-xx, NFR-xx) when the commit implements or tests one.
- **Never commit** passwords, Wi-Fi credentials, device keys or `.env` files.

## 4. Pull requests

1. Update your branch with the latest `main` (`git merge origin/main`) and fix any conflicts.
2. Push your branch and open the pull request. Fill in the template.
3. GitHub requests a review from the code owners automatically. At least **one approval** from someone other than the author is required.
4. Answer review comments by pushing fixes to the same branch, then reply on the PR so the reviewer knows it's ready.
5. After approval, use **Squash and merge**. The branch is deleted automatically.
6. Everyone else runs `git pull` on `main` to get the change.

## 5. Reviewing

Check that the change:

- does what the linked requirements say, including error and rejection cases;
- follows the API contract (paths, field names, status codes), once it exists;
- has tests, or explains how it was tested;
- can be run by someone else from the instructions.

Comment on the code, not the person. Small style preferences shouldn't block a merge.

## 6. Definition of done

- [ ] Acceptance criteria pass.
- [ ] Another member reviewed and approved it.
- [ ] It works with the parts that use it, or the PR says it was tested with simulated data.
- [ ] Setup notes and test evidence are included.

## 7. Planning

The schedule lives in ClickUp. GitHub holds the code, issues and pull requests, linked from ClickUp. Don't keep separate due dates in two places.