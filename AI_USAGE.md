# Generative AI Usage Disclosure

## Tools and dates

**Anthropic Claude (Sonnet 5)** was used to generate the C++ implementation and later to generate a large adversarial `tests.txt` file. The exact Claude model/version was not recorded in the saved transcript, so I am not claiming a specific version.

**OpenAI ChatGPT (GPT-5.6 Sol)** was used to help construct an adversarial testing prompt, troubleshoot how to supply multi-line input in the online compiler, review the test output, and help prepare this disclosure.

These tools were consulted on **September 27, 2026**.

## Code attribution

Claude generated the complete C++ implementation in response to my detailed code-generation prompt.

The exact prompt is preserved verbatim in:

`docs/code_generation_prompt.txt`

Claude's corresponding response is preserved in:

`docs/claude_code_response.md`

The submitted `ipv4_extractor.cpp` uses the generated implementation. I did **not** make parser-logic changes after Claude produced that implementation. For one online compiler, I temporarily used the filename `main.cpp` because that environment required it; this was only a filename/environment change and did not alter the program logic.

My work was therefore not to pretend the implementation was handwritten. My contribution was to specify the requirements precisely, inspect the generated code, understand how each part works, design and run adversarial tests, and verify that the implementation follows the required grammar and restrictions.

## How I critically reviewed the generated code

Because the assignment specifically warns that AI-generated parsers can look correct while containing subtle bugs, I did not treat successful compilation as proof of correctness.

I reviewed the implementation for the following behaviors:

- Each candidate is a maximal run of digits, periods, and colons rather than a valid-looking substring cut from a larger malformed token.
- Each IPv4 octet has 1–3 digits, is in the range 0–255, and rejects disallowed leading zeros.
- An optional port has 1–5 digits, is in the range 0–65535, and rejects disallowed leading zeros.
- If a colon is present and the port is invalid, the entire candidate is rejected rather than falling back to the address without a port.
- Numeric fields are accumulated manually rather than with prohibited conversion functions.
- The combined address is built as a 32-bit value using shifts.
- `0.0.0.0` is treated as a successful address even though its numeric value is zero.
- Invalid candidates do not prevent the scanner from continuing to a later candidate.
- The first valid candidate from left to right is selected.
- Exact `END` terminates the input loop, while variants such as `end`, `END `, and text containing `END` do not.
- End-of-file terminates normally.

## Testing process

I used ChatGPT to help draft a deliberately adversarial test-generation prompt, then supplied that prompt to Claude to create `tests.txt`.

The test file includes normal valid addresses, boundary values, invalid octets and ports, leading-zero cases, empty fields, extra or missing separators, malformed ports, very long digit sequences, multiple candidates on one line, valid-looking substrings inside invalid maximal tokens, punctuation and garbage delimiters, whitespace cases, and `END` variations.

I ran the test suite through the program using the online compiler's multi-line standard-input text mode. I then reviewed the resulting output against the grammar and the corresponding test inputs. No functional mismatch was identified in that run.

The fact that the first generated implementation did not require a parser-logic correction made additional testing more important, not less. I therefore used a much larger adversarial suite rather than assuming that compilation or a few normal examples proved correctness.

## Modifications to AI-generated output

Testing exposed some issues in earlier AI-generated versions of the
parser. 

### Invalid port handling

An earlier implementation could treat an invalid port as though no port
had been supplied. For example, `1.2.3.4:99999` could incorrectly fall
back to `1.2.3.4`.

I changed the parser so that once a colon is encountered, failure to
validate the port causes the entire candidate to fail.

I retested the port boundaries and malformed cases including `0`,
`65535`, `65536`, `080`, an empty port, and multiple colons.

### Leading zeros

Range checking alone was insufficient because values such as `001`
numerically evaluate to a legal octet value. I added a separate
structural check that rejects any multi-digit octet or port beginning
with `0`.

### Verification after corrections

After each correction I reran the relevant small regression cases first.
Once those passed, I reran the complete adversarial `tests.txt` suite to
check that the fix had not broken previously working cases.

## Verification statement

I understand the purpose and behavior of the submitted code, including token scanning, maximal-token validation, manual digit accumulation, octet and port validation, construction of the 32-bit address value, output parameters, output formatting, `END` handling, and end-of-file handling.

I tested the program with the supplied adversarial input file and confirmed that it behaves as intended for the cases reviewed.

I am not aware of a remaining functional parser bug from those tests. This testing is evidence of correctness, not a formal proof that no possible bug exists.

## Known limitation / unexpected behavior

The online compiler's **Standard Input → Text** interface may normalize some carriage-return or line-ending characters when large text is pasted into it. As a result, specially constructed byte-level CR/CRLF test cases may not be preserved exactly by that web interface.

The program itself explicitly handles a trailing carriage return, but the browser-based test interface is not an ideal tool for byte-exact line-ending tests. This is a limitation of the testing environment, not a known parser defect.

## Prompt and response records

For reproducibility, the repository preserves the exact code-generation material:

- `docs/code_generation_prompt.txt` — the exact prompt supplied to Claude for the C++ implementation.
- `docs/claude_code_response.md` — Claude's original response containing the generated implementation.

The later Claude conversation that generated `tests.txt` should also be retained if required by the instructor. If included, it can be saved as `docs/test_generation_transcript.md` rather than placing the entire conversation in the main README.
