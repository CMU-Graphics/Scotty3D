
# Scotty3D

![Ubuntu Build Status](https://github.com/CMU-Graphics/Scotty3D/workflows/Ubuntu/badge.svg) ![MacOS Build Status](https://github.com/CMU-Graphics/Scotty3D/workflows/MacOS/badge.svg) ![Windows Build Status](https://github.com/CMU-Graphics/Scotty3D/workflows/Windows/badge.svg)

Welcome to Scotty3D! This 3D graphics software implements interactive mesh
editing, realistic path tracing, and dynamic animation. Implementing the
functionality of the program constitutes the majority of the coursework for
15-462/662 Computer Graphics at Carnegie Mellon University

Please visit the [documentation website](https://cmu-graphics.github.io/Scotty3D/).


## Tips on Making a private "fork" for working on the Assignment

Since you dont want to make a github fork of this repo and make your work public, you should have your own private repo.
The recommended approach is to have a mirrored github repository that's private and have multiple remote's on your local clone of the code. The following steps go over how to achieve this.

The easiest(not recommended) way is to download a zip from github and then make your own private repo and work on that. The main disadvantage with this is that whenever there is an update to the base code, you will have to again download the zip and manually merge the differences with your code. This is a nightmare and you should not do this. We already have a lot to do in 15462/662, we can let `git` take care of this cumbersome "merging-updates" task. Here's how

1. Do a normal git pull of this repository
    - `git clone https://github.com/CMU-Graphics/Scotty3D.git`
2. Go to your own github account and create a private repository (lets say you choose to call it `MyScotty3D`)
    - Don't initialize this repository with anything, keep it completely empty
    - Lets say the repository is hosted here `https://github.com/your_id/MyScotty3D.git`
3. Now lets get back to your local clone of this repository. Ensure that you understand the concept of `remote`s in git.
    - When you clone from git, the default remote is 'origin' and it is set to the URL you cloned from
    - in our case, we will set the `origin` of our local clone to point to `MyScotty3D.git` and have a new remote called `sourcerepo` for the OG `Scotty3D` repo
4. This is how we do it, 
    - Since we cloned from the `CMU-Graphics/Scotty3D.git` repo, the current value of `origin` should be `https://github.com/CMU-Graphics/Scotty3D.git`
        - You can check this using `git remote -v`
            
            This should show 
            ```
            origin      https://github.com/CMU-Graphics/Scotty3D.git (fetch)
            origin      https://github.com/CMU-Graphics/Scotty3D.git (push)
            ```
    - We will now rename `origin` to `sourcerepo`
        - `git remote rename origin sourcerepo`
    - Now that we have a remote called `sourcerepo` it is time to add our private repo url to this local clone and push the code there
        - We add a new remote called `origin` (since after the last step we dont have any `origin` on our repo)

            `git remote add origin https://github.com/your_id/MyScotty3D.git`
        - Now the origin has been added, time to push all our code to it

            `git push origin -u master`
        - Congratulations! you have successfully _Mirrored_ a git repository!! With each of the past commits intact and multiple remotes. Let's see a case where this becomes very useful
5. Lets say we start doing this Assignment and follow good git practice and commit regularly to our private repo(our `origin`). Now there are a few framework changes from the TAs to Scotty3D. These changes are in the original repo(for your local branch, we had set it to `sourcerepo`, remember? ;)
    - So we now need to pull the changes from the `sourcerepo`. But we dont want to mess up our code that we pushed to `origin`. Well, here's where git comes to the rescue.
    - The steps we will follow include
        - First commit all your changes to your `origin`
        - `git pull sourcerepo master` - This gets all the changes from the sourcerepo into your local folder
        - If both the remotes dont have any changes to common files, this should just work.
        - If there is any file that was modified both in your `origin` and in the `sourcerepo`, git will try it's best to merge the changes. Git may create a "merge" commit for these auto merged changes. Sometimes there might be conflicts. Git would gracefully handle whatever merges it can and it will then tell you which files have conflicts that need manual resolution. You can resolve those conflicts and complete the `merge` commit process.
        - After you have completed the merge commit, you now have all the updates locally, so its time to update your origin and include those changes there too.
            - `git push origin master` should do the trick :)
