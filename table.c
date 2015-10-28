//Author: Nathan Holt, nxh7119
#include "table.h"
#include "hash.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#ifndef UNINSTRUMENTED
#include "amigomem.h"
#endif

Table* create(long (*hash)(void* key), bool (*equals)(void* key1, void* key2), void (*print)(void* key1, void* value)) {
	//Dynamically allocate a table.
	Table *newTable = (Table*) malloc(sizeof(Table));
	
	if (newTable == NULL) {
		//Memory allocation failed.
		fprintf(stderr, "table::create() failed to allocate space");
		assert(NULL);
	}
	
	//Next, let's allocate space for the entries.
	newTable->table = (Entry**) calloc(INITIAL_CAPACITY, sizeof(Entry));
	
	if (newTable->table == NULL) {
		//Memory allocation failed.
		fprintf(stderr, "table::create() failed to allocate space");
		assert(NULL);
	}
	
	//Set the initial size and capacity of the hashtable.
	newTable->size = 0;
	newTable->capacity = INITIAL_CAPACITY;
	
	//Set the initial number of rehashes and collisions to zero.
	newTable->rehashes = 0;
	newTable->collisions = 0;
	
	//Pass the pointer parameters.
	newTable->hash = hash;
	newTable->equals = equals;
	newTable->print = print;
	
	//Use provided hash, equals, and print functions from hash.c
	newTable->hash = hash;
	newTable->equals = equals;
	newTable->print = print;
	
	//Return the hashtable.
	return newTable;
}

void destroy(Table *t) {
	//Loop through all possible entry locations and free the memory.
	//int index = 0;
	for (unsigned int index = 0; index < t->capacity; index++) {
		if (t->table[index] != NULL) {
			free(t->table[index]);
		}
	}
	
	//Lastly, free the table itself.
	free(t->table);
	free(t);
}

void* get(Table* t, void* key) {
	unsigned long hashval = t->hash(key) % t->capacity;
	unsigned long currentIndex = hashval;
	int iteration = 1;
	
	//Keep going as long as you haven't checked every entry.
	while ((hashval != currentIndex) || (iteration == 1)) {
		//First, check if the place isn't null.
		if (t->table[currentIndex] != NULL) {
			//Now that we know it's not NULL, we can check if it is the key we want.
			if (t->equals(t->table[currentIndex]->key, key)) {
				return t->table[currentIndex]->value;
			} else {
				//It's not the key we want. Hence, increment collisions.
				t->collisions += 1;
			}
		} else {
			//If it runs into a null one, we know it's not in the hashtable, since we use open addressing with shifting one to the right.
			fprintf(stderr, "table::get() couldn't find value associated with key");
			assert(NULL);
		}
	
		currentIndex = (currentIndex + 1) % t->capacity;
		iteration += 1;
	}
	
	//Couldn't find the key in the table, and the hashtable is full.
	fprintf(stderr, "table::get() couldn't find value associated with key");
	assert(NULL);
	
	//If the entry at this location isn't null, and the key matches the key you wanted, return the value
	//NOTE: I DID NOT ACCOUNT FOR COLLISIONS. NATHAN, CHANGE THIS LATER TO ACCOUNT FOR COLLISIONS.
	/*if (t->table[hashval] != NULL && t->equals(t->table[hashval]->key, key)) {
		return t->table[hashval]->value;
	} else {
		unsigned long currentIndex = (hashval + 1) % t->capacity;
		while (!(t->equals(t->table[currentIndex]->key, key)) && currentIndex != hashval) {
			//Someone is already there! go to the next index.
			currentIndex = (currentIndex + 1) % t->capacity;
		}
		
		if (currentIndex != hashval) {
			//Yay! Found the key somewhere.
			return t->table[currentIndex]->value;
		} else {
			//Couldn't find the key in the table.
			fprintf(stderr, "table::get() couldn't find value associated with key");
			assert(NULL);
		}
	}
	return NULL;*/
}

//This function rehashes the current values of t->table into newTable and returns newTable.
Entry** rehashTable(Table* t, Entry** newTable) {
	//Loop through the values of t->table and hash them into the newTable.
	for (unsigned int index = 0; index < (t->capacity/RESIZE_FACTOR); index++) {
		if (t->table[index] != NULL) {
			//Generate hash for the key.
			unsigned long hashval = t->hash(t->table[index]->key) % t->capacity;
			
			//Check if the spot on the new table is available.
			if (newTable[hashval] == NULL) {
				//There's no entry there already. Create entry and put it there.
				
				Entry *newEntry = (Entry*) malloc(sizeof(Entry));
				newEntry->key = t->table[index]->key;
				newEntry->value = t->table[index]->value;
				
				newTable[hashval] = newEntry;
			} else {
				//There's an entry there already. Keep moving it over until in the right place.
				hashval = (hashval + 1) % t->capacity;
				while (newTable[hashval] != NULL) {
					//Someone is already there! go to the next index.
					hashval = (hashval + 1) % t->capacity;
					//t->collisions += 1;
				}
				
				//Found an open spot. Add the new entry.
				Entry *newEntry = (Entry*) malloc(sizeof(Entry));
				newEntry->key = t->table[index]->key;
				newEntry->value = t->table[index]->value;
				
				newTable[hashval] = newEntry;
			}
		}
	}

	return newTable;
}


