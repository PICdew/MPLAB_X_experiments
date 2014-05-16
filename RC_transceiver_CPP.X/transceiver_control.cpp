extern "C"
{
#include <peripheral/timer.h>    // for opening and configuring the timer
#include <peripheral/ports.h>    // for initializing pin IO configurations
}

#include "transceiver_control.h"
#include "my_sys_config.h"


// these are global instead of members so that the interrupts and the function
// methods can both access them (the interrupt, not being a member of the
// class, cannot access private members)
static unsigned int g_receiver_port_state;
static int g_read_timer_trigger_counter;

typedef struct receiver_pin_data
{
   unsigned int high_count;
   unsigned int pin_number;
   float fractional_count_on;
} RECEIVER_PIN_DATA;
static RECEIVER_PIN_DATA g_receiver_pin_data_arr[NUM_RECEIVER_PINS];

typedef struct servo_pin_data
{
   unsigned int high_count;
   unsigned int pin_number;
} SERVO_PIN_DATA;
static SERVO_PIN_DATA g_servo_pin_data_arr[NUM_SERVO_PINS];


// timer 2 reads from the RC receiver and is responsible for synchronizing
// with the start of the PWM cycle emitted by the RC receiver
extern "C" void __ISR(_TIMER_2_VECTOR, IPL7AUTO) Timer2Handler(void)
{
   unsigned int pin_state = 0;

   g_read_timer_trigger_counter += 1;
   g_receiver_port_state = PORTRead(IOPORT_E);

   for (int index = 0; index < NUM_RECEIVER_PINS; index += 1)
   {
      pin_state = g_receiver_port_state & g_receiver_pin_data_arr[index].pin_number;
      if (pin_state != 0)
      {
         g_receiver_pin_data_arr[index].high_count += 1;
      }
   }

   // clear the interrupt flag
   mT2ClearIntFlag();
}


// timer 3 sends the processed signals out the servo pins to control things
extern "C" void __ISR(_TIMER_3_VECTOR, IPL7AUTO) Timer3Handler(void)
{
   // clear the interrupt flag
   mT3ClearIntFlag();
}


// the private constructor
transceiver_control::transceiver_control()
{
   m_has_been_initialized = false;

   g_receiver_port_state = 0;
   g_read_timer_trigger_counter = 0;

   // give the pin data structures their first values
   // Note: I use the "this" pointer to explicitly show that the reset function
   // is part of this class;
   this->reset_pin_variables();

   // assign the pin numbers manually because they are #defines and not easily
   // organizable into a loop
   // Note: I could takeadvantage of the fact that my pin numbers are
   // conveniently numbered 0 through 3, which correspond, in the bit
   // definitions, to a 1 left-shifted 0 to 3 times, but that is dependency
   // that I do not want to build into my program, so I'll just do it manually
   // to be clear about what I'm doing.
   g_receiver_pin_data_arr[0].pin_number = RECEIVER_PIN_1;
   g_receiver_pin_data_arr[1].pin_number = RECEIVER_PIN_2;
   g_receiver_pin_data_arr[2].pin_number = RECEIVER_PIN_3;
   g_receiver_pin_data_arr[3].pin_number = RECEIVER_PIN_4;

   g_servo_pin_data_arr[0].pin_number = SERVO_1_PIN;
}

transceiver_control& transceiver_control::get_instance()
{
   static transceiver_control ref;
   
   return ref;
}

void transceiver_control::init()
{
   // only go through initialization if it has not been done before
   if (!m_has_been_initialized)
   {
      // if we are in this non-static function, then there must be an instance
      // of this class, so the constructor must have been called, which means
      // that we can conveniently use the pin settings that were assigned in
      // the constructor

      for (int index = 0; index < NUM_RECEIVER_PINS; index += 1)
      {
         PORTSetPinsDigitalIn(RECEIVER_IOPORT, g_receiver_pin_data_arr[index].pin_number);
      }

      for (int index = 0; index < NUM_RECEIVER_PINS; index += 1)
      {
         PORTSetPinsDigitalOut(RECEIVER_IOPORT, g_servo_pin_data_arr[index].pin_number);
      }
      
      // start the timer, but do not turn on the interrupt
      OpenTimer2(T2_OPEN_CONFIG, T2_TICK_PR);

      m_has_been_initialized = true;
   }
}

