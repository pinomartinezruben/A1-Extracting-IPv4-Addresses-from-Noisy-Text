# IPv4 Extractor

A C++17 program that scans each input line for the first valid IPv4 address, optionally followed by a port number. Candidate tokens are validated character-by-character without regular expressions or library string-to-number conversion functions.

## Build and run

```bash
g++ -std=c++17 -Wall -Wextra -pedantic -o ipv4 ipv4_extractor.cpp
./ipv4
```

To run the adversarial test input file:

```bash
./ipv4 < tests.txt
```

The program also terminates normally at end-of-file, so redirected input does not require interactive keyboard entry.

## Testing

`tests.txt` contains hundreds of valid, invalid, boundary, malformed, and adversarial inputs. It exercises octet and port limits, leading zeros, malformed separators, maximal candidate tokens, garbage delimiters, multiple candidates, `END` variants, long numeric runs, and embedded addresses.

The test suite was run against the program in an online C++ compiler. The output was then reviewed against the grammar and test inputs. No functional mismatch was identified in that run.

## Generative AI disclosure

Generative AI was intentionally used for this assignment, as required by the assignment instructions.

The detailed disclosure is in **[AI_USAGE.md](AI_USAGE.md)**.

The exact prompt used to generate the C++ implementation is preserved verbatim in **[docs/code_generation_prompt.txt](docs/code_generation_prompt.txt)**. The corresponding Claude response is preserved in **[docs/claude_code_response.md](docs/claude_code_response.md)**.

## Files

- `ipv4_extractor.cpp` — submitted C++ implementation
- `tests.txt` — adversarial test inputs
- `AI_USAGE.md` — AI usage, attribution, review, testing, and verification
- `docs/code_generation_prompt.txt` — exact code-generation prompt
- `docs/claude_code_response.md` — Claude's original code-generation response
