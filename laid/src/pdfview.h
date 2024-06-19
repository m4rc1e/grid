#ifndef PDFVIEW_H
#define PDFVIEW_H
#include "models.h"
#include <iostream>
#include <iterator> // For std::next

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


sk_sp<FontCollection> fontCollection = sk_make_sp<FontCollection>();

void setupFontCollection() {
    fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
}

class CursorPos {
    public:
    int x;
    int y;
};

class TextSetter {
public:
    TextSetter(
        float width,
        float height,
        skia::textlayout::ParagraphStyle paragraph_style,
        std::vector<laid::Box>& myBoxes) : 
        width(width),
        height(height),
        paragraph_style(paragraph_style),
        boxes(myBoxes),
        builder(paragraph_style, fontCollection) {
            paragraph = builder.Build();
    }
    int width;
    int height;
    float contentHeight;
    std::string overflowingText;
    std::vector<laid::Box> boxes;
    
    bool hasOverflowingText() {
        return overflowingText.size() > 0;
    }

    void SetText(
        std::string text,
        skia::textlayout::ParagraphStyle& paragraph_style
    ) {
        auto text_style = paragraph_style.getTextStyle();
        builder.pushStyle(text_style);

        std::istringstream ss(text); // " " added at end due to delimiter for std::getline
        std::string token;

        auto maxLines = int(height / text_style.getFontSize());

        while (std::getline(ss, token, ' ')) {
            auto currentCursor = getCursor(paragraph.get());
            auto nextCursor = getNextCursor(token, paragraph_style, paragraph.get());
            auto currentLine = int(paragraph->lineNumber());

            // Handle exceeding height of box
            if (currentLine >= maxLines -1 && nextCursor.x > width || currentLine >= maxLines -1) {
                std::cout << "overflowing" << '\n';
                overflowingText += token + " ";
                continue;
            }
            // Handle collisions with other Boxes
            auto collidedBox = boxCollision(nextCursor);
            if (collidedBox) {
                addPlaceholder(collidedBox->width - (currentCursor.x - collidedBox->x));
            }

            builder.addText(token.data());
            builder.addText(" ");
            paragraph = builder.Build();
            paragraph->layout(width);
            contentHeight = paragraph->getHeight();

        }
    }

    bool HasOverflowingText() {
        return overflowingText.size() > 0;
    }

    CursorPos getNextCursor(std::string token, ParagraphStyle style, Paragraph* paragraph) {
        auto cursor = getCursor(paragraph);

        sk_sp<FontCollection> fontCollection = sk_make_sp<FontCollection>();
        fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
        ParagraphBuilderImpl wordBuilder(style, fontCollection);
        auto text_style = style.getTextStyle();
        wordBuilder.pushStyle(text_style);

        wordBuilder.addText(token.data());
        auto wordParagraph = wordBuilder.Build();
        wordParagraph->layout(width);
        auto wordCursor = getCursor(wordParagraph.get());
        return CursorPos{cursor.x + wordCursor.x, cursor.y};
    }

    void paint(float x, float y, SkCanvas* canvas) {
        paragraph->paint(canvas, x, y);
    }

    CursorPos getCursor(Paragraph* paragraph) {
        RectHeightStyle rect_height_style = RectHeightStyle::kTight;
        RectWidthStyle rect_width_style = RectWidthStyle::kTight;

        auto boxes = paragraph->getRectsForRange(0, 1000000, rect_height_style, rect_width_style);
        int cursorX = boxes.back().rect.fRight;
        int cursorY = paragraph->getHeight();
        return CursorPos{cursorX, cursorY};
    }

    std::optional<laid::Box> boxCollision(CursorPos cursor) {
        for (auto& box : boxes) {
            if (
                cursor.x >= box.x && 
                cursor.x <= box.x + box.width && 
                cursor.y >= box.y && 
                cursor.y <= box.y + box.height
            ) {
                return box;
            }
        }
        return std::nullopt;;
    }

    void addPlaceholder(int width) {
        PlaceholderStyle placeholder1(width, 0, PlaceholderAlignment::kTop, TextBaseline::kAlphabetic, 0);
        builder.addPlaceholder(placeholder1);
    }

private:
    ParagraphBuilderImpl builder;
    
    std::unique_ptr<skia::textlayout::Paragraph>  paragraph;
    skia::textlayout::ParagraphStyle paragraph_style;

};

