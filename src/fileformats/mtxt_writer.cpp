#include "mtxt_writer.h"

#include <QTextDocument>
#include <QTextBlock>
#include <QTextFragment>
#include <QTextCharFormat>
#include <QIODevice>

//--------------------------------------------------

static QString esc(const QString& s)
{
	QString out;
	for (QChar c : s) {
		if (c=='\\' || c=='*' || c=='_' || c=='~')
			out+='\\';
		out+=c;
	}
	return out;
}

//--------------------------------------------------

bool MtxtWriter::write(QIODevice* dev, const QTextDocument* doc)
{
	dev->write("/* ::MTXT1:: */\n");

	struct State {
		bool b=false,i=false,u=false,s=false;
	};

	State cur;

	auto open = [&](bool want, bool& curFlag, const char* on, QByteArray& out){
		if (want && !curFlag) { out += on; curFlag = true; }
	};

	auto close = [&](bool want, bool& curFlag, const char* off, QByteArray& out){
		if (!want && curFlag) { out += off; curFlag = false; }
	};

	for (QTextBlock b = doc->firstBlock(); b.isValid(); b = b.next()) {

		for (auto it = b.begin(); !it.atEnd(); ++it) {
			QTextFragment f = it.fragment();
			if (!f.isValid()) continue;

			QTextCharFormat fmt = f.charFormat();
			bool wantB = fmt.fontWeight()==QFont::Bold;
			bool wantI = fmt.fontItalic();
			bool wantU = fmt.fontUnderline();
			bool wantS = fmt.fontStrikeOut();

			QByteArray out;

			close(wantB, cur.b, "**", out);
			close(wantI, cur.i, "*",  out);
			close(wantU, cur.u, "_",  out);
			close(wantS, cur.s, "~~", out);

			open(wantS, cur.s, "~~", out);
			open(wantU, cur.u, "_",  out);
			open(wantI, cur.i, "*",  out);
			open(wantB, cur.b, "**", out);

			QString txt = esc(f.text());
			out += txt.toUtf8();
			dev->write(out);
		}

		QByteArray end;
		if (cur.b) { end += "**"; cur.b=false; }
		if (cur.i) { end += "*";  cur.i=false; }
		if (cur.u) { end += "_";  cur.u=false; }
		if (cur.s) { end += "~~"; cur.s=false; }

		end += "\n";
		dev->write(end);
	}

	return true;
}
