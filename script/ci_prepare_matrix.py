#!/usr/bin/env python3

"""Prepare the downstream workflow matrix and artifact selection from workflow inputs."""

import json
import os


ALL_PLATFORM_TAGS = [
    'windows-msvc-x64-dll',
    'windows-msvc-x64-lib',
    'windows-msvc-x86-dll',
    'windows-msvc-x86-lib',
    'linux-gcc-x64-dll',
    'linux-gcc-x64-lib',
    'linux-clang-x64-dll',
    'linux-clang-x64-lib',
    'macos-clang-arm64-dll',
    'macos-clang-arm64-lib',
    'macos-clang-x64-dll',
    'macos-clang-x64-lib',
    'uwp-msvc-x64-dll',
    'uwp-msvc-x64-lib',
    'android-clang-arm64-dll',
    'android-clang-arm-dll',
    'android-clang-x64-dll',
    'ios-clang-arm-lib',
    'ios-clang-arm64-lib',
    'web-emscripten-wasm-lib',
]
ANDROID_PLATFORM_TAGS = [tag for tag in ALL_PLATFORM_TAGS if tag.startswith('android-')]


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if value is None:
        raise SystemExit(f'Missing required environment variable: {name}')
    return value


def parse_csv_env(name: str) -> list[str]:
    raw_value = require_env(name)
    return [value.strip() for value in raw_value.split(',') if value.strip()]


def parse_csv_value(raw_value: str) -> list[str]:
    return [value.strip() for value in raw_value.split(',') if value.strip()]


def validate_choice(name: str, value: str, supported_values: set[str]) -> None:
    if value not in supported_values:
        raise SystemExit(f'Unsupported {name}: {value}')


def expand_platform_tokens(tokens: list[str]) -> tuple[list[str], list[str]]:
    expanded_tags: list[str] = []
    unknown_tokens: list[str] = []
    for token in tokens:
        if token == 'all':
            expanded_tags.append(token)
            continue

        token_parts = [part for part in token.split('-') if part]
        if not token_parts:
            unknown_tokens.append(token)
            continue

        matches = []
        for known_tag in ALL_PLATFORM_TAGS:
            known_tag_parts = known_tag.split('-')
            if all(part in known_tag_parts for part in token_parts):
                matches.append(known_tag)

        if matches:
            expanded_tags.extend(matches)
        else:
            unknown_tokens.append(token)

    return list(dict.fromkeys(expanded_tags)), list(dict.fromkeys(unknown_tokens))


def resolve_platform_selector(selector_name: str, requested_tokens: list[str]) -> list[str]:
    include_platform_tags: list[str] = []
    exclude_platform_tags: list[str] = []
    for tag in requested_tokens:
        if tag.startswith('-'):
            normalized_tag = tag[1:].strip()
            if not normalized_tag:
                raise SystemExit(f'{selector_name} may not contain an empty exclusion')
            if normalized_tag == 'all':
                raise SystemExit(f'{selector_name} does not support excluding all')
            exclude_platform_tags.append(normalized_tag)
        else:
            include_platform_tags.append(tag)

    include_platform_tags, unknown_include_platform_tags = expand_platform_tokens(include_platform_tags)
    exclude_platform_tags, unknown_exclude_platform_tags = expand_platform_tokens(exclude_platform_tags)
    unknown_platform_tags = [*unknown_include_platform_tags, *unknown_exclude_platform_tags]
    if unknown_platform_tags:
        raise SystemExit(f'Unsupported {selector_name}: ' + ', '.join(unknown_platform_tags))

    if 'all' in include_platform_tags or not include_platform_tags:
        platform_tags = list(ALL_PLATFORM_TAGS)
    else:
        platform_tags = list(dict.fromkeys(include_platform_tags))

    if exclude_platform_tags:
        excluded_platform_tags = set(exclude_platform_tags)
        platform_tags = [tag for tag in platform_tags if tag not in excluded_platform_tags]

    return platform_tags


def resolve_deploy_artifact_tags(platform_tags: list[str]) -> list[str]:
    deploy_artifacts = require_env('INPUT_DEPLOY_ARTIFACTS').strip()
    normalized_value = deploy_artifacts.lower()
    if normalized_value == 'false' or deploy_artifacts == '':
        return []
    if normalized_value == 'true':
        return list(platform_tags)

    selected_platform_tags = set(platform_tags)
    deploy_artifact_tags = [
        tag for tag in resolve_platform_selector('deploy_artifacts', parse_csv_value(deploy_artifacts))
        if tag in selected_platform_tags
    ]
    if not deploy_artifact_tags:
        raise SystemExit(
            'deploy_artifacts does not match any selected platform_tags. '
            'Adjust deploy_artifacts or platform_tags.'
        )

    return deploy_artifact_tags


def write_output(platform_tags: list[str], deploy_artifact_tags: list[str]) -> None:
    github_output = require_env('GITHUB_OUTPUT')
    with open(github_output, 'a', encoding='utf-8') as output:
        print(f'platform_tags={json.dumps(platform_tags)}', file=output)
        print(f'deploy_artifact_tags={json.dumps(deploy_artifact_tags)}', file=output)


def main() -> None:
    profile = os.environ.get('INPUT_WORKFLOW_PROFILE', 'downstream').strip() or 'downstream'
    validate_choice('workflow_profile', profile, {'engine', 'downstream'})

    mode = require_env('INPUT_RBFX_MODE').strip()
    validate_choice('rbfx_mode', mode, {'sdk', 'source'})

    source_strategy = require_env('INPUT_RBFX_SOURCE_STRATEGY').strip()
    validate_choice('rbfx_source_strategy', source_strategy, {'source', 'sdk'})

    requested_platform_tags = parse_csv_env('INPUT_PLATFORM_TAGS')
    parse_csv_env('INPUT_RBFX_SOURCE_SDK_CMAKE_ARGS')
    android_enabled = bool(require_env('INPUT_ANDROID_GRADLE_DIR').strip())

    platform_tags = resolve_platform_selector('platform_tags', requested_platform_tags)

    if profile == 'downstream' and not android_enabled:
        platform_tags = [tag for tag in platform_tags if tag not in ANDROID_PLATFORM_TAGS]

    if not platform_tags:
        raise SystemExit(
            'No platform tags remain after filtering. '
            'Adjust platform_tags exclusions or set android_gradle_dir.'
        )

    deploy_artifact_tags = resolve_deploy_artifact_tags(platform_tags)
    write_output(platform_tags, deploy_artifact_tags)


if __name__ == '__main__':
    main()