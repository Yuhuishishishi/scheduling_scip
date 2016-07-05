#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

struct test_request
{
	unsigned int dur;
	unsigned int release;
	unsigned int deadline;
	unsigned int tid;
};


struct vehicle
{
	unsigned int release;
	unsigned int vid;
};

typedef struct test_request TEST;
typedef struct vehicle VEHICLE;



#endif