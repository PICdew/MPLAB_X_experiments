/* 
 * File:   WOOP_WOOP_WOOP.h
 * Author: John
 *
 * Created on April 21, 2014, 7:25 PM
 */

#ifndef WOOP_WOOP_WOOP_H
#define	WOOP_WOOP_WOOP_H


// this is an alarm-like function that should be used whenever something is
// very much not ok and I want the user to take notice
// Note: This code runs on an embedded system, so I can't simply exit the
// program.  That hides the error.  I want the error to be very obvious.
void WOOP_WOOP_WOOP(void);

#endif	/* WOOP_WOOP_WOOP_H */

