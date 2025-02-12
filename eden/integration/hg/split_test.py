#!/usr/bin/env python3
#
# Copyright (c) 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

from eden.integration.lib import hgrepo

from .lib.hg_extension_test_base import EdenHgTestCase, hg_test


@hg_test
class SplitTest(EdenHgTestCase):
    def populate_backing_repo(self, repo: hgrepo.HgRepository) -> None:
        repo.write_file("letters", "a\nb\nc\n")
        repo.write_file("numbers", "1\n2\n3\n")
        repo.commit("Initial commit.")

    def test_split_one_commit_into_two(self) -> None:
        """Split one commit with two files into two commits of one file each."""
        commits = self.repo.log(template="{desc}")
        self.assertEqual(["Initial commit."], commits)
        files = self.repo.log(template="{files}")
        self.assertEqual(["letters numbers"], files)

        editor = self.create_editor_that_writes_commit_messages(
            ["first commit", "second commit"]
        )

        # The responses are for the following questions:
        split_commands = (
            "y\n"  # examine changes to 'letters'?
            "y\n"  # record change 1/2 to 'letters'?
            "y\n"  # examine changes to 'numbers'?
            "n\n"  # record change 2/2 to 'numbers'?
            "y\n"  # Done splitting?
        )
        self.hg(
            "split",
            "--config",
            "ui.interactive=true",
            "--config",
            "ui.interface=text",
            input=split_commands,
            hgeditor=editor,
        )

        self.assert_status_empty()
        commits = self.repo.log(template="{desc}")
        self.assertEqual(["first commit", "second commit"], commits)
        files = self.repo.log(template="{files}")
        self.assertEqual(["letters", "numbers"], files)

    def test_abort_split_with_pending_add(self) -> None:
        self.write_file("letters", "abcd\n")
        self.write_file("new.txt", "new!\n")
        self.hg("add", "new.txt")
        self.assert_status({"letters": "M", "new.txt": "A"})
        self.repo.commit("modify letters and add new.txt")
        self.assert_status_empty()
        commits = self.repo.log()

        editor = self.create_editor_that_writes_commit_messages(
            ["just the modification"]
        )

        # The responses are for the following questions:
        split_commands = (
            "y\n"  # examine changes to 'letters'?
            "y\n"  # record change 1/2 to 'letters'?
            "n\n"  # examine changes to 'd.txt'?
            "n\n"  # Done splitting?
            "q\n"  # examine changes to 'd.txt'?
        )
        with self.assertRaises(hgrepo.HgError) as context:
            self.hg(
                "split",
                "--config",
                "ui.interactive=true",
                "--config",
                "ui.interface=text",
                input=split_commands,
                hgeditor=editor,
            )
        self.assert_status_empty()
        self.assertListEqual(commits, self.repo.log())
        # pyre-fixme[16]: `_E` has no attribute `returncode`.
        self.assertEqual(255, context.exception.returncode)
