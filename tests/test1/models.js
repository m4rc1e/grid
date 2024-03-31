export class Document {
  constructor(title) {
    this.title = title;
    this.pages = [];
  }

  addPage(page) {
    this.pages.push(page);
  }

  removePage(pageIndex) {
    this.pages.splice(pageIndex, 1);
  }
}

export class Page {
  constructor() {
    this.elements = [];
  }

  addElement(element) {
    this.elements.push(element);
  }

  removeElement(elementIndex) {
    this.elements.splice(elementIndex, 1);
  }
}

export class Element {
  constructor(type, content, position, size) {
    this.type = type;
    this.content = content;
    this.position = position;
    this.size = size;
  }
}