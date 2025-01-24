# ----------------------------------------------------------------------------
# Copyright 2019-2023 Diligent Graphics LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# In no event and under no legal theory, whether in tort (including negligence),
# contract, or otherwise, unless required by applicable law (such as deliberate
# and grossly negligent acts) or agreed to in writing, shall any Contributor be
# liable for any damages, including any direct, indirect, special, incidental,
# or consequential damages of any character arising as a result of this License or
# out of the use or inability to use the software (including but not limited to damages
# for loss of goodwill, work stoppage, computer failure or malfunction, or any and
# all other commercial damages or losses), even if such Contributor has been advised
# of the possibility of such damages.
# ----------------------------------------------------------------------------

import os
import re
import json
import stat
import jsonschema
import shutil
import pathlib
import subprocess
import xml.dom.minidom as minidom

from argparse import ArgumentParser, BooleanOptionalAction
from enum import Enum


class BuildType(Enum):
    Debug = 'Debug'
    Release = 'Release'

    def __str__(self):
        return self.value


project_paths = {
    "dotnet-proj": "./Graphics/GraphicsEngine.NET",
    "dotnet-tests": "./Tests/DiligentCoreTest.NET",
    "dotnet-build": "./build/.NET",
    "tests-assets": "./Tests/DiligentCoreAPITest/assets"
}

default_settings = {
    "windows": {
        "arch": {
            "x86": {
                "cmake-generator-attribute": "Win32",
                "native-build-folder": "./build/.NET/Win32",
                "nuget-name-folder": "win-x86"
            },
            "x64": {
                "cmake-generator-attribute": "x64",
                "native-build-folder": "./build/.NET/Win64",
                "nuget-name-folder": "win-x64"
            }
        },
        "test-gapi": [
            "d3d11",
            "d3d12",
            "vk",
            "gl"
        ],
        "test-names": [
            "GenerateImagesDotNetTest.GenerateCubeTexture",
            "GenerateArchiveDotNetTest.GenerateCubeArchive"
        ]
    }
}

jsonschema_settings = {
    "type": "object",
    "properties": {
        "windows": {
            "type": "object",
            "properties": {
                "arch": {
                    "type": "object",
                    "properties": {
                        "x86": {
                            "type": "object",
                            "properties": {
                                "native-build-folder": {
                                    "type": "string"
                                }
                            },
                            "additionalProperties": False
                        },
                        "x64": {
                            "type": "object",
                            "properties": {
                                "native-build-folder": {
                                    "type": "string"
                                }
                            },
                            "additionalProperties": False
                        }
                    },
                    "additionalProperties": False
                },
                "test-gapi": {
                    "type": "array"
                },
                "test-names": {
                    "type": "array"
                }
            },
            "additionalProperties": False
        }
    },
    "additionalProperties": False
}


def update_default_settings(updated_settings, values_to_update):
    for key, value in values_to_update.items():
        if key in updated_settings:
            if isinstance(value, dict) and isinstance(updated_settings[key], dict):
                update_default_settings(updated_settings[key], value)
            else:
                updated_settings[key] = value


def parse_build_setting(json_path):
    try:
        with open(json_path) as json_setting:
            file_contents = json_setting.read()
        parsed_json = json.loads(file_contents)
        jsonschema.validate(instance=parsed_json, schema=jsonschema_settings)
        update_default_settings(default_settings, parsed_json)
    except jsonschema.exceptions.ValidationError as exc:
        raise Exception(exc)


