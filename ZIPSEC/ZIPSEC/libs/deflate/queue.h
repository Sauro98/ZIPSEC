#ifndef QUEUE_H
#define QUEUE_H

#include "constants.h"
/*
 * No description
 */
class Queue
{
	public:
		// class constructor
		Queue(int n);
		// class destructor
		~Queue();
		//adds an element
		void enqueue(node_f* new_node);
		//removes an element
		node_f* dequeue();
		//return an element without removing it
		node_f* peek();
		//returns the current array length
		int get_length();
		
		void print_queue();
	private:
        int length;
        int max_length;
        node_f** array;
};

#endif // QUEUE_H
