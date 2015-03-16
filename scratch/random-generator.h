/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */

#ifndef RANDOM_GENERATOR_H
#define RANDOM_GENERATOR_H

#include <stdint.h>
#include <cstdlib>
#include <ctime>
#include "ns3/log.h"
#include "ns3/assert.h"

namespace ns3
{

class RandomGenerator
{
public:
  RandomGenerator ();
  RandomGenerator (uint32_t n);
  ~RandomGenerator ();
  void Reset ();
  uint32_t GetNext ();
  void SetN (uint32_t n);
  static void SetSeed ();
private:
  void CreateTemp ();
  uint32_t n;
  uint32_t *temp;
  uint32_t pos;
};

RandomGenerator::RandomGenerator ()
{
  n = 10;
  temp = 0;
  CreateTemp ();
}

RandomGenerator::RandomGenerator (uint32_t n)
{
  this->n = n;
  temp = 0;
  CreateTemp ();
}

RandomGenerator::~RandomGenerator ()
{
  delete [] temp;
}

void RandomGenerator::Reset ()
{
  for (uint32_t i = 0; i < n; i++)
    temp[i] = i;
  pos = 0;
}

uint32_t RandomGenerator::GetNext ()
{
  if (pos == n)
    Reset ();
  uint32_t pick = pos + rand () % (n - pos);
  uint32_t tempVal = temp[pick];
  temp[pick] = temp[pos];
  temp[pos] = tempVal;
  return temp[pos++];
}

void RandomGenerator::SetN (uint32_t n)
{
  delete [] temp;
  CreateTemp ();
}

void RandomGenerator::CreateTemp ()
{
  if (temp != 0)
    delete [] temp;
  temp = new uint32_t[n];
  Reset ();
}

void RandomGenerator::SetSeed ()
{
  srand (time(NULL));
}

}
#endif
