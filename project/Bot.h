#pragma once
#include "Exam_HelperStructs.h"

class IExamInterface;
namespace Elite
{
	class IDecisionMaking;
	class BehaviorTree;
	class Blackboard;

	class Bot final
	{
	public:
		explicit Bot(IExamInterface* pInterface);

		void SetHouseInfoVector(const std::vector<HouseInfo>& houseInfoVector);
		void SetEntityInfoVector(const std::vector<EntityInfo>& entityInfoVector);

		void SetSteeringTarget(SteeringPlugin_Output* steering);

		void Update(float dt);
	private:
		Blackboard* CreateBlackboard();

		Blackboard* m_pBlackboard{};

		std::vector<HouseInfo> m_HouseInfoVector{};
		std::vector<EntityInfo> m_EntityInfoVector{};
		IExamInterface* m_IExamInterface{};

		std::deque<EntityInfo> m_ItemsToVisit{};

		std::vector<Vector2> m_VisitedHouseCenters{};
		std::deque<Vector2> m_HouseCentersToVisit{};

		std::deque<Vector2> m_WanderPointsVector{};
		// this bool will reset the m_VisitedHouseCenters vector if true
		// (once every wander loop around the map)
		bool m_HasFinishedWorldPatrol = false;

		bool m_IsRunning = false;

		float m_TimeStuck{};
		float m_DeltaTime{};

		float m_TimeSinceLastPurgeSeen{};
		Vector2 m_PurgeFleeLocation{};

		// time spent searching to 360 when damaged
		float m_TimeSpentSearching{};

		IDecisionMaking* m_pDecisionMaking;
	};
}

