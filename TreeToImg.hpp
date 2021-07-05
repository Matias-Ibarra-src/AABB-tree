#ifndef TreeToImg_H_
#define TreeToImg_H_
#define Espacio 1000
#include "EasyBMP.hpp"

using namespace std;

class TreeToImg {
private:
    EasyBMP::Image* img;
public:
    void newImage(string);
    void drawSquare(int, int, int, int, string);
    void save();
    ;
};

void TreeToImg::newImage(string title) {
    EasyBMP::RGBColor black(0, 0, 0);
    this->img = new EasyBMP::Image(Espacio, Espacio, title, black);
}

void TreeToImg::drawSquare(int x1, int y1, int x2, int y2, string color = "white") {
    EasyBMP::RGBColor rgbcolor;
    if (color.compare("white") == 0)
        rgbcolor = EasyBMP::RGBColor(255, 255, 255);
    else if (color.compare("red") == 0)
        rgbcolor = EasyBMP::RGBColor(255, 0, 0);
    else if (color.compare("blue") == 0)
        rgbcolor = EasyBMP::RGBColor(0, 0, 255);
    if (color.compare("green") == 0)
        rgbcolor = EasyBMP::RGBColor(0, 255, 0);

    for (int i = x1; i <= x2; i++) {
        this->img->SetPixel(i, y1, rgbcolor);
        this->img->SetPixel(i, y2, rgbcolor);
    }

    for (int i = y1; i <= y2; i++) {
        this->img->SetPixel(x1, i, rgbcolor);
        this->img->SetPixel(x2, i, rgbcolor);
    }

}

void TreeToImg::save() {
    this->img->Write();
}

#endif