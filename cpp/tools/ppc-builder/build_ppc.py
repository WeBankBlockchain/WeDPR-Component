#!/usr/bin/python
# -*- coding: UTF-8 -*-
# Note: here can't be refactored by autopep
import sys
sys.path.append("src/")

from controller import commandline_helper
from common import utilities


def main():
    try:
        args = commandline_helper.parse_command()
        commandline_helper.execute_command(args)
    except Exception as error:
        utilities.log_error("%s" % error)


if __name__ == "__main__":
    main()
