#include "asciiparsing.h"

bool isVchar(char c) {
    return 0x21 <= c && c <= 0x7E;
}

bool isVchar(QChar c) {
    return isVchar(c.unicode());
}

bool isDelim(char c) {
    switch (c) {
        case '(':
        case ')':
        case ',':
        case '/':
        case ':':
        case ';':
        case '<':
        case '=':
        case '>':
        case '?':
        case '@':
        case '[':
        case '\\':
        case ']':
        case '{':
        case '}':
            return true;
        default:
            return false;
    }
}

bool isDelim(QChar c) {
    return isDelim(c.unicode());
}

bool isHorizontalWhitespace(char c) {
    return c == ' ' || c == '\t';
}

bool isHorizontalWhitespace(QChar c) {
    return isHorizontalWhitespace(c.unicode());
}

bool isTchar(char c) {
    return isVchar(c) && !isDelim(c);
}

bool isTchar(QChar c) {
    return isTchar(c.unicode());
}
