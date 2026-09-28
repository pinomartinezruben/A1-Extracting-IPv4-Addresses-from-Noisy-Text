// =====================================================================================
// ipv4_extractor.cpp
//
// What this program does:
//   It reads lines of text from the keyboard. For each line it looks for ONE valid IPv4
//   address (optionally followed by a port, like 192.168.1.1:80) hiding anywhere in the
//   text. If it finds one, it prints the address, its 32-bit decimal value, and the port.
//   It keeps asking for lines until you type END (exactly, in capital letters).
//
// The grammar in plain words (this describes ONE token):
//   octet.octet.octet.octet            or            octet.octet.octet.octet:port
//   - An octet is 1 to 3 digits, worth 0 to 255, with no leading zero unless it is
//     exactly "0". ("07" is bad, "0" is fine.)
//   - The port is 1 to 5 digits, worth 0 to 65535, with the same no-leading-zero rule.
//   - Only the characters 0-9, '.', and ':' can ever be part of a token. Every other
//     character is garbage: it is skipped, and it also separates one token from the next.
//   - A token must match the grammar COMPLETELY. We never hunt for a valid piece hiding
//     inside a longer bad token ("1.2.3.4.5" is rejected, not read as "1.2.3.4").
//   - If there is a colon, the port must be fully valid, or the whole token is rejected.
//   - The first valid token in the line (reading left to right) is the answer.
//
// How to compile:
//   g++ -std=c++17 -Wall -Wextra -pedantic -o ipv4 ipv4_extractor.cpp
// =====================================================================================

#include <iostream>
#include <string>
#include <cctype>

// ---------- Named constants ----------
// "const" is a promise to the compiler that the value never changes after it is set.
// Giving each number a name means the logic below has no mystery numbers in it.
// (These are fixed constants, not variables that change, so nothing here is "global state".)

const int NUM_OCTETS = 4;                   // an IPv4 address is four numbers
const int MAX_OCTET_DIGITS = 3;             // "255" has 3 digits; a 4th digit is always too many
const unsigned long MAX_OCTET_VALUE = 255;  // one octet is one byte, so 0 to 255
const int MAX_PORT_DIGITS = 5;              // "65535" has 5 digits; a 6th digit is always too many
const unsigned long MAX_PORT_VALUE = 65535; // biggest port number that fits in 16 bits
const int NO_PORT = -1;                     // special value meaning "this token had no port"
const char* const QUIT_WORD = "END";        // typing exactly this ends the program

// These three are also just named numbers, so the math below has no unexplained literals:
const unsigned long DECIMAL_BASE = 10;      // we read numbers in base 10 (the normal way)
const int BITS_PER_OCTET = 8;               // one octet is one byte, and a byte is 8 bits
const unsigned long OCTET_MASK = 0xFF;      // binary 11111111: selects the lowest 8 bits

// ---------- Prototypes ----------
// A prototype tells the compiler "this function exists, and here is what it looks like"
// so that main can be written first and the function bodies can come later.
//
// About the parameter types you will see:
//   - "const std::string&" means: use the caller's real string (no copy is made), and
//     "const" promises we will not change it.
//   - "unsigned long&" (a & with no const) is a reference: it IS the caller's real
//     variable, not a copy, so when we change it, the caller's variable changes too.
//     That is how a function can hand back more than one answer.
//   - "std::size_t" is a whole-number type made for sizes and positions in strings.
//     It can never be negative.
//
// The helpers live in an anonymous namespace, which means they are private to this file.
namespace {
bool isDigitChar(char c);
bool isTokenChar(char c);
bool readNumberField(const std::string& s, std::size_t& pos, std::size_t end, int maxDigits,
                     unsigned long maxValue, unsigned long& outValue);
bool parseCandidate(const std::string& s, std::size_t start, std::size_t end,
                    unsigned long& outAddress, int& outPort);
}

bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort);

