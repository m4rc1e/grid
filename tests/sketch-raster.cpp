// Tomorrow
// Basic start
// Loading a basic xml doc (done)
// build skia (done)
// output a png (done, fucking nightmare)!
// write a paragraph to a skia canvas
// output this skia canvas to a pdf
#include <iostream>
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkData.h"
#include "include/core/SkImage.h"
#include "include/core/SkStream.h"
#include "include/core/SkSurface.h"
#include "include/codec/SkCodec.h"
#include "include/encode/SkPngEncoder.h"


int main() {
    int width = 800;
    int height = 600;

    SkBitmap bitmap;
    bitmap.allocN32Pixels(width, height);

    SkCanvas canvas(bitmap);

    SkPaint paint;
    paint.setColor(SK_ColorRED);

    SkRect rect = SkRect::MakeXYWH(10, 10, 100, 100);
    canvas.drawRect(rect, paint);
    sk_sp<SkImage> image = bitmap.asImage();
    sk_sp<SkData> png = SkPngEncoder::Encode(nullptr, image.get(), {});

    SkFILEWStream out("output.png");
    out.write(png->data(), png->size());

    return 0;
}