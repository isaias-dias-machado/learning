#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdbool.h>

#define SIZE 5000

void swap(int *p1, int *p2) {
	int tmp = *p1;
	*p1 = *p2;
	*p2 = tmp;
}

void quick_r(int *v, int l, int r) {
	if (l >= r)
		return;

	int pivot_value = v[l];
	int i = l;
	for (int j = l+1; j <= r; j++) {
		if (v[j] < pivot_value) {
			i++;
			swap(&v[i], &v[j]);
		}
	}		
	swap(&v[l], &v[i]);

	//Recur around the pivot, excluding it.
	quick_r(v, l, i-1);
	quick_r(v, i+1, r);
}

void quick(int *v, size_t len) {
	quick_r(v, 0, len-1);
}

int main(void) {
	int v[SIZE];
	srand(time(NULL));
	for (int i = 0; i < SIZE; i++) {
		v[i] = rand() % 100;
	}

	quick(v, SIZE);

	for (int i = 0; i < SIZE-1; i++) {
		if (v[i] > v[i+1])
			assert(false);
		printf("%d ", v[i]); 
	}
	printf("%d ", v[SIZE-1]); 
}
