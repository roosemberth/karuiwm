karuiwm coding
==============

In order to maintain easily readable, hackable, debuggable code, a consistent
coding style is necessary.


coding style
------------

First off, read the [Linux kernel coding
style](https://www.kernel.org/doc/Documentation/CodingStyle). Then, apply a few
"patches":

1. When implementing a function, the return type of a function is to be split
   from the function name and to be put on a separate line *above*. This allows
   grepping for function implementations more easily, looks nicer, and has the
   additional benefit of saving columns (remember, there are only 80 of them).

   ```c
   int
   function(int x)
   {
           /* function body */
   }
   ```

2. Use comments sparsely, avoid function annotatations. A reader should be able
   to understand what a function does by looking at its name, its arguments, and
   its code (remember, it is short and sweet).

   If you feel your function needs annotations, rethink it. It is however OK to
   separate multiple parts of a function with a short "title", although it may
   be advisable to split the function if this happens.

3. Module functions are prefixed by the module's name (e.g. `client_`,
   `workspace_`, etc). Function names should be relatively short but accurate
   (no abbreviations!).

   The prefixes are not required for static functions, as there is no
   possibility of a namespace clash (unless you come up with a function name
   occupied by an included library).


git
---

Development happens in the
[development](https://github.com/ayekat/karuiwm/tree/api) branch. You should
never push to master directly. The purpose of this is to keep a stable master
branch, and to be able to test the code before it goes "public" (think of the
master branch as the "release" version).

For smaller projects, this allows to have stable software without a need for
maintaining releases.

### commit messages

Commit messages should consist of a one-line, less than 72 column wide title
explaining what the commit is about, followed by an empty line, and a multi-line
explanation about what the commit does.

### features and merging

When writing a new feature, or working on a specific topic of the project,
create a new branch on top of what it will be merged into. When finished, merge
the branch back into its parent, with *fast-forward*. This keeps the history
clean.

When merging the development branch into the master branch, do *not* use
fast-forwarding, to keep the history separated.
