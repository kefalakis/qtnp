/**
 * @file /src/tnp_update.cpp
 *
 * @brief Triangulation and planning functions
 *
 * @date May 2016
 *
 * @author Fotis Balampanis fbalaban@cs.teiath.gr
 **/

/*****************************************************************************
** Includes
*****************************************************************************/

#include <ros/ros.h>

#include "../include/qtnp/tnp_update.hpp"
#include "../include/qtnp/utilities.hpp"

#include "qtnp/InitialCoordinates.h"
#include "qtnp/Coordinates.h"
#include "qtnp/Placemarks.h"

#include "mavros_msgs/Waypoint.h"
#include "mavros_msgs/WaypointList.h"

struct comp
{
    comp(int const& i) : _i(i) { }

    bool operator () (std::pair<int, int> const& p)
    {
        return (p.first == _i);
    }

    int _i;
};


inline double round( double val )
{
    if( val < 0 ) return ceil(val - 0.5);
    return floor(val + 0.5);
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

bool list_contains(int id, std::vector<std::pair<int,int> > list){

    std::vector<std::pair<int,int> >::iterator it = std::find_if(list.begin(), list.end(), comp(id));
    if (it != list.end()) return true;
    else return false;
}

void clear_list(std::vector<std::pair<int,int> > &list){

    for (int i=0; i<list.size(); i++){
        if (list[i].second == 0) {
            list.erase(list.begin()+i);
        }
    }
}

std::pair<int,int>& get_agent_pair_by_id(int id, std::vector<std::pair<int,int> > &map){

    std::vector<std::pair<int,int> >::iterator it = std::find_if(map.begin(), map.end(), comp(id));
    if (it != map.end()) return *it;
}

namespace qtnp {

/*****************************************************************************
** Implementation
*****************************************************************************/

    int Tnp_update::coordinates_to_cdt_cell_id(double lat, double lon){

        std::vector<std::pair<int, geometry_msgs::Point> > id_center_list(this->rviz_objects_ref.get_center_points_with_cell_id());

        // TODO: they are upside down. if test file is correct, change lat, lon
        double cdt_lat = utilities::convert_range(this->area_extremes.min_lat,this->area_extremes.max_lat,
                                    constants::rviz_range_min,constants::rviz_range_max,lon);//here
        double cdt_lon = utilities::convert_range(this->area_extremes.min_lon,this->area_extremes.max_lon,
                                    constants::rviz_range_min,constants::rviz_range_max,lat); // and here

        kernel_Point_2 initial_position(cdt_lat, cdt_lon);

        int result_id(0);
        int comparison_id(0);

        //FIXME refactor conversion below, its a spaggeti shit
        for (int j=0; j<id_center_list.size(); j++){

            CGAL::Comparison_result result =
                    CGAL::compare_distance_to_point(
                        initial_position,
                          utilities::ros_to_cgal_point(id_center_list[j].second),
                            utilities::ros_to_cgal_point(id_center_list[j+1].second));

            if (result == CGAL::SMALLER){
                if (CGAL::compare_distance_to_point(
                            initial_position,
                              utilities::ros_to_cgal_point(id_center_list[j].second),
                                utilities::ros_to_cgal_point(id_center_list[comparison_id].second)) == CGAL::SMALLER){
                    comparison_id = j;
                    result_id = id_center_list[j].first;
                } else {
                    comparison_id = comparison_id;
                    result_id = id_center_list[comparison_id].first;
                }
            } else {
                if (CGAL::compare_distance_to_point(
                            initial_position,
                              utilities::ros_to_cgal_point(id_center_list[j].second),
                                utilities::ros_to_cgal_point(id_center_list[comparison_id].second)) == CGAL::SMALLER){
                    comparison_id = j;
                    result_id = id_center_list[j].first;
                } else {
                    comparison_id = comparison_id;
                    result_id = id_center_list[comparison_id].first;
                }

            }
        }
        return result_id;
    }

    std::vector<int> Tnp_update::find_path(int a, int b){

        std::vector<int> path;
        std::vector<int> blocked_path;
        bool found(false);
        path.push_back(a);

        do {
            found = false;
            for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
              faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
                if (faces_iterator->info().agent_id == path[path.size()-1]){
                    for (int i=0; i<3; i++){
                        if (faces_iterator->neighbor(i)->info().agent_id == b) {
                            path.push_back(b);
                            return path;
                        } else if ( (faces_iterator->neighbor(i)->is_in_domain()) &&
                                !(std::find(path.begin(), path.end(), faces_iterator->neighbor(i)->info().agent_id) != path.end()) &&
                                !(std::find(blocked_path.begin(), blocked_path.end(), faces_iterator->neighbor(i)->info().agent_id) != blocked_path.end() ) ){
                            path.push_back(faces_iterator->neighbor(i)->info().agent_id);
                            found = true;
                            break;
                        }
                    }
                }
                if (found) break;
            }
            if (!found){
                blocked_path.push_back(path[path.size()-1]);
                path.erase(std::remove(path.begin(), path.end(), path[path.size()-1]), path.end());
            }
        } while (true);
    }

    std::vector<int> Tnp_update::count_agent_cells(){

        std::vector<int> cells_per_agent;

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
          faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            if (faces_iterator->is_in_domain()){
                int agent = faces_iterator->info().agent_id;
                if (agent > cells_per_agent.size()) {
                   for( int i=0; i<= (agent - cells_per_agent.size()); i++){
                    cells_per_agent.push_back(0);
                    }
                }
                cells_per_agent[agent] = cells_per_agent[agent] + 1;
            }
        }

        return cells_per_agent;
    }

    bool Tnp_update::are_neighbors (int a, int b){
        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
          faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            for (int i=0; i<3; i++){
                if ( (faces_iterator->info().agent_id == a) && (faces_iterator->neighbor(i)->info().agent_id == b) ) return true;
            }
        }
        return false;
    }

    int Tnp_update::find_neighbor(std::vector<int> &move_path, std::vector<int> &dead_end){

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
          faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            if (faces_iterator->info().agent_id == move_path[move_path.size()-1]){
                for (int j=0; j<3; j++){
                    if (dead_end.empty()){
                        if (faces_iterator->neighbor(j)->is_in_domain() &&
                            !(std::find(move_path.begin(), move_path.end(), faces_iterator->neighbor(j)->info().agent_id) != move_path.end()) &&
                                faces_iterator->neighbor(j)->info().agent_id != faces_iterator->info().agent_id)
                            return faces_iterator->neighbor(j)->info().agent_id;
                } else if ( !(std::find(dead_end.begin(), dead_end.end(), faces_iterator->neighbor(j)->info().agent_id) != dead_end.end()) &&
                            !(std::find(move_path.begin(), move_path.end(), faces_iterator->neighbor(j)->info().agent_id) != move_path.end()) &&
                             faces_iterator->neighbor(j)->is_in_domain()) {
    //                        &&
    //                        !(faces_iterator->neighbor(j)->info().agent_id != faces_iterator->info().agent_id) ){
                        return faces_iterator->neighbor(j)->info().agent_id;
                    }
                }
            }
        }
        dead_end.push_back(move_path[move_path.size()-1]);
        move_path.erase(std::remove(move_path.begin(), move_path.end(), move_path[move_path.size()-1]), move_path.end());
        return -1;
    }

    void Tnp_update::move_cells(std::pair<int, int> &mapA, std::pair<int,int> &mapB, std::vector<int> path){

        if (abs(mapA.second) <= abs(mapB.second)){
            //move(mapA.second, path);
            moveCOV(mapA.second, path);
            mapB.second = mapB.second - abs(mapA.second);
            mapA.second = 0;
        } else {
            //move(mapB.second, path);
            moveCOV(mapB.second, path);
            mapA.second = mapA.second - abs(mapB.second);
            mapB.second = 0;
        }
    }

    void Tnp_update::clear_aux(){
        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
          faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            faces_iterator->info().aux = false;
        }
    }

    int Tnp_update::count_adjacent_cells(int from_agent, int to_agent){
        int cells(0);
        clear_aux();
        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
                      faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            for (int i=0; i<3; i++){
                if ( (faces_iterator->info().agent_id == from_agent) &&
                     (faces_iterator->neighbor(i)->info().agent_id == to_agent) &&
                     (!faces_iterator->neighbor(i)->info().aux) ) {
                    faces_iterator->neighbor(i)->info().aux = true;
                    cells++;
                }
            }
        }
        clear_aux();
        return cells;
    }

    int Tnp_update::get_max_coverage_depth_against_other(int from_agent, int to_agent){

        int coverage_depth(0);
        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
                      faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            if ( (faces_iterator->info().agent_id == from_agent) &&
                 (faces_iterator->info().coverage_depth > coverage_depth) ) {
                for (int i=0; i<3; i++){
                    if(faces_iterator->neighbor(i)->info().agent_id == to_agent){
                        coverage_depth = faces_iterator->info().coverage_depth;
                    }
                }
            }
        }
        return coverage_depth;
    }

    int Tnp_update::get_lowest_coverage_depth_against_other(int from_agent, int to_agent){

        int current_coverage_depth = get_max_coverage_depth_against_other(from_agent, to_agent);

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
                      faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            if (faces_iterator->info().agent_id == from_agent) {
                for (int z=0; z<3; z++){
                    if ( (faces_iterator->neighbor(z)->info().agent_id == to_agent) &&
                         (faces_iterator->info().coverage_depth < current_coverage_depth) ){
                        current_coverage_depth = faces_iterator->info().coverage_depth;
                    }
                }
            }
        }
        return current_coverage_depth;
    }

    void Tnp_update::exchange_agent_on_border_cells(int from_agent, int to_agent, int cells){

        int current_coverage_depth = get_lowest_coverage_depth_against_other(from_agent, to_agent);
        bool not_inside(false);

        for (int i=0; i< cells; i++){
            not_inside = false;
            for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
              faces_iterator != cdt.finite_faces_end(); ++faces_iterator){

                if ( (faces_iterator->info().agent_id == from_agent) &&
                     (faces_iterator->info().coverage_depth == current_coverage_depth)){
                    for (int j=0; j<3; j++){
                        if ( (faces_iterator->neighbor(j)->info().agent_id == to_agent) && (i < cells) ){
                            faces_iterator->neighbor(j)->info().agent_id = from_agent;
                            faces_iterator->neighbor(j)->info().depth = faces_iterator->info().depth + 1;
                            faces_iterator->neighbor(j)->info().coverage_depth = current_coverage_depth + 10;
                            i++;
                            not_inside = true;
                        }
                    }
                }
            }
            if (!not_inside) {
                current_coverage_depth = get_lowest_coverage_depth_against_other(from_agent, to_agent);
                i--;
            }
        }
    }

    void Tnp_update::moveCOV(int cells, std::vector<int> path){

        std::cout << "the path: [";
        for (int i=0; i<path.size(); i++){
            std::cout << path[i] << " ";
        }
        std::cout << "]" << std::endl;

        for (int i=0; i<path.size() -1; i++){

            int adjacent_cells = count_adjacent_cells(path[i], path[i+1]);
            if (adjacent_cells >= cells){
                exchange_agent_on_border_cells(path[i], path[i+1], cells);
                std::cout << "COV: tried to move " << cells << " cells from agent " << path[i+1] << " to agent " << path[i] << std::endl;
            } else {
                std::vector<int> rest_of_the_path;
                for (int j = i; j < path.size(); j++) {
                    rest_of_the_path.push_back(path[j]);
                }
                std::cout << "]" << std::endl;
                moveCOV(adjacent_cells, rest_of_the_path);
                cells = cells - adjacent_cells;
                std::cout << "COV: tried to move " << adjacent_cells << " ADJACENT CELLS from agent " << path[i+1] << " to agent " << path[i] << std::endl;
                std::cout << "COV: Those: " << cells << " cells remained " << std::endl;
                i--;
            }
        }
    }

    int Tnp_update::move(int cells, std::vector<int> path){

        clear_aux();
        int cells_remaining = cells;
        std::cout << "the path: [";
        for (int i=0; i<path.size(); i++){
            std::cout << path[i] << " ";
        }
        std::cout << "]" << std::endl;

        for (int i=0; i<path.size() -1; i++){

            cells_remaining = cells;
            bool found = false;
            int depth(-1); // FIXME magic number, chooses the closest to the agent. should find a better way. if the furthest is
            // chosen also creates some problems, but probably less

            int that_depth_id(0);

            for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
              faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
              if (faces_iterator->info().agent_id == path[i]) {
                for (int z=0; z<3; z++){
                  if (faces_iterator->neighbor(z)->info().agent_id == path[i+1]){
                   if (faces_iterator->neighbor(z)->info().depth > depth){
                        that_depth_id = faces_iterator->neighbor(z)->info().id;
                        depth = faces_iterator->neighbor(z)->info().depth;
                        found = true;
                   }
                  }
                }
              }
            }

            if (found){
                for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
                  faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
                    if (faces_iterator->info().id == that_depth_id){
                        faces_iterator->info().agent_id = path[i];
                        faces_iterator->info().aux = true;
                        cells_remaining--;
                        break;
                    }
                }
            } else {
                path.erase(std::remove(path.begin(), path.end(), path[i+1]), path.end());
                i--;
                continue;
            }

            for (int j=1; j<cells; j++){
                for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
                  faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
                    if ( (faces_iterator->info().agent_id == path[i]) && (faces_iterator->info().aux == true) ){
                        for (int z=0; z<3; z++){
                            if (faces_iterator->neighbor(z)->info().agent_id == path[i+1] && cells_remaining > 0){
                                faces_iterator->neighbor(z)->info().agent_id = path[i];
                                faces_iterator->neighbor(z)->info().aux = true;
                                faces_iterator->info().aux = false;
                                j++;
                                cells_remaining--;
                            }
                        }
                    }
                }
            }

            clear_aux();

            std::cout << "tried to move " << cells << " cells from agent " << path[i+1] << " to agent " << path[i] << std::endl;
            std::cout << cells_remaining << " cells remained somewhere.." << std::endl;
            if (cells_remaining > 0) {
                std::vector<int> remaining_vector;
                remaining_vector.push_back(path[i]);
                if (are_neighbors(path[i],path[i+1])){
                    remaining_vector.push_back(path[i+1]);
                    cells_remaining = move(cells_remaining, remaining_vector);
                }
                if (cells_remaining > 0){
                    std::cout << "these cells remain:" << cells_remaining << std::endl;

                      std::vector<int> new_path = find_path(path[i], path[i+1]);
                      cells_remaining = move(cells_remaining, new_path);
                }
            }
        }
        return cells_remaining;
    }

    // reference constructor
    //Tnp_update::Tnp_update(Rviz_objects& rvizReference) : rviz_objects_ref(rvizReference){}

    void Tnp_update::init(){

        cdt.clear();
        cdt_polygon_edges.clear();
        rviz_objects_ref.clear_edges();
        rviz_objects_ref.clear_center_points();
        rviz_objects_ref.clear_center_points_with_cell_id();

        area_extremes.max_lat = -constants::max_lat;
        area_extremes.min_lat = -constants::min_lat;
        area_extremes.max_lon = -constants::max_lon;
        area_extremes.min_lon = -constants::min_lon;
    }

    // custom callback function of the ROS listener for polygpn definition
    void Tnp_update::polygon_def_callback(const Placemarks::ConstPtr &msg){

        std::vector<Coordinates> placemarks_array = msg->placemarks;

        // for service calls, performs cdt with default angle, edge constrains
        perform_polygon_definition(placemarks_array, constants::angle_criterion_default, constants::edge_criterion_default);
    }

    // TODO transform edge size to rviz size
    void Tnp_update::perform_polygon_definition(std::vector<Coordinates> placemarks_array, double angle_cons, double edge_cons){

        ROS_INFO_STREAM("Got a new polygon definition");

        init();
        std::cout << std::setprecision(7);

        // define minimum and maximum values of the constrained area so to convert lat,lon to visualization ranges
        for (std::vector<Coordinates>::iterator it = placemarks_array.begin(); it<placemarks_array.end(); it++){

            int size = it->latitude.size();
            std::vector<double> latitude_array(size);
            std::vector<double> longitude_array(size);

            latitude_array  = it->latitude;
            longitude_array = it->longitude;

            if (it->placemark_type == "constrain"){
                for (int i=0; i<size; i++){
                    area_extremes.max_lat = latitude_array[i] > area_extremes.max_lat ? latitude_array[i] : area_extremes.max_lat;
                    area_extremes.max_lon = longitude_array[i] > area_extremes.max_lon ? longitude_array[i] : area_extremes.max_lon;

                    area_extremes.min_lat = latitude_array[i] < area_extremes.min_lat ? latitude_array[i] : area_extremes.min_lat;
                    area_extremes.min_lon = longitude_array[i] < area_extremes.min_lon ? longitude_array[i] : area_extremes.min_lon;
                }
            }
        }

        std::list<CDT::Point> list_of_seeds;
        // convert ranges, draw CDT and visualization objects
        for (std::vector<qtnp::Coordinates>::iterator it = placemarks_array.begin(); it<placemarks_array.end(); it++){

            int size = it->longitude.size();
            std::vector<double> longitude_array(size);
            std::vector<double> latitude_array(size);
            bool is_an_obstacle = (it->placemark_type == "hole") ? true :false;

            if (is_an_obstacle) list_of_seeds.push_back(CDT::Point(
                        utilities::convert_range(area_extremes.min_lat,area_extremes.max_lat,
                                    constants::rviz_range_min,constants::rviz_range_max,it->seed_latitude),
                        utilities::convert_range(area_extremes.min_lon,area_extremes.max_lon,
                                    constants::rviz_range_min,constants::rviz_range_max,it->seed_longitude)));

            longitude_array = it->longitude;
            latitude_array  = it->latitude;

            for (int i=1; i<size; i++){

                double previous_latitude = utilities::convert_range(area_extremes.min_lat,area_extremes.max_lat,
                                                                    constants::rviz_range_min,constants::rviz_range_max,latitude_array[i-1]);
                double previous_longitude = utilities::convert_range(area_extremes.min_lon,area_extremes.max_lon,
                                                                     constants::rviz_range_min,constants::rviz_range_max,longitude_array[i-1]);
                double current_latitude = utilities::convert_range(area_extremes.min_lat,area_extremes.max_lat,
                                                                   constants::rviz_range_min,constants::rviz_range_max,latitude_array[i]);
                double current_longitude = utilities::convert_range(area_extremes.min_lon,area_extremes.max_lon,
                                                                    constants::rviz_range_min,constants::rviz_range_max,longitude_array[i]);

                // inserting the area definition vertexes by drawing points and connecting them
                CDT::Vertex_handle va = cdt.insert(CDT::Point(previous_latitude,previous_longitude));
                CDT::Vertex_handle vb = cdt.insert(CDT::Point(current_latitude,current_longitude));
                cdt.insert_constraint(va,vb);

                // draw only a polygon for contrained area, not for obstacles.
                if (!is_an_obstacle){
                    // adding the previous point
                    cdt_polygon_edges.push_back
                            (kernel_Point_2(previous_latitude,previous_longitude));
                    // also adding it to the referenced rviz edge visualization
                    rviz_objects_ref.push_edge_point
                            (utilities::cgal_point_to_ros_geometry_point(kernel_Point_2(previous_latitude,previous_longitude)));
                    // adding the current point
                    cdt_polygon_edges.push_back
                            (kernel_Point_2(current_latitude,current_longitude));
                    // and this also to the rviz objects
                    rviz_objects_ref.push_polygon_point
                            (utilities::point_to_point_32(utilities::cgal_point_to_ros_geometry_point
                                               (kernel_Point_2(current_latitude,current_longitude))));
                }

            }
        }

        // TODO: seperate rest of function
        // FIXME: edge constrain is in cgal points that doesn't correspond to meters.
        // depending on max values, transform given value
        double crAngle = angle_cons;// 0.125; -- the default angle criteria
        double crEdge = edge_cons; // 25.0; -- the default edge criteria(50m footprint) (it's the number given/500 (the max rviz range))
        std::cout << "Number of vertices before meshing and refining: " << cdt.number_of_vertices() << std::endl;
        std::cout << "Meshing the triangulation with default criteria..." << std::endl;
        Mesher mesher(cdt);
        mesher.refine_mesh();
        std::cout << "Number of vertices after meshing: " << cdt.number_of_vertices() << std::endl;
        std::cout << "Meshing again with new criteria..." << std::endl;

        mesher.set_criteria(Criteria(crAngle, crEdge));
        mesher.refine_mesh();
        std::cout << "Number of vertices after meshing and refining with new criteria: "
                << cdt.number_of_vertices() << std::endl;

        CGAL::lloyd_optimize_mesh_2(cdt,
          CGAL::parameters::max_iteration_number = 20); // TODO hardcoded, put in ui

        //  Adding the seeds which define the holes.
        if (!list_of_seeds.empty()){
            std::cout << "Refining and meshing the domain including seeds defining holes" << std::endl;
            CGAL::refine_Delaunay_mesh_2(cdt, list_of_seeds.begin(), list_of_seeds.end(), Criteria());
            std::cout << "Number of vertices after meshing CDT refining and seeding holes: " << cdt.number_of_vertices() << std::endl;
        }

        std::cout << "Number of vertices AFTER LLOYD (20 iterations): " << cdt.number_of_vertices() << std::endl;

        // ------------- rviz coloring schema ----------------//
        // TODO center (waypoints) coloring should go to coloring function.
        // unfortunately in the same function we also use the center points for further operations before the coloring.
        // So waypoints (centers) should be distinguished from their coloring cousins.

        int initialize_iterator = 0;

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
            faces_iterator != cdt.finite_faces_end(); ++faces_iterator){

          if (faces_iterator->is_in_domain()){

            // initialize face, along with it's id. TODO remove it from partition (initialize_mesh function)
            faces_iterator->info().initialize(initialize_iterator);

            // create a point for each of the edges of the face.
            CDT::Point point1 = cdt.triangle(faces_iterator)[0];
            CDT::Point point2 = cdt.triangle(faces_iterator)[1];
            CDT::Point point3 = cdt.triangle(faces_iterator)[2];
            geometry_msgs::Point center = utilities::face_points_to_center(point1, point2, point3);
            // adding it's respective lat and lon to its struct
            // TODO rviz_range_min and max should not be constants e.g. 500x500 because in conversion,
            // if area is not square like it will create a distortion in visualization. They should be proportional (good luck)
            faces_iterator->info().center_lat = utilities::convert_range(constants::rviz_range_min,constants::rviz_range_max,
                                                                         area_extremes.min_lat,area_extremes.max_lat,center.x);
            faces_iterator->info().center_lon = utilities::convert_range(constants::rviz_range_min,constants::rviz_range_max,
                                                                         area_extremes.min_lon,area_extremes.max_lon,center.y);
            // adding the center of every triangle to rviz
            rviz_objects_ref.push_center_point(center);

            // adding it also to the center-id vector
            rviz_objects_ref.push_center_point_with_cell_id(initialize_iterator,center);

            initialize_iterator++;

          }
        }

        rviz_objects_ref.set_polygon_ready(true);
    }

    // FIXME: DEPRECATED custom callback function of the ROS listener for path planning
    void Tnp_update::path_planning_callback(const InitialCoordinates::ConstPtr &msg){

        std::cout << std::setprecision(7);
        int uav_id = (int) msg->uav_id;
        double longitude = (double) msg->longitude;
        double latitude = (double) msg->latitude;
        // coverage or target
        // if target, cell of target

        ROS_INFO("I heard UAV id: [%i]", uav_id);
        ROS_INFO("I heard the longitude: [%3.7f]", longitude);
        ROS_INFO("I heard the latitude: [%3.7f]", latitude);

        // TODO: in that way you make use of the referenced visualization objects in main ROS node
        // rviz_objects_ref.set_polygon_ready(false);

        // TODO: initializing with hard coded cell numbers...
        // initialize_mesh(cdt);
        // hop_cost_attribution(cdt);
        // coverage_cost_attribution(cdt);

        // 695 is a good target triangle for use with agent 1
        int target_face_number = 595;

        // applying either shortest path to target or coverage algorithms for path production
        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
            faces_iterator != cdt.finite_faces_end(); ++faces_iterator){

          CDT::Face_handle face = faces_iterator;

          if ((face->is_in_domain()) && (face->info().agent_id == uav_id) && (face->info().depth == 1)){
            //TODO: introduce also not over holes in complete coverage-shortest distance
            //complete_path_coverage(cdt, face, uav_id);
            //shortest_path_coverage(cdt, face, uav_id, target_face_number);
            break;
          }
        }
    }

    void Tnp_update::partition(std::vector<std::pair< std::pair<double,double> , int > >  uas_coords_with_percentage){

        rviz_objects_ref.clear_triangulation_mesh();
        int uas_count = uas_coords_with_percentage.size();
        int total_cdt_cells = rviz_objects_ref.count_cells();

        std::vector< std::pair<int,int> > id_cell_count_vector;
        std::vector<int> initial_positions_cell_ids;

        int jumps_ad = 1;

        for (int i=0; i<uas_count; i++){

            int result_id = coordinates_to_cdt_cell_id(uas_coords_with_percentage[i].first.first,
                                                       uas_coords_with_percentage[i].first.second);
            initial_positions_cell_ids.push_back(result_id);

            int cells_for_agent =  ( (uas_coords_with_percentage[i].second * total_cdt_cells) / 100.0) + 0.5 ;
             // +1 for the agent id, -1 for calculating the initial cell
            id_cell_count_vector.push_back(std::pair<int,int>(i+1,cells_for_agent-1));

        }

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
            faces_iterator != cdt.finite_faces_end(); ++faces_iterator){

          if (faces_iterator->is_in_domain()){

            if (std::find(initial_positions_cell_ids.begin(),
                          initial_positions_cell_ids.end(),
                          faces_iterator->info().id) != initial_positions_cell_ids.end()){
              faces_iterator->info().numbered = true;
              faces_iterator->info().depth = 1;
              faces_iterator->info().agent_id = std::find(initial_positions_cell_ids.begin(), initial_positions_cell_ids.end(), faces_iterator->info().id) - initial_positions_cell_ids.begin() + 1;
              for (int j=0; j<3; j++){
                  faces_iterator->neighbor(j)->info().jumps_agent_id = jumps_ad;
                  jumps_ad++;
              }
            }
          }else {
            faces_iterator->info().agent_id = -1;
          }
        }

        // hop cost/partitioning, passing autonomy percentage table
        hop_cost_attribution(id_cell_count_vector);
        coverage_cost_attribution();

        std::vector<int> cells_per_agent = count_agent_cells();
        for (int i=0; i<cells_per_agent.size(); i++){
            std::cout << "agent " << i << " has " << cells_per_agent[i] << " cells." << std::endl;
        }
        mesh_coloring();
    }

    void Tnp_update::hop_cost_attribution(std::vector< std::pair<int,int> > id_cell_count){

        // TODO: seperate hop cost from agent attribution. agent attribution is valid
        // for all tasks and its operations don't have to be repeated or missing..
        std::cout << "-----Beginning jump cost------" << std::endl;

        bool neverInside = false;
        int jumpsIterator = 1;

        // TODO: refactor: hop cost in seprate function, referencing id_cell_count vector. replenishing algo should work only
        // with one list, with positive and negative values not with two lists including agent_id = 0s.
        do {
          jumpsIterator++; // including non domain triangles

          neverInside = true;

          for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
              faces_iterator != cdt.finite_faces_end(); ++faces_iterator){

            // too many comparisons. we need to refactor the algorithm
            if ((faces_iterator->is_in_domain()) && (faces_iterator->info().has_number())
                && !(faces_iterator->info().is_visited()) && !(faces_iterator->info().depth == jumpsIterator)) {

              int that_agent = faces_iterator->info().agent_id;
              // check if that agent has fulfilled his need for cells according to its autonomy percentage
              std::vector<std::pair <int,int> >::iterator it =
                      std::find_if(id_cell_count.begin(), id_cell_count.end(), comp(that_agent));

              neverInside = false;
              faces_iterator->info().visited = true;

              for (int i=0; i<3; i++){

                if ((faces_iterator->neighbor(i)->is_in_domain()) && !(faces_iterator->neighbor(i)->info().has_number())) {

                  if (it->second != 0){

                      // assign jumpers id in order to see which growing function has managed
                      // to reach the end or target.
                      if (faces_iterator->info().depth != 1){
                         faces_iterator->neighbor(i)->info().jumps_agent_id = faces_iterator->info().jumps_agent_id;
                      }
                      faces_iterator->neighbor(i)->info().depth = jumpsIterator;
                      faces_iterator->neighbor(i)->info().numbered = true;
                      // agent id propagation
                      faces_iterator->neighbor(i)->info().agent_id = faces_iterator->info().agent_id;
                      // reducing the cells appointed
                      it->second = it->second -1;
                  }
                }
              }
            }
          }
         } while (neverInside == false);


        // count cells and agent assigned cells
        std::vector<std::pair<int,int> > number_of_assigned_cells(id_cell_count.size() + 1);
        int total_cells(0);
        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
        faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
              if (faces_iterator->is_in_domain()) {
                total_cells++;
                number_of_assigned_cells[faces_iterator->info().agent_id].first = faces_iterator->info().agent_id;
                number_of_assigned_cells[faces_iterator->info().agent_id].second += 1;
          }
        }

        std::cout << "Total cells: " << total_cells << std::endl;
        std::cout << "Assigned cells: " << std::endl;
        for (int i=0; i< id_cell_count.size() + 1; i++){
        std::cout << "agent " << i << ": " << number_of_assigned_cells[i].second << std::endl;
        }
        std::cout << "Remaining cells: " << std::endl;
        for (int i=0; i< id_cell_count.size(); i++){
        std::cout << "agent " << id_cell_count[i].first << ": " << id_cell_count[i].second << std::endl;
        }

        coverage_cost_attribution();

        // FIXME replenishing algorithm // could be refactored
        std::vector<std::pair<int,int> > cell_map;
        std::vector<std::pair<int,int> > map_agent_missing_cells;
        std::vector<std::pair<int,int> > map_agent_surplus_cells;

        cell_map.push_back(std::pair<int,int>(0,number_of_assigned_cells[0].second));
        for (int i=0; i< id_cell_count.size() + 1; i++){
          if (id_cell_count[i].second > 0) map_agent_missing_cells.push_back(std::pair<int,int>(id_cell_count[i].first, id_cell_count[i].second-1));
          cell_map.push_back(std::pair<int,int>(id_cell_count[i].first, - id_cell_count[i].second));
            std::cout << "agent " << cell_map[i].first << " has: " << cell_map[i].second << " cells" << std::endl;
        }

        if (number_of_assigned_cells[0].second > 0) map_agent_surplus_cells.push_back(std::pair<int,int>(0,number_of_assigned_cells[0].second));

        std::vector<int> move_path;
        std::vector<int> dead_end;

        if (number_of_assigned_cells[0].second > 0){

            do {
                // REFACTORING
              // take agent which misses. is agent where cell_map.second is below zero
              int agent_missing = map_agent_missing_cells[0].first;
              if (move_path.empty()) move_path.push_back(agent_missing);
              int current_neighbor_id = find_neighbor(move_path, dead_end); // this find_neighbor, checks if neighbor is already in move_path or dead_end

              // check if the current neighbor is in the list of agents that have a cell surplus
              if (list_contains(current_neighbor_id, map_agent_surplus_cells)){
                  std::pair<int,int> &from_agent = get_agent_pair_by_id(current_neighbor_id, map_agent_surplus_cells);
                  move_path.push_back(current_neighbor_id);
                  move_cells(map_agent_missing_cells[0], from_agent, move_path); // (found = true) ?
                  clear_list(map_agent_missing_cells);
                  clear_list(map_agent_surplus_cells);
                  move_path.clear();
                  dead_end.clear();
              } else if ( (current_neighbor_id != -1) &&
                         !(std::find(move_path.begin(), move_path.end(), current_neighbor_id) != move_path.end()) &&
                          !(std::find(dead_end.begin(), dead_end.end(), current_neighbor_id) != dead_end.end()) ){
                  move_path.push_back(current_neighbor_id); // found = true ?
              } else {
                  dead_end.push_back(current_neighbor_id);
                  move_path.erase(std::remove(move_path.begin(), move_path.end(), current_neighbor_id), move_path.end());
                  current_neighbor_id = move_path[move_path.size() -1];
              }
            } while (!map_agent_missing_cells.empty() && !map_agent_surplus_cells.empty());
        }
        // ENDOF replenishing algorithm

        // initializing again depth and number var in order to perform again hop cost (after replenishing algo)
        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
            faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
                if (faces_iterator->is_in_domain()) {
                    faces_iterator->info().visited = false;
                        if (faces_iterator->info().depth != 1){
                            faces_iterator->info().depth = 0;
                            faces_iterator->info().numbered = false;
                        }
                }
        }

        // performing again hop cost with the moved cells.
        bool finished = false;
        int hopIterator = 1;

        do {
            hopIterator++; // including non domain triangles

            finished = true;

            for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
                faces_iterator != cdt.finite_faces_end(); ++faces_iterator){

                // too many comparisons. we need to refactor the algorithm
                if ((faces_iterator->is_in_domain()) && (faces_iterator->info().has_number())
                    && !(faces_iterator->info().is_visited()) && !(faces_iterator->info().depth == hopIterator)) {
                    finished = false;
                    faces_iterator->info().visited = true;
                    for (int i=0; i<3; i++){
                        if ((faces_iterator->neighbor(i)->is_in_domain()) &&
                                (faces_iterator->neighbor(i)->info().agent_id == faces_iterator->info().agent_id) &&
                                !(faces_iterator->neighbor(i)->info().has_number())) {
                            // assign jumpers id in order to see which growing function has managed
                            // to reach the end or target.
                            if (faces_iterator->info().depth != 1){
                                faces_iterator->neighbor(i)->info().jumps_agent_id = faces_iterator->info().jumps_agent_id;
                            }
                            faces_iterator->neighbor(i)->info().depth = hopIterator;
                            faces_iterator->neighbor(i)->info().numbered = true;
                        }
                    }
                }
            }
        } while (!finished);

        // -------- END OF JUMP COST ALGORITHM -------------------//
    }

    // TODO: color depending on UI decision: hop depth, coverage depth etc
    void Tnp_update::mesh_coloring(){

        int color_iterator = 0;

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
            faces_iterator != cdt.finite_faces_end(); ++faces_iterator){

          color_iterator += 1.0;
          // for every face, we need a face handle to perform the various operations.
          CDT::Face_handle face = faces_iterator;

          // if this face is in the domain, meaning inside the contrained borders but outside the defined holes
          if (face->is_in_domain()){

            // create a point for each of the edges of the face.
            CDT::Point point1 = cdt.triangle(face)[0];
            CDT::Point point2 = cdt.triangle(face)[1];
            CDT::Point point3 = cdt.triangle(face)[2];

            double face_depth = rviz_objects_ref.get_settings().task_cost ? face->info().depth : face->info().coverage_depth;
            float z = -face_depth;

            // each triangle in rviz mesh need three points..
            rviz_objects_ref.push_mesh_point(utilities::cgal_triangulation_point_to_ros_geometry_point(point1, z));
            rviz_objects_ref.push_mesh_point(utilities::cgal_triangulation_point_to_ros_geometry_point(point2, z));
            rviz_objects_ref.push_mesh_point(utilities::cgal_triangulation_point_to_ros_geometry_point(point3, z));

            std_msgs::ColorRGBA triangle_color;
            triangle_color.a = 1.0f;// + (face_depth/900.0);


            //NOTE: agent coloring for partition viz
            if (rviz_objects_ref.get_settings().partition){
                if (face->info().agent_id == 1){
                    triangle_color.r = 0.0f + 100.0;
                    triangle_color.b = 0.0f;
                    triangle_color.g = 0.0f;
                }
                if (face->info().agent_id == 2){
                    triangle_color.r = 0.0f;
                    triangle_color.b = 0.0f + 100.0;
                    triangle_color.g = 0.0f;
                }
                if (face->info().agent_id == 3){
                    triangle_color.r = 0.0f;
                    triangle_color.b = 0.0f;
                    triangle_color.g = 0.0f + 100.0;
                }
            }

            // NOTE: hop cost depth coloring
            if (rviz_objects_ref.get_settings().task_cost){
                triangle_color.r = 0.0f + (face_depth/100.0) +0.02f + (face->info().agent_id*2);// + (the_agent/5.0);
                triangle_color.b = 0.0f + (face_depth/100.0) +0.02f + (face->info().agent_id*2);// + (the_agent/5.0);// + (face->info().depth/45.0);//color_iterator*2.50/100;
                triangle_color.g = 0.0f + (face_depth/100.0) +0.05f+ (face->info().agent_id*2);// + (the_agent/5.0);// + (face_depth/75.0);//color_iterator*8.0/100;
            }

            // NOTE: coverage depth coloring
            if (rviz_objects_ref.get_settings().coverage_cost){
                triangle_color.r = 0.0f + (face_depth/900.0) +0.02f + (face->info().agent_id*2);// + (the_agent/5.0);
                triangle_color.b = 0.0f + (face_depth/900.0)+0.02f + (face->info().agent_id*2);// + (the_agent/5.0);// + (face->info().depth/45.0);//color_iterator*2.50/100;
                triangle_color.g = 0.0f + (face_depth/900.0)+0.05f+ (face->info().agent_id*2);// + (the_agent/5.0);// + (face_depth/75.0);//color_iterator*8.0/100;
            }

            // NOTE: borders coloring
            if (rviz_objects_ref.get_settings().borders){
                if (face->info().coverage_depth == constants::coverage_depth_max){
                    triangle_color.g = 0.7f;
                }
            }
            // NOTE: initial positions are white
            if (face->info().depth == 1){ // also include target coloring
              triangle_color.r = 1.0f;// + (face->info().depth/30.0);
              triangle_color.g = 1.0f;// + (face->info().depth/50.0);//color_iterator*2.50/100;
              triangle_color.b = 1.0f;// + (face->info().depth/60.0);//color_iterator*8.0/100;
            }
            rviz_objects_ref.push_mesh_cell_color(triangle_color);
          }
        }
    }

    // put pair<int, <pair<double, double> > for uas number and lat,lon
    void Tnp_update::path_planning_coverage(std::pair<int, std::pair<double,double> > uas){

        coverage_cost_attribution();
        complete_path_coverage(uas);
        mesh_coloring();
        rviz_objects_ref.set_planning_ready(true) ;
    }

    void Tnp_update::path_planning_to_goal(int uas, double lat, double lon){

        rviz_objects_ref.clear_path();
        path_to_goal(uas, coordinates_to_cdt_cell_id(lat,lon) );
        mesh_coloring();
        rviz_objects_ref.set_planning_ready(true) ;

    }

    // TODO: make it go backwards
    void Tnp_update::path_to_goal(int uas, int goal_cell_id){

        CDT::Face_handle current_face;

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
            faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            // TODO make uas_model class. make current_cell_id member var inside and take it in situations like this
            if( (faces_iterator->info().agent_id == uas) && (faces_iterator->info().depth == 1) ){
                current_face = faces_iterator;
                rviz_objects_ref.push_path_point(utilities::build_pose_stamped(utilities::face_to_center(cdt, current_face)));
                break;
            }
        }


      // TODO: missing initialization function
      CDT::Face_handle target_face;
      // TODO: missing initialization function
      geometry_msgs::Point target_face_center;
      int target_face_depth = 0;

      // put it in the path
      rviz_objects_ref.push_path_point(utilities::build_pose_stamped(utilities::face_to_center(cdt, current_face)));

      Distance_Vector distance_vector;

      // get target face
      for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
          faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
        if (faces_iterator->info().id == goal_cell_id){
          target_face = faces_iterator;
          target_face_center = utilities::face_to_center(cdt, target_face);
          target_face_depth = target_face->info().depth;
          // if it happens our target to be at the borders, we temporaly change its depth
          if (target_face_depth == constants::coverage_depth_max){
            int previous = 0;
              for (int i=0; i<3; i++){
                if (target_face->neighbor(i)->is_in_domain()){
                    target_face_depth = (target_face->neighbor(i)->info().depth > previous) ?
                                target_face->neighbor(i)->info().depth : previous;
                    previous = target_face->neighbor(i)->info().depth;
                }
            }
          }
          break;
        }
      }

      float previous_distance = utilities::calculate_distance(
                  (utilities::face_to_center(cdt, current_face)) , target_face_center);
      std::cout << "Initial distance from start: " << previous_distance << std::endl;
      int depth_runs = 1;
      int branch_id = target_face->info().jumps_agent_id;

      do {

        depth_runs+=4;

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin(); faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
          if ( (faces_iterator->info().agent_id == uas) &&
               (faces_iterator->info().depth < depth_runs) &&
               (faces_iterator->info().jumps_agent_id == branch_id) ){

            float distance = utilities::calculate_distance(utilities::face_to_center(cdt, faces_iterator), target_face_center);
            Distance_Entry this_entry = std::make_pair(faces_iterator, distance);
            distance_vector.push_back(this_entry);
          }
        }

        std::sort(distance_vector.begin(), distance_vector.end(), utilities::distance_comparison);
        // put the nearer to path
        rviz_objects_ref.push_path_point(utilities::build_pose_stamped
                                         (utilities::face_to_center(cdt, distance_vector.front().first)));
        distance_vector.clear();


      }while (depth_runs < target_face_depth);

    }

    void Tnp_update::coverage_cost_attribution(){

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
        faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            faces_iterator->info().coverage_depth = 0;
        }
      std::cout << "----Beginning complete coverage cost attribution----" << std::endl;

      // go through all triangles to give border depth to the borders between agents
      for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
      faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
          // initialize again the path visited attribute
          for (int i=0;i<3;i++){
              if ( (faces_iterator->neighbor(i)->info().agent_id != faces_iterator->info().agent_id) ){
                faces_iterator->info().coverage_depth = constants::coverage_depth_max;
                faces_iterator->info().cover_depth = true;
              }
          }
      }

      int so_many = 0;
      int da_coverage_depth = constants::coverage_depth_max;
      bool never_ever_again = true;
      do {
          da_coverage_depth = da_coverage_depth - 10;
          never_ever_again = true;
          for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
              faces_iterator != cdt.finite_faces_end(); ++faces_iterator){

              if((faces_iterator->info().has_coverage_depth()) && (faces_iterator->info().coverage_depth > da_coverage_depth) ){
                  for (int i=0;i<3;i++){
                      if (!faces_iterator->neighbor(i)->info().has_coverage_depth()){
                          faces_iterator->neighbor(i)->info().coverage_depth = da_coverage_depth;
                          faces_iterator->neighbor(i)->info().cover_depth = true;
                          never_ever_again = false;
                          so_many++;
                      }
                  }
              }

          }
      } while (!never_ever_again);
    }

    // TODO: make starter face a static and remove double reference in body
    void Tnp_update::complete_path_coverage(std::pair<int, std::pair<double, double> > uas){
        int uas_id = uas.first;

        std::cout << "----Beginning complete coverage for agent : " << uas_id << "----" << std::endl;

        // clearing the path object in case it had a previous path
        rviz_objects_ref.clear_path();
        std::vector< std::pair<double, double> > coord_path;

        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
            faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            faces_iterator->info().reset_path_visited();
        };

        CDT::Face_handle initial_cell;
        for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
            faces_iterator != cdt.finite_faces_end(); ++faces_iterator){
            if ( (faces_iterator->info().agent_id == uas_id) && (faces_iterator->info().depth == 1) ) {
                initial_cell = faces_iterator;
                break;
            }
        };

        CDT::Face_handle &starter_cell = initial_cell;
        Face_Handle_Vector borders_vector;
        Distance_Vector borders_distance_vector;
        int current_depth = constants::coverage_depth_max;
        int smallest_depth = constants::coverage_depth_max - 1;
        bool not_finished = true;
        bool initial = true;

        starter_cell->info().path_visited = true;
        rviz_objects_ref.push_path_point(utilities::build_pose_stamped
                                         (utilities::face_to_center(cdt, starter_cell)));
        // lat, lon
        coord_path.push_back(std::pair<double, double>(starter_cell->info().center_lat, starter_cell->info().center_lon));

        do {

            not_finished = false;

            // go through all triangles, get the starter cell and the borders vector
            for(CDT::Finite_faces_iterator faces_iterator = cdt.finite_faces_begin();
                faces_iterator != cdt.finite_faces_end(); ++faces_iterator){

                if ( (faces_iterator->info().coverage_depth >= current_depth)
                     && (faces_iterator->info().agent_id == uas_id)
                     && (!faces_iterator->info().is_path_visited())) {

                    borders_vector.push_back(faces_iterator);
                    not_finished = true;
                }

                if (initial){
                    if (faces_iterator->info().coverage_depth < smallest_depth){
                        smallest_depth = faces_iterator->info().coverage_depth;
                    }
                }
            }

            initial = false;

            if (!not_finished){
                current_depth = current_depth - 10;
            } else {

                // calculate the distance from all borders to the starter cell in order to choose the first border cell to visit
                for (std::vector<CDT::Face_handle>::iterator it = borders_vector.begin(); it != borders_vector.end(); it++){
                    float this_distance = utilities::calculate_distance(utilities::face_to_center(cdt, *it),
                                                             utilities::face_to_center(cdt, starter_cell));
                    Distance_Entry this_entry = std::make_pair(*it, this_distance);
                    borders_distance_vector.push_back(this_entry);
                }

                std::sort(borders_distance_vector.begin(), borders_distance_vector.end(), utilities::distance_comparison);
                // and this is the closest
                CDT::Face_handle &first_of_the_border = borders_distance_vector.front().first;

                // an o geitonas tou starter_cell, diladi toy proigoymenoy vimatos, pou einai pio konta
                // ston first of the border, den exei ton firstOfTHeBor ws geitona,
                // tote vale ayton ton geitona sto path, kanton visited an den einai,
                // valton ws starter cell kai epanelave

                rviz_objects_ref.push_path_point(utilities::build_pose_stamped
                                                 (utilities::face_to_center(cdt, first_of_the_border)));
                first_of_the_border->info().path_visited = true;
                coord_path.push_back(std::pair<double, double>(first_of_the_border->info().center_lat,
                                                               first_of_the_border->info().center_lon));

                starter_cell = first_of_the_border;
            }
            borders_vector.clear();
            borders_distance_vector.clear();
        } while (current_depth >= smallest_depth);


        // TODO: prepei na to kanoyme na min pidaei...
        std::cout << "----Finished complete coverage ----" << std::endl;
        make_mavros_waypoint_list(uas.second, coord_path);
    }


    void Tnp_update::make_mavros_waypoint_list(std::pair<double, double> uas_coords,
                                               std::vector<std::pair<double, double> > path){ // also put uas_id

        mavros_msgs::Waypoint initialWaypoint;
        mavros_msgs::WaypointList &waypoint_list = m_waypoint_list;

        waypoint_list.waypoints.clear();

        double initialLatitude = uas_coords.first;// path.poses[0].position.y;
        double initialLongitude = uas_coords.second; // of the uas agent according to initial position by ui (or later, current)

        std::cout << "Center list: " << std::endl;
        std::cout << std::fixed << std::setprecision(8) << "lat: " << initialLatitude << " lon: " << initialLongitude << std::endl;
        // this is for first initial position // maybe need to change frame, put it global (0)
        initialWaypoint.frame = 3;
        initialWaypoint.command = 16;
        initialWaypoint.is_current = true;
        initialWaypoint.autocontinue = true;
        initialWaypoint.param1 = 0;
        initialWaypoint.param2 = 0;
        initialWaypoint.param3 = 0;
        initialWaypoint.param4 = 0;
        initialWaypoint.x_lat = round(initialLatitude*100000000.0)/100000000.0;
        initialWaypoint.y_long = round(initialLongitude*100000000.0)/100000000.0;
        initialWaypoint.z_alt = 285; // 0? for initial relevant altitude
        waypoint_list.waypoints.push_back(initialWaypoint);

        // -------------------------------------------------//
        // file generation
        int sequence = 1;

        std::stringstream mavlink_filename;
        std::string data_path = "/home/fotis/Dev/Data/Missions/";
        mavlink_filename << data_path << "mavlink_plan_" << currentDateTime() << ".txt";
        const std::string& tmp = mavlink_filename.str();
        const char* cstr = tmp.c_str();
        std::ofstream mavlink_fWPPlan(cstr);
        mavlink_fWPPlan << "QGC WPL 110" << std::endl;

        mavlink_fWPPlan << "0\t1\t0\t16\t0\t0\t0\t0\t" << std::fixed << std::setprecision(7) << initialWaypoint.x_lat << "\t"
                    << std::fixed <<  std::setprecision(7) << initialWaypoint.y_long << "\t585\t1" << std::endl;
        // -------------------------------------------------//


        bool initial = true;

        // begin() +1 ?
        for (std::vector<std::pair<double, double> >::iterator it = path.begin(); it != path.end(); it++){

            mavros_msgs::Waypoint waypoint;

            if (initial){

                // this is for take off
                waypoint.frame = 3;
                waypoint.command = 22;
                waypoint.is_current = false;
                waypoint.autocontinue = true;
                waypoint.param1 = 15;
                waypoint.param2 = 0;
                waypoint.param3 = 0;
                waypoint.param4 = 0;
                waypoint.x_lat = round( (it->second) *100000000.0)/100000000.0;
                waypoint.y_long = round( (it->first)*100000000.0)/100000000.0;
                waypoint.z_alt = 100; // 100? for takeoff
                waypoint_list.waypoints.push_back(waypoint);
                initial = false;
                std::cout << std::fixed << std::setprecision(8) << " lat: " << it->second << " lon: " << it->first << std::endl;

                //--------------------------------//
                // file generation //
                mavlink_fWPPlan << "1\t0\t3\t22\t15\t0\t0\t0\t" << std::fixed << std::setprecision(7) << waypoint.x_lat << "\t"
                        << std::fixed << std::setprecision(7) << waypoint.y_long << "\t100\t1" << std::endl;
                //--------------------------------//

            } else {

                // this is for path:
                waypoint.frame = 3;
                waypoint.command = 16;
                waypoint.is_current = false;
                waypoint.autocontinue = true;
                waypoint.param1 = 0;
                waypoint.param2 = 0;
                waypoint.param3 = 0;
                waypoint.param4 = 0;
                waypoint.x_lat = round( (it->second) *100000000.0)/100000000.0;
                waypoint.y_long = round( (it->first)*100000000.0)/100000000.0;
                waypoint.z_alt = 100; // 100? for takeoff
                waypoint_list.waypoints.push_back(waypoint);
                std::cout << std::fixed << std::setprecision(8) << " lat: " << it->second << " lon: " << it->first << std::endl;

                //-----------------------------------//
                // file generation //
                mavlink_fWPPlan << sequence << "\t0\t3\t16\t0\t0\t0\t0\t" << std::fixed << std::setprecision(7) << waypoint.x_lat << "\t"
                        << std::fixed << std::setprecision(7) << waypoint.y_long << "\t100\t1" << std::endl;
                sequence++;
                //-----------------------------------//

            }
        }

        mavros_msgs::Waypoint waypoint;
        // this is for landing
        waypoint.frame = 3;
        waypoint.command = 21;
        waypoint.is_current = false;
        waypoint.autocontinue = true;
        waypoint.param1 = 480;
        waypoint.param2 = 0;
        waypoint.param3 = 0;
        waypoint.param4 = 25;
        waypoint.x_lat = round(initialLatitude*100000000.0)/100000000.0;
        waypoint.y_long = round(initialLongitude*100000000.0)/100000000.0;
        waypoint.z_alt = 580; // 100? for takeoff
        waypoint_list.waypoints.push_back(waypoint);
        std::cout << std::fixed << std::setprecision(8) << initialLatitude << " " << initialLongitude << std::endl;


        //--------------------------------//
        // file generation //
        mavlink_fWPPlan << sequence << "\t0\t3\t21\t480\t0\t0\t25\t" << std::fixed << std::setprecision(7) << waypoint.x_lat << "\t"
                << std::fixed << std::setprecision(7) << waypoint.y_long << "\t580\t1"; // << std::endl;
        mavlink_fWPPlan.close();
        //--------------------------------//

    }

}
