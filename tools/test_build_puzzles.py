#!/usr/bin/env python3
"""Unit tests for tools/build_puzzles.py validation rules."""

import unittest
from build_puzzles import validate_puzzle, validate_all


def make_valid_puzzle(pid=1, overrides=None):
    """Helper: build a structurally-valid puzzle, then apply overrides."""
    base = {
        "id": pid,
        "categories": [
            {"tier": "yellow", "name": "AAA", "words": ["A", "B", "C", "D"]},
            {"tier": "green",  "name": "BBB", "words": ["E", "F", "G", "H"]},
            {"tier": "blue",   "name": "CCC", "words": ["I", "J", "K", "L"]},
            {"tier": "purple", "name": "DDD", "words": ["M", "N", "O", "P"]},
        ]
    }
    if overrides:
        base.update(overrides)
    return base


class TestValidate(unittest.TestCase):
    def test_valid_puzzle_passes(self):
        validate_puzzle(make_valid_puzzle())  # should not raise

    def test_missing_tier(self):
        # Replace purple with yellow → tiers are {yellow, yellow, green, blue}
        p = make_valid_puzzle()
        p["categories"][3]["tier"] = "yellow"
        with self.assertRaisesRegex(ValueError, r"tier"):
            validate_puzzle(p)

    def test_wrong_word_count(self):
        p = make_valid_puzzle()
        p["categories"][0]["words"] = ["A", "B", "C"]
        with self.assertRaisesRegex(ValueError, r"4 words"):
            validate_puzzle(p)

    def test_name_too_long(self):
        p = make_valid_puzzle()
        p["categories"][0]["name"] = "A" * 13
        with self.assertRaisesRegex(ValueError, r"name"):
            validate_puzzle(p)

    def test_name_invalid_chars(self):
        p = make_valid_puzzle()
        p["categories"][0]["name"] = "bird-1"  # lowercase + hyphen + digit
        with self.assertRaisesRegex(ValueError, r"name"):
            validate_puzzle(p)

    def test_word_too_long(self):
        p = make_valid_puzzle()
        p["categories"][0]["words"][0] = "SPAGHETTI"  # 9 chars
        with self.assertRaisesRegex(ValueError, r"length|word"):
            validate_puzzle(p)

    def test_word_lowercase(self):
        p = make_valid_puzzle()
        p["categories"][0]["words"][0] = "robin"
        with self.assertRaisesRegex(ValueError, r"word"):
            validate_puzzle(p)

    def test_word_with_punctuation(self):
        p = make_valid_puzzle()
        p["categories"][0]["words"][0] = "DON'T"
        with self.assertRaisesRegex(ValueError, r"word"):
            validate_puzzle(p)

    def test_duplicate_word_within_puzzle(self):
        p = make_valid_puzzle()
        p["categories"][1]["words"][0] = "A"  # "A" also in category 0
        with self.assertRaisesRegex(ValueError, r"duplicate"):
            validate_puzzle(p)

    def test_name_word_collision(self):
        # Category 0 is named "AAA"; put "AAA" as a word in category 1.
        # "AAA" is a valid word per rule 4 (3 uppercase chars).
        p = make_valid_puzzle()
        p["categories"][1]["words"][0] = "AAA"
        with self.assertRaisesRegex(ValueError, r"collision|name.*word"):
            validate_puzzle(p)


class TestValidateAll(unittest.TestCase):
    def test_sequential_ids_pass(self):
        data = {"puzzles": [make_valid_puzzle(1), make_valid_puzzle(2), make_valid_puzzle(3)]}
        validate_all(data)  # should not raise

    def test_id_gap_fails(self):
        data = {"puzzles": [make_valid_puzzle(1), make_valid_puzzle(3)]}  # gap at 2
        with self.assertRaisesRegex(ValueError, r"sequential|id"):
            validate_all(data)

    def test_draft_excluded_from_id_check(self):
        # Draft puzzles are excluded; remaining non-drafts must still be 1, 2 in order
        data = {"puzzles": [
            make_valid_puzzle(1),
            {**make_valid_puzzle(99), "draft": True},
            make_valid_puzzle(2),
        ]}
        validate_all(data)  # should not raise


if __name__ == "__main__":
    unittest.main()
