/*
    SPDX-FileCopyrightText: 2025 cbidea
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "format_reader.h"

#include <QStack>
#include <QTextCharFormat>

class BlmReader : public FormatReader
{
public:
    static bool canRead(QIODevice* device);
    void readData(QIODevice* device) override;

private:
    struct BlockState {
        Qt::Alignment alignment = Qt::AlignLeft;
        int heading = 0; // 0 = none
    };

    // ---- block state ----
    BlockState m_block;
    QStack<BlockState> m_blockStack;

    // ---- inline state ----
    QTextCharFormat m_inline;
    QStack<QTextCharFormat> m_inlineStack;

	// ---- baseline ----
    QTextBlockFormat m_baselineBlockFormat;

    // ---- parser state ----
    bool m_inLiteral = false;
    bool m_usedInitialBlock = false;   // Qt implicit first block
    bool m_ignoredAlignment = false;   // 🔑 ignored nested alignment

    void processLine(const QString& line);
};
