#include "ns3/core-module.h"
#include "ns3/internet-module.h"

using namespace ns3;

int main ()
{
  Ipv6Prefix prefix (64);
  Ipv6AddressHelper1 h1, h2, h3;
  for (uint8_t i = 0; i < 10; i++)
    std::cout << h1.NewAddress () << std::endl;
  h2.SetBase ("fe::", prefix);
  std::cout << h2.NewAddress () << std::endl;
  std::cout << h1.NewAddress () << std::endl;
  h1.NewNetwork ();
  std::cout << h2.NewAddress () << std::endl;
  std::cout << h1.NewAddress () << std::endl;
  h1 = Ipv6AddressHelper1 ("ab::", prefix);
  std::cout << h1.NewAddress () << std::endl;
  h3.SetBase ("abcd::", Ipv6Prefix (64));
  h3.NewNetwork ();
  std::cout << h3.NewAddress () << std::endl;
  std::cout << h3.NewAddress () << std::endl;
  h3.NewNetwork ();
  std::cout << h3.NewAddress () << std::endl;
  std::cout << h3.NewAddress () << std::endl;
  return 0;
}
