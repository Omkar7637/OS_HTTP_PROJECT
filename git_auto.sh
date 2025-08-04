#!/bin/bash

# Stage all changes
git add .

# Prompt user for commit message
echo -n "Enter your commit message: "
read commit_msg

# Commit with the entered message
git commit -m "$commit_msg"

# Push to current branch
git push
