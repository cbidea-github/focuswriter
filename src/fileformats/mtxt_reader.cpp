#include "mtxt_reader.h"
#include <QTextStream>
#include <QIODevice>

//--------------------------------------------------

MtxtReader::MtxtReader()
{
}

bool MtxtReader::canRead(QIODevice* device)
{
	return device->peek(32).startsWith("/* ::MTXT1:: */");
}

//--------------------------------------------------

void MtxtReader::readData(QIODevice* device)
{
	QTextStream stream(device);

	if (stream.readLine() != "/* ::MTXT1:: */") {
		m_error = QStringLiteral("Invalid MarkedText file");
		return;
	}

	m_cursor.beginEditBlock();

	QVector<Mark> stack;
	QTextCharFormat base;

	while (!stream.atEnd()) {
		parseLine(stream.readLine(), stack, base);
		m_cursor.insertBlock();
	}

	m_cursor.endEditBlock();
}

//--------------------------------------------------

static bool startsWithAt(const QString& s, int i, const QString& t)
{
	return s.mid(i, t.length()) == t;
}

void MtxtReader::parseLine(const QString& s, QVector<Mark>& stack, QTextCharFormat& base)
{
	int i = 0;

	while (i < s.length()) {
		if (s[i] == '\\') {
			if (i + 1 < s.length())
				m_cursor.insertText(s.mid(++i, 1), base);
			i++;
			continue;
		}

		if (startsWithAt(s, i, "***")) { toggle(stack, "***"); i += 3; continue; }
		if (startsWithAt(s, i, "**"))  { toggle(stack, "**");  i += 2; continue; }
		if (startsWithAt(s, i, "~~"))  { toggle(stack, "~~");  i += 2; continue; }
		if (startsWithAt(s, i, "*"))   { toggle(stack, "*");   i += 1; continue; }
		if (startsWithAt(s, i, "_"))   { toggle(stack, "_");   i += 1; continue; }

		QTextCharFormat f = base;
		for (const Mark& m : stack) {
			if (m.fmt.fontWeight() == QFont::Bold) f.setFontWeight(QFont::Bold);
			if (m.fmt.fontItalic())                f.setFontItalic(true);
			if (m.fmt.fontUnderline())             f.setFontUnderline(true);
			if (m.fmt.fontStrikeOut())             f.setFontStrikeOut(true);
		}

		m_cursor.insertText(s.mid(i, 1), f);
		i++;
	}
}

//--------------------------------------------------

void MtxtReader::toggle(QVector<Mark>& stack, const QString& t)
{
	for (int i = stack.size() - 1; i >= 0; --i) {
		if (stack[i].token == t) {
			stack.resize(i);
			return;
		}
	}

	QTextCharFormat f;
	if (t == "*")   f.setFontItalic(true);
	if (t == "**")  f.setFontWeight(QFont::Bold);
	if (t == "***") { f.setFontWeight(QFont::Bold); f.setFontItalic(true); }
	if (t == "_")   f.setFontUnderline(true);
	if (t == "~~")  f.setFontStrikeOut(true);

	stack.push_back({ t, f });
}
