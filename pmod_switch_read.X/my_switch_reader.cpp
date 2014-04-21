// for linking these definitions with the function declarations
#include "my_switch_reader.h"

my_switch_reader::my_switch_reader()
{
   // set IO ports and pins
}

my_switch_reader my_switch_reader::get_instance()
{
   static my_switch_reader MSR;

   return MSR;
}

void my_switch_reader::do_the_switch_thing()
{
   
}
