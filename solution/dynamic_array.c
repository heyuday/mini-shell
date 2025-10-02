#include "dynamic_array.h"
#include <unistd.h>
#include <stdlib.h> // malloc, realloc, free
#include <string.h> // strdup
#include <stdio.h>  // printf
// Create a new DynamicArray with given initial capacity
DynamicArray *da_create(size_t init_capacity)
{
  DynamicArray *da = malloc(sizeof(DynamicArray));
  da->data = malloc(sizeof(char *) * init_capacity);
  da->size = 0;
  da->capacity = init_capacity;
  return da;
}

// Add element to Dynamic Array at the end. Handles resizing if necessary
void da_put(DynamicArray *da, const char *val)
{
  if (da->size == da->capacity)
  { // Resize
    da->capacity *= 2;
    da->data = realloc(da->data, sizeof(char *) * da->capacity);
  }
  da->data[da->size] = strdup(val);
  da->size++;
}

// Get element at an index (NULL if not found)
char *da_get(DynamicArray *da, const size_t ind)
{
  if (ind > da->size)
  {
    // Some error
  }
  return da->data[ind];
}

// Delete Element at an index (handles packing)
void da_delete(DynamicArray *da, const size_t ind)
{
  if (ind > da->size)
  {
    // Some error
  }

  free(da->data[ind]);
  for (size_t i = ind; i < da->size - 1; i++)
  {
    da->data[i] = da->data[i + 1];
  }
  da->size--;
}

// Print Elements line after line
void da_print(DynamicArray *da)
{
  for (size_t i = 0; i < da->size; i++)
  {
    printf("%s\n", da->data[i]);
  }
}

// Free whole DynamicArray
void da_free(DynamicArray *da)
{
  for (size_t i = 0; i < da->size; i++)
  {
    free(da->data[i]);
  }
  free(da->data);
  free(da);
}
