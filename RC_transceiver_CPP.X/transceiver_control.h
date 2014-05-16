/* 
 * File:   transceiver_control.h
 * Author: John
 *
 * Created on May 16, 2014, 8:14 AM
 */

#ifndef TRANSCEIVER_CONTROL_H
#define	TRANSCEIVER_CONTROL_H

class transceiver_control
{
public:
   static transceiver_control& get_instance();

   void init();
   int do_not_return_until_start_of_20_ms_pwm_cycle();

   int read_receiver_for_2_ms(bool wait_for_next_cycle);
   int process_receiver_readings();
   int send_processed_signals_to_servos_and_motors();

   int get_avg_high_counts_for_200ms(
      unsigned int *pin_1_count_ptr,
      unsigned int *pin_2_count_ptr,
      unsigned int *pin_3_count_ptr,
      unsigned int *pin_4_count_ptr);

private:
   transceiver_control();

   int reset_pin_variables();

   bool m_has_been_initialized;
};

#endif	/* TRANSCEIVER_CONTROL_H */