// ---------- main ----------
// Input: nothing (it reads from the keyboard). Output: returns 0 when it finishes.
// Keeps asking for lines, tries to extract an IPv4 address from each one, and prints
// the result, until the user types END or the input runs out.
int main()
{
    // This holds the line the user typed.
    std::string line;

    while (true) {
        // Show the prompt. "<<" here means "send this to the output". std::flush forces
        // the text onto the screen right now, because there is no newline at the end
        // and otherwise the prompt might sit in a buffer and not show up.
        std::cout << "Enter a line of text (or END to quit): " << std::flush;

        // std::getline reads a whole line, spaces and all, and works for any length.
        // It returns something that acts like false if there is nothing left to read
        // (end of input, like pressing Ctrl-D). In that case we quit as if END was typed.
        if (!std::getline(std::cin, line)) {
            break;
        }

        // Files made on Windows end lines with '\r' then '\n'. getline removes the '\n'
        // but leaves the '\r', so we remove exactly one '\r' if it is the last character.
        // We touch nothing else: spaces at the start or end stay in the line.
        if (!line.empty()) {
            // size() gives the number of characters as a std::size_t. Counting starts at
            // 0, so the last character sits at position size() - 1.
            std::size_t lastIndex = line.size() - 1;
            if (line[lastIndex] == '\r') {
                line.pop_back();
            }
        }

        // Compare the WHOLE line to "END". Upper and lower case count as different, and
        // extra spaces count too, so "end", "END " and " END" do not quit the program.
        if (line == QUIT_WORD) {
            break;
        }

        // These two variables are where extractIPv4 will put its answers. It resets them
        // itself first, so these starting values don't matter, but it is tidy to set them.
        unsigned long address = 0;
        int port = NO_PORT;

        // Whether we succeeded is decided ONLY by this return value. We never guess from
        // address == 0 or port == -1, because "0.0.0.0" is a real, valid address.
        bool found = extractIPv4(line, address, port);

        if (found) {
            // The function gave us one 32-bit number, so we rebuild A.B.C.D from it.
            // A number's bits sit in a row, and ">>" slides them to the right. Sliding
            // right by 8 pushes the lowest byte off the end and brings the next byte
            // down into the lowest spot.
            // "& OCTET_MASK" (that is & 0xFF) keeps only the lowest 8 bits, which is
            // exactly one byte, and clears everything above it.
            // So: D is the lowest byte, then slide 8 and take C, slide 8 and take B,
            // slide 8 and take A. (This gives the same answer as (address >> 24) & 0xFF
            // for A, (address >> 16) & 0xFF for B, and (address >> 8) & 0xFF for C.)
            unsigned long remaining = address;
            unsigned long octetD = remaining & OCTET_MASK;
            remaining = remaining >> BITS_PER_OCTET;
            unsigned long octetC = remaining & OCTET_MASK;
            remaining = remaining >> BITS_PER_OCTET;
            unsigned long octetB = remaining & OCTET_MASK;
            remaining = remaining >> BITS_PER_OCTET;
            unsigned long octetA = remaining & OCTET_MASK;

            // Print the first part of the message, up to where the port goes.
            std::cout << "Extracted IPv4 address: " << octetA << "." << octetB << "."
                      << octetC << "." << octetD << " (decimal value: " << address
                      << ", port: ";

            // NO_PORT (-1) means there was no port, so print the word "none" instead.
            // Port 0 is a real port, so it prints as 0.
            if (port == NO_PORT) {
                std::cout << "none";
            } else {
                std::cout << port;
            }

            // Finish the line.
            std::cout << ")\n";
        } else {
            std::cout << "No valid IPv4 address found.\n";
        }
    }

    // We get here after END or after the input ended.
    std::cout << "Program terminated." << std::endl;
    return 0;
}