int transceiver_control::reset_pin_variables()
{
   int this_ret_val = 0;

   if (!m_has_been_initialized)
   {
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      for (int index = 0; index < NUM_RECEIVER_PINS; index += 1)
      {
         g_receiver_pin_data_arr[index].high_count = 0;
         g_receiver_pin_data_arr[index].fractional_count_on = 0.0;
      }

      for (int index = 0; index < NUM_SERVO_PINS; index += 1)
      {
         g_servo_pin_data_arr[index].high_count = 0;
      }
   }

   return this_ret_val;
}


/// @brief
///   The PWM signal will be high for the first 1-2ms of the cycle, then low
///   until the 20ms mark.  We can enter this function and start reading the
///   states when we are in the high area, the low area, or on the border from
///   high to low or low to high.  We are looking for the transition from low
///   to high.
///   The first reading could be a low or a high.  The end condition of this
///   function's while(...) loop is that the current reading is high and the
///   previous reading was a low.  Therefore, the previous reading should not
///   start low so that the loop's end condition will not risk being triggered
///   on the first reading.
///   If the previous reading starts high though, then we will miss the
///   beginnging of the next 20ms PWM cycle if we start reading just as the
///   first millisecond interval started.  This is an acceptable miss.  It is
///   more important that we start our readings precisely at the beginning of a
///   20ms cycle.
int transceiver_control::do_not_return_until_start_of_20_ms_pwm_cycle(void)
{
   int this_ret_val = 0;

   // start the values as "not low" to avoid triggering the while(...) loop's
   // end condition prematurely
   unsigned int prev_some_pin_state = 1;
   unsigned int curr_some_pin_state = 1;

   if (!m_has_been_initialized)
   {
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      // start the timer interrupt that will read from the pins where the RC
      // receiver is connected to the board, and set it with the
      // Note: The PWM signal standard for servos and ESC states that the
      // signal will be high for the first millisecond in a 20ms cycle, then on
      // for a variable amount of time in the second millisecond, then low for
      // the remaining time in the cycle.  Rinse and repeat.  Therefore, to
      // synchronize, wait until the signal changes from low to high, which
      // indicates the start of the 20ms cycle
      ConfigIntTimer2(T2_INT_CONFIG_ON);

      while(true)
      {
         curr_some_pin_state = g_receiver_port_state & RECEIVER_PIN_1;

         if ((curr_some_pin_state != 0) && (0 == prev_some_pin_state))
         {
            break;
         }

         prev_some_pin_state = curr_some_pin_state;
      }
      
      // done with the interrupt, so turn the timer interrupt off to conserve
      // CPU cycles (it is a very demanding interrupt)
      ConfigIntTimer2(T2_INT_CONFIG_OFF);
   }

   return this_ret_val;
}

int transceiver_control::read_receiver_for_2_ms(bool wait_for_next_cycle)
{
   int this_ret_val = 0;
   unsigned int max_read_timer_trigger_counts = 0;

   if (!m_has_been_initialized)
   {
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      // reset the pin "on" counters
      this->reset_pin_variables();

      // don't perform this (possible) restart until just before enabling the
      // reading timer's interrupt so that the reading happens as close to the
      // synchronizing point as possible
      if (wait_for_next_cycle)
      {
         this->do_not_return_until_start_of_20_ms_pwm_cycle();
      }

      // enable the read timer's interrupt
      ConfigIntTimer2(T2_INT_CONFIG_ON);

      // let it read for 2 milliseconds
      g_read_timer_trigger_counter = 0;
      max_read_timer_trigger_counts = (TRANSCEIVER_TIMERS_TRIGGERS_PER_SEC / 1000) * 2;
      while (g_read_timer_trigger_counter < max_read_timer_trigger_counts);

      // done with the interrupt, so turn the timer interrupt off
      ConfigIntTimer2(T2_INT_CONFIG_OFF);
   }

   return this_ret_val;
}

