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


class TextSetter {
    private:
        ParagraphBuilderImpl builder;
        ParagraphBuilderImpl prevBuilder;

    public:
        TextSetter(
            int width,
            int height,
            skia::textlayout::ParagraphStyle& paragraph_style,
            sk_sp<FontCollection> fontCollection) : 
            width(width),
            height(height),
            paragraph_style(paragraph_style),
            fontCollection(fontCollection),
            builder(paragraph_style, fontCollection),
            prevBuilder(paragraph_style, fontCollection) {
        }
        int width;
        int height;
        int contentHeight;
        bool paintPrev;
        std::string text;
        std::string overflow;
        skia::textlayout::ParagraphStyle paragraph_style;
        sk_sp<FontCollection> fontCollection;
        std::unique_ptr<skia::textlayout::Paragraph>  paragraph;
        std::unique_ptr<skia::textlayout::Paragraph>  prevParagraph;
        
        void SetText(
            std::string text,
            skia::textlayout::ParagraphStyle paragraph_style
        ) {
            auto text_style = paragraph_style.getTextStyle();
            auto  strut_style = paragraph_style.getStrutStyle();

            std::istringstream ss(text); // " " added at end due to delimiter for std::getline
            std::string token;
            
            while(std::getline(ss, token, ' ')) {
                builder.pushStyle(text_style);
                builder.addText(token.data());
                builder.addText(" ");
                paragraph = builder.Build();
                paragraph->layout(width);

                if (paragraph->getHeight() > height) {
                    paintPrev = true;
                    overflow += token + " ";
                    continue;
                }
                prevBuilder.pushStyle(text_style);
                prevBuilder.addText(token.data());
                prevBuilder.addText(" ");
                prevParagraph = prevBuilder.Build();
                prevParagraph->layout(width);
            };
        }
        void Paint(int x, int y, SkCanvas* canvas) {
            if (paintPrev == true) {
                prevParagraph->paint(canvas, x, y);
                contentHeight = prevParagraph->getHeight();
                return;
            }
            paragraph->paint(canvas, x, y);
            contentHeight = paragraph->getHeight();
        }
        bool HasOverflowingText() {
            return overflow.size() > 0;
        }
};


class BuildPDF {
public:
    BuildPDF(std::shared_ptr<laid::Document> laidDoc, const char* out, bool debug) : laidDoc(laidDoc), stream(out), debug(debug) {
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
    bool debug;

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
        if (debug == true) {
            BuildGuides(canvas, page->masterPage);
            BuildBaseline(canvas, page->masterPage);
        }
        for(auto& box : page->boxes) {
            if (box->image_path.size() > 0) {
                BuildImage(canvas, box);
            }
            BuildText(canvas, page, box);
            if (debug == true) {
                BuildBoxRect(canvas, box);
            }
        }
        std::cout << "Ending page: " << page << std::endl;
        pdf->endPage();
    }

    void BuildBaseline(SkCanvas* canvas, laid::MasterPage& masterPage) {
        // no baseline, no draw!
        if (masterPage.baseline == 0) {
            return;
        }
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

    void BuildBoxRect(SkCanvas* canvas, std::shared_ptr<laid::Box> box) {
        SkPaint paintBox;
        paintBox.setColor(SK_ColorRED);
        paintBox.setStyle(SkPaint::kStroke_Style);

        SkPaint paintText;
        paintText.setColor(SK_ColorMAGENTA);
        
        std::string labelText = "x:" + std::to_string(int(box->x)) + " y:" + std::to_string(int(box->y)) + " w:" + std::to_string(int(box->width)) + " h:" + std::to_string(int(box->height)) + " page_idx:" + std::to_string(box->pageIdx) + " first_idx:" + std::to_string(box->getFirst());
        auto font = SkFont();
        font.setSize(2);
        canvas->drawSimpleText(
            labelText.c_str(),
            labelText.size(),
            SkTextEncoding::kUTF8,
            box->x + 2,
            box->y - 2,
            font,
            paintText
        );
        canvas->drawRect(
            SkRect::MakeXYWH(box->x, box->y, box->width, box->height),
            paintBox
        );
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
        int offset = 0;
        for (size_t paragraph_idx = 0; paragraph_idx < box->paragraphs.size(); paragraph_idx++) {
            auto paragraph = box->paragraphs[paragraph_idx];
            auto paraStyle = paragraphStyles[paragraph->style];
            TextSetter textSetter(box->width, box->height, paraStyle, fontCollection);

            for (size_t textrun_idx = 0; textrun_idx < paragraph->text_runs.size(); textrun_idx++) {
                auto& text_run = paragraph->text_runs[textrun_idx];
                textSetter.SetText(text_run.text, paragraphStyles[text_run.style]);
                if (textSetter.HasOverflowingText()) {
                    if (page->overflow == true && box->next == nullptr) {
                        auto newPage = laidDoc->overflowPage(page);
                    }
                    if (box->next) {
                        auto newParagraph = std::make_shared<laid::Paragraph>();
                        box->next->addParagraph(newParagraph);
                        newParagraph->addText(textSetter.overflow, text_run.style);
                        
                        for (size_t i = paragraph_idx + 1; i < box->paragraphs.size(); i++) {
                            auto& overflow_paragraph = box->paragraphs[i];
                            box->next->addParagraph(overflow_paragraph);
                            for (size_t i = textrun_idx + 1; i < paragraph->text_runs.size(); i++) {
                                auto& overflow_text_run = paragraph->text_runs[i];
                                overflow_paragraph->addText(overflow_text_run.text, overflow_text_run.style);
                            }
                        }
                        textSetter.Paint(box->x, box->y, canvas);
                        return;
                    } else {
                        std::cout << "Overflowing text in box!" << std::endl;
                    }
                }
            }
            textSetter.Paint(box->x, box->y+offset, canvas);
            // always add a new line when paragraph ends. 
            // TODO perhaps each box should have a y pos of where content currently is
            offset += textSetter.contentHeight;
    }
};
};
#endif