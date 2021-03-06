/**
 * @file /include/qtnp/rviz_objects.hpp
 *
 * @brief Rviz elements
 *
 * @date May 2016
 *
 * @author Fotis Balampanis fbalaban@cs.teiath.gr
 **/

/*****************************************************************************
** Ifdefs
*****************************************************************************/

#ifndef qtnp_RVIZ_OBJECTS_HPP_
#define qtnp_RVIZ_OBJECTS_HPP_

/*****************************************************************************
** Includes
*****************************************************************************/
#include <ros/ros.h>

#include <geometry_msgs/Polygon.h>
#include <geometry_msgs/PolygonStamped.h>
#include <visualization_msgs/Marker.h>
#include <nav_msgs/Path.h>

/*****************************************************************************
** Namespaces
*****************************************************************************/

namespace qtnp {

/*****************************************************************************
** Class
*****************************************************************************/
struct Rviz_settings {

  bool task_cost;
  bool coverage_cost;
  bool partition;
  bool waypoints;
  bool borders;
};


class Rviz_objects {

public:
    Rviz_objects(){}

    void init();
    bool is_polygon_ready(){ return polygon_ready; }
    bool is_planning_ready(){ return planning_ready; }

    /*****************************************************************************
    ** Getters
    *****************************************************************************/

    Rviz_settings get_settings(){
      return rviz_settings;
    }

    geometry_msgs::Polygon get_polygon(){ return polygon; }
    geometry_msgs::PolygonStamped get_polygonStamped(){

      polygonStamped.polygon = polygon;
      return polygonStamped;

    }
    visualization_msgs::Marker get_edges(){ return edges; }
    visualization_msgs::Marker get_center_points(){ return center_points; }
    visualization_msgs::Marker get_triangulation_mesh(){ return triangulation_mesh; }
    nav_msgs::Path get_path(){ return path; }

    int get_number_of_waypoints(){ return path.poses.size(); }
    std::vector<std::pair<int, geometry_msgs::Point> > get_center_points_with_cell_id(){ return center_points_with_cell_id;}

    const char *get_frame_id(){return this->edges.header.frame_id.c_str();}

    /*****************************************************************************
    ** Setters
    *****************************************************************************/
    void push_edge_point(geometry_msgs::Point point);
    void push_polygon_point(geometry_msgs::Point32 point);
    void push_center_point(geometry_msgs::Point point);
    void push_mesh_point(geometry_msgs::Point point);
    void push_mesh_cell_color(std_msgs::ColorRGBA color);
    void push_path_point(geometry_msgs::PoseStamped point);
    void push_center_point_with_cell_id(int id, geometry_msgs::Point center_point);

    void clear_path();
    void clear_edges();
    void clear_center_points();
    void clear_triangulation_mesh();
    void clear_center_points_with_cell_id();

    void set_settings(bool task, bool coverage, bool partition, bool borders, bool waypoints ){
      rviz_settings.task_cost = task; rviz_settings.coverage_cost = coverage;
      rviz_settings.partition = partition; rviz_settings.waypoints = waypoints; rviz_settings.borders = borders;
    }

    int count_cells(){ return center_points.points.size(); }
    void set_polygon_ready (bool option){ polygon_ready = option; }
    void set_planning_ready (bool option){ planning_ready = option; }

private:
    bool polygon_ready;
    bool planning_ready;

    Rviz_settings rviz_settings;
    geometry_msgs::Polygon polygon;
    geometry_msgs::PolygonStamped polygonStamped;
    visualization_msgs::Marker edges, center_points, triangulation_mesh;
    nav_msgs::Path path;

    std::vector<std::pair<int, geometry_msgs::Point> > center_points_with_cell_id;

};

}  // namespace qtnp

#endif /* qtnp_RVIZ_OBJECTS_HPP_ */
