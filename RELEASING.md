## Releasing

 1. Communicate that a release is about to be released in the [DaSCH Github Channel](https://github.com/orgs/dhlab-basel/teams/dasch), so that no new Pull Requests are merged
 1. Create a new branch, e.g., `releasing-vX.X.X`
 1. Update the version number in `CMakeLists.txt`, `manual/conf.py`.
 1. Remove the `(not released yet)` text in the title of the release notes.
 1. Create a new page with the next version number including the `(not released yet)` text and add page to TOC.
 1. Update links in the new page to point to correct release tag and milestone.
 1. On Github - Create new milestone
 1. On Github - Move any open issues from current release milestone to the next release milestone and so on.
 1. On Github - Close current milestone.
 1. Push and merge PR to `main`.
 1. Travis CI will start a [CI build](https://travis-ci.org/dhlab-basel/Sipi/builds) for the new tag and publish
    artifacts to Docker Hub.
 1. On Github - Tag the commit with the version string, e.g., `vX.X.X` and create a release.
 1. On Github - Copy the release notes from the docs to the release.
 1. Publish documentation.


## Under the Travis hood

Here is what happens in detail when Travis CI is building a git tagged commit. According to the `.travis.yml` file,
the `publish` stage runs the following task:
   
 1. Credentials are read from the `DOCKER_USER` and `DOCKER_PASS` environment variables which are stored encrypted
    inside `.travis.yml`.
 1. The `sipi` docker image is build, tagged, and published.