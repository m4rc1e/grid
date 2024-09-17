#ifndef TEXTSETTER_H
#define TEXTSETTER_H
#include "models.h"

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
#include "include/core/SkPath.h"
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
    float paintX;
    float paintY;
    float contentHeight;
    std::string overflowingText;
    std::vector<laid::Box> boxes;
    
    bool hasOverflowingText() {
        return overflowingText.size() > 0;
    }

    void SetText(
        std::string text,
        skia::textlayout::ParagraphStyle& paragraph_style,
        std::shared_ptr<laid::Box> box) {
        auto text_style = paragraph_style.getTextStyle();
        auto strut_style = paragraph_style.getStrutStyle();
        builder.pushStyle(text_style);


        std::istringstream ss(text); // " " added at end due to delimiter for std::getline
        std::string token;

        auto maxLines = int(height / text_style.getFontSize());

        bool exhausted = false;
        while (std::getline(ss, token, ' ')) {
            auto currentCursor = getCursor(paragraph.get());
            auto nextCursor = getNextCursor(token, paragraph_style, paragraph.get());
            auto currentLine = int(paragraph->lineNumber());

            // Handle collisions with other Boxes
            auto collidedBox = boxCollision(currentCursor, nextCursor);
            while (collidedBox) {
                paragraph = builder.Build();
                paragraph->layout(width);
                currentCursor = getCursor(paragraph.get());
                nextCursor = getNextCursor(token, paragraph_style, paragraph.get());
                collidedBox = boxCollision(currentCursor, nextCursor);
                if (currentCursor.x >= nextCursor.x) {
                    std::cout << "col";
                    addPlaceholder(collidedBox->width);
                } else {
                    addPlaceholder(collidedBox->width - (currentCursor.x - collidedBox->x));
                }
            }
            // Handle exceeding height of box
            if (nextCursor.y > height || exhausted == true || height < strut_style.getFontSize()) {
                exhausted = true;
                std::cout << "overflowing" << '\n';
                overflowingText += token + " ";
                continue;
            }

            // handle tabs
            if (token.find("\t") != std::string::npos) {
                std::istringstream iss(token);
                std::string leftWord, tab, rightWord;
                std::string subToken;
                while (std::getline(iss, subToken, '\t')) {
                    builder.addText(subToken.data());
                    paragraph = builder.Build();
                    paragraph->layout(width);
                    currentCursor = getCursor(paragraph.get());
                    auto tabWidth = box->nextTab(currentCursor.x) - currentCursor.x;
                    addPlaceholder(tabWidth);
                }
            } else {
                builder.addText(token.data());
                builder.addText(" ");
            }
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

        ParagraphBuilderImpl wordBuilder(style, fontCollection);
        auto text_style = style.getTextStyle();
        wordBuilder.pushStyle(text_style);

        wordBuilder.addText(token.data());
        auto wordParagraph = wordBuilder.Build();
        wordParagraph->layout(width);
        auto wordCursor = getCursor(wordParagraph.get());
        if (cursor.x + wordCursor.x + 10 >= width) {
            return CursorPos{wordCursor.x, cursor.y + 12};
        }
        return CursorPos{cursor.x + wordCursor.x, cursor.y};
    }

    void paintCoords(float x, float y) {
        paintX = x;
        paintY = y;
    }
    void paint(SkCanvas* canvas) {
        paragraph->paint(canvas, paintX, paintY);
    }

    CursorPos getCursor(Paragraph* paragraph) {
        RectHeightStyle rect_height_style = RectHeightStyle::kTight;
        RectWidthStyle rect_width_style = RectWidthStyle::kTight;

        auto boxes = paragraph->getRectsForRange(0, 1000000, rect_height_style, rect_width_style);
        int cursorX = boxes.back().rect.fRight;
        int cursorY = paragraph->getHeight();
        return CursorPos{cursorX, cursorY};
    }

    std::optional<laid::Box> boxCollision(CursorPos currentCursor, CursorPos nextCursor) {
        for (auto& box : boxes) {
            if (
                nextCursor.x >= box.x && 
                nextCursor.x <= box.x + box.width && 
                nextCursor.y >= box.y && 
                nextCursor.y <= box.y + box.height
            ) {
                return box;
            }
            // Fix edge case where next word is larger than box starting from new line
            if (
                nextCursor.x < currentCursor.x &&
                nextCursor.y > currentCursor.y &&
                nextCursor.x > box.x + box.width
            ) {
                return box;
            }
        }
        return std::nullopt;;
    }

    void addPlaceholder(int width) {
        PlaceholderStyle placeholder1(width, 5, PlaceholderAlignment::kTop, TextBaseline::kAlphabetic, 0);
        builder.addPlaceholder(placeholder1);
    }

private:
    ParagraphBuilderImpl builder;
    
    std::unique_ptr<skia::textlayout::Paragraph>  paragraph;
    skia::textlayout::ParagraphStyle paragraph_style;

};


#endif