# -*- coding: utf-8 -*-
import shutil
import pkg_resources
from wedpr_python_gateway_sdk.utils.lib_loader import LibLoader


def __bootstrap__():
    global __bootstrap__, __loader__, __file__
    import sys
    import pkg_resources
    import imp
    __file__ = pkg_resources.resource_filename(
        __name__, LibLoader.get_lib_name())
    __loader__ = None
    del __bootstrap__, __loader__
    imp.load_dynamic(__name__, __file__)


__bootstrap__()
