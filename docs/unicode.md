# Unicode

`core/unicode.h` provides allocation-free UTF-8 decoding and per-code-point terminal column widths. It has no locale or platform dependency.

`unicode_decode_utf8(Slice<const char>)` reads the first Unicode scalar from the provided byte range. Its `Unicode_Utf8_Character` result contains `code_point` and `byte_count`. A zero byte count indicates empty input or invalid UTF-8. A valid encoded null character returns code point zero and byte count one. The decoder rejects truncation, invalid continuation bytes, overlong sequences, surrogates, and values above U+10FFFF. It does not consume subsequent characters or replace invalid input; the caller chooses how to recover.

`unicode_get_column_width(U32)` returns a conventional terminal cell width: zero for null, nonspacing/enclosing marks, format characters, and conjoining Hangul vowel/trailing jamo; two for East Asian Wide/Fullwidth characters; one for other Unicode scalars. Ambiguous-width characters use one column. Control characters other than null, line/paragraph separators, and invalid scalars return -1. Tabs require caller-defined expansion.

These are individual code-point widths, not font measurements or grapheme-cluster shaping. Summing widths does not model joined emoji, flags, or presentation sequences. Terminal fonts and width policies can differ. Byte offsets, UTF-16 positions, and display columns are separate quantities.

The tables in `core/unicode.cpp` are derived from Unicode 17.0.0 [EastAsianWidth.txt](https://www.unicode.org/Public/17.0.0/ucd/EastAsianWidth.txt) and [DerivedGeneralCategory.txt](https://www.unicode.org/Public/17.0.0/ucd/extracted/DerivedGeneralCategory.txt). They are compiled directly into Core; no generation step, Python, or network access is required. Updating the Unicode version requires replacing these tables.

The full Unicode V3 notice is retained in the top comment of [core/unicode.cpp](../core/unicode.cpp). Binary distributions that include these tables must also carry the notice; it can be included in their existing license documentation.