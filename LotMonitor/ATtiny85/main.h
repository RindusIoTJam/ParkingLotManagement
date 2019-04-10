
// Define some colors. (cRGB is internal grb !)
static const struct cRGB color_black   = {   0,   0,   0 };
static const struct cRGB color_red     = {   0, 255,   0 };
static const struct cRGB color_green   = { 255,   0,   0 };
static const struct cRGB color_blue    = {   0,   0, 255 };
static const struct cRGB color_yellow  = { 255, 255,   0 };
static const struct cRGB color_cyan    = { 255,   0, 255 };
static const struct cRGB color_magenta = {   0, 255, 255 };
static const struct cRGB color_white   = { 255, 255, 255 };

// This function lights a bar in the given RGB and length.
void inline setLEDS(uint8_t r, uint8_t g, uint8_t b, uint8_t c);

// This function turns all LEDs off
void inline allOFF();

// This function lights all LEDs to a given color
void inline allColor(struct cRGB color);

// This function lights a bar in the given color and length
void inline barColor(struct cRGB color, int bars);

// This function is executed 100 times per sec by timer interupt (See TCCR0B)
ISR(TIM0_COMPA_vect);
