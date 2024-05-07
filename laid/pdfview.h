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


class BuildPDF {
public:
    BuildPDF(std::shared_ptr<laid::Document> laidDoc, const char* out) : laidDoc(laidDoc), stream(out) {
        laidDoc = laidDoc;
        fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
        pdf = SkPDF::MakeDocument(&stream, metadata);
        metadata.fTitle = "My doc";
        metadata.fCreator = "Example";
        metadata.fCreation = {0, 2019, 1, 4, 31, 12, 34, 56};
        metadata.fModified = {0, 2019, 1, 4, 31, 12, 34, 56};
    };

    std::shared_ptr<laid::Document> laidDoc;
    sk_sp<FontCollection> fontCollection = sk_make_sp<FontCollection>();
    SkFILEWStream stream;
    SkPDF::Metadata metadata;
    sk_sp<SkDocument> pdf;
    std::map<std::string, ParagraphStyle> paragraphStyles;

    void BuildStyles() {
        for (auto& [name, style] : laidDoc->paragraph_styles) {
            ParagraphStyle paragraph_style;
            // seems to control leading
            // https://groups.google.com/g/skia-discuss/c/vyPaQY9SGFs
            StrutStyle strut_style;
            strut_style.setFontFamilies({SkString("Helvetica")});
            strut_style.setStrutEnabled(true);
            if (style.leading > 0) {
                strut_style.setFontSize(style.leading);
            } else {
                strut_style.setFontSize(style.fontSize * 1.2);
            }
            strut_style.setForceStrutHeight(true);

            TextStyle text_style;
            text_style.setColor(SK_ColorBLACK);
            text_style.setFontFamilies({SkString(style.fontName)});
            text_style.setFontSize(style.fontSize);
            text_style.setTextBaseline(TextBaseline::kAlphabetic);
            paragraph_style.setTextStyle(text_style);
            paragraph_style.setStrutStyle(strut_style);
            
            paragraphStyles[name] = paragraph_style;
        }

    }
    void Build() {
        BuildStyles();
        BuildPages();
    }
    void BuildPages() {
        std::shared_ptr<laid::Page> head = laidDoc->pages;
        while (head != nullptr) {
            BuildPage(head);
            head = head->next;
        }
        pdf->close();
    }

    void BuildPage(std::shared_ptr<laid::Page> page) {
        std::cout << "Building page: " << page << std::endl;
        int width = page->masterPage.width;
        int height = page->masterPage.height;
        SkCanvas* canvas = pdf->beginPage(width, height);
        //BuildGuides(canvas, page->masterPage);
        //BuildBaseline(canvas, page->masterPage);
        for(auto& box : page->boxes) {
            if (box->image_path.size() > 0) {
                BuildImage(canvas, box);
            }
            BuildText(canvas, page, box);
        }
        std::cout << "Ending page: " << page << std::endl;
        pdf->endPage();
    }

    void BuildBaseline(SkCanvas* canvas, laid::MasterPage& masterPage) {
        SkPaint paintBaseline;
        paintBaseline.setColor(SK_ColorGRAY);
        for (int i=masterPage.marginTop; i<= masterPage.height - masterPage.marginBottom; i+=masterPage.baseline) {
            canvas->drawLine(
                SkPoint::Make(masterPage.marginLeft, i),
                SkPoint::Make(masterPage.width - masterPage.marginRight, i),
                paintBaseline
            );
        }
    }

    void BuildGuides(SkCanvas* canvas, laid::MasterPage& masterPage) {
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
//            std::cout << "Drawing line from " << gridbox.startX << " to " << gridbox.endX << std::endl;
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
    void BuildImage(SkCanvas* canvas, std::shared_ptr<laid::Box> box) {
        std::cout << "img:" << box << std::endl;
        if (box->image_path.size() > 0) {
            std::cout << "Rendering image canvas: " << canvas << std::endl;
            auto data = SkData::MakeFromFileName(box->image_path.c_str());
            auto img = SkImages::DeferredFromEncodedData(data);
            std::cout << "Image width: " << img << std::endl;
            canvas->drawImageRect(
                img,
                SkRect::MakeXYWH(box->x, box->y, box->width, box->height),
                SkSamplingOptions()
            );
        }
    }

    void BuildText(SkCanvas* canvas, std::shared_ptr<laid::Page> page, std::shared_ptr<laid::Box> box) {
        std::cout << "txt:" << box << std::endl;
        float offset = 0;
        for (size_t textrun_idx = 0; textrun_idx < box->text_runs.size(); textrun_idx++) {
            auto& text_run = box->text_runs[textrun_idx];
            auto paragraph_style = paragraphStyles[text_run.style.name];
            auto text_style = paragraph_style.getTextStyle();
            ParagraphBuilderImpl builder(paragraph_style, fontCollection);
            std::istringstream ss(text_run.text);
            std::string token;
            std::string overflow;
            std::unique_ptr<skia::textlayout::Paragraph>  paragraph;
            while(std::getline(ss, token, ' ')) {
                builder.pushStyle(text_style);
                builder.addText(token.data());
                builder.addText(" ");
                paragraph = builder.Build();
                paragraph->layout(box->width);
                if (paragraph->getHeight() > box->height+3) {
                    overflow += token + " ";
                }
            }
            std::cout << "painting" << box->height << box->x << " " << box->y << std::endl;
            paragraph->paint(canvas, box->x, box->y+offset);
            if (overflow.size() > 0) {
                if (box->next == nullptr) {
                    if (page->overflow == true) {
                        auto newPage = overflowPage(page);
                        std::cout << "Overflow page: " << newPage << std::endl;
                        box->next = newPage->boxes[0];
                        box->next->addText(overflow, text_run.style);
                        // add the rest of the text runs to the next box
                        for (int j=textrun_idx+1; j<box->text_runs.size(); j++) {
                            box->next->text_runs.push_back(box->text_runs[j]);
                        }
                        auto tail = page->next;
                        page->next = newPage;
                        std::cout << "added page" << std::endl;
                        newPage->next = tail;
                        return; // stop processing the rest of the text runs
                    }
                }
                else {
                    std::cout << "text on canvas: " << canvas << std::endl;
                    box->next->addText(overflow, text_run.style);
                    for (int j=textrun_idx+1; j<box->text_runs.size(); j++) {
                        box->next->text_runs.push_back(box->text_runs[j]);
                    }
                    break; // stop processing the rest of the text runs
                }
            }
            offset += paragraph->getHeight();
        std::cout << "done" << std::endl;
        }

        for (auto& [idx, children] : box->children) {
            for(auto& child : children) {
                BuildText(canvas, page, child);
            }
        }
    }
};
#endif