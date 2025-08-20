
/* MISTAKES:
 * not using a struct represent matrixes right away
 * not creating a mock bmp struct for testing right away
 *
 * NOTES:
 *
 * The use of free inside of transformation functions doesn't follow
 * conventional good practices and is just an experimental design.
 *
 * Missing logic for bits_per_pixel < 8
 * */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

struct __attribute__((packed)) bmp {
	uint16_t sig;
	uint32_t file_size;
	uint32_t reserved;
	uint32_t pixels_offset;

	uint32_t dib_size;
	int32_t width;
	int32_t height;
	uint16_t color_plane_num;
	uint16_t bits_per_pixel;
	uint32_t compression;
	uint32_t img_size;
	int32_t horizontal_res;
	int32_t vertical_res;
	uint32_t color_num;
	uint32_t important_color_num;
	uint8_t ignored[84];
};

struct matrix {
	int cols;
	int rows;
	int offset;
	uint8_t p[];
};

struct matrix * init_matrix(int cols, int rows, int offset) {
	struct matrix *res = calloc(1, 3*sizeof(*res) + cols*rows);
	res->cols = cols;
	res->rows = rows;
	res->offset = offset;
	return res;
}

int err_exit(char *msg) {
	perror(msg);
	exit(1);
}

uint8_t reverse_pixel_pair(uint8_t b) {
	uint8_t tmp = b << 4;
	b = b >> 4;
	return b + tmp;
}

struct matrix * reverse_pixels_pairs_matrix(struct matrix *m) {
	struct matrix *res = init_matrix(m->cols, m->rows, m->offset);
	for (int i = 0; i < m->rows; i++) {
		for (int j = 0; j < m->cols; j++) {
			res->p[i * m->cols + j] = reverse_pixel_pair(m->p[i * m->cols + j]);
		}
	}
	free(m);
	return res;
}


struct matrix * transpose_matrix(struct matrix *m) {
	struct matrix *res = init_matrix(m->rows*m->offset, m->cols/m->offset, m->offset);
	for (int i = 0; i < m->rows; i++) {
		for (int j = 0; j < m->cols; j=j+m->offset) {
			memcpy(
					&res->p[(j/m->offset) * res->cols + i*m->offset],
					&m->p[i * m->cols + j],
					m->offset
					);
		}
	}
	free(m);
	return res;
}

struct matrix * reverse_rows(struct matrix *m) {
	struct matrix *res = init_matrix(m->cols, m->rows, m->offset);
	for (int i = 0; i < m->rows; i++) {
		for (int j = 0, k = m->cols - m->offset; j <= k; j=j+m->offset, k=k-m->offset) {
			memcpy(&res->p[i * m->cols + j], &m->p[i * m->cols + k], m->offset);
			memcpy(&res->p[i * m->cols + k], &m->p[i * m->cols + j], m->offset);
		}
	}
	free(m);
	return res;
}

struct matrix * reverse_cols(struct matrix *m) {
	struct matrix *res = init_matrix(m->cols,m->rows,m->offset);
	for (int i = 0; i < m->cols; i=i+m->offset) {
		for (int j = i, k = m->cols * (m->rows - 1) + i; j <= k; j=j+m->cols, k=k-m->cols) {
			memcpy(&res->p[j], &m->p[k], m->offset);
			memcpy(&res->p[k], &m->p[j], m->offset);
		}
	}
	free(m);
	return res;
}

struct matrix * rotate_clockwise(struct matrix *m) {
	return reverse_rows(transpose_matrix(m));
}

struct matrix * rotate_counter_clockwise(struct matrix *m) {
	return reverse_cols(transpose_matrix(m));
}

uint8_t * pad_matrix(struct matrix *m, int cols_with_padding, int matrix_true_size) {
	uint8_t *res = calloc(1, matrix_true_size);
	for (int i = 0; i < m->rows; i++) {
		for (int j = 0; j < cols_with_padding; j++) {
			if (j < m->cols)
				res[i * cols_with_padding + j] = m->p[i * m->cols + j];
		}
	}
	return res;
}

