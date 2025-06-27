#!/bin/sh

# Skip Vercel build if no changes were made in the frontend directory
# When there are no changes in "frontend", this script outputs text and Vercel will skip the build.

# Only run diff if we have a previous SHA
if [ -n "$VERCEL_GIT_PREVIOUS_SHA" ]; then
  CHANGED=$(git diff --name-only "$VERCEL_GIT_PREVIOUS_SHA" "$VERCEL_GIT_COMMIT_SHA" -- frontend)
else
  CHANGED=$(git ls-tree --name-only "$VERCEL_GIT_COMMIT_SHA" frontend)
fi

if [ -z "$CHANGED" ]; then
  echo "No changes in frontend, skipping build."
fi
