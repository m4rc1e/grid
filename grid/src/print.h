#ifndef PRINT_H
#define PRINT_H


// We should be thinking about saddle stiching as well.
// Currently, only perfect binding works

namespace laid {
struct PrintSettings {
    enum Composition {
        Single,
        Spreads,
        SaddleStitchSpreads,
    };
    float paperWidth;
    float paperHeight;
    bool cropMarks = false;
    Composition composition = Single;
};

}; // namespace laid
#endif