#include <jansson.h>
#include "json_read.h"
#include <assert.h>
#include <stdlib.h>


// function prototypes
static int get_max_test_id(const char* path);
static void test_id_remap(const char* path, int *id_remap);

int get_tests_size(const char* path)
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

int get_vehicle_size(const char* path)
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

void read_in_tests(const char* path, TEST *testArr)
{
	json_t *root, *tests_json;
	json_error_t err;
	int i;

	root = json_load_file(path, 0, &err);
	tests_json = json_object_get(root, "tests");

	for (i=0; i < json_array_size(tests_json); ++i)
	{
		int tid, dur, release, deadline;
		json_t *test;

		test = json_array_get(tests_json, i);
		tid = json_integer_value(json_object_get(test, "test_id"));
		dur = json_integer_value(json_object_get(test, "dur"));
		release = json_integer_value(json_object_get(test, "release"));
		deadline = json_integer_value(json_object_get(test, "deadline"));

		// create the new struct
		testArr[i].tid = i;
		testArr[i].dur = dur;
		testArr[i].release = release;
		testArr[i].deadline = deadline;

		json_decref(test);

	}

	json_decref(tests_json);
	json_decref(root);
}

void read_in_vehicles(const char* path, VEHICLE *vehicleArr)
{
	json_t *root, *vehicle_json;
	json_error_t err;

	int i;

	root = json_load_file(path, 0, &err);
	vehicle_json = json_object_get(root, "vehicles");

	for (i=0; i<json_array_size(vehicle_json); ++i)
	{
		int vid, release;
		json_t *vehicle;

		vehicle = json_array_get(vehicle_json, i);

		vid = json_integer_value(json_object_get(vehicle, "vehicle_id"));
		release = json_integer_value(json_object_get(vehicle, "release"));

		vehicleArr[i].vid = vid;
		vehicleArr[i].release = release;

		json_decref(vehicle);
	}

	json_decref(vehicle_json);
	json_decref(root);

}


void read_in_rehit_rules(const char* path, int **rule)
{
	// create the remap of tid to fake id used in this program
	json_t *root, *rules;
	json_error_t err;
	int *id_remap;

	void *iter, *inner_iter;
	int max_test_id = get_max_test_id(path);
	int num_test = get_tests_size(path);

	// create a very large array
	id_remap = (int*) malloc(max_test_id * sizeof(int));
	// initialize all value to -1
	for (int i=0; i < max_test_id; ++i)
	{
		id_remap[i] = -1;
	}

	// fill in the remap
	test_id_remap(path, id_remap);

	// read the rules
	root = json_load_file(path, 0, &err);
	rules = json_object_get(root, "rehit");

	iter = json_object_iter(rules);



	while (iter)
	{
		const char* key;
		json_t *val;
		int id1, id2, real_id1, real_id2;
		int ok;

		key = json_object_iter_key(iter);
		val = json_object_iter_value(iter);

		id1 = atoi(key);

		inner_iter = json_object_iter(val);

		while (inner_iter)
		{
			const char* id2_str; 
			json_t *allowable;

			id2_str = json_object_iter_key(inner_iter);
			id2 = atoi(id2_str);
			allowable = json_object_iter_value(inner_iter);

			if (json_is_true(allowable))
			{
				ok = 1;
			} 
			else 
			{
				ok = 0;
			}

			// write the rule
			real_id1 = id_remap[id1];
			real_id2 = id_remap[id2];

			assert(real_id1 >= 0 && real_id1 < num_test);
			assert(real_id2 >= 0 && real_id2 < num_test);


			rule[real_id1][real_id2] = ok;

			json_decref(allowable);
			inner_iter = json_object_iter_next(val, inner_iter);
		}

		json_decref(val);
		iter = json_object_iter_next(rules, iter);
	}


	json_decref(rules);
	json_decref(root);

	free(id_remap);


}

static 
int get_max_test_id(const char* path)
{
	json_t *root, *tests_json;
	json_error_t err;

	int max_test_id = -1;
	int num_test;

	root = json_load_file(path, 0, &err);
	tests_json = json_object_get(root, "tests");

	// find the max test id
	num_test = json_array_size(tests_json);
	for (int i=0; i<num_test; ++i)
	{
		json_t *test;
		int tid;
		test = json_array_get(tests_json, i);

		tid = json_integer_value(json_object_get(test, "test_id"));

		if (tid > max_test_id)
		{
			max_test_id = tid;
		}

		json_decref(test);
	}

	json_decref(tests_json);
	json_decref(root);

	return max_test_id;
}


static 
void test_id_remap(const char* path, int *id_remap)
{
	// read in test id
	json_t *root, *tests_json;
	json_error_t err;

	int i;

	root = json_load_file(path, 0, &err);
	tests_json = json_object_get(root, "tests");

	for (i=0; i<json_array_size(tests_json); ++i)
	{
		json_t *test;
		int tid;
		test = json_array_get(tests_json, i);
		tid = json_integer_value(json_object_get(test, "test_id"));

		id_remap[tid] = i;

		json_decref(test);
	}

	json_decref(tests_json);
	json_decref(root);

}