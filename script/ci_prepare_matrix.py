#!/usr/bin/env python3

"""Prepare the workflow platform matrix from action inputs."""

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


def require_env(name: str) -> str:
    value = os.environ.get(name)
    if value is None:
        raise SystemExit(f'Missing required environment variable: {name}')
    return value


def parse_csv_env(name: str) -> list[str]:
    raw_value = require_env(name)
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


def resolve_runs_on(platform_tag: str) -> str:
    if platform_tag.startswith(('windows-', 'uwp-')):
        return 'windows-latest'
    if platform_tag.startswith(('macos-', 'ios-')):
        return 'macos-latest'
    return 'ubuntu-latest'


def resolve_host_platform_tag(platform_tag: str) -> str:
    platform, _compiler, _arch, lib_type = platform_tag.split('-')
    if platform in {'android', 'web'}:
        return f'linux-gcc-x64-{lib_type}'
    if platform == 'ios':
        return f'macos-clang-x64-{lib_type}'
    if platform == 'uwp':
        return f'windows-msvc-x64-{lib_type}'
    return platform_tag


def build_platform_matrix_entry(
    platform_tag: str,
    requested_platform_tags: set[str],
) -> dict[str, bool | str]:
    host_platform_tag = resolve_host_platform_tag(platform_tag)
    host_platform, host_compiler, host_arch, _host_lib_type = host_platform_tag.split('-')
    return {
        'ci_platform_tag': platform_tag,
        'ci_host_platform_tag': host_platform_tag,
        'ci_host_platform': host_platform,
        'ci_host_compiler': host_compiler,
        'ci_host_arch': host_arch,
        'runs_on': resolve_runs_on(platform_tag),
        'requested': platform_tag in requested_platform_tags,
    }


def build_platform_matrix(
    platform_tags: list[str],
    requested_platform_tags: list[str],
) -> dict[str, list[dict[str, bool | str]]]:
    requested_platform_tag_set = set(requested_platform_tags)

    return {
        'include': [
            build_platform_matrix_entry(tag, requested_platform_tag_set)
            for tag in platform_tags
        ]
    }


def write_output(
    requested_platform_tags: list[str],
    platform_tags: list[str],
) -> None:
    github_output = require_env('GITHUB_OUTPUT')
    platform_matrix = build_platform_matrix(
        platform_tags,
        requested_platform_tags,
    )
    with open(github_output, 'a', encoding='utf-8') as output:
        print(f'requested_platform_tags={json.dumps(requested_platform_tags)}', file=output)
        print(f'platform_tags={json.dumps(platform_tags)}', file=output)
        print(f'platform_matrix={json.dumps(platform_matrix)}', file=output)


def main() -> None:
    profile = os.environ.get('INPUT_PROFILE', 'downstream').strip() or 'downstream'
    validate_choice('profile', profile, {'engine', 'downstream'})

    requested_platform_tags = parse_csv_env('INPUT_PLATFORMS')

    selected_platform_tags = resolve_platform_selector('platforms', requested_platform_tags)

    if not selected_platform_tags:
        raise SystemExit('No platform tags remain after filtering. Adjust platforms exclusions.')

    platform_tags = selected_platform_tags
    write_output(selected_platform_tags, platform_tags)


if __name__ == '__main__':
    main()
