/*
	SPDX-FileCopyrightText: 2025 cbidea

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_MTXT_READER_H
#define FOCUSWRITER_MTXT_READER_H

#include "format_reader.h"
#include <QVector>
#include <QString>
#include <QTextCharFormat>

class MtxtReader : public FormatReader
{
public:
	MtxtReader();
	static bool canRead(QIODevice* device);

private:
	struct Mark {
		QString token;
		QTextCharFormat fmt;
	};

	void readData(QIODevice* device) override;
	void parseLine(const QString& s, QVector<Mark>& stack, QTextCharFormat& base);
	void toggle(QVector<Mark>& stack, const QString& token);
};

#endif
