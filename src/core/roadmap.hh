/**
 *   Copyright (C) 2012-2013 IFSTTAR (http://www.ifsttar.fr)
 *   Copyright (C) 2012-2013 Oslandia <infos@oslandia.com>
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Library General Public
 *   License as published by the Free Software Foundation; either
 *   version 2 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Library General Public License for more details.
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEMPUS_ROADMAP_HH
#define TEMPUS_ROADMAP_HH

#include <map>
#include <boost/ptr_container/ptr_vector.hpp>

#include "common.hh"
#include "path_trace.hh"

namespace Tempus {
/**
   A Roadmap is an object used to model steps involved in a multimodal route.
   It is a base for result values of a request.

   Some of the fields may stay empty and will be filled by a request on the
   auxiliary storage (db), since there is no need for them to be always present
   in memory (geometries, names, etc.). They are marked as "retrieved from db"
*/
class Roadmap {
public:
    ///
    /// A Step is a part of a route, where the transport type is constant
    /// This a generic class
    class Step {
    public:
        enum StepType {
            RoadStep,
            PublicTransportStep,
            TransferStep
        };
        DECLARE_RW_PROPERTY( step_type, StepType );

        DECLARE_RO_PROPERTY( costs, Costs );
        /// Gets a cost
        double cost( CostId id ) const;
        /// Sets a cost
        void set_cost( CostId id, double c );

        /// (Initial) transport mode id
        DECLARE_RW_PROPERTY( transport_mode, db_id_t );

        /// Geometry of the step, described as a WKB, for visualization purpose
        /// Retrieved from the db
        DECLARE_RW_PROPERTY( geometry_wkb, std::string );

        Step( StepType type ) : step_type_( type ) {}

        virtual Step* clone() const {
            return new Step( *this );
        }
    };

    ///
    /// A Step that occurs on the road, either by a pedestrian or a private vehicle
    /// If the path goes along the same road (same name) in the same "direction",
    /// there is no need to store one step for each edge, they can be merged.
    struct RoadStep : public Step {
        RoadStep() : Step( Step::RoadStep ) {}

        ///
        /// The road section where to start from
        DECLARE_RW_PROPERTY( road_edge_id, db_id_t );

        /// Name of the road - retrieved from the db
        DECLARE_RW_PROPERTY( road_name, std::string );

        ///
        /// Distance to walk/drive (in km). -1 if we have to go until the end of the section
        ///
        DECLARE_RW_PROPERTY( distance_km, double );

        /// The movement that is to be done at the end of the section
        enum EndMovement {
            GoAhead,
            TurnLeft,
            TurnRight,
            UTurn,
            RoundAboutEnter,
            FirstExit, ///< in a roundabout
            SecondExit,
            ThirdExit,
            FourthExit,
            FifthExit,
            SixthExit,
            YouAreArrived = 999
        };
        DECLARE_RW_PROPERTY( end_movement, EndMovement );

        virtual RoadStep* clone() const {
            return new RoadStep( *this );
        }
    };

    ///
    /// A Step made with a public transport
    ///
    /// For a trip from station A to station C that passes through the station B on the same trip_id
    /// only one step is stored
    struct PublicTransportStep : public Step {
        PublicTransportStep() : Step( Step::PublicTransportStep ), wait_(0.0) {}

        ///
        /// Public transport network id
        DECLARE_RW_PROPERTY( network_id, db_id_t );

        ///
        /// Wait time at this step (in min)
        DECLARE_RW_PROPERTY( wait, double );
        /// Departure time
        DECLARE_RW_PROPERTY( departure_time, double );
        /// Arrival time
        DECLARE_RW_PROPERTY( arrival_time, double );

        /// Of which trip this step is part of
        DECLARE_RW_PROPERTY( trip_id, db_id_t );
        /// PT stop on where to depart
        DECLARE_RW_PROPERTY( departure_stop, db_id_t );
        /// departure stop name
        DECLARE_RW_PROPERTY( departure_name, std::string );
        /// PT stop on where to arrive
        DECLARE_RW_PROPERTY( arrival_stop, db_id_t );
        /// arrival stop name
        DECLARE_RW_PROPERTY( arrival_name, std::string );

        /// Name of the route - retrieved from the db
        DECLARE_RW_PROPERTY( route, std::string );

        virtual PublicTransportStep* clone() const {
            return new PublicTransportStep( *this );
        }
    };

    ///
    /// A generic step from a vertex to another
    /// This is used to represent a step from a mode to another (road, public transport, poi, etc)
    struct TransferStep : public Step, MMEdge {
        TransferStep( const MMVertex& v1, const MMVertex& v2 ) : Step( Step::TransferStep ), MMEdge( v1, v2 ) {}

        /// Final transport mode id
        DECLARE_RW_PROPERTY( final_mode, db_id_t );

        /// Name of the first part of the transfer - retrieved from the db
        DECLARE_RW_PROPERTY( initial_name, std::string );

        /// Name of the second part of the transfer - retrieved from the db
        DECLARE_RW_PROPERTY( final_name, std::string );

        virtual TransferStep* clone() const {
            return new TransferStep( *this );
        }
    };

    typedef boost::ptr_vector<Step> StepList;
    typedef StepList::iterator StepIterator;
    typedef StepList::const_iterator StepConstIterator;

    /// Read-only access to steps, begin iterator
    StepConstIterator begin() const;
    /// Write access to steps
    StepIterator begin();

    /// Read-only access to steps, end iterator
    StepConstIterator end() const;
    /// Write access to steps
    StepIterator end();

    /// Random access to a given step, with bound checking
    const Step& step( size_t idx ) const;

    /// Add a step
    void add_step( std::auto_ptr<Step> step );

    /// Starting date time
    DECLARE_RW_PROPERTY( starting_date_time, DateTime );

    /// A path trace, for debugging purpose
    DECLARE_RW_PROPERTY( trace, PathTrace );

private:
    StepList steps_;
};

///
/// Convenience function to compute the sum of costs for a roadmap
Costs get_total_costs( const Roadmap& );

///
/// A Result is a list of Roadmap, ordered by relevance towards optimizing criteria
typedef std::list<Roadmap> Result;

///
/// Used internally by boost::ptr_vector when copying
inline Roadmap::Step* new_clone( const Roadmap::Step& step )
{
    return step.clone();
}

} // Tempus namespace

#endif
