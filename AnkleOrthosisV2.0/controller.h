#include <stdint.h>        /* Includes uint16_t definition                    */

struct st_impedance
{
   int  st ;
   float impedance;
   float percent_new;
};


int controller(float angle, float ankle_velocity,int16_t ac_x,float current_factor);
struct st_impedance controller_impedance(float angle, float ankle_velocity,int16_t ac_x, float current_factor);