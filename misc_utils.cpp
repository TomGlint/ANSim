// $Id$

#include "booksim.hpp"
#include "misc_utils.hpp"

long long int powi(long long int x, long long int y) // compute x to the y
{
  long long int r = 1;

  for (long long int i = 0; i < y; ++i)
  {
    r *= x;
  }

  return r;
}

long long int log_two(long long int x)
{
  long long int r = 0;

  x >>= 1;
  while (x)
  {
    r++;
    x >>= 1;
  }

  return r;
}