void* put(Table* t, void* key, void* value) {
	unsigned long hashval = t->hash(key) % t->capacity;
	bool updatedTable = false;
	
	if (t->table[hashval] == NULL) {
		//There's no entry there already. Create entry and put it there.
		
		Entry *newEntry = (Entry*) malloc(sizeof(Entry));
		newEntry->key = key;
		newEntry->value = value;
		
		t->table[hashval] = newEntry;
		
		t->size += 1;
		
		updatedTable = true;
	} else {
		//Already an entry there.
		if (t->equals(t->table[hashval]->key, key)) {
			//If this place isn't free, but it the correct key, update the value.
			void *oldValue = t->table[hashval]->value;
			t->table[hashval]->value = value;
			return oldValue;
		}
			t->collisions += 1;
			
			hashval = (hashval + 1) % t->capacity;
			while (t->table[hashval] != NULL) {
				//Someone is already there! go to the next index.
				if (t->equals(t->table[hashval]->key, key)) {
					//If this place isn't free, but it the correct key, update the value.
					void *oldValue = t->table[hashval]->value;
					t->table[hashval]->value = value;
					return oldValue;
				}
				hashval = (hashval + 1) % t->capacity;
				t->collisions += 1;
			}
			
			//Found an open spot. Add the new entry.
			Entry *newEntry = (Entry*) malloc(sizeof(Entry));
			newEntry->key = key;
			newEntry->value = value;
			
			t->table[hashval] = newEntry;
			
			t->size += 1;
			
			updatedTable = true;
	}
	
	if (updatedTable) {
		//We added an entry. Hence, check and see if we need to rehash now.
		if ((t->size / (float)t->capacity) >= LOAD_THRESHOLD) {
			//First, allocate space for the newTable.
			Entry **newTable = (Entry**) calloc(RESIZE_FACTOR * t->capacity, sizeof(Entry));
			
			//Update the capacity of the table and number of rehashes.
			t->capacity = RESIZE_FACTOR * t->capacity;
			t->rehashes += 1;
			
			//Rehash the values into newTable.
			newTable = rehashTable(t, newTable);
			
			//Free t->table and entries.
			for (unsigned int index = 0; index < (t->capacity/RESIZE_FACTOR); index++) {
				if (t->table[index] != NULL)
					free(t->table[index]);
			}
			
			free(t->table);
			
			//Set t->table equal to the new table we created.
			t->table = newTable;
		}
	}
	return NULL;
}

bool has(Table* t, void* key) {
	unsigned long hashval = t->hash(key) % t->capacity;
	unsigned long currentIndex = hashval;
	int iteration = 1;
	
	//As long as we haven't check all of them, keep looping:
	while ((hashval != currentIndex) || (iteration == 1)) {
		//First, check if the place isn't null.
		if (t->table[currentIndex] != NULL) {
			//Now that we know it's not NULL, we can check if it is the key we want.
			if (t->equals(t->table[currentIndex]->key, key)) {
				return true;
			} else {
				//Not the key we want. So increment collisions.
				t->collisions += 1;
			}
		} else {
			//Since we use open addressing, we know it would only be shifted to the right, hence: return false
			return false;
		}
	
		currentIndex = (currentIndex + 1) % t->capacity;
		iteration += 1;
	}
	//Couldn't find the key by the end of the loop. Return false.
	return false;
}

void dump(Table* t, bool full) {
	printf("Size: %zu\n", t->size);
	printf("Capacity: %zu\n", t->capacity);
	printf("Collisions: %zu\n", t->collisions);
	printf("Rehashes: %zu\n", t->rehashes);
	
	if (full == true) {
		//They want a detailed report.
		//Loop through all entries and print the keys and values.
		//int index = 0;
		for (unsigned int index = 0; index < t->capacity; index++) {
			if (t->table[index] != NULL) {
				//There's an entry at this location. Print it out.
				printf("%d: (", index);
				t->print(t->table[index]->key, t->table[index]->value);
				printf(")\n");
			} else {
				printf("%d: null\n", index);
			}
		}
	}
}

void** keys(Table* t) {
	void **keyList = (void**) malloc(sizeof(void*) * t->capacity);
	
	int currentIndexInList = 0;
	
	//Loop through the table's key's and add them to the list.
	for (unsigned int currentIndexInTable = 0; currentIndexInTable < t->capacity; currentIndexInTable++) {
		if (t->table[currentIndexInTable] != NULL) {
			keyList[currentIndexInList] = t->table[currentIndexInTable]->key;
			currentIndexInList += 1;
		}
	}
	
	return keyList;
}
void** values(Table* t) {
	void **valueList = (void**) malloc(sizeof(void*) * t->capacity);
	
	int currentIndexInList = 0;
	
	//Loop through the table's key's and add them to the list.
	for (unsigned int currentIndexInTable = 0; currentIndexInTable < t->capacity; currentIndexInTable++) {
		if (t->table[currentIndexInTable] != NULL) {
			valueList[currentIndexInList] = t->table[currentIndexInTable]->value;
			currentIndexInList += 1;
		}
	}
	
	return valueList;
}
