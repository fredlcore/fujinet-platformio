#include "pdf_printer.h"

void pdfPrinter::pdf_header()
{
    pdf_Y = 0;
    pdf_X = 0;
    pdf_pageCounter = 0;
    _file->printf("%%PDF-1.4\n");
    // first object: catalog of pages
    pdf_objCtr = 1;
    objLocations[pdf_objCtr] = _file->position();
    _file->printf("1 0 obj\n<</Type /Catalog /Pages 2 0 R>>\nendobj\n");
    // object 2 0 R is printed at bottom of PDF before xref
    pdf_objCtr = 2; // set up counter for pdf_add_font()
}

void pdfPrinter::pdf_add_font()
{
    pdf_objCtr++; // should now = 3 coming from pdf_header()


}

void pdfPrinter::pdf_new_page()
{ // open a new page
    pdf_objCtr++;
    pageObjects[pdf_pageCounter] = pdf_objCtr;
    objLocations[pdf_objCtr] = _file->position();
    _file->printf("%d 0 obj\n<</Type /Page /Parent 2 0 R /Resources 3 0 R /MediaBox [0 0 %g %g] /Contents [ ", pdf_objCtr, pageWidth, pageHeight);
    pdf_objCtr++; // increment for the contents stream object
    _file->printf("%d 0 R ", pdf_objCtr);
    _file->printf("]>>\nendobj\n");

    // open content stream
    objLocations[pdf_objCtr] = _file->position();
    _file->printf("%d 0 obj\n<</Length ", pdf_objCtr);
    idx_stream_length = _file->position();
    _file->printf("00000>>\nstream\n");
    idx_stream_start = _file->position();

    // open new text object
    pdf_begin_text(pageHeight);
}

void pdfPrinter::pdf_begin_text(float Y)
{
    // open new text object
    _file->printf("BT\n");
    TOPflag = false;
    _file->printf("/F%u %u Tf\n", fontNumber, fontSize);
    _file->printf("%g %g Td\n", leftMargin, Y);
    pdf_Y = Y; // reset print roller to top of page
    pdf_X = 0; // set carriage to LHS
    BOLflag = true;
}

void pdfPrinter::pdf_new_line()
{
    // position new line and start text string array
    _file->printf("0 %g Td [(", -lineHeight);
    pdf_X = 0; // reinforce?
    BOLflag = false;
}

void pdfPrinter::pdf_end_line()
{
    _file->printf(")]TJ\n"); // close the line
    pdf_Y -= lineHeight;     // line feed
    pdf_X = 0;               // CR
    BOLflag = true;
}

void pdfPrinter::pdf_end_page()
{
    // close text object & stream
    if (!BOLflag)
        pdf_end_line();
    _file->printf("ET\n");
    idx_stream_stop = _file->position();
    _file->printf("endstream\nendobj\n");
    size_t idx_temp = _file->position();
    _file->flush();
    _file->seek(idx_stream_length);
    _file->printf("%5u", (idx_stream_stop - idx_stream_start));
    _file->flush();
    _file->seek(idx_temp);
    // set counters
    pdf_pageCounter++;
    TOPflag = true;
}

void pdfPrinter::pdf_xref()
{
    int max_objCtr = pdf_objCtr;
    pdf_objCtr = 2;
    objLocations[pdf_objCtr] = _file->position(); // hard code page catalog as object #2
    _file->printf("2 0 obj\n<</Type /Pages /Kids [ ");
    for (int i = 0; i < pdf_pageCounter; i++)
    {
        _file->printf("%d 0 R ", pageObjects[i]);
    }
    _file->printf("] /Count %d>>\nendobj\n", pdf_pageCounter);
    size_t xref = _file->position();
    max_objCtr++;
    _file->printf("xref\n");
    _file->printf("0 %u\n", max_objCtr);
    _file->printf("0000000000 65535 f\n");
    for (int i = 1; i < max_objCtr; i++)
    {
        _file->printf("%010u 00000 n\n", objLocations[i]);
    }
    _file->printf("trailer <</Size %u/Root 1 0 R>>\n", max_objCtr);
    _file->printf("startxref\n");
    _file->printf("%u\n", xref);
    _file->printf("%%%%EOF\n");
}

bool pdfPrinter::process(const byte *buf, byte n)
{
    int i = 0;
    byte c;

    // algorithm for graphics:
    // if textMode, then can do the regular stuff
    // if !textMode, then don't deal with BOL, EOL.
    // check for TOP always just in case.
    // can graphics mode ignore over-flowing a page for now?
    // textMode is set inside of pdf_handle_char at first character, so...
    // need to test for textMode inside the loop

    if (TOPflag)
        pdf_new_page();

    // loop through string
    do
    {
        c = byte(buf[i++]);

        if (!textMode)
        {
            //this->
            pdf_handle_char(c);
        }
        else
        {
            if (BOLflag && c == EOL)
                pdf_new_line();

            // check for EOL or if at end of line and need automatic CR
            if (!BOLflag && ((c == EOL) || (pdf_X > (printWidth - charWidth))))
                pdf_end_line();

            // start a new line if we need to
            if (BOLflag)
                pdf_new_line();

            // disposition the current byte
            //this->
            pdf_handle_char(c);

#ifdef DEBUG
            Debug_printf("c: %3d  x: %6.2f  y: %6.2f  ", c, pdf_X, pdf_Y);
            Debug_printf("\n");
#endif
        }

    } while (i < n && c != EOL);

    // if wrote last line, then close the page
    if (pdf_Y < lineHeight + bottomMargin)
        pdf_end_page();

    return true;
}

void pdfPrinter::pageEject()
{
    if (paperType == PDF)
    {
        if (TOPflag && pdf_pageCounter == 0)
            pdf_new_page();
        if (!BOLflag)
            pdf_end_line();
        // to do: close the text string array if !BOLflag
        if (!TOPflag || pdf_pageCounter == 0)
            pdf_end_page();
        pdf_xref();
    }
}

void asciiPrinter::initPrinter(File *f)
{
    _file = f;
    paperType = PDF;

    pageWidth = 612.0;
    pageHeight = 792.0;
    leftMargin = 18.0;
    bottomMargin = 0;
    printWidth = 576.0; // 8 inches
    lineHeight = 12.0;
    charWidth = 7.2;
    fontNumber = 1;
    fontSize =12;

    pdf_header();
    pdf_fonts();
}

void asciiPrinter::pdf_fonts()
{
    // 3rd object: font catalog
    pdf_objCtr = 3;
    objLocations[pdf_objCtr] = _file->position();
    _file->printf("3 0 obj\n<</Font << /F1 4 0 R >>>>\nendobj\n");

    // 1027 standard font
    pdf_objCtr = 4;
    objLocations[pdf_objCtr] = _file->position();
    _file->printf("4 0 obj\n<</Type /Font /Subtype /Type1 /BaseFont /Courier /Encoding /WinAnsiEncoding>>\nendobj\n");
}

void asciiPrinter::pdf_handle_char(byte c)
{
    // simple ASCII printer
    if (c > 31 && c < 127)
    {
        if (c == '\\' || c == '(' || c == ')')
            _file->write('\\');
        _file->write(c);

        pdf_X += charWidth; // update x position
    }
}
