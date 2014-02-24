#include "ns3/core-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

int main ()
{
  Ipv6Prefix prefix (64);
  Ipv6AddressGenerator::Init ("a:b:c:1::", prefix);
  std::cout << "Network: " << Ipv6AddressGenerator::GetNetwork (prefix) << " Address: " << Ipv6AddressGenerator::GetAddress (prefix) << std::endl;
  Ipv6AddressGenerator::NextAddress (prefix);
  std::cout << "Network: " << Ipv6AddressGenerator::GetNetwork (prefix) << " Address: " << Ipv6AddressGenerator::GetAddress (prefix) << std::endl;
  Ipv6AddressGenerator::NextNetwork (prefix);
  Ipv6AddressGenerator::NextAddress (prefix);
  Ipv6AddressGenerator::NextAddress (prefix);
  std::cout << "Network: " << Ipv6AddressGenerator::GetNetwork (prefix) << " Address: " << Ipv6AddressGenerator::GetAddress (prefix) << std::endl;
  Ipv6AddressGenerator::NextNetwork (prefix);
  Ipv6AddressGenerator::NextAddress (prefix);
  Ipv6AddressGenerator::NextAddress (prefix);
  std::cout << "Network: " << Ipv6AddressGenerator::GetNetwork (prefix) << " Address: " << Ipv6AddressGenerator::GetAddress (prefix) << std::endl;
  Ipv6Address network1 = Ipv6AddressGenerator::GetNetwork (prefix);
  Ipv6AddressGenerator::Init ("d:e:f:1::", prefix);
  std::cout << "Network: " << Ipv6AddressGenerator::GetNetwork (prefix) << " Address: " << Ipv6AddressGenerator::GetAddress (prefix) << std::endl;
  Ipv6AddressGenerator::NextAddress (prefix);
  std::cout << "Network: " << Ipv6AddressGenerator::GetNetwork (prefix) << " Address: " << Ipv6AddressGenerator::GetAddress (prefix) << std::endl;
  Ipv6AddressGenerator::NextNetwork (prefix);
  Ipv6AddressGenerator::NextAddress (prefix);
  Ipv6AddressGenerator::NextAddress (prefix);
  std::cout << "Network: " << Ipv6AddressGenerator::GetNetwork (prefix) << " Address: " << Ipv6AddressGenerator::GetAddress (prefix) << std::endl;
  Ipv6AddressGenerator::NextNetwork (prefix);
  Ipv6AddressGenerator::NextAddress (prefix);
  Ipv6AddressGenerator::NextAddress (prefix);
  std::cout << "Network: " << Ipv6AddressGenerator::GetNetwork (prefix) << " Address: " << Ipv6AddressGenerator::GetAddress (prefix) << std::endl;
  Ipv6AddressGenerator::Init (network1, prefix, "::3");
  Ipv6AddressGenerator::NextAddress (prefix);
  std::cout << "Network: " << Ipv6AddressGenerator::GetNetwork (prefix) << " Address: " << Ipv6AddressGenerator::GetAddress (prefix) << std::endl;
  Ipv6Address network = Ipv6AddressGenerator::GetNetwork (prefix);
  Ipv6Address address = Ipv6AddressGenerator::GetAddress (prefix);
  uint8_t networkBuffer[16], addressBuffer[16], baseBuffer[16];
  network.GetBytes (networkBuffer);
  address.GetBytes (addressBuffer);
  for (uint8_t i = 0; i < 16; i++)
    {
      baseBuffer[i] = networkBuffer[i] ^ addressBuffer[i];
    }
  Ipv6Address base (baseBuffer);
  std::cout << "Network: " << network << " Address: " << address << " Base: " << base << std::endl;
  return 0;
}
