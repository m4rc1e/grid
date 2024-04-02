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

using namespace skia::textlayout;


int main() {
    auto fontCollection = sk_make_sp<FontCollection>();
    fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
    

    int width = 800;
    int height = 600;

    SkBitmap bitmap;
    bitmap.allocN32Pixels(width, height);

    SkCanvas canvas(bitmap);


    TextStyle text_style;
    text_style.setColor(SK_ColorBLACK);
    text_style.setFontFamilies({SkString("Maven Pro")});
    text_style.setFontSize(100.0f);
    ParagraphStyle paragraph_style;
    paragraph_style.setTextStyle(text_style);

    ParagraphBuilderImpl builder(paragraph_style, fontCollection);
    builder.pushStyle(text_style);
    builder.addText("Hello, World!");

    auto paragraph = builder.Build();
    paragraph->layout(SK_ScalarInfinity);

    paragraph->paint(&canvas, 0, 0);
    

    sk_sp<SkImage> image = bitmap.asImage();
    sk_sp<SkData> png = SkPngEncoder::Encode(nullptr, image.get(), {});

    SkFILEWStream out("text.png");
    out.write(png->data(), png->size());


    return 0;
}