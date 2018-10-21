// Path.cpp

#include "Main.hpp"
#include "TileWorld.hpp"

#include <queue>

#include "Engine.hpp"
#include "Path.hpp"

PathFinder::PathFinder(TileWorld& world) :
	world(world)
{
	//
}

PathFinder::~PathFinder()
{
	// wait for tasks to finish
	for (auto& task : tasks) {
		if (task.valid()) {
			task.wait();
		}
	}
	tasks.clear();
}

void PathFinder::generateSimpleMap() {
	mapWidth = world.getWidth();
	mapHeight = world.getHeight();
	map.resize(mapWidth * mapHeight);

	for (size_t index = 0; index < map.getSize(); ++index) {
		map[index] = world.getTiles()[index].hasVolume() ? 1 : 0;
	}
}

std::future<PathFinder::Path*> PathFinder::generateAStarPath(Sint32 startX, Sint32 startY, Sint32 endX, Sint32 endY) {
	if (map.getSize() == 0) {
		generateSimpleMap();
	}

	if (map.getSize() == 0) {
		mainEngine->fmsg(Engine::MSG_WARN, "Pathfinder is returning an invalid path future due to failing to generate simple map!");
		return std::future<Path*>(); // return invalid future
	}

	if (startX < 0 || startY < 0 || endX < 0 || endY < 0 || startX > (Sint32)world.getWidth() || startY > (Sint32)world.getHeight() || endX > (Sint32)world.getWidth() || endY > (Sint32)world.getHeight()) {
		mainEngine->fmsg(Engine::MSG_WARN, "Pathfinder is returning an invalid path future due to invalid pathing bounds!");
		return std::future<Path*>(); // return invalid future
	}
	
	AStarTask task(map, world.getWidth(), world.getHeight(), startX, startY, endX, endY);
	return std::async(std::launch::async, task);
}

PathFinder::Path* PathFinder::AStarTask::findPath()
{
	LinkedList<PathFinder::PathNode*> openList, closedList;

	PathNode startnode(nullptr, std::min(std::max(0, startX), (Sint32)mapWidth - 1),
						std::min(std::max(0, startY), (Sint32)mapHeight - 1), nullptr, 1);
	PathNode endnode(nullptr, std::min(std::max(0, endX), (Sint32)mapWidth - 1),
						std::min(std::max(0, endY), (Sint32)mapHeight - 1), nullptr, 1);
	startnode.h = heuristic(startnode.x, startnode.y, endnode.x, endnode.y);

	// final path
	std::priority_queue<PathNode*> visitList;
	Path* path = new Path;

	if (startnode == endnode)
	{
		return path;
	}

	// create starting node in list
	PathNode* currentNode = new PathNode(&openList, startnode.x, startnode.y, nullptr, 0, startnode.h, startnode.g);
	visitList.push(currentNode);
	int tries = 0;
	bool foundPath = false;
	while ( openList.getSize() > 0 && tries < MAX_TRIES && visitList.size() > 0 )
	{
		currentNode = visitList.top();
		//mainEngine->fmsg(Engine::MSG_WARN, "Pathfinder current node coords: (%d, %d)!", currentNode->x, currentNode->y);
		visitList.pop();
		if ( *currentNode == endnode )
		{
			mainEngine->fmsg(Engine::MSG_WARN, "Pathfinder just hit the endnode!");
			foundPath = true;
			break;
		}

		// Move the current pathnode from the openList to the close list.
		PathNode *movedNode = new PathNode(&closedList, currentNode->x, currentNode->y, currentNode->parent, 1, currentNode->h, currentNode->g);
		openList.removeNode(currentNode->node);
		delete currentNode;
		currentNode = movedNode;

		// expand search to neighbor tiles
		for ( Sint32 y = -1; y <= 1; y++ ) {
			for ( Sint32 x = -1; x <= 1; x++ ) {
				if ( x == 0 && y == 0 ) {
					continue; //Don't check yourself :)
				}
				if (currentNode->x <= 0 && x < 0)
				{
					continue; //Don't look off the left edge of the map.
				}
				if (currentNode->x >= (Sint32)mapWidth - 1 && x > 0)
				{
					continue; //Don't look off the right edge of the map.
				}
				if (currentNode->y <= 0 && y < 0)
				{
					continue; //Don't look off the top edge of the map.
				}
				if (currentNode->y >= (Sint32)mapHeight - 1 && y > 0)
				{
					continue; //Don't look off the bottom edge of the map.
				}

                // check neighboring tiles are not obstacles
                if ( !map[(currentNode->y + y) + (currentNode->x + x)*mapHeight] ) {
                    continue;
                } else if ( x && y ) {
                    if ( !map[(currentNode->y) + (currentNode->x + x)*mapHeight] ) {
                        continue;
                    }
                    if ( !map[(currentNode->y + y) + (currentNode->x)*mapHeight] ) {
                        continue;
                    }
                }

                bool alreadyAdded = false;
                for ( auto &pathNode : closedList ) {
                    if ( pathNode->x == currentNode->x + x && pathNode->y == currentNode->y + y ) {
                        alreadyAdded = true;
                        break;
                    }
                }

				for ( auto &pathNode : openList ) {
					if ( pathNode->x == currentNode->x + x && pathNode->y == currentNode->y + y ) {
						alreadyAdded = true;
						if ( x && y ) {
							if ( pathNode->g > currentNode->g + COST_DIAGONAL ) {
								pathNode->parent = currentNode;
								pathNode->g = currentNode->g + COST_DIAGONAL;
							}
						} else {
							if ( pathNode->g > currentNode->g + COST_STRAIGHT ) {
								pathNode->parent = currentNode;
								pathNode->g = currentNode->g + COST_STRAIGHT;
							}
						}
						break;
					}
				}
				
                if ( alreadyAdded == false ) {
                    if ( openList.getSize() >= 1000 ) {
						mainEngine->fmsg(Engine::MSG_WARN, "Pathfinder exceeded maximum tries!");
                        return path;
                    }
                    PathNode *childnode = new PathNode(&openList, currentNode->x + x, currentNode->y + y, currentNode, 1);
                    if ( x && y ) {
                        childnode->g = currentNode->g + COST_DIAGONAL;
                    }
                    else {
                        childnode->g = currentNode->g + COST_STRAIGHT;
                    }
					childnode->h = heuristic(childnode->x, childnode->y, endnode.x, endnode.y);
                    visitList.push(childnode);
					//mainEngine->fmsg(Engine::MSG_WARN, "Queueing up pathnode for (%d, %d)", childnode->x, childnode->y);
                }
			}
		}
		++tries;
	}

	// if target is found, retrace path
	if ( true == foundPath ) {
		if (currentNode->parent == nullptr)
		{
			return path; //Already on the goal tile!
		}

		PathNode* parent = currentNode->parent;
		for ( PathNode *childnode = currentNode; childnode != nullptr; childnode = currentNode->parent )
		{
			if ( childnode->parent == nullptr && nullptr != parent )
			{
				parent->parent = nullptr;
				break;
			}
			path->addNodeLast(PathWaypoint(childnode->x, childnode->y));
			currentNode = currentNode->parent;
		}
	}
	else
	{
		mainEngine->fmsg(Engine::MSG_WARN, "Pathfinder could not find path!");
	}

	return path;
}
