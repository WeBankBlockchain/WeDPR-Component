# -*- coding: utf-8 -*-
import unittest
from wedpr_python_gateway_sdk.utils.lib_loader import LibLoader


class TestLibLoader(unittest.TestCase):
    def test_load_lib(self):
        LibLoader.get_lib_name()


if __name__ == '__main__':
    unittest.main()
