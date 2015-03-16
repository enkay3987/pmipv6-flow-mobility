/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Naveen Kamath <cs11m02@iith.ac.in>
 */

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WaypointTest");

const int positionSize = 6;

Waypoint prevWaypoint;
int nextPosition;

Vector3D position[] = {
    Vector3D (0, 270, 0),
    Vector3D (-210, 180, 0),
    Vector3D (-210, -160, 0),
    Vector3D (0, -250, 0),
    Vector3D (230, -160, 0),
    Vector3D (250, 130, 0),
};

double CalcDistance (Vector3D p1, Vector3D p2)
{
  return sqrt (pow (p1.x - p2.x, 2) + pow (p1.y - p2.y, 2) + pow (p1.z - p2.z, 2));
}

void CopyWaypoint (Waypoint &src, Waypoint &dst)
{
  src.position = dst.position;
  src.time = dst.time;
}

void CourseChange (Ptr<Node> node, int v, Ptr<const MobilityModel> mobilityModel)
{
  Ptr<WaypointMobilityModel> waypointMobModel = node->GetObject<WaypointMobilityModel> ();
  Waypoint wp;
  wp.position = position[nextPosition];
  wp.time = Seconds (CalcDistance (position[nextPosition], prevWaypoint.position) / v) + prevWaypoint.time + Simulator::Now ();
  nextPosition = (nextPosition + 1) % 6;
  CopyWaypoint (prevWaypoint, wp);
  NS_LOG_DEBUG ("Prev waypoint: " << wp);
  NS_LOG_UNCOND (nextPosition << " " << wp);
  waypointMobModel->AddWaypoint (wp);
}

int
main (int argc, char *argv[])
{
  double simTime = 40;
  double v = 10;
  int noOfWaypoints = 6;
  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue ("velocity", "Velocity in m/s", v);
  cmd.Parse(argc, argv);

  Ptr<Node> node = CreateObject<Node> ();
  MobilityHelper mobHelper;
  mobHelper.SetMobilityModel ("ns3::WaypointMobilityModel");
  mobHelper.Install (node);

  Ptr<MobilityModel> mobModel = node->GetObject<MobilityModel> ();
  if (mobModel == 0)
    {
      return 0;
    }
  Ptr<WaypointMobilityModel> wpMobModel = DynamicCast<WaypointMobilityModel> (mobModel);
  if (wpMobModel == 0)
    return 0;

  Waypoint wp;
  Time t;
  for (int j = 0; j < noOfWaypoints; j++)
    {
      int i = j % positionSize;
      wp.position = position[i];
      if (j == 0)
        wp.time = Time (Seconds (0));
      else
        t = wp.time = Seconds (CalcDistance (position[i], position[(i + 1) % positionSize]) / v) + t;
      NS_LOG_DEBUG (wp);
      wpMobModel->AddWaypoint (wp);
    }
  CopyWaypoint (prevWaypoint, wp);
  nextPosition = 0;
  wpMobModel->TraceConnectWithoutContext ("CourseChange", MakeBoundCallback (&CourseChange, node, v));

  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
