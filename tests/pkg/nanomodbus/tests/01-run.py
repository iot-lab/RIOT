#!/usr/bin/env python3

# SPDX-FileCopyrightText: 2026 Inria
# SPDX-License-Identifier: LGPL-2.1-only

import sys
from testrunner import run


def testfunc(child):
    child.expect_exact("write 3 registers at 2: no error")
    child.expect_exact("read 5 registers at 1: no error")
    child.expect_exact("registers 1 to 5: 0x0000 0x1234 0xbeef 0x002a 0x0000")
    child.expect_exact("read 2 registers at 7: modbus exception 2: illegal data address")
    child.expect_exact("SUCCESS")


if __name__ == "__main__":
    sys.exit(run(testfunc))
