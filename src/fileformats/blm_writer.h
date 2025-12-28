/*
    SPDX-FileCopyrightText: 2025 cbidea
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QTextDocument>
#include <QIODevice>

class BlmWriter
{
public:
    bool write(QIODevice* out, const QTextDocument* doc);
};
