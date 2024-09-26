# -*- coding: utf-8 -*-
import unittest


class TestLibLoader(unittest.TestCase):
    def test_load_lib(self):
        from utils.lib_loader import LibLoader
        _wedpr_python_transport = LibLoader.load_lib()


if __name__ == '__main__':
    unittest.main()