// ---------- Helper functions ----------
namespace {

// isDigitChar: takes one character, gives back true if it is '0' through '9'.
bool isDigitChar(char c)
{
    // "char" might be a signed type on this computer, so a non-English character could
    // show up as a negative number. isdigit is only safe with values 0..255 (or EOF), so
    // we first turn the char into an unsigned char, which is always 0..255.
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

// isTokenChar: takes one character, gives back true if it can be part of a token.
// The only characters that can be part of a token are digits, '.', and ':'.
// Everything else (letters, spaces, '-', ',', non-English characters...) is garbage.
bool isTokenChar(char c)
{
    if (isDigitChar(c)) {
        return true;
    }
    if (c == '.') {
        return true;
    }
    if (c == ':') {
        return true;
    }
    return false;
}

// readNumberField: reads ONE number (an octet or a port) from s, starting at pos and
// never looking at position end or later.
// In:  the text s, the current position pos, the end of the token, the most digits
//      allowed, and the biggest value allowed.
// Out: returns true if a valid number was read. Then outValue holds the number and pos
//      has moved forward to just past the digits. Returns false if the number is bad.
// Note: pos is a reference, so it is the caller's real position variable. When we move
//       it forward here, the caller's position moves too.
// The same function handles octets (3 digits, max 255) and ports (5 digits, max 65535).
bool readNumberField(const std::string& s, std::size_t& pos, std::size_t end, int maxDigits,
                     unsigned long maxValue, unsigned long& outValue)
{
    // Remember where the digits began, so we can look at the first digit afterwards.
    std::size_t digitStart = pos;
    int count = 0;
    unsigned long value = 0;

    // Keep going while we are still inside the token AND the current character is a digit.
    while (pos < end && isDigitChar(s[pos])) {
        // Check the digit count BEFORE adding this digit. If we already have the most
        // digits allowed, this one would be one too many (like the 4th digit in "1234").
        // Checking first means value can never grow big enough to overflow, and a huge
        // run of digits like "99999999999999999999" fails quickly.
        if (count == maxDigits) {
            return false;
        }

        // s[pos] is one character. Characters are secretly stored as numbers, and the
        // digits '0'..'9' sit right next to each other in order, so s[pos] - '0' turns
        // the character '7' into the real number 7.
        // Multiplying value by DECIMAL_BASE (10) slides the old digits one place left,
        // then we add the new digit (like building 4 -> 45 -> 456 as we read '4','5','6').
        value = value * DECIMAL_BASE + static_cast<unsigned long>(s[pos] - '0');
        count++;
        pos++;
    }

    // No digits at all means an empty field, like the middle of "1..3" or a missing
    // port in "1.2.3.4:".
    if (count == 0) {
        return false;
    }

    // More than one digit and the first one is '0' is a leading zero: "01", "007" and
    // "00" are thrown out. A single "0" is fine.
    if (count > 1 && s[digitStart] == '0') {
        return false;
    }

    // Too big: "256" is over 255 for an octet, and "65536" is over 65535 for a port.
    if (value > maxValue) {
        return false;
    }

    // Everything checked out, so hand the number back.
    outValue = value;
    return true;
}

// parseCandidate: checks ONE token, the characters s[start] up to (but not including)
// s[end], against the whole grammar.
// In:  the text s, where the token starts, and where it ends (one past its last char).
// Out: returns true if the entire token is valid. Then outAddress holds the 32-bit
//      address and outPort holds the port, or NO_PORT if the token had none.
//      If it returns false, outAddress and outPort are left alone.
bool parseCandidate(const std::string& s, std::size_t start, std::size_t end,
                    unsigned long& outAddress, int& outPort)
{
    // pos is our current spot in the token. address is built in a local variable, so a
    // half-finished token can never leak into the caller's variables.
    std::size_t pos = start;
    unsigned long address = 0;

    // Read the four octets. i counts which octet we are on (0, 1, 2, 3).
    for (int i = 0; i < NUM_OCTETS; i++) {
        unsigned long octetValue = 0;

        // Read one octet: 1 to 3 digits, 0 to 255, no leading zero.
        // This rejects "256", "01", "1234", and an empty octet as in "1..3".
        if (!readNumberField(s, pos, end, MAX_OCTET_DIGITS, MAX_OCTET_VALUE, octetValue)) {
            return false;
        }

        // Add this octet to the address. "<<" on a number slides its bits to the left.
        // Sliding left by 8 makes room for one more byte at the bottom (the way 12 becomes
        // 1200 when you slide decimal digits left). "|" then drops the new octet into
        // that empty room. Example: 192 then 168 gives (192 << 8) | 168, so the first
        // octet you write ends up as the most significant byte.
        address = (address << BITS_PER_OCTET) | octetValue;

        // After every octet except the last, there must be exactly one '.'.
        // This rejects "1.2.3" (runs out too soon) and "1:2.3.4" (wrong separator).
        if (i < NUM_OCTETS - 1) {
            if (pos >= end || s[pos] != '.') {
                return false;
            }
            pos++;
        }
    }

    // Assume there is no port until we find one.
    int port = NO_PORT;

    // If there are characters left after the 4th octet, the only thing allowed is
    // ':' followed by a port.
    if (pos < end) {
        // Anything other than ':' here is a problem. For example "1.2.3.4." has a stray
        // period after the last octet, so we throw it out.
        if (s[pos] != ':') {
            return false;
        }
        pos++;

        // Read the port: 1 to 5 digits, 0 to 65535, no leading zero.
        // This rejects "1.2.3.4:" (empty port), "1.2.3.4:080" (leading zero),
        // "1.2.3.4:65536" (too big), and "1.2.3.4::80" (a colon where digits should be).
        // If the port is bad we reject the WHOLE token. We do not fall back to returning
        // the address with "no port".
        unsigned long portValue = 0;
        if (!readNumberField(s, pos, end, MAX_PORT_DIGITS, MAX_PORT_VALUE, portValue)) {
            return false;
        }
        port = static_cast<int>(portValue);
    }

    // If we did not land exactly on the end of the token, something was left over.
    // For example "1.2.3.4:5:6" has a second colon, and "1.2.3.4:80." has a period at
    // the end. Either way the token does not match the grammar completely.
    if (pos != end) {
        return false;
    }

    // The whole token is valid, so it is now safe to write the answers.
    outAddress = address;
    outPort = port;
    return true;
}

}  // end of anonymous namespace

// extractIPv4: finds the first valid IPv4 address (with an optional port) in str.
// In:  the text to search, str.
// Out: returns true if a valid address was found, false otherwise.
//      On success, outAddress is the 32-bit value and outPort is the port (or -1 for
//      no port). On failure, outAddress is 0 and outPort is -1.
// It never prints anything and never reads input.
// outAddress and outPort are references, so they are the caller's real variables.
bool extractIPv4(const std::string& str, unsigned long& outAddress, int& outPort)
{
    // Start by setting the failure values. If we find nothing, these are what the
    // caller sees. We only overwrite them after a token is fully validated.
    outAddress = 0;
    outPort = NO_PORT;

    std::size_t i = 0;
    while (i < str.size()) {
        // A character that cannot be part of a token is garbage, so skip it.
        // Garbage acts as a separator: it splits the line into separate tokens and the
        // pieces on either side are NEVER glued back together. For example "192.168.1x.5"
        // is two tokens, "192.168.1" and ".5", and neither is valid. If we deleted the 'x'
        // we would wrongly get "192.168.1.5", which is not what was typed.
        if (!isTokenChar(str[i])) {
            i++;
            continue;
        }

        // We found the first character of a token. Now extend over EVERY token character
        // next to it, so the token is the maximal run. We never begin in the middle of a
        // run and never stop early, so "1.2.3.4.5" stays one token and gets rejected as a
        // whole instead of being cut down to "1.2.3.4".
        std::size_t start = i;
        while (i < str.size() && isTokenChar(str[i])) {
            i++;
        }
        // i now sits one past the last character of the token.
        std::size_t end = i;

        // Check the whole token. We use local temporaries so that a token that fails
        // can never leave half-finished values in the caller's variables.
        unsigned long foundAddress = 0;
        int foundPort = NO_PORT;
        if (parseCandidate(str, start, end, foundAddress, foundPort)) {
            // First valid token wins, so copy the answers out and stop right away.
            outAddress = foundAddress;
            outPort = foundPort;
            return true;
        }

        // This token was invalid, so just move on. i is already at the end of the bad
        // token, so the loop carries on with whatever comes after it.
    }

    // We went through the whole line and no token was valid.
    return false;
}
