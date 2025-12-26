/*
	SPDX-FileCopyrightText: 2025 cbidea

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "mtxt_reader.h"

#include <QTextStream>
#include <QTextBlockFormat>

//-----------------------------------------------------------------------------

bool MtxtReader::canRead(QIODevice* device)
{
    return device->peek(9) == "/*MTXT1*/";
}

//-----------------------------------------------------------------------------

void MtxtReader::readData(QIODevice* device)
{
    device->setTextModeEnabled(true);

    QByteArray header = device->readLine();
    if (!header.startsWith("/*MTXT1*/")) {
        m_error = QStringLiteral("Not a Marked Text file.");
        return;
    }

    QTextStream stream(device);
    stream.setEncoding(QStringConverter::Utf8);

    m_cursor.beginEditBlock();

    bool first = true;

    while (!stream.atEnd()) {
        QString line = stream.readLine();

        if (!first) {
			m_cursor.insertBlock();

			QTextBlockFormat clean;
			clean.setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
			m_cursor.setBlockFormat(clean);
		}
		first = false;

        QTextCharFormat current;
        QString buffer;

        bool centered = false;

        auto flush = [&]() {
            if (!buffer.isEmpty()) {
                m_cursor.insertText(buffer, current);
                buffer.clear();
            }
        };

        for (int i = 0; i < line.length(); ++i) {
            const QChar c = line.at(i);

            if (c == '\\' && i + 1 < line.length()) {
                buffer += line.at(++i);
                continue;
            }

            if (i == 0 && c == '>') {
                QTextBlockFormat bf = m_cursor.blockFormat();
                bf.setAlignment(Qt::AlignCenter);
                m_cursor.mergeBlockFormat(bf);
                centered = true;
                continue;
            }

            if (centered && c == '<' && i == line.length() - 1)
                continue;

            if (c == '*' && line.mid(i,3) == "***") {
                flush();
                current.setFontWeight(current.fontWeight()==QFont::Bold ? QFont::Normal : QFont::Bold);
                current.setFontItalic(!current.fontItalic());
                i += 2;
                continue;
            }
            if (c == '*' && line.mid(i,2) == "**") {
                flush();
                current.setFontWeight(current.fontWeight()==QFont::Bold ? QFont::Normal : QFont::Bold);
                i += 1;
                continue;
            }
            if (c == '*') {
                flush();
                current.setFontItalic(!current.fontItalic());
                continue;
            }
            if (c == '_') {
                flush();
                current.setFontUnderline(!current.fontUnderline());
                continue;
            }
            if (c == '~' && line.mid(i,2) == "~~") {
                flush();
                current.setFontStrikeOut(!current.fontStrikeOut());
                i += 1;
                continue;
            }

            buffer += c;
        }

        flush();
    }

    m_cursor.endEditBlock();
}
