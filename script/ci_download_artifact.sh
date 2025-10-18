#!/usr/bin/env bash

# Download or check existence of matching SDK artifact from previous builds
# Usage: ci_download_artifact.sh <artifact_prefix> <github_token> <github_repository> <output_dir> [--check-only]
# Outputs: "true" or "false" to stdout, log messages to stderr

set -e

# Parse arguments
ARTIFACT_PREFIX=$1
GITHUB_TOKEN=$2
GITHUB_REPOSITORY=$3
OUTPUT_DIR=$4
FLAG=${5:-}

if [ $# -lt 4 ] || [ $# -gt 5 ]; then
    echo "Usage: $0 <artifact_prefix> <github_token> <github_repository> <output_dir> [--check-only|--required]" >&2
    exit 1
fi

# Only show searching message if not in check-only mode
if [ "$FLAG" != "--check-only" ]; then
    echo "Searching for artifacts matching prefix: $ARTIFACT_PREFIX" >&2
fi

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

    if [ "$FLAG" = "--check-only" ]; then
        echo "true"
        exit 0
    fi

    echo "Found matching artifact: $ARTIFACT_NAME (ID: $ARTIFACT_ID, Version: $ARTIFACT_VERSION)" >&2

    # Download the artifact
    URL="https://api.github.com/repos/${GITHUB_REPOSITORY}/actions/artifacts/$ARTIFACT_ID/zip"
    echo "Downloading artifact from $URL" >&2

    if curl -L -H "Authorization: token ${GITHUB_TOKEN}" \
        $URL \
        -o sdk-artifact.zip; then

        # Extract to a temporary location
        mkdir -p "$OUTPUT_DIR"
        echo "Extracting artifact to $OUTPUT_DIR..." >&2

        if unzip -q sdk-artifact.zip -d "$OUTPUT_DIR"; then
            echo "Successfully downloaded and extracted SDK artifact" >&2
            echo "true"
        else
            echo "Failed to extract SDK artifact" >&2
            echo "false"
        fi
    else
        echo "Failed to download SDK artifact" >&2
        echo "false"
    fi
else
    echo "No matching SDK artifact found" >&2
    echo "false"
fi