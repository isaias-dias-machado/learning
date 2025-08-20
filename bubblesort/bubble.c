#include <stdio.h>
#include <stdlib.h>
#define SIZE 50

/* The largest element bubbles to the top of the list */
int main(void) {
	int v[SIZE];
	for (int i = 0; i < SIZE; i++) {
		v[i] = rand() % 200;
	}

	for (int i = SIZE - 1; i >= 0; i--) {
		int swapped = 0;
		for (int j = 0; j < i; j++) {
			if (v[j] > v[j+1]) {
				int tmp = v[j];
				v[j] = v[j+1];
				v[j+1] = tmp;
				swapped = 1;
			}
		}
		if (swapped == 0)
			break;
	}

	for (int i = 0; i < SIZE; i++) {
		printf("%d ", v[i]);
	}
}
