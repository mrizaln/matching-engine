from conan import ConanFile
from conan.tools.cmake import cmake_layout

class Recipe(ConanFile):
    settings      = ["os", "compiler", "build_type", "arch"]
    generators    = ["CMakeToolchain", "CMakeDeps"]
    requires      = ["fmt/11.0.2", "asio/1.29.0", "spdlog/1.15.0", "rapidjson/cci.20230929"]
    test_requires = ["boost-ext-ut/1.1.9"]

    def layout(self):
        cmake_layout(self)
