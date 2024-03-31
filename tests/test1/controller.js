import { Document, Page, Element } from './models.js';
import { DocumentView } from './views.js';

export class DocumentController {
  constructor(document, view) {
    this.document = document;
    this.view = view;
  }

  addPage() {
    const page = new Page();
    this.document.addPage(page);
    this.view.render();
  }

  addElementToPage(pageIndex, type, content, position, size) {
    const element = new Element(type, content, position, size);
    this.document.pages[pageIndex].addElement(element);
    this.view.render();
  }
}