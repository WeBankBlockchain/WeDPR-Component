# -*- coding: utf-8 -*-

import platform
import os
from enum import Enum
import sys
import ctypes


class OSType(Enum):
    MACOS_X64 = 1
    MACOS_ARM64 = 2
    WINDOWS = 3
    LINUX_X64 = 4
    LINUX_ARM64 = 5


class ArchType(Enum):
    ARM64 = 1
    X86_64 = 2


class LibLoader:
    # the library information
    TRANSPORT_LIB_PATH = "libs"
    TRANSPORT_LIB_NAME = "wedpr_python_transport"
    LIB_PREFIX = "lib"
    MACOS_LIB_POSTFIX = "dylib"
    WINDOWS_LIB_POSTFIX = "dll"
    LINUX_LIB_PORSTFIX = "so"
    # the OS name
    MACOS_OS = "darwin"
    WINDOWS_OS = "windows"
    LINUX_OS = "linux"

    AARCH64 = "aarch64"
    ARM64 = "arm64"
    X86_64 = "x86_64"

    @staticmethod
    def get_arch_type():
        arch_type = platform.machine().lower()
        if LibLoader.AARCH64 in arch_type or LibLoader.ARM64 in arch_type:
            return ArchType.ARM64
        if LibLoader.X86_64 in arch_type:
            return ArchType.X86_64
        raise Exception(f"Unsupported arch type: {arch_type}")

    @staticmethod
    def get_os_type():
        os_type = platform.system().lower()
        arch_type = LibLoader.get_arch_type()
        if LibLoader.MACOS_OS in os_type:
            if arch_type == ArchType.ARM64:
                return OSType.MACOS_ARM64
            else:
                return OSType.MACOS_X64
        if LibLoader.LINUX_OS in os_type:
            if arch_type == ArchType.ARM64:
                return OSType.LINUX_ARM64
            else:
                return OSType.LINUX_X64
        if LibLoader.WINDOWS_OS in os_type:
            return OSType.WINDOWS
        raise Exception(f"Unsupported os {os_type}, arch: {arch_type}")

    @staticmethod
    def get_lib_name():
        os_type = LibLoader.get_os_type()
        if os_type == OSType.WINDOWS:
            return "{}{}.{}".format(LibLoader.LIB_PREFIX, LibLoader.TRANSPORT_LIB_NAME, LibLoader.WINDOWS_LIB_POSTFIX)
        if os_type == OSType.MACOS_ARM64:
            return "{}{}-arm64.{}".format(LibLoader.LIB_PREFIX, LibLoader.TRANSPORT_LIB_NAME, LibLoader.MACOS_LIB_POSTFIX)
        if os_type == OSType.MACOS_X64:
            return "{}{}.{}".format(LibLoader.LIB_PREFIX, LibLoader.TRANSPORT_LIB_NAME, LibLoader.MACOS_LIB_POSTFIX)
        if os_type == OSType.LINUX_ARM64:
            return "{}{}.{}-arm64".format(LibLoader.LIB_PREFIX, LibLoader.TRANSPORT_LIB_NAME, LibLoader.LINUX_LIB_PORSTFIX)
        if os_type == OSType.LINUX_X64:
            return "{}{}.{}".format(LibLoader.LIB_PREFIX, LibLoader.TRANSPORT_LIB_NAME, LibLoader.LINUX_LIB_PORSTFIX)
        raise Exception(
            f"get_lib_name failed for not support the os_type: {os_type}")
