#!/usr/bin/env bash

# Download matching SDK artifact from previous builds
# Usage: ci_download_artifact.sh <artifact_prefix> <github_token> <github_repository> <workspace_dir>

set -e

# Parse arguments
ARTIFACT_PREFIX=$1
GITHUB_TOKEN=$2
GITHUB_REPOSITORY=$3
WORKSPACE_DIR=$4

if [ $# -ne 4 ]; then
    echo "Usage: $0 <artifact_prefix> <github_token> <github_repository> <workspace_dir>"
    exit 1
fi

echo "Searching for artifacts matching prefix: $ARTIFACT_PREFIX"

# Fetch artifacts list
ARTIFACTS_JSON=$(curl -s -H "Authorization: token ${GITHUB_TOKEN}" \
    "https://api.github.com/repos/${GITHUB_REPOSITORY}/actions/artifacts?per_page=100")

# Extract artifact that matches our prefix (excluding the SHA part)
MATCHING_ARTIFACT=$(echo "$ARTIFACTS_JSON" | jq -r --arg prefix "$ARTIFACT_PREFIX" \
    '.artifacts[] | select(.name | startswith($prefix)) | select(.expired == false) | {id: .id, name: .name, created_at: .created_at}' | \
    jq -s 'sort_by(.created_at) | reverse | .[0]')

if [ "$MATCHING_ARTIFACT" != "null" ] && [ -n "$MATCHING_ARTIFACT" ]; then
    ARTIFACT_ID=$(echo "$MATCHING_ARTIFACT" | jq -r '.id')
    ARTIFACT_NAME=$(echo "$MATCHING_ARTIFACT" | jq -r '.name')
    
    # Extract version from artifact name (everything between prefix and last dash)
    ARTIFACT_VERSION=$(echo "$ARTIFACT_NAME" | sed -n "s/^${ARTIFACT_PREFIX}\([^-]*\)-.*$/\1/p")
    
    echo "Found matching artifact: $ARTIFACT_NAME (ID: $ARTIFACT_ID, Version: $ARTIFACT_VERSION)"
    
    # Output for GitHub Actions
    echo "FOUND_SDK=true" >> $GITHUB_OUTPUT
    echo "ARTIFACT_ID=$ARTIFACT_ID" >> $GITHUB_OUTPUT
    echo "ARTIFACT_VERSION=$ARTIFACT_VERSION" >> $GITHUB_OUTPUT
    echo "ARTIFACT_NAME=$ARTIFACT_NAME" >> $GITHUB_OUTPUT
    
    # Download the artifact
    echo "Downloading artifact..."
    curl -L -H "Authorization: token ${GITHUB_TOKEN}" \
        "https://api.github.com/repos/${GITHUB_REPOSITORY}/actions/artifacts/$ARTIFACT_ID/zip" \
        -o sdk-artifact.zip
    
    # Extract to a temporary location
    CACHED_SDK_DIR="${WORKSPACE_DIR}/cached-sdk"
    mkdir -p "$CACHED_SDK_DIR"
    echo "Extracting artifact to $CACHED_SDK_DIR..."
    unzip -q sdk-artifact.zip -d "$CACHED_SDK_DIR"
    
    # Update CMAKE_PREFIX_PATH
    echo "CMAKE_PREFIX_PATH=${CACHED_SDK_DIR}:$CMAKE_PREFIX_PATH" >> $GITHUB_ENV
    echo "Successfully downloaded and extracted SDK artifact"
    
    exit 0
else
    echo "No matching SDK artifact found"
    echo "FOUND_SDK=false" >> $GITHUB_OUTPUT
    exit 0
fi