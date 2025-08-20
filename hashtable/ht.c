#include <stdio.h>
#include <stdlib.h>

#define SIZE 1001

int hash(char* key) {
	int value = 59;
	for (char* p = key; *p != '\0'; p++) {
		value *= *p;
	}
	return (value * 11) % SIZE;
} 

void insert(int *ht, char *key, int value) {
	ht[hash(key)] = value;
}

int get(int *ht, char *key) {
	return ht[hash(key)];
}

int main(void) {
	int ht[SIZE];
	insert(ht, "five", 5);
	insert(ht, "five", 10);
	printf("fetched value: %d\n", get(ht, "five"));
	return 0;
}