struct matrix * remove_padding(uint8_t *m, struct bmp *s) {
  int bytes_per_row = ((s->width * s->bits_per_pixel + 31) / 32) * 4;
	int offset = s->bits_per_pixel/8;
	int non_padding_bytes_per_row = s->width * offset;
	offset = offset < 1 ? 1 : offset;

	struct matrix *res = init_matrix(non_padding_bytes_per_row, s->height, offset);
	
	for (int i = 0; i < s->height; i++) {
		for (int j = 0; j < bytes_per_row; j++) {
			if (j < res->cols)
				res->p[i * non_padding_bytes_per_row + j] = m[i * bytes_per_row + j];
		}
	}

	return res;
}

uint8_t * rotate_file(uint8_t *m, struct bmp *s, int flag) {
	struct matrix *res;

	int true_cols = ((s->height * s->bits_per_pixel + 31) / 32) * 4;
	int bytes_in_row = s->width * (s->bits_per_pixel / 8);
	int matrix_size = true_cols * bytes_in_row;

	res = remove_padding(m, s);

	if (flag) {
		res = rotate_counter_clockwise(res);
	} else {
		res = rotate_clockwise(res);
	}
	uint8_t *new_pixel_array = pad_matrix(res, true_cols, matrix_size);

	//Adjust the state of the header to match the flip

	s->img_size = matrix_size;

	int32_t tmp = s->width;
	s->width = s->height;
	s->height = tmp;

	tmp = s->vertical_res;
	s->vertical_res = s->horizontal_res;
	s->horizontal_res = tmp;

	return new_pixel_array;
}


#ifndef _TEST
int main(int argc, char **argv) {
	struct bmp *s = malloc(sizeof *s);
	char *filename;
	int opt, flag = 0;

	while ((opt = getopt(argc, argv, "r")) != -1) {
		switch (opt) {
			case 'r':
				flag = 1;
				break;
			default: 
				fprintf(stderr, "Usage: %s [-r] filename\n",
						argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if (argc > optind)
		filename = argv[optind];
	else
		filename = "sample1.bmp";

	char prefix[256];
	if (snprintf(prefix, 256, "./%s", filename) < 0)
		err_exit("snprintf");

	FILE *fd = fopen(prefix, "r");
	int n = fread(s, 1, sizeof *s, fd);
	if (n = 0)
		err_exit("fread s. No data read.");

	uint8_t *pixel_data = malloc(s->img_size);

	if (fseek(fd, s->pixels_offset, SEEK_SET) < 0)
		err_exit("fseek");
	if (fread(pixel_data, 1, s->img_size, fd) == 0)
		err_exit("fread pixel_data. No data read.");

	uint8_t *pixel_data_rotated = rotate_file(pixel_data, s, flag);

	FILE *fd2 = fopen("./output.bmp", "w");
	if (fwrite(s, 1, sizeof *s, fd2) == 0)
		err_exit("fwrite");
	if (fwrite(pixel_data_rotated, 1, s->img_size, fd2) == 0)
		err_exit("fwrite");

	return 0;
}

#else
#include <time.h>
#include <assert.h>
#include <stdbool.h>
void print_matrix(uint8_t *m, int cols, int rows) {
	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			printf("%02d ", m[i * cols + j]);
		}
		puts("");
	}
}

bool compare_matrix(uint8_t *result, uint8_t *expected, int cols, int rows) {
	puts("Expected:");
	print_matrix(expected, cols, rows);
	puts("==========");

	puts("Result:");
	print_matrix(result, cols, rows);
	puts("==========");

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if (result[i * cols + j] != expected[i * cols + j])
				return false;
		}
	}
	return true;
}

