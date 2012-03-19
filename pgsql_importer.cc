#include <libpq-fe.h>

#include <iostream>
#include <pqxx/pqxx>

#include <boost/mpl/vector.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include "pgsql_importer.hh"

namespace Tempus
{

    ProgressionCallback null_progression_callback;

    PQImporter::PQImporter( const std::string& pg_options ) :
	connection_( pg_options )
    {
    }
    
    ///
    /// Function used to import the road and public transport graphs from a PostgreSQL database.
    void PQImporter::import_graph( MultimodalGraph& graph, ProgressionCallback& progression )
    {
	Road::Graph& road_graph = graph.road;
	PublicTransport::Graph pt_graph_;
	graph.public_transports.push_back( pt_graph_ );
	PublicTransport::Graph& pt_graph = graph.public_transports.back();

	// locally maps db ID to Node or Section
	std::map<Tempus::db_id_t, Road::Vertex> road_nodes_map;
	std::map<Tempus::db_id_t, Road::Edge> road_sections_map;
	std::map<Tempus::db_id_t, PublicTransport::Vertex> pt_nodes_map;

	pqxx::work w( connection_ );
	
	pqxx::result res = w.exec( "SELECT id, junction, bifurcation FROM tempus.road_node" );
	
	for ( pqxx::result::size_type i = 0; i < res.size(); i++ )
	{
	    Road::Node node;
	    
	    node.db_id = res[i][0].as<int>();
	    node.is_junction = res[i][1].as<bool>();
	    node.is_bifurcation = res[i][2].as<bool>();
	    
	    Road::Vertex v = boost::add_vertex( node, road_graph );
	    
	    road_nodes_map[ node.db_id ] = v;
	    road_graph[v].vertex = v;
	    
	    progression( static_cast<float>((i + 0.) / res.size() / 4.0) );
	}
	
	std::string query = "SELECT id, road_type, node_from, node_to, transport_type_ft, transport_type_tf, length, car_speed_limit, "
	    "car_average_speed, bus_average_speed, road_name, address_left_side, address_right_side, lane, "
	    "roundabout, bridge, tunnel, ramp, tollway FROM tempus.road_section";
	res = w.exec( query );
	
	for ( pqxx::result::size_type i = 0; i < res.size(); i++ )
	{
	    Road::Section section;
	    
	    res[i][0] >> section.db_id;
	    if ( !res[i][1].is_null() )
		section.road_type = static_cast<RoadType>(res[i][1].as<int>());
	    
	    int node_from_id = res[i][2].as<int>();
	    int node_to_id = res[i][3].as<int>();
	    
	    int j = 4;
	    res[i][j++] >> section.transport_type_ft;
	    res[i][j++] >> section.transport_type_tf;
	    res[i][j++] >> section.length;
	    res[i][j++] >> section.car_speed_limit;
	    res[i][j++] >> section.car_average_speed;
	    res[i][j++] >> section.bus_average_speed;
	    res[i][j++] >> section.road_name;
	    res[i][j++] >> section.address_left_side;
	    res[i][j++] >> section.address_right_side;
	    res[i][j++] >> section.lane;
	    res[i][j++] >> section.is_roundabout;
	    res[i][j++] >> section.is_bridge;
	    res[i][j++] >> section.is_tunnel;
	    res[i][j++] >> section.is_ramp;
	    res[i][j++] >> section.is_tollway;	    

	    Road::Vertex v_from = road_nodes_map[ node_from_id ];
	    Road::Vertex v_to = road_nodes_map[ node_to_id ];

	    Road::Edge e;
	    bool is_added;
	    boost::tie( e, is_added ) = boost::add_edge( v_from, v_to, section, road_graph );
	    road_graph[e].edge = e;
	    road_sections_map[ section.db_id ] = e;

	    progression( static_cast<float>(((i + 0.) / res.size() / 4.0) + 0.25) );
	}

	res = w.exec( "SELECT id, name, location_type, parent_station, road_section_id, zone_id, abscissa_road_section FROM tempus.pt_stop" );
	
	for ( pqxx::result::size_type i = 0; i < res.size(); i++ )
	{
	    PublicTransport::Stop stop;

	    int j = 0;
	    res[i][j++] >> stop.db_id;
	    res[i][j++] >> stop.name;
	    res[i][j++] >> stop.is_station;

	    stop.has_parent = false;
	    int parent_station;
	    res[i][j++] >> parent_station;
	    if ( pt_nodes_map.find( parent_station ) != pt_nodes_map.end() )
	    {
		stop.parent_station = pt_nodes_map[ parent_station ];
		stop.has_parent = true;
	    }

	    int road_section;
	    res[i][j++] >> road_section;
	    stop.road_section = road_sections_map[ road_section ];

	    res[i][j++] >> stop.zone_id;
	    res[i][j++] >> stop.abscissa_road_section;

	    PublicTransport::Vertex v = boost::add_vertex( stop, pt_graph );
	    pt_nodes_map[ stop.db_id ] = v;
	    pt_graph[v].vertex = v;

	    progression( static_cast<float>(((i + 0.) / res.size() / 4.0) + 0.5) );
	}

	res = w.exec( "SELECT stop_from, stop_to FROM tempus.pt_section" );
	
	for ( pqxx::result::size_type i = 0; i < res.size(); i++ )
	{
	    PublicTransport::Section section;

	    int stop_from_id, stop_to_id;
	    res[i][0] >> stop_from_id;
	    res[i][1] >> stop_to_id;

	    PublicTransport::Vertex stop_from = pt_nodes_map[ stop_from_id ];
	    PublicTransport::Vertex stop_to = pt_nodes_map[ stop_to_id ];
	    PublicTransport::Edge e;
	    bool is_added;
	    boost::tie( e, is_added ) = boost::add_edge( stop_from, stop_to, pt_graph );
	    pt_graph[e].edge = e;

	    progression( static_cast<float>(((i + 0.) / res.size() / 4.0) + 0.75) );
	}

	progression( 1.0, /* finished = */ true );

    }

};
