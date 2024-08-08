#ifndef PDFVIEW_H
#define PDFVIEW_H
#include "models.h"
#include "print.h"
#include "textsetter.h"
#include "debug.h"
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
        secondPassDoc = std::make_shared<laid::Document>(*laidDoc);
        std::cout << "Building PDF" << std::endl;
    };

    std::shared_ptr<laid::Document> laidDoc;
    sk_sp<FontCollection> fontCollection = sk_make_sp<FontCollection>();
    SkFILEWStream stream;
    SkPDF::Metadata metadata;
    sk_sp<SkDocument> pdf;
    std::map<std::string, ParagraphStyle> paragraphStyles;
    std::map<std::string, SkPaint> boxStyles;
    std::map<std::string, SkPaint> strokeStyles;
    bool debug;
    DebugGuides debugGuides;
    // on the second pass, we'll rebuild the doc and populate variables
    // that we uncovered on the first pass. This is quite inefficient but
    // it is simple.
    bool secondPass = false;
    std::shared_ptr<laid::Document> secondPassDoc;

    void BuildBoxStyles() {
        for (auto& [name, style] : laidDoc->boxStyles) {
            SkPaint paint;
            auto color = laidDoc->colors[style.color];
            paint.setColor(SkColorSetRGB(color.r, color.g, color.b));
            boxStyles[name] = paint;
        }
    }

    void BuildStrokeStyles() {
        for (auto& [name, style] : laidDoc->strokeStyles) {
            SkPaint paint;
            auto color = laidDoc->colors[style.color];
            paint.setColor(SkColorSetRGB(color.r, color.g, color.b));
            paint.setStrokeWidth(style.thickness);
            paint.setStyle(SkPaint::kStroke_Style);
            strokeStyles[name] = paint;
        }
    }

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
            if (style.color != "") {
                auto color = laidDoc->colors[style.color];
                text_style.setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
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
            
            // baseline shift
            auto ff = SkFontMetrics{};
            text_style.setFontSize(style.leading);
            text_style.getFontMetrics(&ff);
            text_style.setFontSize(style.fontSize);
            text_style.setBaselineShift(ff.fDescent);
            
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
        BuildStrokeStyles();
        BuildBoxStyles();
        BuildStyles();
        BuildPages();

        secondPass = true;
        pdf = SkPDF::MakeDocument(&stream, metadata);
        secondPassDoc->pageLinks = laidDoc->pageLinks;
        laidDoc = secondPassDoc;
        BuildPages();
        std::cout << "EE" << std::endl;
    }

    void offsetCanvas(SkCanvas* canvas, float width, float height) {
        if (laidDoc->printSettings.paperWidth < width || laidDoc->printSettings.paperHeight < height) {
            throw std::invalid_argument("Paper size is smaller than page size");
        }
        auto widthOffset = (laidDoc->printSettings.paperWidth - width) / 2;
        auto heightOffset = (laidDoc->printSettings.paperHeight - height) / 2;
        canvas->translate(widthOffset, heightOffset);
    }

    void BuildSinglePage(std::shared_ptr<laid::Page> head, int currentPage) {
        auto width = head->masterPage.width;
        auto height = head->masterPage.height;
        auto canvas = pdf->beginPage(laidDoc->printSettings.paperWidth, laidDoc->printSettings.paperHeight);
        offsetCanvas(canvas, width, height);
        canvas->save();
        canvas->clipRect(SkRect::MakeXYWH(0, 0, width, height));
        head->number = currentPage;
        BuildPage(head, canvas);
        canvas->restore();

        if (laidDoc->printSettings.cropMarks == true) {
            BuildCrops(canvas, head->masterPage.width, head->masterPage.height);
        }

        pdf->endPage();
    }

    void BuildSpread(std::shared_ptr<laid::Page> head, int currentPage) {
        auto canvas = pdf->beginPage(laidDoc->printSettings.paperWidth, laidDoc->printSettings.paperHeight);
        auto width = head->masterPage.width + head->next->masterPage.width;
        auto height = head->masterPage.height;
        offsetCanvas(canvas, width, height);

        canvas->save();
        canvas->clipRect(SkRect::MakeXYWH(0, 0, width, height));
        head->number = currentPage;
        BuildPage(head, canvas);
        canvas->restore();
        canvas->translate(head->masterPage.width, 0);
        currentPage++;

        canvas->save();
        canvas->clipRect(SkRect::MakeXYWH(0, 0, width, height));
        head->next->number = currentPage;
        BuildPage(head->next, canvas);
        canvas->restore();

        // translate canvas back to origin so we can add crops
        canvas->translate(-head->masterPage.width, 0);
        if (laidDoc->printSettings.cropMarks == true) {
            BuildCrops(canvas, width, height);
        }
        pdf->endPage();
    }

    void BuildPages() {
        // TODO bring back print paper sizing
        std::shared_ptr<laid::Page> head = laidDoc->pages;
        auto currentPage = 0;
        if (laidDoc->printSettings.composition == laid::PrintSettings::Composition::Single) {
            while (head != nullptr) {
                BuildSinglePage(head, currentPage);
                head = head->next;
                currentPage++;
            }
        } else if (laidDoc->printSettings.composition == laid::PrintSettings::Composition::Spreads) {
            while (head != nullptr) {
                if (head->type == laid::Page::PageType::Single) {
                    BuildSinglePage(head, currentPage);
                    head = head->next;
                } else if (head->type == laid::Page::PageType::Left) {
                    BuildSpread(head, currentPage);
                    head = head->next->next;
                    currentPage += 2;
                }
            }
        }
        pdf->close();
        laidDoc->page_count = currentPage;
    }
    void StyleBox(std::shared_ptr<laid::Box> box, SkCanvas* canvas) {
        if (box->style != "") {
            canvas->drawRect(
                SkRect::MakeXYWH(box->x, box->y, box->width, box->height),
                boxStyles[box->style]
            );
        }
    }

    void interpolateVariables(laid::TextRun* run, std::shared_ptr<laid::Page> page) {
        auto text = run->text;

        // interpolate page numbers. TODO make this its own func
        auto foundPageNum = text.find("{{ page_number }}");
        if (foundPageNum != std::string::npos) {
            text.replace(foundPageNum, 17, std::to_string(page->number));
            run->text = text;
        }

        // interpolate page names. Same structure as above so if we add another let's refactor
        auto foundPageName = text.find("{{ page_name }}");
        if (foundPageName != std::string::npos) {
            text.replace(foundPageName, 15, page->name);
            run->text = text;
        }

        // interpolate page links
        if (secondPass == false) {
            return;
        }
        auto foundPageLink = text.find("{{ Story_number }}");
        if (foundPageLink != std::string::npos) {
            auto link = laidDoc->pageLinks[std::string("The Story")];
            text.replace(foundPageLink, 18, std::to_string(link));
            run->text = text;
        }
    }

    SkCanvas* BuildPage(std::shared_ptr<laid::Page> page, SkCanvas* canvas) {
        int width = page->masterPage.width;
        int height = page->masterPage.height;
        
        // add page link if page name isn't already linked
        if (page->name != "" && laidDoc->pageLinks.find(page->name) == laidDoc->pageLinks.end()) {
            laidDoc->pageLinks[page->name] = page->number;
        }

        // Add master page boxes to page boxes
        for(const auto& box: page->masterPage.boxes) {
            auto clone = std::make_shared<laid::Box>(*box);
            page->boxes.push_back(clone);
        }

        for(auto box : page->boxes) {
            if (box->image_path.size() > 0) {
                BuildImage(canvas, box);
            }
            StyleBox(box, canvas);
            BuildText(canvas, page, box);
            if (debug == true) {
                debugGuides.BuildBoxRect(canvas, box);
            }
        }
        if (debug == true) {
            debugGuides.BuildGuides(canvas, page->masterPage);
            debugGuides.BuildBaseline(canvas, page->masterPage);
        }
        std::cout << "Ending page: " << page << std::endl;
        return canvas;
    }

    void BuildCrops(SkCanvas* canvas, float width, float height) {
        SkPaint paintCrop;
        paintCrop.setColor(SK_ColorBLACK);
        paintCrop.setStrokeWidth(.25f);
        paintCrop.setStyle(SkPaint::kStroke_Style);
        // top left
        canvas->drawLine(SkPoint::Make(-3, 0), SkPoint::Make(-10, 0), paintCrop);
        canvas->drawLine(SkPoint::Make(0, -3), SkPoint::Make(0, -10), paintCrop);
        // top right
        canvas->drawLine(SkPoint::Make(width + 3, 0), SkPoint::Make(width + 10, 0), paintCrop);
        canvas->drawLine(SkPoint::Make(width, - 3), SkPoint::Make(width, - 10), paintCrop);
        // bottom left
        canvas->drawLine(SkPoint::Make(-3, height), SkPoint::Make(-10, height), paintCrop);
        canvas->drawLine(SkPoint::Make(0, height + 3), SkPoint::Make(0, height + 10), paintCrop);
        // bottom right
        canvas->drawLine(SkPoint::Make(width + 3, height), SkPoint::Make(width + 10, height), paintCrop);
        canvas->drawLine(SkPoint::Make(width, height + 3), SkPoint::Make(width, height + 10), paintCrop);
        std::cout << "Done crops" << std::endl;
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

    // Find boxes that collide with the current box and have a zIndex greater than current box
    // offset the box relative to the main box
    std::vector<laid::Box> collidingBoxes(std::shared_ptr<laid::Page> page, std::shared_ptr<laid::Box> box, int offset) {
        std::vector<laid::Box> collisionBoxes;
        for (auto childBox : page->boxes) {
            for (auto& [paraIdx, children] : childBox->children) {
                for (auto& child : children) {
                    if (child->zIndex > box->zIndex) {
                        auto collideBox = laid::Box(child->x - box->x, child->y - box->y, child->width, child->height - offset);
                        collisionBoxes.push_back(collideBox);
                    }
                }
            }
            if (childBox == box) {
                continue;
            }
            if (childBox->zIndex > box->zIndex) {
                auto collideBox = laid::Box(childBox->x - box->x, childBox->y - box->y, childBox->width, childBox->height - offset);
                collisionBoxes.push_back(collideBox);
            }

        }
        return collisionBoxes;
    }

    void BuildRule(SkCanvas* canvas, SkPaint paint, float x1, float y1, float x2, float y2) {
        canvas->drawLine(SkPoint::Make(x1, y1), SkPoint::Make(x2, y2), paint);
    }

    void BuildText(SkCanvas* canvas, std::shared_ptr<laid::Page> page, std::shared_ptr<laid::Box> box) {
        int offset = 0;
        auto collisionBoxes = collidingBoxes(page, box, offset);
        std::vector<TextSetter*> textSetters;
        for (size_t paraIdx = 0; paraIdx < box->paragraphs.size(); paraIdx++) {
            collisionBoxes = collidingBoxes(page, box, offset);
            auto paragraph = box->paragraphs[paraIdx];
            auto laidStyle = laidDoc->paragraph_styles[paragraph->style];
            auto paragraphStyle = paragraphStyles[paragraph->style];

            paragraphStyle.setTextHeightBehavior(TextHeightBehavior::kDisableFirstAscent);
            TextSetter* textSetter = new TextSetter(box->width, box->height - offset, paragraphStyle, collisionBoxes);
            
            // space before padding
            offset += laidDoc->paragraph_styles[paragraph->style].spaceBefore;

            // rule above
            auto strokeStyleAbove = laidDoc->strokeStyles[laidStyle.ruleAbove];
            if (strokeStyleAbove.name != "") {
                BuildRule(
                    canvas,
                    strokeStyles[strokeStyleAbove.name],
                    box->x,
                    box->y+offset+strokeStyleAbove.yOffset,
                    box->x+box->width,
                    box->y+offset+strokeStyleAbove.yOffset
                );
            }

            for (size_t runIdx = 0; runIdx < paragraph->text_runs.size(); runIdx++) {
                auto text_run = paragraph->text_runs[runIdx];

                interpolateVariables(&text_run, page);
                textSetter->SetText(text_run.text, paragraphStyles[text_run.style], box);
                if (textSetter->hasOverflowingText()) {

                    // if there isn't a next box and the page is overflowing, add another page
                    if (box->next == nullptr && page->overflow == true) {
                        if (page->type == laid::Page::PageType::Single) {
                            laidDoc->overflowPage(page);
                        } else if (page->type == laid::Page::PageType::Left) {
                            auto rightPage = page->next;
                            laidDoc->overflowSpread(page, rightPage);
                        } else if (page->type == laid::Page::PageType::Right) {
                            auto leftPage = page->prev;
                            laidDoc->overflowSpread(leftPage, page);
                        }
                    }
                    
                    // push content to next box
                    if (box->next != nullptr) {
                        auto nextBox = box->next;
                        auto overflow_run = laid::TextRun{
                            textSetter->overflowingText,
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
                            for (int j = 0; j < box->children[i].size(); j++) {
                                nextBox->addChild(i, box->children[i+paraIdx][j]);
                            }
                        }

                    } else {
                        std::cout << "overflowing text" << '\n';
                        std::cout << textSetter->overflowingText << '\n';
                    }
                    // Finally paint boxes based on box vertalignment
                    for (auto& setter : textSetters) {
                        if (box->vertAlign == laid::Box::VertAlignChoices::Middle) {
                            setter->paintY = setter->paintY + (box->height - offset) / 2;
                        } else if (box->vertAlign == laid::Box::VertAlignChoices::Bottom) {
                            setter->paintY = setter->paintY + box->height - offset;
                        }
                        setter->paint(canvas);
                        delete setter;
                    }
                    return;
                }
            }
            if (box->children.find(paraIdx) != box->children.end()) {
                std::cout << "Key exists." << std::endl;
                for (auto& child : box->children[paraIdx]) {
                    if (child->y == -1) {
                        child->y = box->y + offset;
                    }
                    if (debug == true) {
                        debugGuides.BuildBoxRect(canvas, child);
                    }
                    StyleBox(child, canvas);
                    BuildText(canvas, page, child);
                }
            }
            textSetter->paintCoords(box->x, box->y+offset);
            textSetters.push_back(textSetter);
            // textSetter.paint(box->x, box->y+offset, canvas);
            // rule below
            auto strokeStyleBelow = laidDoc->strokeStyles[laidStyle.ruleBelow];
            if (strokeStyleBelow.name != "") {
                BuildRule(
                    canvas,
                    strokeStyles[strokeStyleBelow.name],
                    box->x,
                    box->y+offset+strokeStyleAbove.yOffset,
                    box->x+box->width,
                    box->y+offset+strokeStyleAbove.yOffset
                );
            }
            offset += textSetter->contentHeight;
            offset += laidDoc->paragraph_styles[paragraph->style].spaceAfter;
        }

        // Finally paint boxes based on box vertalignment
        for (auto& setter : textSetters) {
            if (box->vertAlign == laid::Box::VertAlignChoices::Middle) {
                setter->paintY = setter->paintY + (box->height - offset) / 2;
            } else if (box->vertAlign == laid::Box::VertAlignChoices::Bottom) {
                setter->paintY = setter->paintY + box->height - offset;
            }
            setter->paint(canvas);
            delete setter;
        }
    };
};
#endif