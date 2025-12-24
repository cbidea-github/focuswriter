/*
	SPDX-FileCopyrightText: 2025 cbidea

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QIODevice>

class QTextDocument;

class MtxtWriter
{
public:
    bool write(QIODevice* device, const QTextDocument* document);
};
