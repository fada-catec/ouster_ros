#include "ouster_ros/client/lidar_scan.h"

#include <vector>

namespace ouster
{
XYZLut make_xyz_lut(LidarScan::index_t w, LidarScan::index_t h, double range_unit,
                    double lidar_origin_to_beam_origin_mm, const std::vector<double>& azimuth_angles_deg,
                    const std::vector<double>& altitude_angles_deg)
{
   Eigen::ArrayXd azimuth(w * h);
   Eigen::ArrayXd altitude(w * h);
   const double azimuth_radians = M_PI * 2.0 / w;
   for (LidarScan::index_t v = 0; v < w; v++)
   {
      for (LidarScan::index_t u = 0; u < h; u++)
      {
         LidarScan::index_t i = u * w + v;
         azimuth(i)           = azimuth_angles_deg[u] * M_PI / 180.0 + (v + w / 2) * azimuth_radians;
         altitude(i)          = altitude_angles_deg[u] * M_PI / 180.0;
      }
   }
   XYZLut lut;
   lut.direction        = LidarScan::Points{w * h, 3};
   lut.direction.col(0) = altitude.cos() * azimuth.cos();
   lut.direction.col(1) = -altitude.cos() * azimuth.sin();
   lut.direction.col(2) = altitude.sin();

   const double lidar_origin_to_beam_origin_m = lidar_origin_to_beam_origin_mm / 1000.0;

   lut.offset        = LidarScan::Points{w * h, 3};
   lut.offset.col(0) = azimuth.cos() - lut.direction.col(0);
   lut.offset.col(1) = - azimuth.sin() - lut.direction.col(1);
   lut.offset.col(2) = -lut.direction.col(2);               
   lut.offset *= lidar_origin_to_beam_origin_m;

   lut.direction *= range_unit;
   
   return lut;
}

XYZLut make_xyz_lut_mirror(LidarScan::index_t w, LidarScan::index_t h, double range_unit,
                    double lidar_origin_to_beam_origin_mm, const std::vector<double>& azimuth_angles_deg,
                    const std::vector<double>& altitude_angles_deg)
{
   double new_range;
   Eigen::ArrayXd a(w *h);
   Eigen::ArrayXd azimuth(w * h);
   Eigen::ArrayXd altitude(w * h);
   Eigen::ArrayXd new_altitude(w * h);
   const double azimuth_radians = M_PI * 2.0 / w;
   for (LidarScan::index_t v = 0; v < w; v++)
   {
      for (LidarScan::index_t u = 0; u < h; u++)
      {
         // TODO: if altitude < 0, el valor de h es negativo, sacar h y r de sen y cos siendo conocido el angulo?
         LidarScan::index_t i = u * w + v;
         azimuth(i)           = azimuth_angles_deg[u] * M_PI / 180.0 + (v + w / 2) * azimuth_radians;
         altitude(i)          = altitude_angles_deg[u] * M_PI / 180.0;
         a(i)                 = ((0.525 - lidar_origin_to_beam_origin_mm / 1000) * 0.766) / sin(50 - std::abs(altitude(i)));
         if (altitude(i) > 0)
         {
            new_altitude(i)   = altitude(i) + acos(((range_unit - a(i)) * sin(2 * (40 + altitude (i)))) / 
                                sqrt(2 * pow(a(i), 2) + pow(range_unit, 2) - 2 * a(i) * range_unit - 2 * a(i) * (range_unit - a(i) * cos(2 * (40 + altitude(i))))));
         }
         else
         {
            new_altitude(i)   = altitude(i) - acos(((range_unit - a(i)) * sin(2 * (40 + std::abs(altitude (i))))) / 
                                sqrt(2 * pow(a(i), 2) + pow(range_unit, 2) - 2 * a(i) * range_unit - 2 * a(i) * (range_unit - a(i) * cos(2 * (40 + std::abs(altitude(i)))))));
         }
         new_range = sqrt(2 * pow(a(i), 2) + pow(range_unit, 2) - 2 * a(i) * range_unit - 2 * a(i) * (range_unit - a(i) * cos(2 * (40 + std::abs(altitude(i)))))) * cos(new_altitude(i));
      }
   }
   XYZLut lut;
   lut.direction        = LidarScan::Points{w * h, 3};
   lut.direction.col(0) = new_altitude.cos() * azimuth.cos();
   lut.direction.col(1) = -new_altitude.cos() * azimuth.sin();
   lut.direction.col(2) = new_altitude.sin();
   lut.direction *= new_range;

   const double lidar_origin_to_beam_origin_m = lidar_origin_to_beam_origin_mm / 1000.0;

   lut.offset        = LidarScan::Points{w * h, 3};
   lut.offset.col(0) = azimuth.cos() - lut.direction.col(0) * lidar_origin_to_beam_origin_m;
   lut.offset.col(1) = azimuth.sin() - lut.direction.col(0) * lidar_origin_to_beam_origin_m;
   lut.offset.col(2) = Eigen::ArrayXd::Zero(w * h);
   lut.offset *= lidar_origin_to_beam_origin_m;

   return lut;
}

}  // namespace ouster
