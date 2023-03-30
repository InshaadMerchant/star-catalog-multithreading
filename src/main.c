// MIT License
// 
// Copyright (c) 2023 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include "utility.h"
#include "star.h"
#include "float.h"
#include <pthread.h> //Include the pthread header file

#define NUM_STARS 30000
#define MAX_LINE 1024
#define DELIMITER " \t\n"

struct Star star_array[NUM_STARS];
uint8_t (*distance_calculated)[NUM_STARS];

double min = FLT_MAX;
double max = FLT_MIN;
double distance = 0.0;

uint32_t thread_count = 1; //Declaring and initializing a variable to count threads
pthread_mutex_t mutex;

int numOfThreads; //Variable to store number of threads used.
int count;

void showHelp()
{
  printf("Use: findAngular [options]\n");
  printf("Where options are:\n");
  printf("-t          Number of threads to use\n");
  printf("-h          Show this help\n");
}

float determineAverageAngularDistance(struct Star arr[], int start, int end)
{
  uint32_t i, j;

  for (i = start; i < end; i++)
  {
    for (j = 0; j < NUM_STARS; j++)
    {
      if (i != j && distance_calculated[i][j] == 0)
      {
        double spacing = calculateAngularDistance(arr[i].RightAscension, arr[i].Declination, //Calculating spacing/distance
                                               arr[j].RightAscension, arr[j].Declination);   //between stars
        distance_calculated[i][j] = 1;
        distance_calculated[j][i] = 1;
        pthread_mutex_lock( &mutex ); // Locking Mutex for a thread to execute

        thread_count++; //Incrementing thread counter

        if (min > spacing) //Determining Min Distance
        {
          min = spacing;
        }

        if (max < spacing) //Determining Max Distance
        {
          max = spacing;
        }
        pthread_mutex_unlock( &mutex ); // Unlocking mutex after that thread has been executed.
      }
    }
  }
  return distance; //returning distance between stars
}

void *threading_Function(void *a)
{
  int count = (intptr_t)a;
  int start = 0, end = 0;
  int interval = NUM_STARS / numOfThreads; //Calculating interval between stars.

  start = interval * count; 
  end = interval * (count + 1);
  determineAverageAngularDistance(star_array, start, end); // Calling Function to find avg angular distance.
  return NULL;
}

int main(int argc, char *argv[])
{
  FILE *fp; //Declaring File Pointer
  uint32_t star_count = 0;

  uint32_t n;

  distance_calculated = malloc(sizeof(uint8_t[NUM_STARS][NUM_STARS]));

  if (distance_calculated == NULL)
  {
    uint64_t size = NUM_STARS * NUM_STARS * sizeof(uint8_t);
    printf("Could not allocate %ld bytes\n", size);
    exit(EXIT_FAILURE);
  }

  memset(distance_calculated, 0, sizeof(uint8_t[NUM_STARS][NUM_STARS]));

  for (n = 1; n < argc; n++)
  {
    if (strcmp(argv[n], "-help") == 0)
    {
      showHelp();
      exit(0);
    }
    else if (strcmp(argv[n], "-t") == 0)
    {
      numOfThreads = atoi(argv[n + 1]);
    }
  }
  //opening file and checking if it exists or not.
  fp = fopen("data/tycho-trimmed.csv", "r");

  if (fp == NULL)
  {
    printf("ERROR: Unable to open the file data/tycho-trimmed.csv\n");
    exit(1);
  }

  char line[MAX_LINE];
  while (fgets(line, 1024, fp))
  {
    uint32_t column = 0;

    char *tok;
    for (tok = strtok(line, " "); tok && *tok; tok = strtok(NULL, " "))
    {
      switch (column)
      {
      case 0:
        star_array[star_count].ID = atoi(tok);
        break;

      case 1:
        star_array[star_count].RightAscension = atof(tok);
        break;

      case 2:
        star_array[star_count].Declination = atof(tok);
        break;

      default:
        printf("ERROR: line %d had more than 3 columns\n", star_count);
        exit(1);
        break;
      }
      column++;
    }
    star_count++;
  }
  printf("%d records read\n", star_count);
  pthread_t threads[numOfThreads];
  // Time Calculation starts here.............
  clock_t NumOfClocks; // declaring a variable to count number of clocks
  NumOfClocks = clock(); // clock() function included in time.h to count number of clocks
  // Find the average angular distance in the most inefficient way possible
  clock_t start_time = clock(); // Record start time
  pthread_mutex_init(&mutex, NULL); //Initializing Mutex
  for (int i = 0; i < numOfThreads; i++)
  {
    pthread_create(&threads[i], NULL, threading_Function, (void *)(intptr_t)i); //Create a new thread.
  }
  for (int i = 0; i < numOfThreads; i++)
  {
    pthread_join(threads[i], NULL); //Waiting for a thread to terminate.
  }
  clock_t end_time = clock(); // Record end time

  double total = 0.0;
  uint64_t count = 0;
  for (int i = 0; i < NUM_STARS; i++)
  {
    for (int j = i + 1; j < NUM_STARS; j++)
    {
      if (distance_calculated[i][j])
      {
        distance = calculateAngularDistance(
            star_array[i].RightAscension, star_array[i].Declination,
            star_array[j].RightAscension, star_array[j].Declination);
        total += distance; // Adding all the distances returned from calculateAngularDistance Function
        count++; //Incrementing count for all the distances returned.
      }
    }
  }

  double avg_distance = total / count; //Calculating average distance between stars
  // Printing Results onto Screen...........
  printf("Average distance found is %lf\n", avg_distance);
  printf("Minimum distance found is %lf\n", min);
  printf("Maximum distance found is %lf\n", max);
  double time_Spent = ((double)(end_time - start_time)) / CLOCKS_PER_SEC; //Calculating time by (diff of start and end clocks)/CLOCKS_PER_SEC
  printf("Time Taken: %f seconds \n", time_Spent);

  fclose(fp); //Closing File
  pthread_exit(NULL); // Exiting pthread

  return 0;
}

