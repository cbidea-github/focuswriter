/*
	SPDX-FileCopyrightText: 2025 cbidea

	SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef FOCUSWRITER_MTXT_WRITER_H
#define FOCUSWRITER_MTXT_WRITER_H

class QTextDocument;
class QIODevice;

class MtxtWriter
{
public:
	bool write(QIODevice* device, const QTextDocument* document);
};

#endif
