// Path.hpp

#pragma once

#include "Main.hpp"
#include "LinkedList.hpp"

#include <future>

class TileWorld;

/*
 * Pathfinder usage flow (after constructing with a valid world):
 * * pathfinder.generateSimpleMap(); //This updates the pathfinder's copy. This only needs to be done after world generation, and if the terrain ever changes.
 * * std::future pathTask = pathfinder.generateAStarPath(x1, y1, x2, y2);
 * Then you just keep querying the pathTask via wait_for with 0 duration
 */

 /*
  * Class PathFinder
  *
  * There is one pathfinder per world. It provides asynchronous pathfinding.
  */
class PathFinder {
protected:
	class PathNode;
public:
	PathFinder(TileWorld& world);
	~PathFinder();

	// Path Waypoint
	class PathWaypoint {
	public:
		PathWaypoint(int x, int y) :
			x(x), y(y) {}
		~PathWaypoint() {}

		bool operator==(const PathNode& rhs) const {
			if (rhs.x == x && rhs.y == y) {
				return true;
			} else {
				return false;
			}
		}

		Sint32 x = 0, y = 0;
	};
	using Path = LinkedList<PathWaypoint>;

	// Asynchronous Path Task (Path process)
	class Task {
	public:
		Task(const ArrayList<Uint32>& _map, Uint32 _mapWidth, Uint32 _mapHeight,
			Sint32 _startX, Sint32 _startY, Sint32 _endX, Sint32 _endY) :
			map(_map),
			mapWidth(_mapWidth),
			mapHeight(_mapHeight),
			startX(_startX),
			startY(_startY),
			endX(_endX),
			endY(_endY) {}
		virtual ~Task() {}

		virtual PathFinder::Path* findPath() = 0;

		Path* operator()() {
			return findPath();
		}

	protected:
		ArrayList<Uint32> map;
		Uint32 mapWidth, mapHeight;
		Sint32 startX, startY;
		Sint32 endX, endY;
	};

	// class Future : pathTask {
	// public:
	// 	virtual bool isDone();
	// 	virtual Path getPath();
	// private:
	// 	std::future<Path> pathFuture;
	// };

	// A* Path Task (derived from above)
	class AStarTask : public Task {
	public:
		static const Uint32 COST_STRAIGHT = 10;
		static const Uint32 COST_DIAGONAL = 14;
		static const Uint32 MAX_TRIES = 10000;

		AStarTask(const ArrayList<Uint32>& _map, Uint32 _mapWidth, Uint32 _mapHeight,
			Sint32 _startX, Sint32 _startY, Sint32 _endX, Sint32 _endY) :
			Task(_map, _mapWidth, _mapHeight, _startX, _startY, _endX, _endY) {}

		Path* findPath() override;

	protected:
		static Uint32 heuristic(Sint32 x1, Sint32 y1, Sint32 x2, Sint32 y2) {
			return (std::abs(x2 - x1) + std::abs(y2 - y1)) * COST_STRAIGHT;
		}
	};

	/*
	 * Creates a PathTask object. Asynchronous pathfinding.
	 *
	 * (You can use wait_for with 0 duration :) )
	 */
	std::future<Path*> generateAStarPath(Sint32 startX, Sint32 startY, Sint32 endX, Sint32 endY);

protected:
	TileWorld& world;

	ArrayList<std::future<PathFinder::Path*>> tasks;

	// Path node
	class PathNode {
	public:
		PathNode(LinkedList<PathNode*>* list, int x, int y, PathNode* parent, int pos, int h = 0, int g = 0) {
			this->list = list;
			this->x = x;
			this->y = y;
			this->parent = parent;
			this->g = g;
			this->h = h;

			if (nullptr != list)
			{
				if (0 == pos) {
					node = list->addNodeFirst(this);
				} else {
					node = list->addNodeLast(this);
				}
			} else
			{
				node = nullptr;
			}
		}
		~PathNode() {}

		PathNode** add(PathNode& child, Uint32 pos);
		PathNode** remove(PathNode& child, Uint32 pos);

		bool operator==(const PathNode& rhs) const {
			if (rhs.x == x && rhs.y == y) {
				return true;
			} else {
				return false;
			}
		}

		PathNode* parent = nullptr;
		Sint32 x = 0, y = 0;
		Uint32 g = 0, h = 0; //These are the A* heuristics. We're using Manhattan Distance.
		LinkedList<PathNode*>* list = nullptr;
		Node<PathNode*>* node = nullptr;
	};

	ArrayList<Uint32> map;
	Uint32 mapWidth = 0;
	Uint32 mapHeight = 0;
	void generateSimpleMap(); //TODO Make sure accessing this result is threadsafe...every thread should probably get a copy of the map on creation.
};

// class pathTask
// {
// public:
// 	virtual bool isDone() = 0;
// 	virtual PathFinder::Path getPath() = 0;
// };
