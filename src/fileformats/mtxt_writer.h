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
