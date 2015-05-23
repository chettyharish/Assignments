#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#define TRUE 1
#define FALSE 0
//#define INT_MAX 99999

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

int notSelected(int curr, int *clusterLineList, int numClusters) {
	int i = 0;
	for (i = 0; i < numClusters; i++) {
		if (clusterLineList[i] == curr) {
			return FALSE;
		}
	}
	return TRUE;
}

int findNewLeader(int clusterAllocations[][2], int *clusterLineList,
		int numClusters, int totalInputLines) {
	int i = 0, j = 0, max = -1, pos = -1;
	for (i = 0; i < totalInputLines; i++) {
		if (clusterAllocations[i][0]
				> max&& notSelected(i, clusterLineList,numClusters) == TRUE) {
			max = clusterAllocations[i][0];
			pos = i;

		}
	}
	return pos;

}

int spearmanFootrule(int *a, int size, int min) {
	int dist = 0, i = 0, v = 0;
	for (i = 0; i < size; i++) {
		if (dist < min) {
			dist += abs(a[i] - i);
		} else {
			// If new distance is already greater than min, dont bother with completing the entire point.
			return INT_MAX;
		}
	}
	// New distance is less than previous min
	return dist;
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		printf("Error : Incorrect number of arguments\n");
		printf("Usage : %s input.txt numCluster permSize\n", argv[0]);
		exit(1);
	}
	int j = 0, k = 0, clusterCount = 0, numClusters = atoi(argv[2]),
			totalInputLines = 0;
	int size = atoi(argv[3]), i = 0, map[size], lineSize = 0;
	char buffer[size*3];
	int maxClusters = 2000;

	FILE *file = fopen(argv[1], "r");
	if (file == NULL) {
		perror("Error");
		exit(1);
	}

	while (fgets(buffer, sizeof buffer, file)) {
		++totalInputLines;
	}
	fclose(file);

	char (*clusterLeader) = malloc(maxClusters*sizeof(char)*size * 3);//[10000];
	int (*clusterLineList)= malloc(maxClusters*sizeof(int));//[numClusters];
	char (*clusterLeaderList)[maxClusters] = malloc(
			numClusters * sizeof *clusterLeaderList);
	int (*distances)[totalInputLines] = malloc(numClusters * sizeof *distances);
	int (*clusterAllocations)[2] = malloc(
			totalInputLines * sizeof *clusterAllocations);
	char (*filedata)[size * 3] = malloc(totalInputLines * sizeof *filedata);

	memset(clusterLineList, -1, sizeof clusterLineList);

	file = fopen(argv[1], "r");
	for (i = 0; i < totalInputLines; i++) {
		if (fscanf(file, "%s\n", filedata[i]) == 0) {
			printf("Error reading input.txt");
		}
	}
	rewind(file);

	double start_time, end_time;
	struct timeval t;
	gettimeofday(&t, NULL);
	start_time = 1.0e-6 * t.tv_usec + t.tv_sec;

	for (i = 0; i < numClusters; i++) {
		for (j = 0; j < totalInputLines; j++) {
			distances[i][j] = -1;
			clusterAllocations[j][0] = INT_MAX;
			clusterAllocations[j][1] = -1;
		}
	}

	if (fscanf(file, "%s\n", clusterLeader) == 0) {
		printf("Error reading input.txt");
	}
	strcpy(clusterLeaderList[clusterCount], clusterLeader);
	clusterLineList[clusterCount] = 0;
	lineSize = (int) ftell(file) - 1;
	rewind(file);

	while (clusterCount < numClusters) {
		printf("Creating cluster : %d\n", clusterCount);
		int lineNo = 0, temp[size];
		char *token, *rest, *ptr;
		char inputLine[size*3];
		i = 0, j = 0;
		ptr = clusterLeader;
		while (token = strtok_r(ptr, " ,", &rest)) {
			map[atoi(token)] = i;
			ptr = rest;
			i++;
		}

#pragma openmp parallel for
		for (j = 0; j < totalInputLines; j++) {
			strcpy(inputLine, filedata[j]);
			char *token2, *rest2, *ptr2;
			int input[size];
			ptr2 = inputLine;
			i = 0;
			while (token2 = strtok_r(ptr2, " ,", &rest2)) {
				input[i] = map[atoi(token2)];
				ptr2 = rest2;
				i++;
			}
//			distances[clusterCount][lineNo] = 10;
			distances[clusterCount][lineNo] = mergeSort(input, temp, 0,
					size - 1);
//			distances[clusterCount][lineNo] = spearmanFootrule(input, size,
//					clusterAllocations[j][0]);

			if (distances[clusterCount][lineNo] < clusterAllocations[lineNo][0]
					&& distances[clusterCount][lineNo] > -1) {
				clusterAllocations[lineNo][0] = distances[clusterCount][lineNo];
				clusterAllocations[lineNo][1] = clusterCount;
			}
			lineNo++;
		}

//#pragma openmp parallel for
//		for (j = 0; j < totalInputLines; j++) {
//			int min = INT_MAX, pos = -1;
//			for (i = 0; i < numClusters; i++) {
//#pragma openmp critical
//				{
//					if (distances[clusterCount][j] < clusterAllocations[j][0]
//							&& distances[clusterCount][j] > -1) {
//						clusterAllocations[j][0] = distances[clusterCount][j];
//						clusterAllocations[j][1] = clusterCount;
//					}
//				}
//			}
//		}

		if (clusterCount < numClusters - 1) {
			int gotoLine = findNewLeader(clusterAllocations, clusterLineList,
					numClusters, totalInputLines);
			int length = (gotoLine) * (lineSize + 1) - 1;
			fseek(file, length, SEEK_SET);
			if (fscanf(file, "%s\n", clusterLeader) == 0) {
				printf("Error reading input.txt");
			}
			strcpy(clusterLeaderList[clusterCount + 1], clusterLeader);

			rewind(file);
			clusterLineList[clusterCount + 1] = gotoLine;

		}
		clusterCount++;
	}

	close(file);

	int clusterStats[numClusters][2];
	for (i = 0; i < numClusters; i++) {
		int max = -1, num = 0;
		for (j = 0; j < totalInputLines; j++) {
			if (clusterAllocations[j][1] == i) {
				num++;
				if (clusterAllocations[j][0] > max) {
					max = clusterAllocations[j][0];
				}
			}
		}
		clusterStats[i][0] = num;
		clusterStats[i][1] = max;
	}

	gettimeofday(&t, NULL);
	end_time = 1.0e-6 * t.tv_usec + t.tv_sec;
	printf("Before file time =  %lf seconds\n", end_time - start_time);

	FILE *file2 = fopen("data.dat", "w");
	for (i = 0; i < numClusters; i++) {
		fprintf(file2, "%s;%06d;%06d\n", clusterLeaderList[i],
				clusterStats[i][0], clusterStats[i][1]);
	}

	for (i = 0; i < numClusters; i++) {
		for (j = 0; j < totalInputLines; j++) {
			if (clusterAllocations[j][1] == i) {
				fprintf(file2, "%s;%06d;%06d\n", filedata[j],
						clusterLineList[clusterAllocations[j][1]], j);
			}
		}
	}

	gettimeofday(&t, NULL);
	end_time = 1.0e-6 * t.tv_usec + t.tv_sec;
	printf("Execution time =  %lf seconds\n", end_time - start_time);

}
