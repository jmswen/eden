#!/usr/bin/env python3
#
# Copyright (c) 2016-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

import unittest

from eden.cli.tabulate import tabulate


eol = ""


class TabulateTest(unittest.TestCase):
    def test_tabulate(self):
        output = tabulate(
            ["a", "b", "c"],
            rows=[
                {"a": "a_1", "b": "b_1", "c": "see_1"},
                {"a": "a_two", "b": "b_2", "c": "c_2"},
            ],
        )
        self.assertEqual(
            output,
            f"""\
A     B   C    {eol}
a_1   b_1 see_1{eol}
a_two b_2 c_2  {eol}""",
        )

    def test_tabulate_header_labels(self):
        output = tabulate(
            ["a", "b", "c"],
            rows=[
                {"a": "a_1", "b": "b_1", "c": "see_1"},
                {"a": "a_two", "b": "b_2", "c": "c_2"},
            ],
            header_labels={
                "a": "Col1",
                "b": "bee",
                # omitting c so that we can test defaulting
            },
        )
        self.assertEqual(
            output,
            f"""\
Col1  bee C    {eol}
a_1   b_1 see_1{eol}
a_two b_2 c_2  {eol}""",
        )
