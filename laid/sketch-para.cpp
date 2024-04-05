#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPoint.h"
#include "include/core/SkRect.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkScalar.h"
#include "include/core/SkSpan.h"
#include "include/core/SkStream.h"
#include "include/core/SkString.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkTypes.h"
#include "include/encode/SkPngEncoder.h"
#include "modules/skparagraph/include/DartTypes.h"
#include "modules/skparagraph/include/FontCollection.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphCache.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/include/TextShadow.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "modules/skparagraph/include/TypefaceFontProvider.h"
#include "src/core/SkOSFile.h"
#include "src/utils/SkOSPath.h"

#include <iostream>
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/src/ParagraphBuilderImpl.h"

#include "include/core/SkImage.h"
#include "include/core/SkStream.h"
#include "include/core/SkDocument.h"
#include "include/docs/SkPDFDocument.h"
#include <string>



doc = Document()

style = Style;
style.fontFamily = "Arial"
doc.addParagraphStyle("p1", style)

masterPage = MasterPage;
masterPage.dimensions(595, 842)
masterPage.cols = 5
masterPage.rows = 4
doc.addMasterPage("dflt", page)

page = doc.addPage("dflt")

box = page.addBox(10, 10, 100, 100)
box.addText("Hello World", "p1")
box.addImage("image.png")




using namespace skia::textlayout;


int main() 
    auto fontCollection = sk_make_sp<FontCollection>();
    fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
    
    SkFILEWStream stream("output.pdf");

    SkPDF::Metadata metadata;
    metadata.fTitle = "Wordly";
    metadata.fCreator = "Example WritePDF() Function";
    metadata.fCreation = {0, 2019, 1, 4, 31, 12, 34, 56};
    metadata.fModified = {0, 2019, 1, 4, 31, 12, 34, 56};
    auto doc = SkPDF::MakeDocument(&stream, metadata);
    SkCanvas* canvas = doc->beginPage(595, 842);

    canvas->clear(SK_ColorWHITE);


    TextStyle text_style;
    text_style.setColor(SK_ColorBLACK);
    text_style.setFontFamilies({SkString("Damascus")});
    text_style.setFontSize(12.0f);
    ParagraphStyle paragraph_style;
    paragraph_style.setTextStyle(text_style);

    ParagraphBuilderImpl builder(paragraph_style, fontCollection);
    builder.pushStyle(text_style);
    std::basic_string<char16_t> text = u"كما قالت المنظمة إن عمالها الذين قتلو كانوا بريطانيين وبولنديين وأستراليين و كذلك فلسطينيين، من بينهم مواطن مزدوج الجنسية، يحمل الجنسيتين الأمريكيةالكندية.";
    builder.addText(text);

    ParagraphBuilderImpl builder2(paragraph_style, fontCollection);
    builder2.pushStyle(text_style);
    builder2.addText("The American pop star entered the Forbes World's Billionaires List for the first time with $1.1bn (£877m), along with Sam Altman, creator of the AI chatbot ChatGPT on $1bn (£800m). This is the only way back home if you want to live. This is never going to work");

    auto paragraph = builder.Build();
    paragraph->updateTextAlign(TextAlign::kRight);
    paragraph->layout(200);
    paragraph->paint(canvas, 50, 50);
    
    auto paragraph2 = builder2.Build();
    paragraph2->layout(200);
    paragraph2->paint(canvas, 300, 50);

    doc->endPage();
    doc->close();
    

    return 0;
}