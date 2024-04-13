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
#include <ranges>
#include <string_view>

#include <iostream>
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skparagraph/src/ParagraphBuilderImpl.h"

#include "include/core/SkImage.h"
#include "include/core/SkStream.h"
#include "include/core/SkDocument.h"
#include "include/docs/SkPDFDocument.h"
#include <string>

#include <iostream>
#include <sstream>



using namespace skia::textlayout;

void renderGuides(SkCanvas* canvas, laid::MasterPage& masterPage) {
    // grids
    SkPaint paintGrids;
    paintGrids.setStrokeWidth(1.0f);
    paintGrids.setColor(SK_ColorCYAN);
    int workingWidth = masterPage.width - masterPage.marginLeft - masterPage.marginRight;
    // grid cols
    for (int i=0; i< masterPage.cols; i++) {
        auto gridbox = masterPage.getRect(i, 0);
        canvas->drawLine(
            SkPoint::Make(gridbox.startX, masterPage.marginTop),
            SkPoint::Make(gridbox.startX, masterPage.height - masterPage.marginBottom),
            paintGrids
        );
        canvas->drawLine(
            SkPoint::Make(gridbox.endX, masterPage.marginTop),
            SkPoint::Make(gridbox.endX, masterPage.height - masterPage.marginBottom),
            paintGrids
        );
    }
    for (int i=0; i< masterPage.rows; i++) {
        auto gridbox = masterPage.getRect(0, i);
        canvas->drawLine(
            SkPoint::Make(masterPage.marginLeft, gridbox.startY),
            SkPoint::Make(masterPage.width - masterPage.marginRight, gridbox.startY),
            paintGrids
        );
        canvas->drawLine(
            SkPoint::Make(masterPage.marginLeft, gridbox.endY),
            SkPoint::Make(masterPage.width - masterPage.marginRight, gridbox.endY),
            paintGrids
        );
    }
    //margins
    SkPaint paintMargins;
    paintMargins.setColor(SK_ColorMAGENTA);
    paintMargins.setStrokeWidth(1.0f);
    canvas->drawLine(SkPoint::Make(masterPage.marginLeft, masterPage.marginTop), SkPoint::Make(masterPage.width - masterPage.marginRight, masterPage.marginTop), paintMargins);
    canvas->drawLine(SkPoint::Make(masterPage.marginLeft, masterPage.marginTop), SkPoint::Make(masterPage.marginLeft, masterPage.height - masterPage.marginBottom), paintMargins);
    canvas->drawLine(SkPoint::Make(masterPage.width - masterPage.marginRight, masterPage.marginTop), SkPoint::Make(masterPage.width - masterPage.marginRight, masterPage.height - masterPage.marginBottom), paintMargins);
    canvas->drawLine(SkPoint::Make(masterPage.marginLeft, masterPage.height - masterPage.marginBottom), SkPoint::Make(masterPage.width - masterPage.marginRight, masterPage.height - masterPage.marginBottom), paintMargins);
    

}


void renderText(SkCanvas* canvas, std::shared_ptr<laid::Box> box, sk_sp<FontCollection> fontCollection) {
    for(auto& text_run : box->text_runs) {
        ParagraphStyle paragraph_style;
        ParagraphBuilderImpl builder(paragraph_style, fontCollection);
        std::istringstream ss(text_run.text);
        std::string token;
        std::string overflow;
        while(std::getline(ss, token, ' ')) {
            TextStyle text_style;
            text_style.setColor(SK_ColorBLACK);
            text_style.setFontFamilies({SkString("Helvetica")});
            text_style.setFontSize(12.0f);
            paragraph_style.setTextStyle(text_style);
            builder.pushStyle(text_style);
            builder.addText(token.data());
            builder.addText(" ");
            auto paragraph = builder.Build();
            paragraph->layout(box->width);
            if (paragraph->getHeight() > box->height+3) {
                overflow += token + " ";
            } else {
                paragraph->paint(canvas, box->x, box->y);
            }
        }
        if (overflow.size() > 0) {
            box->next->addText(overflow, text_run.style);
        }
    }

    for (auto& [idx, children] : box->children) {
        for(auto& child : children) {
            renderText(canvas, child, fontCollection);
        }
    }
}

void renderImage(SkCanvas* canvas, std::shared_ptr<laid::Box> box) {
    // render image
    if (box->image_path.size() > 0) {
        auto data = SkData::MakeFromFileName(box->image_path.c_str());
        auto foo = SkImages::DeferredFromEncodedData(data);
        canvas->drawImageRect(
            foo,
            SkRect::MakeXYWH(box->x, box->y, box->width, box->height),
            SkSamplingOptions()
        );
    }
}


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
        auto paint = SkPaint();
        renderGuides(canvas, page->masterPage);
        for(auto& box : page->boxes) {
            renderImage(canvas, box);
            renderText(canvas, box, fontCollection);
        }
        doc->endPage();

    }
    doc->close();
    return 0;


    return 0;
}

#endif