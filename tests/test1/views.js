import { Document, Page, Element } from './models.js';
import { CanvasKitInit } from 'canvaskit-wasm';

export class DocumentView {
  constructor(document, canvasId) {
    this.document = document;
    this.canvasId = canvasId;
  }

  render() {
    CanvasKitInit({
      locateFile: (file) => `https://unpkg.com/canvaskit-wasm@0.27.0/bin/${file}`
    }).then((CanvasKit) => {
      const surface = CanvasKit.MakeCanvasSurface(this.canvasId);
      const canvas = surface.getCanvas();
      const paint = new CanvasKit.Paint();

      this.document.pages.forEach((page, pageIndex) => {
        // Draw a rectangle for the page
        const pageRect = CanvasKit.LTRBRect(0, pageIndex * 1100, 850, (pageIndex + 1) * 1100 - 50);
        paint.setColor(CanvasKit.Color(255, 255, 255, 1.0));  // white
        canvas.drawRect(pageRect, paint);

        page.elements.forEach((element) => {
          const paragraphStyle = new CanvasKit.ParagraphStyle({
            textStyle: {
              color: CanvasKit.BLACK,
              fontSize: 24,
              fontFamilies: ['Arial'],
            },
          });

          const builder = CanvasKit.ParagraphBuilder.Make(paragraphStyle, CanvasKit.FontMgr.RefDefault());
          builder.addText(element.content);

          const paragraph = builder.build();
          paragraph.layout(300);  // width in pixels

          // Adjust the y position of the element based on the page index
          canvas.drawParagraph(paragraph, element.position.x, element.position.y + pageIndex * 1100);
        });
      });

      surface.flush();
    });
  }
}