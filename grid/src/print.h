#ifndef PRINT_H
#define PRINT_H


// We should be thinking about saddle stiching as well.
// Currently, only perfect binding works

namespace laid {
struct PrintSettings {
    // A4 by default. Maybe it should be US Letter
    enum Composition {
        Single,
        Spreads,
        SaddleStitchSpreads,
    };
    float paperWidth = 595;
    float paperHeight = 842;
    bool cropMarks = false;
    Composition composition = Single;
};

}; // namespace laid
#endif