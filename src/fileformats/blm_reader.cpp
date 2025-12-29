/*
    SPDX-FileCopyrightText: 2025 cbidea
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "blm_reader.h"

#include <QIODevice>
#include <QTextStream>
#include <QFont>
#include <QTextBlockFormat>

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

	// Capturar el formato base real del documento (párrafo normal del tema)
	m_baselineBlockFormat = m_cursor.blockFormat();

    // Inline must start from real document format
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

    if (!m_inLiteral && line.trimmed().isEmpty()) {

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
			// Primer bloque real: usar baseline del documento
			bf = m_baselineBlockFormat;
			m_cursor.mergeBlockFormat(bf);
			m_usedInitialBlock = true;
		} else {
			// Bloques siguientes: heredar del bloque anterior
			bf = m_cursor.blockFormat();
			m_cursor.insertBlock(bf, m_inline);
		}

		// Aplicar SOLO las propiedades de bloque que cambian
		bf.setAlignment(m_block.alignment);
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

        /* ===== Literal (absolute priority) ===== */

        if (!m_inLiteral &&
            i + 11 <= line.size() &&
            line.mid(i, 11) == "\\{@literal@") {

            m_inLiteral = true;
            i += 11;
            continue;
        }

        if (m_inLiteral) {
            if (i + 11 <= line.size() &&
                line.mid(i, 11) == "\\@literal@}") {

                m_inLiteral = false;
                i += 11;
                continue;
            }

            buffer += line.at(i++);
            continue;
        }

        /* ===== Page break token (state only) ===== */

        if (i + 4 <= line.size() &&
            line.mid(i, 4) == "\\{p}") {

            i += 4;
            continue;
        }

        /* ===== Block open ===== */

        if (i + 3 <= line.size() &&
            line.at(i) == '\\' &&
            line.at(i + 1) == '{') {

            QChar t = line.at(i + 2);

            // ---- alignment (non-nesting) ----
            if (t == 'l' || t == 'r' || t == 'c' || t == 'j') {

                // ignore nested alignment and remember it
                if (m_block.alignment != Qt::AlignLeft) {
                    m_ignoredAlignment = true;
                    i += 3;
                    continue;
                }

                BlockState next = m_block;

                if (t == 'l') next.alignment = Qt::AlignLeft;
                if (t == 'r') next.alignment = Qt::AlignRight;
                if (t == 'c') next.alignment = Qt::AlignCenter;
                if (t == 'j') next.alignment = Qt::AlignJustify;

                m_blockStack.push(m_block);
                m_block = next;

                i += 3;
                continue;
            }

            // ---- heading (non-nesting) ----
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

        // alignment close
        if (i + 3 <= line.size() &&
            line.at(i) == '\\' &&
            (line.at(i + 1) == 'l' || line.at(i + 1) == 'r' ||
             line.at(i + 1) == 'c' || line.at(i + 1) == 'j') &&
            line.at(i + 2) == '}') {

            // ignore close of ignored nested alignment
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

        // heading close
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