/*
	SPDX-FileCopyrightText: 2025 cbidea

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "mtxt_writer.h"

#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextDocument>

//-----------------------------------------------------------------------------

struct StyleState {
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strike = false;
};

//-----------------------------------------------------------------------------

static StyleState getState(const QTextCharFormat& f)
{
    StyleState s;
    s.bold = (f.fontWeight() == QFont::Bold);
    s.italic = f.fontItalic();
    s.underline = f.fontUnderline();
    s.strike = f.fontStrikeOut();
    return s;
}

//-----------------------------------------------------------------------------

static void emitStyle(QIODevice* d, const StyleState& from, const StyleState& to)
{
    auto close = [&](bool f, bool t, const char* m){
        if (f && !t) d->write(m);
    };
    auto open = [&](bool f, bool t, const char* m){
        if (!f && t) d->write(m);
    };

    // Close in reverse nesting order
    close(from.strike, to.strike, "~~");
    close(from.underline, to.underline, "_");
    close(from.italic, to.italic, "*");
    close(from.bold, to.bold, "**");

    // Open in canonical nesting order
    open(from.bold, to.bold, "**");
    open(from.italic, to.italic, "*");
    open(from.underline, to.underline, "_");
    open(from.strike, to.strike, "~~");
}

//-----------------------------------------------------------------------------

static QString escape(const QString& s, bool inStrike)
{
    QString out;
    for (int i = 0; i < s.length(); ++i) {
        const QChar c = s.at(i);

        if (!inStrike && c == '~' && i + 1 < s.length() && s.at(i+1) == '~') {
            out += "\\~~";
            ++i;
            continue;
        }

        if (c == '*' || c == '_' || c == '>' || c == '<' || c == '\\')
            out += '\\';

        out += c;
    }
    return out;
}

//-----------------------------------------------------------------------------

bool MtxtWriter::write(QIODevice* device, const QTextDocument* document)
{
    device->write("/*MTXT1*/\n");

    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {

        QTextBlockFormat bf = block.blockFormat();
        if (bf.alignment() == Qt::AlignCenter)
            device->write(">");

        StyleState current;

        for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
            QTextFragment frag = it.fragment();
            StyleState next = getState(frag.charFormat());

            emitStyle(device, current, next);
            device->write(escape(frag.text(), current.strike).toUtf8());
            current = next;
        }

        emitStyle(device, current, StyleState());

        if (bf.alignment() == Qt::AlignCenter)
            device->write("<");

        device->write("\n");
    }

    return true;
}
