# Scotty3D

Scotty3D is the 3D modeling, rendering, and animation package that students complete as part of [15-462/662 Computer Graphics](http://15462.courses.cs.cmu.edu) at Carnegie Mellon University.

The current version of the starter code is available at https://github.com/CMU-Graphics/Scotty3D .

User documentation and example works are available at https://cmu-graphics.github.io/Scotty3D-docs/ .


## GitHub Setup

Please do not use a public GitHub fork of this repository! We do not want solutions to be public. You should work in your own private repo.
We recommended creating a mirrored private repository with multiple remotes. The following steps go over how to achieve this.

The easiest (but not recommended) way is to download a zip from GitHub and make a private repository from that. The main disadvantage with this is that whenever there is an update to the base code, you will have to re-download the zip and manually merge the differences into your code. This is a pain, and you already have a lot to do in 15462/662, so instead, let `git` take care of this cumbersome "merging-updates" task:

1. Clone Scotty3D normally
    - `git clone https://github.com/CMU-Graphics/Scotty3D.git`

2. Create a new private repository (e.g. `MyScotty3D`)
    - Do not initialize this repository - keep it completely empty.
    - Let's say your repository is now hosted here: `https://github.com/your_id/MyScotty3D.git`

3. Ensure that you understand the concept of `remote`s in git.
    - When you clone a git repository, the default remote is named 'origin' and set to the URL you cloned from.
    - We will set the `origin` of our local clone to point to `MyScotty3D.git`, but also have a remote called `sourcerepo` for the public `Scotty3D` repository.

4. Now go back to your clone of Scotty3D. This is how we add the private remote:
    - Since we cloned from the `CMU-Graphics/Scotty3D.git` repository, the current value of `origin` should be `https://github.com/CMU-Graphics/Scotty3D.git`
        - You can check this using `git remote -v`, which should show:
            ```
            origin      https://github.com/CMU-Graphics/Scotty3D.git (fetch)
            origin      https://github.com/CMU-Graphics/Scotty3D.git (push)
            ```
    - Rename `origin` to `sourcerepo`:
        - `git remote rename origin sourcerepo`
    - Add a new remote called `origin`:
        - `git remote add origin https://github.com/your_id/MyScotty3D.git`
    - We can now push the starter code to our private copy:
        - `git push origin -u main`

5. Congratulations! you have successfully _mirrored_ a git repository with all past commits intact. 

Now, let's see why this setup may be useful: say we start doing an assignment and commit regularly to our private repo (our `origin`). Then the 15-462 staff push some new changes to the Scotty3D skeleton code that we want to pull in. But, we don't want to mess up the changes we've added to our private copy. Here's where git comes to the rescue:

1. Commit all local changes to your `origin`.
2. Run `git pull sourcerepo main` - this pulls all the changes from `sourcerepo` into your local copy.
    - If there are files with changes in both `origin` and `sourcerepo`, git will attempt to automatically merge the updates. Git may create a "merge" commit for this.
    - Unfortunately, there may be conflicts. Git will handle as many merges as it can, then then tell you which files have conflicts that need manual resolution. You can resolve the conflicts in your text editor and create a new commit to complete the `merge` process.
3. After you have completed the merge, you now have all the updates locally. Push to your private origin to publish changes there too:
    - `git push origin main`


## General Setup

1. Install a C++ compiler:
  - Windows: Visual Studio 2022
  - MacOS: XCode (latest available)
  - Linux: g++ (latest available)
2. Install [node](https://nodejs.org) (our build system is written in command-line javascript.)
3. Clone this repository.
4. Download and extract the nest-libs as a child of the repository folder:
  - Linux: https://github.com/15-466/nest-libs/releases/download/v0.10/nest-libs-linux-v0.10.tar.gz
  - MacOS: https://github.com/15-466/nest-libs/releases/download/v0.10/nest-libs-macos-v0.10.tar.gz
  - Windows: https://github.com/15-466/nest-libs/releases/download/v0.10/nest-libs-windows-v0.10.zip


## Building and Running

Run, from a command prompt that has your compiler available:
```
#change to the directory with the repository:
$ cd Scotty3D
#use Maek to build the code:
Scotty3D$ node Maekfile.js
#run the UI:
Scotty3D$ ./Scotty3D
#run the tests:
Scotty3D$ ./Scotty3D --run-tests
```
Note that you _should_ read `Maekfile.js`. about the available command line options and how to configure your own build. All the code has been nicely documented to help you understand the building process and reinforce your learning.

When using with `--run-tests` option, you may realize that the program is running every test file under tests directory. If you wish to specify which tests to run, you can specify the substring your desired tests cases include. For example, running `./Scotty3D --run-tests "A0"` runs all test cases with substring "A0" in its test function name.

Below is a non-exhaustive list of common build issues along with their suggested solutions. Let us know if you encounter a problem that is not addressed here.

- When in doubt, feel free to delete `objs/` and `maek-cache.json` and rebuild with `node Maekfile.js` again to see if anything has changed.

### macOS
> `unknown warning option '-Wno-unused-but-set-variable'`

`'-Wno-unused-but-set-variable'` is a [new compiler flag introduced in Clang 13.0.0](https://releases.llvm.org/13.0.0/tools/clang/docs/ReleaseNotes.html#new-compiler-flags), so you may encounter this error if your Clang is not up to date. To resolve this, do the following:

1. Check that your macOS version is Monterey or above, then upgrade Xcode to the latest version (should be at least Version 13.0).
2. Open Xcode and navigate to the top menu bar. Select **Xcode->Preferences->Locations**, then set **Command Line Tools** to your latest Xcode version (e.g. Xcode 14.2 (14C18)). 
3. Run `clang -v` in your terminal to check if thel Clang version is $\ge$ 13:
```
Apple clang version 14.0.0 (clang-1400.0.29.202)
Target: ...
Thread model: ...
InstalledDir: ...
```
4. Re-try building Scotty3D. If the error still shows up, go to `Maekfile.js` and comment out the line where the flag is located. We included this flag because it is beneficial to your learning and omitting it may cause it to not build on some other computers.

### Linux
> `/usr/bin/ld: cannot find -lasound`

Your machine is missing a package. Simply install it by running `apt-get install libasound2-dev`.

### Windows
> `library machine type 'x64' conflicts with target machine type 'x86'`

You are probably building from a shell with the developer tools set to target x86 (older, 32-bit instruction set) instead of x64 (modern, 64-bit flavor of x86, also sometimes known as “x86_64”). Check [this guide](https://learn.microsoft.com/en-us/cpp/build/how-to-enable-a-64-bit-visual-cpp-toolset-on-the-command-line?view=msvc-170) for enabling an x64 toolset on the command line.

> `spawn cl.exe ENOENT`

You are probably not building from a command prompt that has your compiler (cl.exe) available. Make sure that you are using **x64 Native Tools Command Prompt for VS 2022** (or run the proper `vcvars.bat` from whatever shell you happen to be using). If the error still shows up, run `cl.exe` from the prompt to check that it is indeed working. If not, your visual studio install might have been set up incorrectly.

## Updating
When switching to a new assignment, you should `git pull sourcerepo main` to obtain the latest handout version, resolve conflicts, and try building Scotty3D before starting the new assignment. Some build issues might occur:

> `duplicate symbols for architecture x86_64`

This is likely because you have declared a helper function in a file `A.cpp` that is included in a different file `B.cpp`. When linking, your helper function appears in both `A.o` and `B.o`, which is typically not allowed as the error suggests. To avoid this, declare your helper function as `static` or `inline` to tell the linker it is okay to have it appear multiple times in different object files. More detail:

* `static` means “internal linkage” (function name isn’t exported by the object file) - See [reference](https://en.cppreference.com/w/cpp/language/storage_duration)
* `inline` means “okay to have multiple [identical] definitions” (function name is exported but linker will ignore one of them) - See [reference](https://en.cppreference.com/w/cpp/language/inline)

## Useful Resources
More info about Scotty3D can be found in the [User Guide](https://cmu-graphics.github.io/Scotty3D-docs/guide/) (and again, `Maekfile.js`!). We will also post on [Piazza](https://piazza.com/class/l7euxsj4kf4ht/) if there's an update you should be aware of. Make sure you have access to these, and don't hesitate to ask questions!
