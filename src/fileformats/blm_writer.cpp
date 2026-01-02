/*
    SPDX-FileCopyrightText: 2025 cbidea
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "blm_writer.h"

#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QFont>
#include <QStack>
#include <QRegularExpression>

//------------------------------------------------------------
// Helpers
//------------------------------------------------------------

static QByteArray alignmentToken(Qt::Alignment a)
{
    if (a & Qt::AlignRight)   return "r";
    if (a & Qt::AlignCenter)  return "c";
    if (a & Qt::AlignJustify) return "j";
    return QByteArray(); // izquierda
}

static bool isBold(const QTextCharFormat& f)      { return f.fontWeight() == QFont::Bold; }
static bool isItalic(const QTextCharFormat& f)   { return f.fontItalic(); }
static bool isUnderline(const QTextCharFormat& f){ return f.fontUnderline(); }
static bool isStrike(const QTextCharFormat& f)   { return f.fontStrikeOut(); }
static bool isSuper(const QTextCharFormat& f)    { return f.verticalAlignment() == QTextCharFormat::AlignSuperScript; }
static bool isSub(const QTextCharFormat& f)      { return f.verticalAlignment() == QTextCharFormat::AlignSubScript; }

//------------------------------------------------------------
// Literal-safe write (escapes canónicos)
//------------------------------------------------------------

static void writeSafe(QIODevice* out, const QString& text)
{
    // Forma canónica \{t\t} se respeta y no se re-escapa
    static const QRegularExpression rx(
        R"(\\\{t\t\}|\\\{t|\\t\}|\\\{[lrcj]\d*|\\[lrcj]\}|\\\{h\d|\\h\d\}|\\\{[bius^_]|\\[bius^_]\})"
    );

    int pos = 0;
    auto it = rx.globalMatch(text);

    while (it.hasNext()) {
        auto m = it.next();

        if (m.capturedStart() > pos)
            out->write(text.mid(pos, m.capturedStart() - pos).toUtf8());

        const QString tok = m.captured();

        // forma canónica: copiar tal cual
        if (tok == "\\{t\\t}") {
            out->write(tok.toUtf8());
        }
        // literal "\t}"
        else if (tok == "\\t}") {
            out->write("\\{t\\t}");
        }
        // resto de marcas
        else {
            out->write("\\{t");
            out->write(tok.toUtf8());
            out->write("\\t}");
        }

        pos = m.capturedEnd();
    }

    if (pos < text.size())
        out->write(text.mid(pos).toUtf8());
}

//------------------------------------------------------------

bool BlmWriter::write(QIODevice* out, const QTextDocument* doc)
{
    if (!out || !doc)
        return false;

    out->write("::BLM1::\n");

    QTextCharFormat prevChar;
    QStack<char> inlineStack;

    for (QTextBlock block = doc->begin();
         block.isValid();
         block = block.next()) {

        QString plain = block.text();

        // ---------- Empty block ----------
        if (plain.trimmed().isEmpty()) {
            out->write("\n");
            prevChar = QTextCharFormat();
            continue;
        }

        QTextBlockFormat bf = block.blockFormat();

        // ---------- Alignment ----------
        Qt::Alignment a = bf.alignment();
        if (!(a & (Qt::AlignRight | Qt::AlignCenter | Qt::AlignJustify))) {
            if (block.layout()) {
                Qt::Alignment la = block.layout()->textOption().alignment();
                if (la & Qt::AlignRight)        a = Qt::AlignRight;
                else if (la & Qt::AlignCenter)  a = Qt::AlignCenter;
                else if (la & Qt::AlignJustify) a = Qt::AlignJustify;
            }
        }

        QByteArray alignTok = alignmentToken(a);
        bool isLeft = alignTok.isEmpty();

        // ---------- Indent ----------
        int indent = bf.indent();
        if (indent > 9)
            indent = 9;
        if (indent < 0)
            indent = 0;

        // ---------- Heading ----------
        int heading = bf.headingLevel();
        bool hasHeading = heading > 0;

        // ---------- Block open ----------
        if (!isLeft || indent > 0) {
            out->write("\\{");
            if (!isLeft)
                out->write(alignTok);
            else
                out->write("l");

            if (indent > 0)
                out->write(QByteArray::number(indent));

            out->write("\n");
        }

        if (hasHeading)
            out->write("\\{h" + QByteArray::number(heading) + "\n");

        // ---------- Text ----------
        for (QTextBlock::iterator it = block.begin();
             !it.atEnd(); ++it) {

            QTextFragment frag = it.fragment();
            if (!frag.isValid())
                continue;

            QTextCharFormat cur = frag.charFormat();

            // Close inline
            for (int i = inlineStack.size() - 1; i >= 0; --i) {
                char t = inlineStack.at(i);
                bool close =
                    (t == 'b' && isBold(prevChar)      && !isBold(cur)) ||
                    (t == 'i' && isItalic(prevChar)    && !isItalic(cur)) ||
                    (t == 'u' && isUnderline(prevChar) && !isUnderline(cur)) ||
                    (t == 's' && isStrike(prevChar)    && !isStrike(cur)) ||
                    (t == '^' && isSuper(prevChar)     && !isSuper(cur)) ||
                    (t == '_' && isSub(prevChar)       && !isSub(cur));

                if (close) {
                    inlineStack.pop();
                    out->write("\\" + QByteArray(1, t) + "}");
                }
            }

            // Open inline
            const char order[] = { 'b','i','u','s','^','_' };
            for (char t : order) {
                bool open =
                    (t == 'b' && !isBold(prevChar)      && isBold(cur)) ||
                    (t == 'i' && !isItalic(prevChar)    && isItalic(cur)) ||
                    (t == 'u' && !isUnderline(prevChar) && isUnderline(cur)) ||
                    (t == 's' && !isStrike(prevChar)    && isStrike(cur)) ||
                    (t == '^' && !isSuper(prevChar)     && isSuper(cur)) ||
                    (t == '_' && !isSub(prevChar)       && isSub(cur));

                if (open) {
                    inlineStack.push(t);
                    out->write("\\{" + QByteArray(1, t));
                }
            }

            writeSafe(out, frag.text());
            prevChar = cur;
        }

        // ---------- Close inline ----------
        while (!inlineStack.isEmpty())
            out->write("\\" + QByteArray(1, inlineStack.pop()) + "}");

        out->write("\n");

        // ---------- Block close ----------
        if (hasHeading)
            out->write("\\h" + QByteArray::number(heading) + "}\n");

        if (!isLeft || indent > 0) {
            out->write("\\");
            out->write(!isLeft ? alignTok : QByteArray("l"));
            out->write("}\n");
        }

        prevChar = QTextCharFormat();
    }

    return true;
}
