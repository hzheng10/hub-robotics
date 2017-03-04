#include "controller.h"
#include "StateFormulas.h"
#include "rotation.h"
#include <stdbool.h>
#include <math.h>
#include <libpic30.h>
enum states{
    ST_EARLY_STANCE,
    ST_MIDDLE_STANCE,
    ST_LATE_STANCE,
    ST_EARLY_SWING,
    ST_LATE_SWING,
};
enum states state = ST_EARLY_STANCE;


double impedance = 0;
double desired_force = 0.0, desired_current = 0;
float percent = 0,percent_new = 0,percent_old = 0;  // 0.5 percent means 50% duty cycle
float peak_current = 20.0;
float duty_cycle_max = 0.2; // 0.2
float late_stance_pwm_max = 0.24;    // 0.25
bool flag_begin_SW = false, flag_begin_ES = true;

struct st_impedance my_st_impedance;

struct st_impedance controller_impedance(float angle, float ankle_velocity,int16_t ac_x, float current_factor)
{
    //bool cond_HS = ((angle >= 2)&&(ankle_velocity >= 1))? true:false; //Heel Strike condition is met.
    //bool  cond_HS = (ac_x > 15000) ? true : false;  //Heel Strike condition is met.
    //printf("%d\n",state);

    switch (state)
    {
    // state 0
    case ST_EARLY_STANCE:
        //LATBbits.LATB15 = 1;
        if ((angle >= -1)&&(ankle_velocity >= 5))   //velocity threshold previous 15, angle >= 2
       {
            //state = ST_MIDDLE_STANCE;
           state = ST_MIDDLE_STANCE;
           break;
       }


        //CONTROL ACTION  K1 = 1.7
        impedance = Impedance(angle, ankle_velocity, 0.8, 0.01, -10.0);    // theta_e = -10, k = 0.8, b = 0.001

        //impedance = ((impedance >= - 8.5)&&(impedance <= -5.1))? (-5.1): (impedance);

        //printf("\ntau:%f\n",impedance);
        //desired_current = impedance/210/0.9*1000/37;
        desired_current = DesiredCurrent (impedance,angle);

        //percent = PIDCurrent(desired_current, current, 0.0001, percent); // calculate the valve_command from the PID controller
        percent = desired_current/peak_current;
        //printf("\np:%f\n",-percent);
        //limit the duty cycle to 25% range, so the current will in the range of 0 to 25%*20A = 0 to 5A
        // in practical limit to 20% percent

        if (percent >= duty_cycle_max)
            percent = duty_cycle_max;
        else if(percent <= - duty_cycle_max)
            percent = - duty_cycle_max;


        //Rate Limiter
        percent_new = RateLimiter(percent_old,percent);
        percent_old = percent_new;
        //printf("\np:%f\n",percent_new);
        
        
        if (flag_begin_ES == true)
        {

            flag_begin_ES = false;
            PF (0.2);
            int16_t p = 1000000;    // 1000000
            while (p--);

            break;
        }

        if (desired_current < 0)    //output the pwm command
            PF (-percent_new);
        else if (desired_current >0)
            DF (percent_new);
        else
            Stop();

        break;
    //state 1
    case ST_MIDDLE_STANCE:
        if (angle > 5)
        {
           state = ST_LATE_STANCE;
           break;
       }

        //CONTROL ACTION previous k is 1.8
        impedance = Impedance(angle, 0.0, 3.75, 0.01, -3); // -24 Nm to -51Nm, theta_e = -18, k = 0.9, b = 0.01
        //printf("\ntau:%f\n",impedance);
        //desired_current = impedance/210/0.9*1000/37;
        desired_current = DesiredCurrent (impedance,angle);

        //percent = PIDCurrent(desired_current, current, 0.0001, percent); // calculate the valve_command from the PID controller
        percent = desired_current/peak_current;
        //printf("\np:%f\n",-percent);
        //limit the duty cycle to 25% range, so the current will in the range of 0 to 25%*20A = 0 to 5A
        // in practical limit to 20% percent

        if (percent >= duty_cycle_max)
            percent = duty_cycle_max;
        else if(percent <= - duty_cycle_max)
            percent = - duty_cycle_max;
        //printf("\np:%f\n",-percent);

        //Rate Limiter
        percent_new = RateLimiter(percent_old,percent);
        percent_old = percent_new;
        //percent_new =0;
        if (desired_current < 0)    //output the pwm command
            PF (-percent_new*current_factor);
        else if (desired_current >0)
            DF (percent_new*current_factor);
        else
            Stop();

        break;

    //state 2
    case ST_LATE_STANCE:
        if ((angle < -18)&&(ankle_velocity >= -20))     // -18, -10
        {
            state = ST_EARLY_SWING;
            flag_begin_SW = true;
            //Stop(); //stop the motor
            break;
        }

        //CONTROL ACTION  2.5
        impedance = Impedance(angle, 0.0, 1.8, 0.01, -20.0); // -52 Nm to -7.2Nm, 2, 0.01, -20.0
        impedance = ((impedance >= - 8.5)&&(impedance <= -5.4))? (-8.5): (impedance);


        //impedance = Impedance(angle, 0.0, 1, 0.01, -20.0);
        //printf("\ntau:%f\n",impedance);
        //desired_current = impedance/210/0.9*1000/37;
        desired_current = DesiredCurrent (impedance,angle);

        //percent = PIDCurrent(desired_current, current, 0.0001, percent); // calculate the valve_command from the PID controller
        percent = desired_current/peak_current;
        //printf("\np:%f\n",-percent);
        //limit the duty cycle to 25% range, so the current will in the range of 0 to 25%*20A = 0 to 5A
        // in practical limit to 20% percent

        if (percent >= late_stance_pwm_max)
            percent = late_stance_pwm_max;
        else if(percent <= - late_stance_pwm_max)
            percent = - late_stance_pwm_max;
        //printf("\np:%f\n",-percent);

        percent_new = RateLimiter(percent_old,percent);
        percent_old = percent_new;

        if (desired_current < 0)    //output the pwm command
            PF (-percent_new*current_factor);
        else if (desired_current >0)
            DF (percent_new*current_factor);
        else
            Stop();
        break;
      //state 3
      case ST_EARLY_SWING:
          //TRISBbits.TRISB15 = 1;
        if ((angle>=1.5)&&(fabsf(ankle_velocity)<= 5))  // ankle_velocity < -5: did not transit, ankle_velocity < -3: transit
        {
            //Stop(); //stop the motor

            //LATBbits.LATB15 = 0;
            //__delay_ms(50);
           state = ST_LATE_SWING;
           break;
        }

        //CONTROL ACTION 1.5
        impedance = Impedance(angle, ankle_velocity, 0.7, 0.01, 5);  // theta_e = 10, k = 0.9, b = 0.01
        impedance = ((impedance >= 6)&&(impedance <= 8.5))? (8.5): (impedance);


        //printf("\ntau:%f\n",impedance);
        //desired_current = impedance/210/0.9*1000/37;
        desired_current = DesiredCurrent (impedance,angle);

        //percent = PIDCurrent(desired_current, current, 0.0001, percent); // calculate the valve_command from the PID controller
        percent = desired_current/peak_current;
        //printf("\np:%f\n",-percent);
        //limit the duty cycle to 25% range, so the current will in the range of 0 to 25%*20A = 0 to 5A
        // in practical limit to 20% percent

        if (percent >= duty_cycle_max)
            percent = duty_cycle_max;
        else if(percent <= - duty_cycle_max)
            percent = - duty_cycle_max;
        //printf("\np:%f\n",-percent);

        //Rate Limiter
        percent_new = RateLimiter(percent_old,percent);
        percent_old = percent_new;

        if (flag_begin_SW == true)
        {

            flag_begin_SW = false;
            DF (0.2);
            int16_t p = 10000;  // 10000
            while (p--);
            break;
        }

        if (desired_current < 0)    //output the pwm command
            PF (-percent_new);
        else if (desired_current >0)
            DF (percent_new);
        else
            Stop();
        break;

      //state 4
      case ST_LATE_SWING:

          //TRISBbits.TRISB15 = 1;
        // Timedelay = 1.65 sec
        //if  ((ankle_velocity <= -10)||(ankle_velocity >= 10))// delay 1.65 sec  (770 for 1 sec)
        //{
       //     state = ST_EARLY_STANCE;
      //      break;
      //  }
          if (fabsf(ac_x)>= 32500)  // 32767 is 2g
          {
              state = ST_EARLY_STANCE;
              flag_begin_ES = true;
              break;
          }

        //CONTROL ACTION
        impedance = Impedance(angle, 0.0, 0, 0.01, 2.0);    // zero stiffness
        //printf("\ntau:%f\n",impedance);
        //desired_current = impedance/210/0.9*1000/37;
        desired_current = DesiredCurrent (impedance,angle);

        //percent = PIDCurrent(desired_current, current, 0.0001, percent); // calculate the valve_command from the PID controller
        percent = desired_current/peak_current;
        //printf("\np:%f\n",-percent);
        //limit the duty cycle to 25% range, so the current will in the range of 0 to 25%*20A = 0 to 5A
        // in practical limit to 20% percent

        if (percent >= duty_cycle_max)
            percent = duty_cycle_max;
        else if(percent <= - duty_cycle_max)
            percent = - duty_cycle_max;
        //printf("\np:%f\n",-percent);

        //Rate Limiter
        percent_new = RateLimiter(percent_old,percent);
        percent_old = percent_new;

        if (desired_current < 0)    //output the pwm command
            PF (-percent_new*current_factor);
        else if (desired_current >0)
            DF (percent_new*current_factor);

        break;
    }

    my_st_impedance.st = state;
    my_st_impedance.impedance = impedance;
    my_st_impedance.percent_new = percent_new;
    return my_st_impedance;
}

