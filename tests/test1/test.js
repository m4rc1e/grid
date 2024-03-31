import { CanvasKitInit } from './canvaskit.js';

CanvasKitInit({
  locateFile: (file) => `https://unpkg.com/canvaskit-wasm@0.27.0/bin/${file}`
}).then((CanvasKit) => {
  // You can use CanvasKit here
  const surface = CanvasKit.MakeCanvasSurface('myCanvas');
  const paint = new CanvasKit.Paint();

  // Draw something with CanvasKit
  const canvas = surface.getCanvas();
  const paragraphStyle = new CanvasKit.ParagraphStyle({
    textStyle: {
      color: CanvasKit.BLUE,
      fontSize: 24,
      fontFamilies: ["arial"],
    },
  });

  const builder = CanvasKit.ParagraphBuilder.Make(paragraphStyle, CanvasKit.FontMgr.RefDefault());
  builder.addText('This is a paragraphs of text.\lines are respected.\nकदकगहतु');

  const paragraph = builder.build();
  paragraph.layout(300);  // width in pixels

  canvas.drawParagraph(paragraph, 10, 10);
  surface.flush();
});