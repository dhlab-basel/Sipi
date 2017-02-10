#!/usr/bin/env bash

# This script updates the documentation on http://dhlab-basel.github.io/Sipi/ by pushing a commit to the gh-pages branch
# of the Sipi repository on GitHub.

# Before you run this script, you must be on the develop branch, having committed any changes
# you made there.

# If you don't want git to ask you for your username and password, use SSH instad of HTTPS
# to clone the Knora repository.

set -e

TMP_HTML="/tmp/sipi-html" # The temporary directory where we store built HTML.
CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD) # The current git branch.

if [ "$CURRENT_BRANCH" != "develop" ]
then
    echo "Current branch is $CURRENT_BRANCH, not develop"
    exit 1
fi

# Build the HTML docs.

make html

# Copy the built HTML docs to a temporary directory.

rm -rf $TMP_HTML
mkdir $TMP_HTML
cp -R _build/html $TMP_HTML/manual

git checkout gh-pages

git rm -rf ../documentation

mv $TMP_HTML/manual/* ../documentation

# Commit the changes to the gh-pages branch, and push to origin.

git add ../documentation/
#git commit -m "Update gh-pages."
#git push origin gh-pages

# Switch back to the develop branch, and remove the leftover documentation directory.

#git checkout develop
#rm -rf ../documentation
