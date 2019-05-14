#Contributing

**Fork, then clone the repo:**

```bash
git clone git@github.com:<your-username>/circ.git
```

**Install the dependencies:**

* `cmake`
* `make`
* a C compiler
* `libunistring-dev`
* `libev-dev`

**Install the development dependencies (this is important!):**

* `clang` (for `clang-format`, `clang-tidy`)

**Install the Git Hooks**

Symlink `.githooks` to `.git/hooks`.

Push to your fork, then [submit a pull request](https://github.com/nihilist-space/circ/compare/)

## Styleguide & Linting

* Please get rid of any warnings `clang-format` and `clang-tidy` spew.
* Put a space before parens.
* Use snake case.
* Don't code like a rockstar.

See editor configuration for integrating with the above.

## Editor Configuration

### Vim

If you use `Vim` + `ALE`, you can set it up to lint source files with:

```viml
let g:ale_fixers = { }
let g:ale_fixers['c'] = ['clang-format']

let g:ale_linters = { }
let g:ale_linters['c'] = ['clangtidy']
```

Reformat source files with `clang-format` with `ALEFix`.

Please ensure you have the development dependencies installed.
