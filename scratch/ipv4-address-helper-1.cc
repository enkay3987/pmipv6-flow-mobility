#include "ns3/core-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

int main ()
{
  Ipv4Mask mask ("255.255.255.248");
  Ipv4AddressGenerator::Init ("192.168.0.0", mask);
  Ipv4AddressGenerator::NextAddress (mask);
  std::cout << "Network: " << Ipv4AddressGenerator::GetNetwork (mask) << " Address: " << Ipv4AddressGenerator::GetAddress (mask) << std::endl;
  Ipv4AddressGenerator::NextNetwork (mask);
  Ipv4AddressGenerator::NextAddress (mask);
  Ipv4AddressGenerator::NextAddress (mask);
  std::cout << "Network: " << Ipv4AddressGenerator::GetNetwork (mask) << " Address: " << Ipv4AddressGenerator::GetAddress (mask) << std::endl;
  Ipv4AddressGenerator::NextNetwork (mask);
  Ipv4AddressGenerator::NextAddress (mask);
  Ipv4AddressGenerator::NextAddress (mask);
  std::cout << "Network: " << Ipv4AddressGenerator::GetNetwork (mask) << " Address: " << Ipv4AddressGenerator::GetAddress (mask) << std::endl;
  return 0;
}
