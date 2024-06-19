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
        std::vector<laid::Box> boxes) : 
        width(width),
        height(height),
        paragraph_style(paragraph_style),
        boxes(boxes),
        builder(paragraph_style, fontCollection) {
    }
    int width;
    int height;
    std::string overflowingText;
    
    void SetText(
        std::string text,
        skia::textlayout::ParagraphStyle paragraph_style
    ) {
        auto text_style = paragraph_style.getTextStyle();
        builder.pushStyle(text_style);

        std::istringstream ss(text); // " " added at end due to delimiter for std::getline
        std::string token;

        auto maxLines = int(height / text_style.getFontSize());

        paragraph = builder.Build();
        while (std::getline(ss, token, ' ')) {
            auto currentCursor = getCursor(paragraph.get());
            auto nextCursor = getNextCursor(token, paragraph_style, paragraph.get());
            auto currentLine = int(paragraph->lineNumber());

            // Handle exceeding height of box
            if (currentLine >= maxLines -1 && nextCursor.x > width || currentLine >= maxLines) {
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

    std::vector<laid::Box>& boxes;
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

        TextStyle text_style;
        text_style.setFontFamilies({SkString("Inter")});
        text_style.setColor(SK_ColorBLACK);
        text_style.setFontSize(14);
        text_style.setWordSpacing(5);
        text_style.setLetterSpacing(1);
        text_style.setDecorationColor(SK_ColorBLACK);
        text_style.setDecoration(TextDecoration::kUnderline);
        
        ParagraphStyle paragraph_style;
        paragraph_style.turnHintingOff();
        paragraph_style.setTextStyle(text_style);

        std::vector<laid::Box> boxes = {
            laid::Box{300, 0, 200, 100},
            laid::Box{200, 150, 100, 100}
        };
        TextSetter textSetter(box->width, box->height, paragraph_style, boxes);
        textSetter.SetText("On 15 June 1974 the National Front held a march through central London in support of the compulsory repatriation of immigrants. The march was to end at Conway Hall in Red Lion Square. A counter-demonstration was planned by Liberation, an anti-colonial pressure group. During the late 1960s and early 1970s, the London council of Liberation had been increasingly infiltrated by hard-left political activists, and they invited several hard-left organisations to join them in the march. When the Liberation march reached Red Lion Square, the International Marxist Group (IMG) twice charged the police cordon blocking access to Conway Hall. Police reinforcements, including mounted police and units of the Special Patrol Group, forced the rioting demonstrators out of the square. As the ranks of people moved away from the square, Gately was found unconscious on the ground. He was taken to hospital and died later that day. Two further disturbances took place in the vicinity, both involving clashes between the police and the IMG contingent. A public inquiry into the events was conducted by Lord Scarman. He found no evidence that Gately had been killed by the police, as had been alleged by some elements of the hard-left press, and concluded that those who started the riot carry a measure of moral responsibility for his death; and the responsibility is a heavy one.[1] He found fault with some actions of the police on the day. The events in the square made the National Front a household name in the UK, although it is debatable if this had any impact on their share of the vote in subsequent general elections. Although the IMG was heavily criticised by the press and public, there was a rise in localised support and the willingness to demonstrate against the National Front and its policies. There was further violence associated with National Front marches and the counter-demonstrations they faced, including in Birmingham, Manchester, the East End of London (all 1977) and in 1979 in Southall, which led to the death of Blair Peach. After Peach's death, the Labour Party Member of Parliament Syd Bidwell, who had been about to give a speech in Red Lion Square when the violence started, described Peach and Gately as martyrs against fascism and racism.",
            paragraph_style
        );
        textSetter.paint(box->x, box->y, canvas);
//        int offset = 0;
//        for (size_t paragraph_idx = 0; paragraph_idx < box->paragraphs.size(); paragraph_idx++) {
//            auto paragraph = box->paragraphs[paragraph_idx];
//            auto paraStyle = paragraphStyles[paragraph->style];
//            std::cout << "style: " << paragraph->style << std::endl;
//            TextSetter textSetter(box->width, box->height - offset, paraStyle, std::vector<laid::Box>{});
//
//            for (size_t textrun_idx = 0; textrun_idx < paragraph->text_runs.size(); textrun_idx++) {
//                auto& text_run = paragraph->text_runs[textrun_idx];
//                textSetter.SetText(text_run.text, paragraphStyles[text_run.style]);
//                if (textSetter.HasOverflowingText()) {
//                    if (page->overflow == true && box->next == nullptr) {
//                        auto newPage = laidDoc->overflowPage(page);
//                    }
//
//                        auto newParagraph = std::make_shared<laid::Paragraph>();
//                        newParagraph->style = paragraph->style;
//                        box->next->addParagraph(newParagraph);
//                        std::cout << "text_run_style: " << text_run.style << std::endl;
//                        newParagraph->addText(textSetter.overflowingText, text_run.style);
//                        
//                        for (size_t i = paragraph_idx + 1; i < box->paragraphs.size(); i++) {
//                            auto& overflow_paragraph = box->paragraphs[i];
//                            box->next->addParagraph(overflow_paragraph);
//                            for (size_t i = textrun_idx + 1; i < paragraph->text_runs.size(); i++) {
//                                auto& overflow_text_run = paragraph->text_runs[i];
//                                std::cout << "overflowing text run: " << overflow_text_run.style << std::endl;
//                                overflow_paragraph->addText(overflow_text_run.text, overflow_text_run.style);
//                            }
//                        }
//                        textSetter.paint(box->x, box->y, canvas);
//                    } else {
//                        std::cout << "Overflowing text in box!" << std::endl;
//                    }
//                }
//            }
//            textSetter.Paint(box->x, box->y+offset, canvas);
//            // always add a new line when paragraph ends. 
//            // TODO perhaps each box should have a y pos of where content currently is
//            offset += textSetter.contentHeight;
//        }
    };
};
#endif