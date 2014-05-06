#include "my_function_queue.h"

#define MAX_FUNCTION_NODES 10
typedef struct function_node
{
   void (*function_ptr)(void);
   struct function_node *next_ptr;
} Node;

static Node g_function_queue[MAX_FUNCTION_NODES];
static Node *g_curr_node_ptr;

void function_queue_init(void)
{
   int count = 0;

   // make a circularly linked list
   for (count = 0; count < MAX_FUNCTION_NODES - 1; count += 1)
   {
      g_function_queue[count].function_ptr = 0;
      g_function_queue[count].next_ptr = &(g_function_queue[count + 1]);
   }

   // handle the last one in the queue to make the list circularly linked
   g_function_queue[MAX_FUNCTION_NODES - 1].function_ptr = 0;
   g_function_queue[MAX_FUNCTION_NODES - 1].next_ptr = &(g_function_queue[0]);

   // assign the pointer to the first function node
   g_curr_node_ptr = &(g_function_queue[0]);
}

void execute_next_function_in_queue(void)
{
   if (0 != g_curr_node_ptr->function_ptr)
   {
      // execute the function, then erase the pointer to empty it out so that
      // other functions can be queued up
      g_curr_node_ptr->function_ptr();
      g_curr_node_ptr->function_ptr = 0;

      // jump to the next item in the queue
      g_curr_node_ptr = g_curr_node_ptr->next_ptr;
   }
   else
   {
      // no function assigned, so queue must be empty and we will do nothing
   }
}

int add_function_to_queue(void *function_ptr)
{
   int count = 0;
   Node *node_ptr = g_curr_node_ptr;

   // search for the next empty function node, starting at the current one, and
   // add the new function there, if possible
   for (count = 0; count < MAX_FUNCTION_NODES; count += 1)
   {
      if (0 == node_ptr->function_ptr)
      {
         // node is empty, so add the function here
         node_ptr->function_ptr = function_ptr;
         break;
      }
   }

   if (count == MAX_FUNCTION_NODES)
   {
      // no empty node could be found, so return error
      return -1;
   }
   else
   {
      // a node was found and assigned, so all went well
      return 0;
   }
}


