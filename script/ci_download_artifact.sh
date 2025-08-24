#!/usr/bin/env bash

# Download or check existence of matching SDK artifact from previous builds
# Usage: ci_download_artifact.sh <artifact_prefix> <github_token> <github_repository> <output_dir> [--check-only]

set -e

# Parse arguments
ARTIFACT_PREFIX=$1
GITHUB_TOKEN=$2
GITHUB_REPOSITORY=$3
OUTPUT_DIR=$4
CHECK_ONLY=${5:-}

if [ $# -lt 4 ] || [ $# -gt 5 ]; then
    echo "Usage: $0 <artifact_prefix> <github_token> <github_repository> <output_dir> [--check-only]"
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

    if [ "$CHECK_ONLY" = "--check-only" ]; then
        echo "Check-only mode: Artifact exists"
        exit 0
    fi

    # Download the artifact
    URL="https://api.github.com/repos/${GITHUB_REPOSITORY}/actions/artifacts/$ARTIFACT_ID/zip"
    echo "Downloading artifact from $URL"
    curl -L -H "Authorization: token ${GITHUB_TOKEN}" \
        $URL \
        -o sdk-artifact.zip

    # Extract to a temporary location
    mkdir -p "$OUTPUT_DIR"
    echo "Extracting artifact to $OUTPUT_DIR..."
    unzip -q sdk-artifact.zip -d "$OUTPUT_DIR"

    echo "Successfully downloaded and extracted SDK artifact"

    exit 0
else
    echo "No matching SDK artifact found"
    if [ "$CHECK_ONLY" = "--check-only" ]; then
        exit 1
    fi
    echo "FOUND_SDK=false" >> $GITHUB_OUTPUT
    exit 0
fi