#define COLS 5
#define ROWS 3
int main(void) {

	puts("\nTransposition test...");
	uint8_t tmp[5*3] = {1, 2, 2, 2, 2,
		0, 1, 2, 2, 2,
		0, 0, 1, 2, 2};
	struct matrix *input = init_matrix(5,3,1);
	memcpy(input->p,	tmp, sizeof tmp);

	uint8_t expected[COLS*ROWS] = {1, 0, 0,
		2, 1, 0,
		2, 2, 1,
		2, 2, 2,
		2, 2, 2};

	struct matrix *result = transpose_matrix(input);
	assert(compare_matrix(result->p, expected, 3, 5));

	puts("\nTransposition test. Offset = 2...");
	uint8_t tmp1[6*3] = {1, 1, 2, 2, 2, 2,
		0, 0, 1, 1, 2, 2,
		0, 0, 0, 0, 1, 1};
	struct matrix *input1 = init_matrix(6,3,2);
	memcpy(input1->p, tmp1, sizeof tmp1);

	uint8_t expected1[6*3] = {1, 1, 0, 0, 0, 0,
		2, 2, 1, 1, 0, 0,
		2, 2, 2, 2, 1, 1};

	struct matrix *result1 = transpose_matrix(input1);
	assert(compare_matrix(result1->p, expected1, 6, 3));

	puts("\nReverse rows test...");
	uint8_t tmp2[5*3] = {1, 2, 3, 4, 5,
		1, 2, 3, 4, 5,	
		1, 2, 3, 4, 5,};
	struct matrix *input2 = init_matrix(5,3,1);
	memcpy(input2->p, tmp2, sizeof tmp2);

	uint8_t expected2[5*3] = {5, 4, 3, 2, 1,
		5, 4, 3, 2, 1,	
		5, 4, 3, 2, 1};

	struct matrix *result2 = reverse_rows(input2);
	assert(compare_matrix(result2->p, expected2, 5, 3));

	puts("\nReverse cols test...");
	uint8_t tmp21[5*3] = {1, 1, 1, 1, 1,
		2, 2, 2, 2, 2,	
		3, 3, 3, 3, 3,};
	struct matrix *input21 = init_matrix(5,3,1);
	memcpy(input21->p, tmp21, sizeof tmp21);

	uint8_t expected21[5*3] = {3, 3, 3, 3, 3,
		2, 2, 2, 2, 2,	
		1, 1, 1, 1, 1,};

	struct matrix *result21 = reverse_cols(input21);
	assert(compare_matrix(result21->p, expected21, 5, 3));

	puts("\nPixel pair inversion test...");
	uint8_t input4 = 0x1f;

	uint8_t expected4 =	0xf1;

	uint8_t result4 = reverse_pixel_pair(input4);
	printf("result = %x; expected = %x\n", result4, expected4);
	assert(result4 == expected4);

	puts("\nPad Matrix test...");
	uint8_t tmp_pad_test[9*3] = {2, 1, 1, 3, 1, 1, 4, 1, 1,
															2, 1, 1, 3, 1, 1, 4, 1, 1,
															2, 1, 1, 3, 1, 1, 4, 1, 1};
	struct matrix *input_pad_test = init_matrix(9,3,3);
	memcpy(input_pad_test->p, tmp_pad_test, sizeof tmp_pad_test);

	uint8_t expected_pad_test[12*3] = {2, 1, 1, 3, 1, 1, 4, 1, 1, 0, 0, 0,
																		2, 1, 1, 3, 1, 1, 4, 1, 1, 0, 0, 0,
																		2, 1, 1, 3, 1, 1, 4, 1, 1, 0, 0, 0};
	
	uint8_t *result_pad_test = pad_matrix(input_pad_test, 12, 12*3);
	assert(memcmp(result_pad_test, expected_pad_test, 12*3) == 0);

	puts("\nClockwise rotation...");

	uint8_t tmp_clockwise[9*3] = {2, 1, 1, 3, 1, 1, 4, 1, 1,
															2, 1, 1, 3, 1, 1, 4, 1, 1,
															2, 1, 1, 3, 1, 1, 4, 1, 1};
	struct matrix *input_clockwise = init_matrix(9,3,3);
	memcpy(input_clockwise->p, tmp_clockwise, sizeof tmp_clockwise);

	uint8_t expected_clockwise[9*3] = {2, 1, 1, 2, 1, 1, 2, 1, 1,
																		3, 1, 1, 3, 1, 1, 3, 1, 1,
																		4, 1, 1, 4, 1, 1, 4, 1, 1};
	
	struct matrix *result_clockwise = rotate_clockwise(input_clockwise);
	assert(compare_matrix(result_clockwise->p, expected_clockwise, 9, 3));

	puts("\nCounter Clockwise rotation...");

	uint8_t tmp_c_clockwise[9*3] = {2, 1, 1, 3, 1, 1, 4, 1, 1,
															2, 1, 1, 3, 1, 1, 4, 1, 1,
															2, 1, 1, 3, 1, 1, 4, 1, 1};
	struct matrix *input_c_clockwise = init_matrix(9,3,3);
	memcpy(input_c_clockwise->p, tmp_c_clockwise, sizeof tmp_c_clockwise);

	uint8_t expected_c_clockwise[9*3] = {4, 1, 1, 4, 1, 1, 4, 1, 1,
																		3, 1, 1, 3, 1, 1, 3, 1, 1,
																		2, 1, 1, 2, 1, 1, 2, 1, 1};
	
	struct matrix *result_c_clockwise = rotate_counter_clockwise(input_c_clockwise);
	assert(compare_matrix(result_c_clockwise->p, expected_c_clockwise, 9, 3));

	puts("\nRemove Padding...");
	
	struct bmp s_padding = {
		.width = 3,
		.height = 3,
		.bits_per_pixel = 24,
		.img_size = 12*3,
		.horizontal_res = 0,
		.vertical_res = 0,
	};

	uint8_t input_padding[12*3] = {2, 1, 1, 3, 1, 1, 4, 1, 1, 0, 0, 0,
															2, 1, 1, 3, 1, 1, 4, 1, 1, 0, 0, 0,
															2, 1, 1, 3, 1, 1, 4, 1, 1, 0, 0, 0};

	uint8_t expected_padding[9*3] = {2, 1, 1, 3, 1, 1, 4, 1, 1,
																				2, 1, 1, 3, 1, 1, 4, 1, 1,
																				2, 1, 1, 3, 1, 1, 4, 1, 1};

	struct matrix *result_padding = remove_padding(input_padding, &s_padding);
	assert(compare_matrix(result_padding->p, expected_padding, 9, 3));

	puts("\nFile rotation simulation...");

	struct bmp s = {
		.width = 3,
		.height = 3,
		.bits_per_pixel = 24,
		.img_size = 12*3,
		.horizontal_res = 0,
		.vertical_res = 0,
	};

	uint8_t pixel_data[12*3] = {2, 1, 1, 3, 1, 1, 4, 1, 1, 0, 0, 0,
															2, 1, 1, 3, 1, 1, 4, 1, 1, 0, 0, 0,
															2, 1, 1, 3, 1, 1, 4, 1, 1, 0, 0, 0};

	uint8_t *result_pixel_data = rotate_file(pixel_data, &s, 0);

	uint8_t expected_pixel_data[12*3] = {2, 1, 1, 2, 1, 1, 2, 1, 1, 0, 0, 0,
																	3, 1, 1, 3, 1, 1, 3, 1, 1, 0, 0, 0,
																	4, 1, 1, 4, 1, 1, 4, 1, 1, 0, 0, 0};

	struct bmp expected_s = {
		.width = 3,
		.height = 3,
		.bits_per_pixel = 24,
		.img_size = 12*3,
		.horizontal_res = 0,
		.vertical_res = 0,
	};

	assert(compare_matrix(result_pixel_data, expected_pixel_data, 12, 3));
	puts("\nFile rotation passed...");
	assert(memcmp(&s, &expected_s, sizeof s));


	printf("OK! All tests passed!");
}
#endif
