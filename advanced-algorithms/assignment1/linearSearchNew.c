#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <time.h>
#define TRUE 0
#define FALSE 1
#define INT_MAX 999999

int merge(int *a, int *temp, int low, int mid, int high) {
	int i, j, pos, inversion = 0;
	for (i = low; i <= high; i++) {
		temp[i] = a[i];
	}

	i = low;
	j = mid + 1;
	for (pos = low; pos <= high; pos++) {
		if (i > mid) {
			a[pos] = temp[j++];
		} else if (j > high) {
			a[pos] = temp[i++];
		} else if (temp[i] <= temp[j]) {
			a[pos] = temp[i++];
		} else {
			a[pos] = temp[j++];
			inversion += (mid - i) + 1;
		}
	}
	return inversion;
}

int mergeSort(int *a, int *temp, int low, int high) {
	if (low < high) {
		int inversion = 0;
		int mid = (low + high) / 2;
		inversion = mergeSort(a, temp, low, mid);
		inversion += mergeSort(a, temp, mid + 1, high);
		inversion += merge(a, temp, low, mid, high);
		return inversion;
	} else {
		return 0;
	}

}

struct LLNode {
	struct LLNode *next;
	int line;
	int minDist;
};

struct htab {
	struct LLNode *heads;
	struct LLNode *tails;
};

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("Error : Incorrect number of arguments\n");
		printf("Usage : %s input.txt query.txt permSize\n", argv[0]);
		exit(1);
	}

	int j = 0, k = 0, uc = 0, totalInputLines = 0;
	char buffer[1000];

	double start_time, end_time;
	struct timeval t;
	gettimeofday(&t, NULL);
	start_time = 1.0e-6 * t.tv_usec + t.tv_sec;

	FILE *file = fopen(argv[1], "r");
	if (file == NULL) {
		perror("Error");
		exit(1);
	}

	while (fgets(buffer, sizeof buffer, file)) {
		++totalInputLines;
	}
	fclose(file);

	file = fopen(argv[2], "r");
	if (file == NULL) {
		perror("Error");
		exit(1);
	}
	while (fgets(buffer, sizeof buffer, file)) {
		++uc;
	}
	fseek(file, 0, SEEK_SET);


	char filedata[uc][1000];
	struct htab usertab[uc];
	struct htab *tableptr[uc];
	for (k = 0; k < uc; k++) {
		if(fscanf(file, "%s\n", filedata[k])==0){
			printf("Error reading query.txt");
		}
		usertab[k].heads = NULL;
		usertab[k].tails = NULL;
	}
	fclose(file);

#pragma omp parallel for shared(usertab)
	for (j = 0; j < uc; j++) {
		int size = atoi(argv[3]), i = 0, l = 0, min = INT_MAX;
		int orig[size], map[size];
		int next = FALSE, ic = 0, total = 0, ipsize = 1000;
		char *token, *rest, *ptr;

		ptr = filedata[j];
		i = 0;
		while (token = strtok_r(ptr, " ,", &rest)) {
			map[atoi(token)] = i;
			ptr = rest;
			i++;
		}

		FILE *file2 = fopen(argv[1], "r");

		for (; total < totalInputLines;) {
			char inputdata[ipsize][1000];
			while (next == FALSE) {
				fscanf(file2, "%s", inputdata[ic]);
				if (ic == ipsize - 1) {
					next = TRUE;
				}
				ic++;
			}
#pragma omp parallel for
			for (l = 0; l < ipsize; l++) {
				char *token2, *rest2, *ptr2;
				int input[size], temp[size];
				int inversion = -1, line = 0, dir_swaps = 0, lrcounter = 0;
				int m = 0, mid = size / 2;
				ptr2 = inputdata[l];

				while (token2 = strtok_r(ptr2, " ,", &rest2)) {
					input[m] = map[atoi(token2)];
					ptr2 = rest2;

					if (input[m] > m) {
						dir_swaps += input[m] - m;
						if (input[m] >= mid) {
							if (input[m] - mid < lrcounter) {
								dir_swaps += lrcounter - input[m] + mid;
							}
						}

						if (input[m] < mid) {
							dir_swaps += lrcounter;
						}

					} else if (input[m] < mid) {
						if (input[m] - m + lrcounter > 0)
							dir_swaps += input[m] - m + lrcounter;
					} else {
						if (mid - m + lrcounter > 0)
							dir_swaps += mid - m + lrcounter;
					}

					if (dir_swaps > min) {
						break;
					}
					m++;
				}

				if (dir_swaps <= min) {
					inversion = mergeSort(input, temp, 0, size - 1);
					if (inversion < min) {

						min = inversion;
						struct LLNode *newNode = malloc(sizeof(struct LLNode));
						newNode->next = NULL;
						newNode->line = total + l;
						newNode->minDist = min;
						usertab[j].heads = newNode;
						usertab[j].tails = newNode;
					} else if (inversion == min) {
						struct LLNode *newNode = malloc(sizeof(struct LLNode));
						newNode->next = NULL;
						newNode->line = total + l;
						newNode->minDist = min;

						if (usertab[j].heads == NULL) {
							usertab[j].heads = newNode;
							usertab[j].tails = newNode;
						} else {
							usertab[j].tails->next = newNode;
							usertab[j].tails = newNode;
						}
					}
				}
			} // input parallel for
			total += ic;
			ic = 0;
			next = FALSE;
		} // Outside for

		tableptr[j] = &usertab[j];
		fclose(file2);
	} // user parallel for



	struct LLNode *curr;

	for (k = 0; k < uc; k++) {
		if (tableptr[k]->heads != NULL) {
			curr = tableptr[k]->heads;
			while (curr != NULL) {
				if (curr->next != NULL) {
					printf("%d,", curr->line);
				} else {
					printf("%d", curr->line);
				}
				curr = curr->next;
			}
			printf("\n");
		}
	}

	gettimeofday(&t, NULL);
	end_time = 1.0e-6 * t.tv_usec + t.tv_sec;
	printf("Execution time =  %lf seconds\n", end_time - start_time);

	return 0;
}
