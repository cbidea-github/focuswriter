/*
    SPDX-FileCopyrightText: 2025 cbidea
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "blm_reader.h"

#include <QIODevice>
#include <QTextStream>
#include <QFont>
#include <QTextBlockFormat>
#include <QTextFormat>

//------------------------------------------------------------

bool BlmReader::canRead(QIODevice* device)
{
    return device && device->isOpen() && device->peek(8) == "::BLM1::";
}

//------------------------------------------------------------

void BlmReader::readData(QIODevice* device)
{
    device->setTextModeEnabled(true);

    QTextStream stream(device);
    stream.setEncoding(QStringConverter::Utf8);

    QString header = stream.readLine();
    if (!header.startsWith("::BLM1::"))
        return;

    m_cursor.beginEditBlock();

    m_baselineBlockFormat = m_cursor.blockFormat();
    m_inline = m_cursor.charFormat();

    while (!stream.atEnd()) {
        processLine(stream.readLine());
    }

    m_cursor.endEditBlock();
}

//------------------------------------------------------------

void BlmReader::processLine(const QString& line)
{
    /* ===== Empty line → real newline ===== */

    if (line.trimmed().isEmpty()) {

        if (m_usedInitialBlock)
            m_cursor.insertBlock();
        else
            m_usedInitialBlock = true;

        return;
    }

    bool blockReady = false;
    QString buffer;

    auto ensureBlock = [&]() {
        if (blockReady)
            return;

        QTextBlockFormat bf;

        if (!m_usedInitialBlock) {
            bf = m_baselineBlockFormat;
            m_cursor.mergeBlockFormat(bf);
            m_usedInitialBlock = true;
        } else {
            bf = m_cursor.blockFormat();
            m_cursor.insertBlock(bf, m_inline);
        }

        bf.setAlignment(m_block.alignment);
        bf.setIndent(m_block.indent);
        bf.setHeadingLevel(m_block.heading);
        m_cursor.mergeBlockFormat(bf);

        blockReady = true;
    };

    auto flush = [&]() {
        if (!buffer.isEmpty()) {
            ensureBlock();
            m_cursor.insertText(buffer, m_inline);
            buffer.clear();
        }
    };

    int i = 0;
    while (i < line.size()) {

        /* ===== Page break: \{p}  (OPTION B) ===== */

        if (i + 4 <= line.size() &&
            line.mid(i, 4) == "\\{p}") {

            flush();

            // cerrar inline
            m_inlineStack.clear();
            m_inline = QTextCharFormat();

            // resetear bloque
            m_block = BlockState();
            m_blockStack.clear();
            m_ignoredAlignment = false;

            // nuevo bloque con salto de página
            QTextBlockFormat bf = m_baselineBlockFormat;
            bf.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
            m_cursor.insertBlock(bf);

            m_usedInitialBlock = true;

            i += 4;
            continue;
        }

        /* ===== Escape: \{t … \t} ===== */

        if (i + 3 <= line.size() &&
            line.at(i) == '\\' &&
            line.at(i + 1) == '{' &&
            line.at(i + 2) == 't') {

            i += 3;

            // caso especial: \{t\t} → literal "\t}"
            if (i + 2 < line.size() &&
                line.at(i) == '\\' &&
                line.at(i + 1) == 't' &&
                line.at(i + 2) == '}') {

                buffer += "\\t}";
                i += 3;
                continue;
            }

            // escape normal
            while (i < line.size()) {
                if (i + 2 < line.size() &&
                    line.at(i) == '\\' &&
                    line.at(i + 1) == 't' &&
                    line.at(i + 2) == '}') {
                    i += 3;
                    break;
                }
                buffer += line.at(i++);
            }

            continue;
        }

        /* ===== Block open: alignment + optional indent ===== */

        if (i + 3 <= line.size() &&
            line.at(i) == '\\' &&
            line.at(i + 1) == '{') {

            QChar t = line.at(i + 2);

            if (t == 'l' || t == 'r' || t == 'c' || t == 'j') {

                int j = i + 3;
                int indent = 0;
                bool hasDigits = false;

                while (j < line.size() && line.at(j).isDigit()) {
                    hasDigits = true;
                    indent = indent * 10 + line.at(j).digitValue();
                    ++j;
                }

                if (indent > 9)
                    indent = 9;

                if (m_block.alignment != Qt::AlignLeft) {
                    m_ignoredAlignment = true;
                    i = j;
                    continue;
                }

                BlockState next = m_block;

                switch (t.unicode()) {
                    case 'l': next.alignment = Qt::AlignLeft;    break;
                    case 'r': next.alignment = Qt::AlignRight;   break;
                    case 'c': next.alignment = Qt::AlignCenter;  break;
                    case 'j': next.alignment = Qt::AlignJustify; break;
                }

                next.indent = hasDigits ? indent : 0;

                m_blockStack.push(m_block);
                m_block = next;

                i = j;
                continue;
            }

            /* ===== Heading open ===== */

            if (t == 'h' &&
                i + 3 < line.size() &&
                line.at(i + 3).isDigit()) {

                int lvl = line.at(i + 3).digitValue();

                if (m_block.heading == 0 && lvl >= 1 && lvl <= 6) {
                    m_blockStack.push(m_block);
                    m_block.heading = lvl;
                }

                i += 4;
                continue;
            }
        }

        /* ===== Block close ===== */

        if (i + 3 <= line.size() &&
            line.at(i) == '\\' &&
            (line.at(i + 1) == 'l' || line.at(i + 1) == 'r' ||
             line.at(i + 1) == 'c' || line.at(i + 1) == 'j') &&
            line.at(i + 2) == '}') {

            if (m_ignoredAlignment) {
                m_ignoredAlignment = false;
                i += 3;
                continue;
            }

            if (!m_blockStack.isEmpty())
                m_block = m_blockStack.pop();

            i += 3;
            continue;
        }

        if (i + 4 <= line.size() &&
            line.at(i) == '\\' &&
            line.at(i + 1) == 'h' &&
            line.at(i + 2).isDigit() &&
            line.at(i + 3) == '}') {

            if (!m_blockStack.isEmpty())
                m_block = m_blockStack.pop();
            else
                m_block.heading = 0;

            i += 4;
            continue;
        }

        /* ===== Inline open ===== */

        if (i + 3 <= line.size() &&
            line.at(i) == '\\' &&
            line.at(i + 1) == '{' &&
            (line.at(i + 2) == 'b' || line.at(i + 2) == 'i' ||
             line.at(i + 2) == 'u' || line.at(i + 2) == 's' ||
             line.at(i + 2) == '^' || line.at(i + 2) == '_')) {

            flush();
            m_inlineStack.push(m_inline);

            QTextCharFormat f = m_inline;
            QChar t = line.at(i + 2);

            if (t == 'b') f.setFontWeight(QFont::Bold);
            if (t == 'i') f.setFontItalic(true);
            if (t == 'u') f.setFontUnderline(true);
            if (t == 's') f.setFontStrikeOut(true);
            if (t == '^')
                f.setVerticalAlignment(QTextCharFormat::AlignSuperScript);
            if (t == '_')
                f.setVerticalAlignment(QTextCharFormat::AlignSubScript);

            m_inline = f;

            i += 3;
            continue;
        }

        /* ===== Inline close ===== */

        if (i + 3 <= line.size() &&
            line.at(i) == '\\' &&
            (line.at(i + 1) == 'b' || line.at(i + 1) == 'i' ||
             line.at(i + 1) == 'u' || line.at(i + 1) == 's' ||
             line.at(i + 1) == '^' || line.at(i + 1) == '_') &&
            line.at(i + 2) == '}') {

            flush();
            if (!m_inlineStack.isEmpty())
                m_inline = m_inlineStack.pop();
            else
                m_inline = QTextCharFormat();

            i += 3;
            continue;
        }

        /* ===== Plain text ===== */

        buffer += line.at(i++);
    }

    flush();
}
