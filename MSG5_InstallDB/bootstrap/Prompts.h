#ifndef MSG5_BOOTSTRAP_PROMPTS_H
#define MSG5_BOOTSTRAP_PROMPTS_H

#include <string>
#include <iostream>
#include <cstdio>
#include <cctype>
#include <stdexcept>

#ifdef _WIN32
#  include <conio.h>
#else
#  include <termios.h>
#  include <unistd.h>
#endif

namespace bootstrap {

// Read a line with optional default value shown in brackets.
static inline std::string prompt_line(const char* label, const std::string& def = "") {
    std::string v;
    std::cout << label;
    if (!def.empty()) std::cout << " [" << def << "]";
    std::cout << ": ";
    std::getline(std::cin, v);
    if (v.empty()) v = def;
    return v;
}

// Read a password (hidden input). Uses _getch on Windows; termios on POSIX.
static inline std::string prompt_hidden(const char* label) {
    std::cout << label << ": ";
    std::string s;
#ifdef _WIN32
    for (;;) {
        int ch = _getch();
        if (ch == '\r' || ch == '\n') break;
        if (ch == 8 /*backspace*/) {
            if (!s.empty()) s.pop_back();
            continue;
        }
        if (ch == 3 /*Ctrl+C*/) { std::cout << "\n"; throw std::runtime_error("^C"); }
        if (ch >= 32 && ch <= 126) s.push_back(static_cast<char>(ch));
    }
    std::cout << "\n";
#else
    termios oldt{};
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    std::getline(std::cin, s);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    std::cout << "\n";
#endif
    return s;
}

// Yes/No prompt; returns true for y/yes (case-insensitive).
static inline bool msg5_prompt_yes_no(const std::string& question) {
    std::fprintf(stdout, "[confirm] %s [y/N]: ", question.c_str());
    std::fflush(stdout);
    std::string line;
    std::getline(std::cin, line);
    for (auto& ch : line) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return (line == "y" || line == "yes");
}

} // namespace bootstrap
#endif // MSG5_BOOTSTRAP_PROMPTS_H
