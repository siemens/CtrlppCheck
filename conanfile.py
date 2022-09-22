from conans import ConanFile, CMake

class CtrlppCheckConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

#    requires = (
#        "tinyxml2/7.0.1@nicolastagliani/stable",
#    )

    generators = "cmake"

