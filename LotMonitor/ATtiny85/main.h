// cRGB is internal grb !

#include "light_ws2812.h"

#define TIM0_NO_CLOCK					(~(1<<CS02 | 1<<CS01 | 1<<CS00))
#define TIM0_PREESCALER_1				1<<CS00
#define TIM0_PREESCALER_8				1<<CS01
#define TIM0_PREESCALER_64				1<<CS01 | 1<<CS00
#define TIM0_PREESCALER_256				1<<CS02
#define TIM0_PREESCALER_1024			1<<CS02 | 1<<CS00

static const struct cRGB color_black = {   0,   0,   0 };
static const struct cRGB color_red   = {   0, 255,   0 };
static const struct cRGB color_green = { 255,   0,   0 };
static const struct cRGB color_blue  = {   0,   0, 255 };
static const struct cRGB color_white = { 255, 255, 255 };

void allOFF(uint8_t update_available);
void all(uint8_t r, uint8_t g, uint8_t b);
void allRGB(uint8_t r, uint8_t g, uint8_t b);
void allColor(struct cRGB color);

void set_timer0_preescaler_and_compare_registers(uint8_t preescaler,uint8_t compare); 

#define True 1
#define False 0


enum US_states
{
	trigger,
	waiting_for_echo,
	echo_received,
	time_out,
	
};

struct US_variables
{
	uint16_t trigger_elapsed;
	uint16_t echo_elapsed;
	uint16_t echo_total_time;
	uint16_t echo_buffered_total_time;
	uint8_t updating_measures; 
	enum US_states state;
}