def cmake_build_project(config, settings):
    subprocess.run(f"cmake -S . -B {settings['native-build-folder']} \
            -D CMAKE_BUILD_TYPE={config} \
            -D CMAKE_INSTALL_PREFIX={settings['native-build-folder']}/install -A {settings['cmake-generator-attribute']} \
            -D DILIGENT_BUILD_CORE_TESTS=ON", check=True)
    subprocess.run(f"cmake --build {settings['native-build-folder']} --target install --config {config}", check=True)

    native_dll_path = f"{project_paths['dotnet-build']}/{project_paths['dotnet-proj']}/native/{settings['nuget-name-folder']}"
    pathlib.Path(native_dll_path).mkdir(parents=True, exist_ok=True)
    for filename in os.listdir(native_dll_path):
        os.remove(os.path.join(native_dll_path, filename))

    for filename in os.listdir(f"{settings['native-build-folder']}/install/bin/{config}"):
        shutil.copy(f"{settings['native-build-folder']}/install/bin/{config}/{filename}", native_dll_path)

    for filename in os.listdir(native_dll_path):
        arguments = filename.split(".")
        dll_name = arguments[0].split("_")[0]
        src = f"{native_dll_path}/{filename}"
        dst = f"{native_dll_path}/{dll_name}.{arguments[-1]}"
        if not os.path.exists(dst):
            os.rename(src, dst)


def get_latest_tag_without_prefix():
    try:
        output = subprocess.check_output(['git', 'tag', '--list', '--sort=-creatordate'], encoding='utf-8')
        tags = output.strip().split('\n')
        tag_lst = tags[0]
        tag_api = next(filter(lambda item: item.startswith("API"), tags), None)
        tag_ver = next(filter(lambda item: item.startswith("v"), tags), None)
        if tag_lst.startswith("API"):
            postfix_tag_api = tag_lst.lstrip("API")
            postfix_tag_ver = tag_ver.lstrip("v").replace(".", "")
            if postfix_tag_api.startswith(postfix_tag_ver):
                postfix = postfix_tag_api[len(postfix_tag_ver):].lstrip("0")
                result = f"{tag_ver[1:]}-api{postfix}"
            else:
                raise Exception(f"Not expected tag: {tag_lst}")

        elif tag_lst.startswith("v"):
            result = tag_lst.lstrip("v")
        else:
            raise Exception(f"Not expected tag: {tag_lst}")
    except subprocess.CalledProcessError as exc:
        raise Exception(exc.output)
    return result


def dotnet_generate_version(path, is_local=True):
    version_str = get_latest_tag_without_prefix()
    if is_local:
        version_str += "-local"

    doc = minidom.Document()
    root = doc.createElement('Project')
    doc.appendChild(root)
    property_group = doc.createElement('PropertyGroup')
    root.appendChild(property_group)
    version = doc.createElement('PackageGitVersion')
    version_text = doc.createTextNode(version_str)
    version.appendChild(version_text)
    property_group.appendChild(version)
    xml_string = doc.toprettyxml(indent='\t')
    xml_string_without_declaration = '\n'.join(xml_string.split('\n')[1:])

    pathlib.Path(path).mkdir(parents=True, exist_ok=True)
    with open(f"{path}/Version.props", 'w') as file:
        file.write(xml_string_without_declaration)


def dotnet_pack_project(config, is_local):
    dotnet_generate_version(f"{project_paths['dotnet-build']}/{project_paths['dotnet-proj']}", is_local)
    subprocess.run(f"dotnet build -c {config} -p:Platform=x86 {project_paths['dotnet-proj']}", check=True)
    subprocess.run(f"dotnet build -c {config} -p:Platform=x64 {project_paths['dotnet-proj']}", check=True)
    subprocess.run(f"dotnet pack -c {config} {project_paths['dotnet-proj']}", check=True)


def dotnet_run_native_tests(config, settings, arch, gapi):
    for test_name in settings['test-names']:
        subprocess.run(f"{settings['arch'][arch]['native-build-folder']}/Tests/DiligentCoreAPITest/{config}/DiligentCoreAPITest.exe \
                    --mode={gapi} \
                    --gtest_filter={test_name}", cwd=project_paths['tests-assets'], check=True)


def dotnet_run_managed_tests(config, arch, gapi):
    os.environ["DILIGENT_GAPI"] = gapi
    os.environ["DOTNET_ROLL_FORWARD"] = "Major"
    subprocess.run(f"dotnet test -c {config} -p:Platform={arch} {project_paths['dotnet-tests']}", check=True)


