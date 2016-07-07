#ifndef JSON_READ_H
#define JSON_READ_H

#include "data_structure.h"


extern size_t 
get_tests_size(const char* path);

extern size_t
get_vehicle_size(const char* path);


extern void 
read_in_tests(const char* path, TEST *testArr);

extern void 
read_in_vehicles(const char* path, VEHICLE *vehicleArr);

extern void
read_in_rehit_rules(const char* path, int **rule);

#endif