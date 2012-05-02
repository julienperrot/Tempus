// Tempus core data structures
// (c) 2012 Oslandia
// MIT License

/**
   A Roadmap is an object used to model steps involved in a multimodal route.
   It is a base for result values of a request.
 */

#ifndef TEMPUS_ROADMAP_HH
#define TEMPUS_ROADMAP_HH

#include <map>

#include "common.hh"

namespace Tempus
{
    class Roadmap
    {
    public:
	///
	/// A Step is a part of a route, where the transport type is constant
	/// This a generic class
	struct Step
	{
	    enum StepType
	    {
		VertexStep = 0,
		RoadStep,
		PublicTransportStep,
	    };
	    StepType step_type;

	    Costs costs;

	    Step( StepType type ) : step_type( type ) {}
	};

	///
	/// A Step that occurs on the road, either by a pedestrian or a private vehicle
	struct RoadStep : public Step
	{
	    RoadStep() : Step( Step::RoadStep ) {}

	    ///
	    /// The road section where to start from
	    Road::Edge road_section;
	    ///
	    /// The road section where to go in the direction of
	    Road::Edge road_direction;
	    ///
	    /// Distance to walk/drive (in km). -1 if we have to go until the end of the section
	    ///
	    double distance_km;
	    /// The movement that is to be done at the end of the section
	    enum EndMovement
	    {
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
	    EndMovement end_movement;
	};

	///
	/// A Step made with a public transport
	struct PublicTransportStep : public Step
	{
	    PublicTransportStep() : Step( Step::PublicTransportStep ) {}

	    db_id_t network_id;
	    PublicTransport::Vertex departure_stop;
	    PublicTransport::Vertex arrival_stop;
	    db_id_t trip_id; ///< used to indicate the direction
	};

	///
	/// A Roadmap is a list of Step augmented with some total costs.
	/// Ownership : pointers are allocated by the caller but freed on Roadmap destruction
	typedef std::vector<Step*> StepList;
	StepList steps;
	Costs total_costs;

	///
	/// Optional overview path, which is designed for display purposes, and may be simplified
	typedef std::vector<Point2D> PointList;
	PointList overview_path;

	virtual ~Roadmap()
	{
	    for ( StepList::iterator it = steps.begin(); it != steps.end(); it++ )
	    {
		delete *it;
	    }
	}
    };

    ///
    /// A Result is a list of Roadmap, ordered by relevance towards optimizing criteria
    typedef std::list<Roadmap> Result;

}; // Tempus namespace

#endif
