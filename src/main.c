#include <stdio.h>
#include <stdlib.h>

#include "data_structure.h"

int main(int argc, char *argv[])
{
	const char* jsonpath = "/home/yuhui/Documents/TP3S_bp/data/156.tp3s";

	size_t num_test = get_tests_size(jsonpath);
	size_t num_vehicle = get_vehicle_size(jsonpath);

	printf("num test: %d, num vehicle: %d\n", num_test, num_vehicle);
	return 0;
}