def dotnet_copy_assets(src_dir, dst_dir):
    pathlib.Path(dst_dir).mkdir(parents=True, exist_ok=True)
    for root, dirs, files in os.walk(src_dir):
        for file in files:
            if "DotNet" in file:
                src_path = os.path.join(root, file)
                dst_path = os.path.join(dst_dir, file)
                shutil.copy2(src_path, dst_path)


def dotnet_copy_nuget_package(src_dir, dst_dir):
    pathlib.Path(dst_dir).mkdir(parents=True, exist_ok=True)
    for filename in os.listdir(src_dir):
        if filename.endswith(".nupkg"):
            shutil.copy(f"{src_dir}/{filename}", dst_dir)


def dotnet_restore_nuget_package(src_dir):
    shutil.rmtree(f"{src_dir}/RestoredPackages", ignore_errors=True)
    subprocess.run(f"dotnet restore --no-cache {src_dir}", check=True)


def dotnet_build_and_run_tests(config, settings, arch, is_local):
    dotnet_generate_version(f"{project_paths['dotnet-build']}/{project_paths['dotnet-tests']}", is_local)
    dotnet_copy_nuget_package(f"{project_paths['dotnet-build']}/{project_paths['dotnet-proj']}/bin/{config}",
                              f"{project_paths['dotnet-build']}/{project_paths['dotnet-tests']}/LocalPackages")
    dotnet_restore_nuget_package(project_paths['dotnet-tests'])

    for gapi in settings['test-gapi']:
        dotnet_run_native_tests(config, settings, arch, gapi)
        dotnet_copy_assets(f"{project_paths['tests-assets']}",
                           f"{project_paths['dotnet-build']}/{project_paths['dotnet-tests']}/assets")
        dotnet_run_managed_tests(config, arch, gapi)


def free_disk_memory(settings):
    build_folder = settings['native-build-folder']
    exceptions = [
        os.path.join(build_folder, "Tests"),
        os.path.join(build_folder, "install")
    ]
    for item in os.listdir(build_folder):
        item_path = os.path.join(build_folder, item)
        if item_path not in exceptions:
            if os.path.isfile(item_path):
                os.remove(item_path)
            elif os.path.isdir(item_path):
                shutil.rmtree(item_path, onerror=lambda func, path, exc_info: (os.chmod(path, stat.S_IWRITE), func(path)))


def main():
    parser = ArgumentParser("Build NuGet package")
    parser.add_argument("-c", "--configuration",
                        type=BuildType, choices=list(BuildType),
                        required=True)
    parser.add_argument("-d", "--root-dir",
                        required=True)
    parser.add_argument("-s", "--settings",
                        required=False)
    parser.add_argument('--dotnet-tests',
                       action=BooleanOptionalAction,
                       help="Use this flag to build and run .NET tests")
    parser.add_argument('--dotnet-publish',
                       action=BooleanOptionalAction,
                       help="Use this flag to publish nuget package")
    parser.add_argument('--free-memory',
                        action=BooleanOptionalAction,
                        help="Use this flag if there is not enough disk space to build this project")

    args = parser.parse_args()

    if args.settings:
        parse_build_setting(args.settings)

    os.chdir(args.root_dir)
    os_settings = default_settings['windows']
    arch_settings = os_settings['arch']

    for arch in arch_settings.keys():
        cmake_build_project(args.configuration, arch_settings[arch])

    if args.free_memory:
        for arch in arch_settings.keys():
            free_disk_memory(arch_settings[arch])

    dotnet_pack_project(args.configuration, not args.dotnet_publish)
    if args.dotnet_tests:
        for arch in arch_settings.keys():
            dotnet_build_and_run_tests(args.configuration, os_settings, arch, not args.dotnet_publish)


if __name__ == "__main__":
    main()
