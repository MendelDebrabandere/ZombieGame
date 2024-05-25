/*=============================================================================*/
// Copyright 2020-2021 Elite Engine
/*=============================================================================*/
// Behaviors.h: Implementation of certain reusable behaviors for the BT version of the Agario Game
/*=============================================================================*/
#ifndef ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
#define ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
//-----------------------------------------------------------------
// Includes & Forward Declarations
//-----------------------------------------------------------------
#include "stdafx.h"

#include "IExamInterface.h"

#include "BehaviorTree.h"

//-----------------------------------------------------------------
// Behaviors
//-----------------------------------------------------------------

namespace BT_Actions
{
	void MoveTowardsPoint(IExamInterface* pInterface, SteeringPlugin_Output* steering, const Elite::Vector2& dest)
	{
		auto agentInfo = pInterface->Agent_GetInfo();
		auto nextTargetPos = pInterface->NavMesh_GetClosestPathPoint(dest);

		//Simple Seek Behaviour (towards Target)
		std::cout << "GOING TO " << nextTargetPos << '\n';
		steering->LinearVelocity = nextTargetPos - agentInfo.Position; //Desired Velocity
		steering->LinearVelocity.Normalize(); //Normalize Desired Velocity
		steering->LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed
	}

	
	Elite::BehaviorState TurnAndShoot(Elite::Blackboard* pBlackboard)
	{
		SteeringPlugin_Output* steering{};
		if (!pBlackboard->GetData("Steering", steering) || steering == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		std::vector<EntityInfo>* entityVec;
		if (!pBlackboard->GetData("EntityInfoVector", entityVec) || entityVec == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		EnemyInfo enemyInfo{};
		for (const EntityInfo& info : *entityVec)
		{
			examInterface->Enemy_GetInfo(info, enemyInfo);
		}

		steering->AutoOrient = false;

		constexpr float angularSpeed{ 2.f };
		if (AngleBetween(	Elite::Vector2(cosf(agentInfo.Orientation), sinf(agentInfo.Orientation)),
							enemyInfo.Location - agentInfo.Position)
			< 0.f)
		{
			steering->AngularVelocity -= angularSpeed;
		}
		else
		{
			steering->AngularVelocity += angularSpeed;
		}

		//if the angle is small enough
		if (fabs(AngleBetween(	Elite::Vector2(cosf(agentInfo.Orientation), sinf(agentInfo.Orientation)),
								enemyInfo.Location - agentInfo.Position))
			<= 0.05f)
		{
			ItemInfo occupiedItemInfo{};
			//Try to shoot shotgun
			if (examInterface->Inventory_GetItem(1, occupiedItemInfo))
			{
				if (examInterface->Weapon_GetAmmo(occupiedItemInfo) <= 0)
					examInterface->Inventory_RemoveItem(1);
				else
					examInterface->Inventory_UseItem(1);
				//std::cout << "SHOOTING!!!!\n";
			}

			//Try to shoot pistol
			else if (examInterface->Inventory_GetItem(0, occupiedItemInfo))
			{
				if (examInterface->Weapon_GetAmmo(occupiedItemInfo) <= 0)
					examInterface->Inventory_RemoveItem(0);
				else
					examInterface->Inventory_UseItem(0);
				//std::cout << "SHOOTING!!!!\n";
			}


			return Elite::BehaviorState::Success;
		}

		return Elite::BehaviorState::Failure;
		}

	Elite::BehaviorState Heal(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		ItemInfo occupiedItemInfo{};
		if (examInterface->Inventory_GetItem(2, occupiedItemInfo) != false)
		{
			examInterface->Inventory_UseItem(2);
			examInterface->Inventory_RemoveItem(2);
			return Elite::BehaviorState::Success;
		}
		return Elite::BehaviorState::Failure;
	}

	Elite::BehaviorState Eat(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		ItemInfo occupiedItemInfo{};
		if (examInterface->Inventory_GetItem(3, occupiedItemInfo) != false)
		{
			examInterface->Inventory_UseItem(3);
			examInterface->Inventory_RemoveItem(3);
			return Elite::BehaviorState::Success;
		}
		if (examInterface->Inventory_GetItem(4, occupiedItemInfo) != false)
		{
			examInterface->Inventory_UseItem(4);
			examInterface->Inventory_RemoveItem(4);
			return Elite::BehaviorState::Success;
		}
		return Elite::BehaviorState::Failure;
	}

	Elite::BehaviorState MoveStraightForward(Elite::Blackboard* pBlackboard)
	{
		SteeringPlugin_Output* steering{};
		if (!pBlackboard->GetData("Steering", steering) || steering == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		const Elite::Vector2 moveDir{ agentInfo.Position.x + 2.5f * cosf(agentInfo.Orientation),
								agentInfo.Position.y + 2.5f * sinf(agentInfo.Orientation) };

		//std::cout << "Getting unstuck\n";
		MoveTowardsPoint(examInterface, steering, moveDir);
		return Elite::BehaviorState::Success;
	}

	Elite::BehaviorState Wander(Elite::Blackboard* pBlackboard)
	{
		SteeringPlugin_Output* steering{};
		if (!pBlackboard->GetData("Steering", steering) || steering == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		std::deque<Elite::Vector2>* wanderPointsVector;
		if (!pBlackboard->GetData("WanderPointsVector", wanderPointsVector) || wanderPointsVector == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		constexpr float delta{ 0.5f };
		// Check if bot is at the wanderpoint
		if (Elite::DistanceSquared(agentInfo.Position, (*wanderPointsVector)[0]) <= delta)
		{
			// push point to back of queue and get a new one at the front
			wanderPointsVector->push_back((*wanderPointsVector)[0]);
			wanderPointsVector->pop_front();
		}

		if ((*wanderPointsVector)[0] == Elite::Vector2{ -1000,-1000 })
		{
			wanderPointsVector->push_back((*wanderPointsVector)[0]);
			wanderPointsVector->pop_front();

			std::vector<Elite::Vector2>* visitedHouseCenters;
			if (!pBlackboard->GetData("VisitedHouseCenters", visitedHouseCenters) || visitedHouseCenters == nullptr)
			{
				return Elite::BehaviorState::Failure;
			}

			visitedHouseCenters->clear();
			// magic number 4 idk, just so its not 20,
			// and doesnt have to resized for the first 2 houses it visits
			visitedHouseCenters->reserve(4);
		}


		bool* isRunning;
		if (!pBlackboard->GetData("IsRunning", isRunning) || isRunning == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}
		//sprint if i have the energy for it
		if (agentInfo.Stamina >= 9.5f)
		{
			*isRunning = true;
		}
		else if (agentInfo.Stamina <= 1.f)
		{
			*isRunning = false;
		}

		steering->RunMode = *isRunning;

		//std::cout << "Wandering\n";
		MoveTowardsPoint(examInterface, steering, (*wanderPointsVector)[0]);
		return Elite::BehaviorState::Success;
	}

	Elite::BehaviorState GoInHouse(Elite::Blackboard* pBlackboard)
	{
		SteeringPlugin_Output* steering{};
		if (!pBlackboard->GetData("Steering", steering) || steering == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		std::vector<Elite::Vector2>* visitedHouseCenters;
		if (!pBlackboard->GetData("VisitedHouseCenters", visitedHouseCenters) || visitedHouseCenters == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		std::deque<Elite::Vector2>* houseCentersToVisit;
		if (!pBlackboard->GetData("HouseCentersToVisit", houseCentersToVisit) || houseCentersToVisit == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}


		if (houseCentersToVisit->size() != 0)
		{
			MoveTowardsPoint(examInterface, steering, (*houseCentersToVisit)[0]);
			return Elite::BehaviorState::Success;
		}

		return Elite::BehaviorState::Failure;

	}

	Elite::BehaviorState GoToItem(Elite::Blackboard* pBlackboard)
	{
		SteeringPlugin_Output* steering{};
		if (!pBlackboard->GetData("Steering", steering) || steering == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		std::deque<EntityInfo>* itemsToVisit;
		if (!pBlackboard->GetData("ItemsToVisit", itemsToVisit) || itemsToVisit == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		//std::cout << "GoingToItem\n";
		MoveTowardsPoint(examInterface, steering, (*itemsToVisit)[0].Location);
		return Elite::BehaviorState::Success;
	}

	Elite::BehaviorState PickUpItem(Elite::Blackboard * pBlackboard)
	{
		std::vector<EntityInfo>* entityVec;
		if (!pBlackboard->GetData("EntityInfoVector", entityVec) || entityVec == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		std::deque<EntityInfo>* itemsToVisit;
		if (!pBlackboard->GetData("ItemsToVisit", itemsToVisit) || itemsToVisit == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		ItemInfo itemInfo{};

		EntityInfo entityInfo = (*itemsToVisit)[0];
		examInterface->Item_GetInfo(entityInfo, itemInfo);


		// pick up if in range
		if (Elite::DistanceSquared(entityInfo.Location, agentInfo.Position) < Elite::Square(agentInfo.GrabRange))
		{
			// Destroy if garbage
			switch (itemInfo.Type)
			{
			case eItemType::PISTOL:
			{
				ItemInfo occupiedItemInfo{};
				if (examInterface->Inventory_GetItem(0, occupiedItemInfo) == false)
				{
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(0, itemInfo);
				}
				else if (examInterface->Weapon_GetAmmo(occupiedItemInfo) < examInterface->Weapon_GetAmmo(itemInfo))
				{
					examInterface->Inventory_RemoveItem(0);
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(0, itemInfo);
				}
				else
				{
					examInterface->Item_Destroy(entityInfo);
				}
				break;
			}
			case eItemType::SHOTGUN:
			{
				ItemInfo occupiedItemInfo{};
				if (examInterface->Inventory_GetItem(1, occupiedItemInfo) == false)
				{
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(1, itemInfo);
				}
				else if (examInterface->Weapon_GetAmmo(occupiedItemInfo) < examInterface->Weapon_GetAmmo(itemInfo))
				{
					examInterface->Inventory_RemoveItem(1);
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(1, itemInfo);
				}
				else
				{
					examInterface->Item_Destroy(entityInfo);
				}
				break;
			}
			case eItemType::MEDKIT:
			{
				ItemInfo occupiedItemInfo{};
				if (examInterface->Inventory_GetItem(2, occupiedItemInfo) == false)
				{
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(2, itemInfo);
				}
				else if (examInterface->Medkit_GetHealth(occupiedItemInfo) < examInterface->Medkit_GetHealth(itemInfo))
				{
					examInterface->Inventory_UseItem(2);
					examInterface->Inventory_RemoveItem(2);
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(2, itemInfo);
				}
				else if (examInterface->Agent_GetInfo().Health <= 6.f)
				{
					examInterface->Inventory_UseItem(2);
					examInterface->Inventory_RemoveItem(2);
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(2, itemInfo);
				}
				else
				{
					examInterface->Item_Destroy(entityInfo);
				}
				break;
			}
			case eItemType::FOOD:
			{
				ItemInfo occupiedItemInfo3{};
				ItemInfo occupiedItemInfo4{};
					//slot 3
				if (examInterface->Inventory_GetItem(3, occupiedItemInfo3) == false)
				{
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(3, itemInfo);
				}
					//slot 4
				else if (examInterface->Inventory_GetItem(4, occupiedItemInfo4) == false)
				{
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(4, itemInfo);
				}
					//slot 3
				else if (examInterface->Food_GetEnergy(occupiedItemInfo3) < examInterface->Food_GetEnergy(itemInfo))
				{
					examInterface->Inventory_UseItem(3);
					examInterface->Inventory_RemoveItem(3);
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(3, itemInfo);
				}
					//slot 4
				else if (examInterface->Food_GetEnergy(occupiedItemInfo4) < examInterface->Food_GetEnergy(itemInfo))
				{
					examInterface->Inventory_UseItem(4);
					examInterface->Inventory_RemoveItem(4);
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(4, itemInfo);
				}
					//slot 3
				else if (examInterface->Agent_GetInfo().Health <= 6.f)
				{
					examInterface->Inventory_UseItem(3);
					examInterface->Item_Grab(entityInfo, itemInfo);
					examInterface->Inventory_AddItem(3, itemInfo);
				}
				else
				{
					examInterface->Item_Destroy(entityInfo);
				}

				break;
			}
			case eItemType::GARBAGE:
			{
				examInterface->Item_Destroy(entityInfo);
				break;
			}
			default:
			{
			}
			}

			// Pop out of deque
			itemsToVisit->pop_front();

			//std::cout << "Picked Up item\n";
			return Elite::BehaviorState::Success;
		}
		return Elite::BehaviorState::Failure;
	}

	Elite::BehaviorState AvoidPurge(Elite::Blackboard* pBlackboard)
	{
		SteeringPlugin_Output* steering{};
		if (!pBlackboard->GetData("Steering", steering) || steering == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		std::vector<EntityInfo>* entityVec;
		if (!pBlackboard->GetData("EntityInfoVector", entityVec) || entityVec == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		float* timeSinceLastPurge;
		if (!pBlackboard->GetData("TimeSinceLastPurgeSeen", timeSinceLastPurge) || timeSinceLastPurge == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		Elite::Vector2* purgeFleeLocation;
		if (!pBlackboard->GetData("PurgeFleeLocation", purgeFleeLocation) || purgeFleeLocation == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}


		PurgeZoneInfo purgeInfo{};
		bool foundPurge{ false };
		for (const EntityInfo& info : *entityVec)
		{
			foundPurge = examInterface->PurgeZone_GetInfo(info, purgeInfo);
			if (foundPurge)
				break;
		}

		Elite::Vector2 goToPoint{};
		if (foundPurge)
		{
			Elite::Vector2 leaveDirection{};

			leaveDirection = agentInfo.Position - purgeInfo.Center;
			leaveDirection = leaveDirection.GetNormalized();
			leaveDirection *= purgeInfo.Radius * 0.4f;
			goToPoint = agentInfo.Position + leaveDirection;
			*purgeFleeLocation = goToPoint;
		}
		else if (*timeSinceLastPurge > 0.f)
		{
			goToPoint = *purgeFleeLocation;
		}
		else
			return Elite::BehaviorState::Failure;


		//Sprint if possible
		steering->RunMode = true;


		MoveTowardsPoint(examInterface, steering, goToPoint);
		return Elite::BehaviorState::Success;
	}

	Elite::BehaviorState SearchEnemy(Elite::Blackboard* pBlackboard)
	{
		SteeringPlugin_Output* steering{};
		if (!pBlackboard->GetData("Steering", steering) || steering == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		float* timeSpentSearching;
		if (!pBlackboard->GetData("TimeSpentSearching", timeSpentSearching) || timeSpentSearching == nullptr)
		{
			return Elite::BehaviorState::Failure;
		}

		if (*timeSpentSearching < 0.f)
			return Elite::BehaviorState::Success;


		steering->AutoOrient = false;

		constexpr float angularSpeed{ 2.f };
		steering->AngularVelocity -= angularSpeed;

		return Elite::BehaviorState::Success;
	}
}





namespace BT_Conditions
{
	bool HasEnemyInVision(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* entityVec;
		if (!pBlackboard->GetData("EntityInfoVector", entityVec) || entityVec == nullptr)
		{
			return false;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return false;
		}

		EnemyInfo enemyInfo{};
		bool foundEnemy{ false };
		for (const EntityInfo& info : *entityVec)
		{
			foundEnemy = examInterface->Enemy_GetInfo(info, enemyInfo);
			if (foundEnemy)
				break;
		}
		if (foundEnemy)
		{
			if (examInterface->Inventory_GetItem(0, ItemInfo{}))
			{
				return true;
			}
			if (examInterface->Inventory_GetItem(1, ItemInfo{}))
			{
				return true;
			}
		}

		return false;
	}

	bool IsLowHP(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return false;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		ItemInfo occupiedItemInfo{};
		if (examInterface->Inventory_GetItem(2, occupiedItemInfo) != false)
		{
			if (10 - agentInfo.Health >= examInterface->Medkit_GetHealth(occupiedItemInfo))
				return true;
		}

		return false;
	}

	bool IsHungry(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return false;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		ItemInfo occupiedItemInfo{};
		if (examInterface->Inventory_GetItem(3, occupiedItemInfo) != false)
		{
			if (10 - agentInfo.Energy >= examInterface->Food_GetEnergy(occupiedItemInfo))
				return true;
		}

		if (examInterface->Inventory_GetItem(4, occupiedItemInfo) != false)
		{
			if (10 - agentInfo.Energy >= examInterface->Food_GetEnergy(occupiedItemInfo))
				return true;
		}

		return false;
	}

	bool IsStuck(Elite::Blackboard* pBlackboard)
	{
		float* timeStuck;
		if (!pBlackboard->GetData("TimeStuck", timeStuck) || timeStuck == nullptr)
		{
			return false;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return false;
		}
		AgentInfo agentInfo = examInterface->Agent_GetInfo();

		if (agentInfo.LinearVelocity.MagnitudeSquared() <= 5.f || *timeStuck >= 2.5f)
		{
			float* deltaTime;
			if (!pBlackboard->GetData("DeltaTime", deltaTime) || deltaTime == nullptr)
			{
				return false;
			}

			*timeStuck += *deltaTime;

			if (*timeStuck < 2.5f)
				return false;
			if (*timeStuck >= 5.0f)
			{
				*timeStuck = 0.f;
				return false;
			}
			return true;
		}
		else if (agentInfo.LinearVelocity.MagnitudeSquared() >= 1.5f)
		{
			*timeStuck = 0.f;
			return false;
		}

		*timeStuck = 0.f;
		return false;
	}

	bool IsHouseInVision(Elite::Blackboard* pBlackboard)
	{
		constexpr float delta{ 1 };

		std::vector<HouseInfo>* houseVec;
		if (!pBlackboard->GetData("HouseInfoVector", houseVec) || houseVec == nullptr)
		{
			return false;
		}

		std::vector<Elite::Vector2>* visitedHouseCenters;
		if (!pBlackboard->GetData("VisitedHouseCenters", visitedHouseCenters) || visitedHouseCenters == nullptr)
		{
			return false;
		}

		std::deque<Elite::Vector2>* houseCentersToVisit;
		if (!pBlackboard->GetData("HouseCentersToVisit", houseCentersToVisit) || houseCentersToVisit == nullptr)
		{
			return false;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return false;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		if (houseCentersToVisit->size() > 0)
		{
			// Check if bot is in the center of this house
			if (Elite::DistanceSquared(agentInfo.Position, (*houseCentersToVisit)[0]) <= delta)
			{
				// Ignore this house from now on
				visitedHouseCenters->push_back((*houseCentersToVisit)[0]);
				houseCentersToVisit->pop_front();
				return false;
			}
		}


		for (const HouseInfo& houseInfo : *houseVec)
		{
			bool oneIsEqual{ false };

			for (const Elite::Vector2& center2 : *houseCentersToVisit)
			{
				if (Elite::DistanceSquared(houseInfo.Center, center2) <= delta)
				{
					oneIsEqual = true;
				}
			}
			for (const Elite::Vector2& center1 : *visitedHouseCenters)
			{
				if (Elite::DistanceSquared(houseInfo.Center, center1) <= delta)
				{
					oneIsEqual = true;
				}
			}
			if (oneIsEqual == false)
			{
				houseCentersToVisit->push_back(houseInfo.Center);
			}
		}

		if (houseCentersToVisit->size() > 0)
		{
			return true;
		}
	}

	bool IsItemInVision(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* entityVec;
		if (!pBlackboard->GetData("EntityInfoVector", entityVec) || entityVec == nullptr)
		{
			return false;
		}

		std::deque<EntityInfo>* itemsToVisit;
		if (!pBlackboard->GetData("ItemsToVisit", itemsToVisit) || itemsToVisit == nullptr)
		{
			return false;
		}

		if (entityVec->size() >= 1)
		{
			IExamInterface* examInterface;
			if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
			{
				return false;
			}

			for (const EntityInfo& entity : *entityVec)
			{
				ItemInfo itemInfo{};
				if (examInterface->Item_GetInfo(entity, itemInfo))
				{
					bool hasSeenAlready{ false };
					for (const EntityInfo& itemInfoToVisist : *itemsToVisit)
					{
						if (entity.EntityHash == itemInfoToVisist.EntityHash)
						{
							hasSeenAlready = true;
						}
					}
					if (hasSeenAlready == false)
					{
						itemsToVisit->push_back(entity);
					}
				}
			}
		}

		if (itemsToVisit->size() != 0)
		{
			return true;
		}


		return false;
	}

	bool IsOnItem(Elite::Blackboard* pBlackboard)
	{
		std::deque<EntityInfo>* itemsToVisit;
		if (!pBlackboard->GetData("ItemsToVisit", itemsToVisit) || itemsToVisit == nullptr)
		{
			return false;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return false;
		}
		AgentInfo agentInfo = examInterface->Agent_GetInfo();


		// If it sees entities
		if (itemsToVisit->size() < 1)
			return false;

		if (Elite::DistanceSquared((*itemsToVisit)[0].Location, agentInfo.Position) < Elite::Square(agentInfo.GrabRange))
		{
			return true;
		}
	}

	bool SeesPurge(Elite::Blackboard* pBlackboard)
	{
		std::vector<EntityInfo>* entityVec;
		if (!pBlackboard->GetData("EntityInfoVector", entityVec) || entityVec == nullptr)
		{
			return false;
		}

		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return false;
		}

		float* timeSinceLastPurge;
		if (!pBlackboard->GetData("TimeSinceLastPurgeSeen", timeSinceLastPurge) || timeSinceLastPurge == nullptr)
		{
			return false;
		}

		float* deltaTime;
		if (!pBlackboard->GetData("DeltaTime", deltaTime) || deltaTime == nullptr)
		{
			return false;
		}

		PurgeZoneInfo purgeInfo{};
		for (const EntityInfo& info : *entityVec)
		{
			if (examInterface->PurgeZone_GetInfo(info, purgeInfo))
			{
				*timeSinceLastPurge += *deltaTime;
				return true;
			}
		}
		if (*timeSinceLastPurge > 0.f && *timeSinceLastPurge <= 5.f)
		{
			*timeSinceLastPurge += *deltaTime;
			return true;
		}
		if (*timeSinceLastPurge > 5.f)
		{
			*timeSinceLastPurge = 0.f;
		}

		return false;
	}

	bool HasBeenDamaged(Elite::Blackboard* pBlackboard)
	{
		IExamInterface* examInterface;
		if (!pBlackboard->GetData("ExamInterface", examInterface) || examInterface == nullptr)
		{
			return false;
		}
		const AgentInfo agentInfo = examInterface->Agent_GetInfo();

		float* timeSpentSearching;
		if (!pBlackboard->GetData("TimeSpentSearching", timeSpentSearching) || timeSpentSearching == nullptr)
		{
			return false;
		}

		float* deltaTime;
		if (!pBlackboard->GetData("DeltaTime", deltaTime) || deltaTime == nullptr)
		{
			return false;
		}

		if (agentInfo.WasBitten)
		{
			*timeSpentSearching += *deltaTime;
		}

		if (*timeSpentSearching > 0.f && *timeSpentSearching <= 2.f)
		{
			*timeSpentSearching += *deltaTime;
			if (examInterface->Inventory_GetItem(0, ItemInfo{}))
			{
				return true;
			}
			if (examInterface->Inventory_GetItem(1, ItemInfo{}))
			{
				return true;
			}
		}

		if (*timeSpentSearching >= 2.f)
		{
			*timeSpentSearching = 0.f;
			return false;
		}

		return false;
	}
}

#endif