class BuildPDF {
public:
    BuildPDF(std::shared_ptr<laid::Document> laidDoc, const char* out, bool debug) : laidDoc(laidDoc), stream(out), debug(debug) {
        setupFontCollection();
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
        // build non-inherited styles first
        for (auto& [name, style] : laidDoc->paragraph_styles) {
            if (style.inherit.size() > 0) {
                continue;
            }

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
            if (style.swatch != "") {
                auto color = laidDoc->swatches[style.swatch].parseRGB();
                text_style.setColor(SkColorSetRGB(color.r, color.g, color.b));
            } else {
                text_style.setColor(SK_ColorBLACK);
            }
            
            text_style.setFontFamilies({SkString(style.fontName)});
            text_style.setFontSize(style.fontSize);
            
            SkFontStyle::Weight weight = static_cast<SkFontStyle::Weight>(style.weight);
            SkFontStyle::Width width = static_cast<SkFontStyle::Width>(style.width);
            SkFontStyle::Slant slant = static_cast<SkFontStyle::Slant>(style.slant);
            SkFontStyle fontStyle(weight, width, slant);
            text_style.setFontStyle(fontStyle);
            
            text_style.setTextBaseline(TextBaseline::kAlphabetic);
            paragraph_style.setTextStyle(text_style);
            paragraph_style.setStrutStyle(strut_style);
            
            paragraphStyles[name] = paragraph_style;
        }
        // build inheritable styles
        for (auto& [name, style] : laidDoc->paragraph_styles) {
            if (style.inherit.size() == 0) {
                continue;
            }
            auto inherit = paragraphStyles[style.inherit];
            auto text_style = inherit.getTextStyle();
            // TODO we probably wanna dedup this effort
            SkFontStyle::Weight weight = static_cast<SkFontStyle::Weight>(style.weight);
            SkFontStyle::Width width = static_cast<SkFontStyle::Width>(style.width);
            SkFontStyle::Slant slant = static_cast<SkFontStyle::Slant>(style.slant);
            SkFontStyle fontStyle(weight, width, slant);
            text_style.setFontStyle(fontStyle);
            ParagraphStyle paragraph_style;
            paragraph_style.setTextStyle(text_style);
            paragraph_style.setStrutStyle(inherit.getStrutStyle());
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
            //SolveBoxes(head);
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
        paintBox.setStrokeWidth(1.0f);
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
        // Find boxes that collide with the current box and have a zIndex greater than current box
        std::vector<laid::Box> collisionBoxes;
        for (auto childBox : page->boxes) {
            if (childBox == box) {
                continue;
            }
            if (childBox->zIndex > box->zIndex) {
                auto collideBox = laid::Box(childBox->x - box->x, childBox->y - box->y, childBox->width, childBox->height);
                collisionBoxes.push_back(collideBox);
            }

        }
        int offset = 0;
        for (size_t paraIdx = 0; paraIdx < box->paragraphs.size(); paraIdx++) {
            auto paragraph = box->paragraphs[paraIdx];
            auto paragraphStyle = paragraphStyles[paragraph->style];
            TextSetter textSetter(box->width, box->height - offset, paragraphStyle, collisionBoxes);

            for (size_t runIdx = 0; runIdx < paragraph->text_runs.size(); runIdx++) {
                auto& text_run = paragraph->text_runs[runIdx];
                textSetter.SetText(text_run.text, paragraphStyles[text_run.style]);
                if (textSetter.hasOverflowingText()) {

                    // if there isn't a next box and the page is overflowing, add another page
                    if (box->next == nullptr && page->overflow == true) {
                        laidDoc->overflowPage(page);
                    }
                    
                    // push content to next box
                    if (box->next != nullptr) {
                        auto nextBox = box->next;
                        auto overflow_run = laid::TextRun{
                            textSetter.overflowingText,
                            text_run.style
                        };
                        auto para = laid::Paragraph{
                            std::vector<laid::TextRun>{overflow_run},
                            text_run.style
                        };
                        for (size_t i = runIdx + 1; i < paragraph->text_runs.size(); i++) {
                            para.text_runs.push_back(paragraph->text_runs[i]);
                        }
                        nextBox->addParagraph(std::make_shared<laid::Paragraph>(para));
                        for (size_t i = paraIdx + 1; i < box->paragraphs.size(); i++) {
                            nextBox->addParagraph(box->paragraphs[i]);
                        }

                    }
                    textSetter.paint(box->x, box->y+offset, canvas);
                    return;
                }
            }
            textSetter.paint(box->x, box->y+offset, canvas);
            offset += textSetter.contentHeight;
        }
    };
};
#endif