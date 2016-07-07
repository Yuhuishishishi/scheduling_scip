#include <stdio.h>
#include <stdlib.h>

#include "data_structure.h"
#include <assert.h>s

int main(int argc, char *argv[])
{
	const char* jsonpath = "/home/yuhui/Documents/TP3S_bp/data/157.tp3s";
	TEST *testArr;
	VEHICLE *vehicleArr;
	int **rule;

	size_t num_test = get_tests_size(jsonpath);
	size_t num_vehicle = get_vehicle_size(jsonpath);

	printf("num test: %d, num vehicle: %d\n", num_test, num_vehicle);

	// initialize the test arr
	testArr = (TEST*) malloc(num_test * sizeof(TEST));

	read_in_tests(jsonpath, testArr);

	for (int i = 0; i < num_test; ++i)
	{
		printf("%d, %d, %d, %d\n", testArr[i].tid, testArr[i].release, testArr[i].dur, testArr[i].deadline);
	}

	vehicleArr = (VEHICLE*) malloc(num_vehicle * sizeof(VEHICLE));

	read_in_vehicles(jsonpath, vehicleArr);

	for (int i=0; i < num_vehicle; ++i)
	{
		printf("%d, %d\n", vehicleArr[i].vid, vehicleArr[i].release);
	}

	rule = malloc(sizeof(int*) * num_test);
	for (int i=0; i < num_test; i++)
	{
		rule[i] = malloc(sizeof(int) * num_test);
	}


	read_in_rehit_rules(jsonpath, rule);

	for (int i=0; i < num_test; ++i)
	{
		for (int j=0; j<num_test; ++j)
		{
			printf("%d, vs %d, %d\n", i,j,rule[i][j]);
			assert(rule[i][j]==0 || rule[i][j]==1);
		}
	}


	free(testArr);
	free(vehicleArr);

	for (int i=0; i < num_test; ++i)
	{
		free(rule[i]);
	}
	free(rule);

	return 0;
}