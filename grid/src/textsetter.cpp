
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


#define TestCanvasWidth 1000
#define TestCanvasHeight 600

using namespace skia::textlayout;

sk_sp<FontCollection> fontCollection = sk_make_sp<FontCollection>();

void setupFontCollection() {
    fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
}

class TestCanvas {
public:
    TestCanvas(const char* testName) : name(testName) {
        bitmap.allocN32Pixels(TestCanvasWidth, TestCanvasHeight);
        
        canvas = new SkCanvas(bitmap);
        canvas->clear(SK_ColorWHITE);
    }
    SkBitmap bitmap;
    SkCanvas* canvas;
    const char* name;

    ~TestCanvas() {
        SkString path = SkOSPath::Join("/Users/marcfoley/Desktop/skia_test", name);
        SkFILEWStream file(path.c_str());
        canvas->flush();
        SkPngEncoder::Encode(&file, bitmap.pixmap(), {});
        delete canvas;
    }

    void drawRects(SkColor color, std::vector<TextBox>& result, bool fill = false) {

        SkPaint paint;
        if (!fill) {
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setAntiAlias(true);
            paint.setStrokeWidth(1);
        }
        paint.setColor(color);
        for (auto& r : result) {
            canvas->drawRect(r.rect, paint);
        }
    }

    void drawLine(SkColor color, SkRect rect, bool vertical = true) {

        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setAntiAlias(true);
        paint.setStrokeWidth(1);
        paint.setColor(color);
        if (vertical) {
            canvas->drawLine(rect.fLeft, rect.fTop, rect.fLeft, rect.fBottom, paint);
        } else {
            canvas->drawLine(rect.fLeft, rect.fTop, rect.fRight, rect.fTop, paint);
        }
    }

    void drawLines(SkColor color, std::vector<TextBox>& result) {

        for (auto& r : result) {
            drawLine(color, r.rect);
        }
    }
    SkCanvas* get() { return canvas; }
};


class Box {
    public:
    int x;
    int y;
    int width;
    int height;
};

class CursorPos {
    public:
    int x;
    int y;
};

class TextSetter {
public:
    TextSetter(
        int width,
        int height,
        skia::textlayout::ParagraphStyle& paragraph_style,
        std::vector<Box>& boxes) : 
        width(width),
        height(height),
        paragraph_style(paragraph_style),
        boxes(boxes),
        builder(paragraph_style, fontCollection) {
    }
    int width;
    int height;
    std::string overflowingText;
    
    void setText(
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
            paragraph->layout(TestCanvasWidth);

        }
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
        wordParagraph->layout(TestCanvasWidth);
        auto wordCursor = getCursor(wordParagraph.get());
        return CursorPos{cursor.x + wordCursor.x, cursor.y};
    }

    void paint(SkCanvas* canvas) {
        paragraph->paint(canvas, 0, 0);
    }

    CursorPos getCursor(Paragraph* paragraph) {
        RectHeightStyle rect_height_style = RectHeightStyle::kTight;
        RectWidthStyle rect_width_style = RectWidthStyle::kTight;

        auto boxes = paragraph->getRectsForRange(0, 1000000, rect_height_style, rect_width_style);
        int cursorX = boxes.back().rect.fRight;
        int cursorY = paragraph->getHeight();
        return CursorPos{cursorX, cursorY};
    }

    std::optional<Box> boxCollision(CursorPos cursor) {
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

    std::vector<Box>& boxes;
};

int main() {
    setupFontCollection();
    TestCanvas canvas("box_collision2.png");

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

    std::vector<Box> boxes = {
        Box{300, 0, 200, 100},
        Box{200, 150, 100, 100}
    };
    TextSetter textSetter(1000, 200, paragraph_style, boxes);
    textSetter.setText("On 15 June 1974 the National Front held a march through central London in support of the compulsory repatriation of immigrants. The march was to end at Conway Hall in Red Lion Square. A counter-demonstration was planned by Liberation, an anti-colonial pressure group. During the late 1960s and early 1970s, the London council of Liberation had been increasingly infiltrated by hard-left political activists, and they invited several hard-left organisations to join them in the march. When the Liberation march reached Red Lion Square, the International Marxist Group (IMG) twice charged the police cordon blocking access to Conway Hall. Police reinforcements, including mounted police and units of the Special Patrol Group, forced the rioting demonstrators out of the square. As the ranks of people moved away from the square, Gately was found unconscious on the ground. He was taken to hospital and died later that day. Two further disturbances took place in the vicinity, both involving clashes between the police and the IMG contingent. A public inquiry into the events was conducted by Lord Scarman. He found no evidence that Gately had been killed by the police, as had been alleged by some elements of the hard-left press, and concluded that those who started the riot carry a measure of moral responsibility for his death; and the responsibility is a heavy one.[1] He found fault with some actions of the police on the day. The events in the square made the National Front a household name in the UK, although it is debatable if this had any impact on their share of the vote in subsequent general elections. Although the IMG was heavily criticised by the press and public, there was a rise in localised support and the willingness to demonstrate against the National Front and its policies. There was further violence associated with National Front marches and the counter-demonstrations they faced, including in Birmingham, Manchester, the East End of London (all 1977) and in 1979 in Southall, which led to the death of Blair Peach. After Peach's death, the Labour Party Member of Parliament Syd Bidwell, who had been about to give a speech in Red Lion Square when the violence started, described Peach and Gately as martyrs against fascism and racism.",
        paragraph_style
    );
    textSetter.paint(canvas.get());
//    auto blue = SkPaint();
//    blue.setStyle(SkPaint::kStroke_Style);
//    blue.setAntiAlias(true);
//    blue.setStrokeWidth(1);
//    blue.setColor(SK_ColorBLUE);
//    canvas.get()->drawRect(SkRect::MakeLTRB(300, 0, 500, 100), blue);

    RectHeightStyle rect_height_style = RectHeightStyle::kTight;
    RectWidthStyle rect_width_style = RectWidthStyle::kTight;

//    auto boxes2 = textSetter.paragraph->getRectsForRange(0, 10000, rect_height_style, rect_width_style);
//    canvas.drawRects(SK_ColorGREEN, boxes2);
//
//
//    boxes2 = textSetter.paragraph->getRectsForPlaceholders();
//    canvas.drawRects(SK_ColorRED, boxes2);
//
    return 0;
} 