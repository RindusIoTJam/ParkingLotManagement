// cRGB is internal grb !
static const struct cRGB color_black = {   0,   0,   0 };
static const struct cRGB color_red   = {   0, 255,   0 };
static const struct cRGB color_green = { 255,   0,   0 };
static const struct cRGB color_blue  = {   0,   0, 255 };
static const struct cRGB color_white = { 255, 255, 255 };

void allOFF();
void all(uint8_t r, uint8_t g, uint8_t b);
void allRGB(uint8_t r, uint8_t g, uint8_t b);
void allColor(struct cRGB color);
