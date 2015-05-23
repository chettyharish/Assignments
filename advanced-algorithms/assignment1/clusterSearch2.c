#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TRUE 1
#define FALSE 0
#define INT_MAX 99999

struct LLNode {
	struct LLNode *next;
	int line;
	int minDist;
	char *data[1000];
};

struct htab {
	struct LLNode *heads;
	struct LLNode *tails;
};

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

int findMin(int *leaderDistances, int numClusters, int least , int xmin) {
	int i = 0, min = 0, pos = -1;

	for (i = 0; i < numClusters; i++) {
		//Select least alpha cluster
		if(leaderDistances[i] + (xmin - least)<= min){
			min = leaderDistances[i] + (xmin - least);
			pos = i;
		}
	}

	if (pos > -1)
		leaderDistances[pos] = INT_MAX;
	return pos;

}

struct leader {
	char perm[1000];
	int num;
	int d;
	int foffset;
};

int main(int argc, char *argv[]) {
	if (argc != 5) {
		printf("Error : Incorrect number of arguments\n");
		printf("Usage : %s data.dat query.txt numCluster permSize\n", argv[0]);
		exit(1);
	}
	int numClusters = atoi(argv[3]), i = 0, j = 0, k = 0, lineSize = 0, size =
			atoi(argv[4]), totalQuery = 0, query = 0;
	struct leader leaders[numClusters];
	char *token, *rest, *ptr;
	char buffer[1000];

	double start_time, end_time;
	struct timeval t;
	gettimeofday(&t, NULL);
	start_time = 1.0e-6 * t.tv_usec + t.tv_sec;

	FILE *file = fopen(argv[2], "r");
	if (file == NULL) {
		perror("Error");
		exit(1);
	}
	while (fgets(buffer, sizeof buffer, file)) {
		++totalQuery;
	}
	fseek(file, 0, SEEK_SET);

	char querydata[totalQuery][1000];
	struct htab usertab[totalQuery];
	struct htab *tableptr[totalQuery];

	for (k = 0; k < totalQuery; k++) {
		usertab[k].heads = NULL;
		usertab[k].tails = NULL;
		if (fscanf(file, "%s\n", querydata[k]) == 0) {
			printf("Error reading query.txt");
		}
	}
	fclose(file);

#pragma openmp parallel for
	for (query = 0; query < totalQuery; query++) {
		int map[size], leaderDistances[numClusters], input[size], temp[size];
		int least = INT_MAX, foffset = numClusters;
		char ipread[1000];

		ptr = querydata[query];
		i = 0;
		while (token = strtok_r(ptr, " ,", &rest)) {
			map[atoi(token)] = i;
			ptr = rest;
			i++;
		}

		FILE *file = fopen(argv[1], "r");
		if (file == NULL) {
			perror("Error");
			exit(1);
		}
		for (i = 0; i < numClusters; i++) {
			if (fscanf(file, "%s\n", ipread) == 0) {
				printf("Error reading data.dat");
			}
			ptr = ipread;

			token = strtok_r(ptr, " ;", &rest);
			strcpy(leaders[i].perm, token);
			ptr = rest;

			token = strtok_r(ptr, " ;", &rest);
			leaders[i].num = atoi(token);
			ptr = rest;

			token = strtok_r(ptr, " ;", &rest);
			leaders[i].d = atoi(token);
			ptr = rest;
			if (i == 0)
				lineSize = (int) ftell(file) - 1;

			leaders[i].foffset = foffset;
			foffset += leaders[i].num;
		}

		for (i = 0; i < numClusters; i++) {
			ptr = leaders[i].perm;
			int m = 0;
			while (token = strtok_r(ptr, " ,", &rest)) {
				input[m] = map[atoi(token)];
				ptr = rest;
				m++;
			}
			leaderDistances[i] = mergeSort(input, temp, 0, size - 1);
		}


		//finding x_min
		int xmin = INT_MAX;
		for (i = 0; i < numClusters; i++) {
			if(leaderDistances[i] < xmin){
				xmin = leaderDistances[i];
				least = leaderDistances[i];
			}
		}


		int RCounter=0;
		for (i = 0; i < numClusters; i++) {
			printf("%d\t%d\t%d\n",leaderDistances[i] , leaders[i].d,leaderDistances[i] - xmin - 100);
			if(leaderDistances[i] - xmin - leaders[i].d >0)
				RCounter++;
		}

		printf("RCounter --> %d\n",RCounter);

		//Calculating Alpha
		for (i = 0; i < numClusters; i++) {
			leaderDistances[i] = leaderDistances[i] - xmin - leaders[i].d;
		}

		least = 60;
		for (i = 0; i < numClusters; i++) {
			int pos = findMin(leaderDistances, numClusters, least , xmin);
			int next = FALSE, ic = 0, total = 0, ipsize = -1, j = 0;
			if (pos == -1) {
//				printf("Skipping %d clusters out of %d clusters!\n",
//						numClusters - i, numClusters);
				break;
			}
			int length = (leaders[pos].foffset) * (lineSize + 1) - 1;
			fseek(file, length, SEEK_SET);

			for (; total < leaders[pos].num;) {
				ipsize =
						(1000 > leaders[pos].num - total) ?
								leaders[pos].num - total : 1000;
				char inputdata[ipsize][1000];
				while (next == FALSE) {
					if (fscanf(file, "%s", inputdata[ic]) == 0) {
						printf("Error reading data.dat");
					}
					if (ic == ipsize - 1) {
						next = TRUE;
					}
					ic++;
				}
#pragma openmp parallel for
				for (j = 0; j < ipsize; j++) {
					int m = 0, dir_swaps = 0, mid = size / 2, rcounter = 0;
					int lineNo = -1, clusterId = -1, inversion = INT_MAX;
					char inputdata2[1000];

					ptr = inputdata[j];
					token = strtok_r(ptr, " ;", &rest);
					strcpy(inputdata2, token);
					ptr = rest;

					token = strtok_r(ptr, " ;", &rest);
					clusterId = atoi(token);
					ptr = rest;

					token = strtok_r(ptr, " ;", &rest);
					lineNo = atoi(token);

					ptr = inputdata2;
					while (token = strtok_r(ptr, " ,", &rest)) {
						input[m] = map[atoi(token)];
						ptr = rest;
						if (input[m] > m) {
							dir_swaps += input[m] - m;

							if (input[m] > mid) {
								if (input[m] - mid < rcounter) {
									dir_swaps += rcounter - input[m] + mid;
								}
								rcounter++;
							}
						} else if (input[m] < mid) {
							if (input[m] - m + rcounter > 0)
								dir_swaps += input[m] - m + rcounter;
						}

						if (dir_swaps > least) {
							break;
						}
						m++;
					} // inputdata2 while ends

					if (dir_swaps <= least) {
						inversion = mergeSort(input, temp, 0, size - 1);
						if (inversion < least) {
							least = inversion;
							struct LLNode *newNode = malloc(
									sizeof(struct LLNode));
							newNode->next = NULL;
							newNode->line = lineNo;
							newNode->minDist = least;
							usertab[query].heads = newNode;
							usertab[query].tails = newNode;
						} else if (inversion == least) {
							struct LLNode *newNode = malloc(
									sizeof(struct LLNode));
							newNode->next = NULL;
							newNode->line = lineNo;
							newNode->minDist = least;

							if (usertab[query].heads == NULL) {
								usertab[query].heads = newNode;
								usertab[j].tails = newNode;
							} else {
								usertab[query].tails->next = newNode;
								usertab[query].tails = newNode;
							}
						}
					}

				} // Lines inside cluster for ends
				total += ic;
				ic = 0;
				next = FALSE;
			} // Outside for
		} // clusters for ends
		tableptr[query] = &usertab[query];
		int TEMP , COUNTTEMP , TOTAL = 0;
		for (TEMP = 0; TEMP < numClusters; TEMP++) {
			if (leaderDistances[TEMP] < INT_MAX){
				TOTAL += leaders[TEMP].num;
			}
		}
		printf("Eliminated [%d] elements \n" , TOTAL);
//		printf("%d\n" , TOTAL);
	} // query for ends

	struct LLNode *curr;

	for (k = 0; k < totalQuery; k++) {
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
		} else {
			printf("NULL\n");
		}
	}
	gettimeofday(&t, NULL);
	end_time = 1.0e-6 * t.tv_usec + t.tv_sec;
	printf("Execution time =  %lf seconds\n", end_time - start_time);

} // main ends

