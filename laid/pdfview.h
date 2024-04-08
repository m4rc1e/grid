#ifndef PDFVIEW_H
#define PDFVIEW_H
#include "models.h"
#include <iostream>


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

using namespace skia::textlayout;


int RenderPDF(laid::Document& laidDoc) {
    auto fontCollection = sk_make_sp<FontCollection>();
    fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());

    SkFILEWStream stream("output2.pdf");

    SkPDF::Metadata metadata;
    metadata.fTitle = "My doc";
    metadata.fCreator = "Example";
    metadata.fCreation = {0, 2019, 1, 4, 31, 12, 34, 56};
    metadata.fModified = {0, 2019, 1, 4, 31, 12, 34, 56};

    auto doc = SkPDF::MakeDocument(&stream, metadata);
    for(auto& page : laidDoc.pages) {
        int width = page->masterPage.width;
        int height = page->masterPage.height;
        SkCanvas* canvas = doc->beginPage(width, height);
        for(auto& box : page->boxes) {
            ParagraphStyle paragraph_style;
            ParagraphBuilderImpl builder(paragraph_style, fontCollection);
            for(auto& text_run : box.text_runs) {
                std::cout << text_run.text << std::endl;
                TextStyle text_style;
                text_style.setColor(SK_ColorBLACK);
                text_style.setFontFamilies({SkString("Damascus")});
                text_style.setFontSize(12.0f);
                paragraph_style.setTextStyle(text_style);

                builder.pushStyle(text_style);
                builder.addText(text_run.text.data());

                auto paragraph = builder.Build();
                paragraph->layout(box.width);
                paragraph->paint(canvas, box.x, box.y);
            }
        }
        doc->endPage();

    }
    doc->close();
    return 0;


//    canvas->clear(SK_ColorWHITE);
//
//
//    TextStyle text_style;
//    text_style.setColor(SK_ColorBLACK);
//    text_style.setFontFamilies({SkString("Damascus")});
//    text_style.setFontSize(12.0f);
//    ParagraphStyle paragraph_style;
//    paragraph_style.setTextStyle(text_style);
//
//    ParagraphBuilderImpl builder(paragraph_style, fontCollection);
//    builder.pushStyle(text_style);
//    std::basic_string<char16_t> text = u"كما قالت المنظمة إن عمالها الذين قتلو كانوا بريطانيين وبولنديين وأستراليين و كذلك فلسطينيين، من بينهم مواطن مزدوج الجنسية، يحمل الجنسيتين الأمريكية والكندية.";
//    builder.addText(text);
//
//    ParagraphBuilderImpl builder2(paragraph_style, fontCollection);
//    builder2.pushStyle(text_style);
//    builder2.addText("The American pop star entered the Forbes World's Billionaires List for the first time with $1.1bn (£877m), along with Sam Altman, creator of the AI chatbot ChatGPT on $1bn (£800m).");
//
//    auto paragraph = builder.Build();
//    paragraph->layout(200);
//    paragraph->paint(canvas, 50, 50);
//    
//    auto paragraph2 = builder2.Build();
//    paragraph2->layout(200);
//    paragraph2->paint(canvas, 300, 50);
//
//    doc->endPage();
//    doc->close();
    

    return 0;
}

#endif