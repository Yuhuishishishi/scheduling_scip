#include <jansson.h>
#include "json_read.h"

size_t get_tests_size(const char* path)
{
	json_t *root, *tests_json;
	json_error_t err;
	size_t result;

	root = json_load_file(path,0, &err);

	tests_json = json_object_get(root, "tests");

	result = json_array_size(tests_json);

	json_decref(root);
	json_decref(tests_json);

	return result;

}

size_t get_vehicle_size(const char* path)
{
	json_t *root, *vehicles_json;
	json_error_t err;
	size_t result;

	root = json_load_file(path,0, &err);

	vehicles_json = json_object_get(root, "vehicles");

	result = json_array_size(vehicles_json);

	json_decref(root);
	json_decref(vehicles_json);

	return result;
}


void read_in_tests(const char* path, TEST testArr[])
{


}

void read_in_vehicles(const char* path, VEHICLE vehicleArr[])
{


}