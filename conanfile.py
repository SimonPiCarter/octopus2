import os
import re
import glob

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv

# extracts the project name and version from CMake configuration file
def get_infos():
    try:
        content = load("CMakeLists.txt")
        infos = re.search(r"project\s*\(\s*(\w+)\s*\bVERSION\s*(\d+(\.\d+)*)", content, re.DOTALL)
        name = infos.group(1).strip()
        version = infos.group(2).strip()
        return name, version
    except Exception:
        return None,None

class octopusConan(ConanFile):
    name, version = get_infos()
    license = "Octopus Engine"
    url = "https://github.com/SimonPiCarter/octopus2"
    description = "Octopus Engine octopus2"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        }
    default_options = {
        "shared": False,
        "fPIC": True,
        }
    #build_policy = # can be never, missing, always
    #exports = # exports required files for setup
    #exports_sources = # exports required files for rebuilding (conan build)
    no_copy_source = True
    short_paths = True

    # manages options propagation
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    # configure
    #def configure(self):

    # manages requirements
    # allows for defining optional requirements
    def requirements(self):
        self.requires("boost/1.83.0")

    def build_requirements(self):
        self.test_requires("gtest/1.12.1")
        self.tool_requires("cmake/3.27.9")

    # copy required files from local store to project
    # handy for copying required shared libraries when testing
    #def imports(self):
    #    self.copy("*.dll", dst="bin", src="bin")
    #    self.copy("*.dylib*", dst="bin", src="lib")
    #    self.copy('*.so*', dst='bin', src='lib')

    # retrieves sources
    # def source(self):
    #     git = tools.Git()
    #     git.clone("git@edgitlab.eurodecision.Command:cfl/cfllpsp.git", branch=self.version, shallow=True)

    # builds project using standard CMake commands
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    # copies artifacts
    # uses CMake installed files
    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_find_mode", "non")

    def _gen_build_dir(self):
        return os.path.join("builds", str(self.settings.build_type))

    def layout(self):
        build_dir = self._gen_build_dir()
        cmake_config_path = os.path.join("lib", "cmake", f"{self.name}-{self.version}")
        cmake_config_path_flecs = os.path.join("lib", "cmake", "flecs")

        self.folders.build = build_dir
        self.folders.generators = os.path.join(self.folders.build)

        self.cpp.package.builddirs = [cmake_config_path, cmake_config_path_flecs]

        self.cpp.build.builddirs = [
            os.path.join("install", f"{self.name}-{self.version}", cmake_config_path),
            os.path.join("install", f"{self.name}-{self.version}", cmake_config_path_flecs),
        ]

    def generate(self):
        be = VirtualBuildEnv(self)
        be.generate()

        re = VirtualRunEnv(self)
        re.generate()

        tc = CMakeToolchain(self)
        tc.user_presets_path = False

        tc.variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"

        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    #def layout(self):
    #def test(self):