int transceiver_control::process_receiver_readings()
{
   int this_ret_val = 0;
   unsigned int max_pin_high_count = 0;
   unsigned int min_pin_high_count = 0;
   unsigned int pin_high_count = 0;
   unsigned int hypothetical_pin_high_count_in_20_ms = 0;

   if (!m_has_been_initialized)
   {
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      // trim the reading so that they are in a valid range
      // Note: The standard says that the signal must be high from 1ms to 2ms.
      // All variable behavior is between these two ranges.  Depending on the
      // resolution of our read timer, we will have to trim it differently.
      // For example, if we read once per microsecond, then the receiver pins'
      // "on" counters would range from 1000 to 2000, but if we were only able
      // to read once per millisecond, then the counters would range from 1 to
      // 2.  Anything slower than that is a decimal less than one, which is not
      // acceptable.  We must read at least once in the 1ms interval when the
      // pin is guaranteed to be high at at least once in the 1ms interval of
      // variable behavior.

      max_pin_high_count = (TRANSCEIVER_TIMERS_TRIGGERS_PER_SEC / 1000) * 2;
      min_pin_high_count = (TRANSCEIVER_TIMERS_TRIGGERS_PER_SEC / 1000);
      hypothetical_pin_high_count_in_20_ms = min_pin_high_count * 20;

      // trim counters that counted higher than the standard range
      for (int index = 0; index < NUM_RECEIVER_PINS; index += 1)
      {
         // pluck out the high count from the pin to make typeing easier
         pin_high_count = g_receiver_pin_data_arr[index].high_count;

         if (pin_high_count > max_pin_high_count)
         {
            pin_high_count = max_pin_high_count;
         }
         else if (pin_high_count < min_pin_high_count)
         {
            pin_high_count = min_pin_high_count;
         }

         // stick the (possibly) modified value back in
         g_receiver_pin_data_arr[index].high_count = pin_high_count;

         // calculate the fraction of the 20ms cycle that the pin was high
         g_receiver_pin_data_arr[index].fractional_count_on = (float)pin_high_count / hypothetical_pin_high_count_in_20_ms;
      }
      
      // ??additional processing here??
      g_servo_pin_data_arr[0].high_count = g_receiver_pin_data_arr[0].high_count;
   }

   return this_ret_val;
}

int transceiver_control::send_processed_signals_to_servos_and_motors()
{
   int this_ret_val = 0;

   if (!m_has_been_initialized)
   {
      this_ret_val = -1;
   }

   if (0 == this_ret_val)
   {
      // turn on timer 3 to start transmitting the signals
   }

   return this_ret_val;
}

int transceiver_control::get_avg_high_counts_for_200ms(
   unsigned int *pin_1_count_ptr,
   unsigned int *pin_2_count_ptr,
   unsigned int *pin_3_count_ptr,
   unsigned int *pin_4_count_ptr)
{
   int this_ret_val = 0;
   unsigned int max_read_timer_trigger_counts = 0;

   if (!m_has_been_initialized)
   {
      this_ret_val = -1;
   }
   else if(0 == pin_1_count_ptr)
   {
      this_ret_val = -2;
   }
   else if(0 == pin_2_count_ptr)
   {
      this_ret_val = -3;
   }
   else if(0 == pin_3_count_ptr)
   {
      this_ret_val = -4;
   }
   else if(0 == pin_4_count_ptr)
   {
      this_ret_val = -5;
   }

   if (0 == this_ret_val)
   {
      // reset the pin "on" counters
      this->reset_pin_variables();

      // wait for synchronization with the next 20ms cycle
      //this->do_not_return_until_start_of_20_ms_pwm_cycle();

      // enable the read timer's interrupt
      ConfigIntTimer2(T2_INT_CONFIG_ON);

      // let it read for 200 milliseconds
      g_read_timer_trigger_counter = 0;
      max_read_timer_trigger_counts = (TRANSCEIVER_TIMERS_TRIGGERS_PER_SEC / 1000) * 200;
      while (g_read_timer_trigger_counter < max_read_timer_trigger_counts);

      // done with the interrupt, so turn the timer interrupt off
      ConfigIntTimer2(T2_INT_CONFIG_OFF);

      // the average high count will be the high count divided by the number of
      // 20ms cycles
      // Note: This function is for 200ms, so we can hard code 10x 20ms cycles.
      *pin_1_count_ptr = g_receiver_pin_data_arr[0].high_count / 10;
      *pin_2_count_ptr = g_receiver_pin_data_arr[1].high_count / 10;
      *pin_3_count_ptr = g_receiver_pin_data_arr[2].high_count / 10;
      *pin_4_count_ptr = g_receiver_pin_data_arr[3].high_count / 10;
   }

   return this_ret_val;
}

