/*
	SPDX-FileCopyrightText: 2025 cbidea

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "format_reader.h"
#include <QIODevice>
#include <QStringConverter>

class MtxtReader : public FormatReader
{
public:
    static bool canRead(QIODevice* device);
    void readData(QIODevice* device) override;
};
