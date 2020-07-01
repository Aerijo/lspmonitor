#ifndef ASCIIPARSING_H
#define ASCIIPARSING_H

#include <QChar>

bool isDelim(char c);

bool isHorizontalWhitespace(char c);

bool isVchar(char c);

bool isTchar(char c);

bool isDelim(QChar c);

bool isHorizontalWhitespace(QChar c);

bool isVchar(QChar c);

bool isTchar(QChar c);

#endif // ASCIIPARSING_H
