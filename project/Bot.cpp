#include "stdafx.h"
#include "Bot.h"
#include "behaviorTree.h"
#include "blackboard.h"
#include "behaviors.h"

using namespace Elite;

Bot::Bot(IExamInterface* pInterface)
	:m_IExamInterface { pInterface }
{

	//Initializing wander path with hard coded values
	//The world generates houses within -200 and 200 X and Y
	//so we go to every corner and the middle
	m_WanderPointsVector = { {0,190}, {190,190}, {190, -190}, {-190,-190}, {-190,190}, {-1000,-1000}, {0,0} };
	// {-1000,-1000} is the point where the m_VisitedHouseCenters variable gets reset
	// The agent doesnt actually go to {-1000,-1000}


	//1. Create Blackboard
	Blackboard* pBlackboard = CreateBlackboard();

	//2. Create BehaviorTree
	BehaviorTree* pBehaviorTree = new BehaviorTree(pBlackboard,
		new BehaviorSelector
		{
			std::vector<IBehavior*>
			{
				//USE ITEMS IF NEEDED
				////////////////////
				new BehaviorSelector
				{
					std::vector<IBehavior*>
					{
					//HEALING
					new BehaviorSequence
					{
						std::vector<IBehavior*>
						{
							new BehaviorConditional{ BT_Conditions::IsLowHP },
							new BehaviorAction{ BT_Actions::Heal }
						}
					},
					//EATING
					new BehaviorSequence
					{
						std::vector<IBehavior*>
						{
							new BehaviorConditional{ BT_Conditions::IsHungry },
							new BehaviorAction{ BT_Actions::Eat }
						}
					}
				}
				},//////////////////


				//SHOOTING
				new BehaviorSequence
				{
					std::vector<IBehavior*>
					{
						new BehaviorConditional{ BT_Conditions::HasEnemyInVision },
						new BehaviorAction{ BT_Actions::TurnAndShoot }
					}
				},
				//AVOID PURGE ZONE
				new BehaviorSequence
				{
					std::vector<IBehavior*>
					{
						new BehaviorConditional{ BT_Conditions::SeesPurge },
						new BehaviorAction{ BT_Actions::AvoidPurge }
					}
				},
				//IF DAMAGED SEARCH ENEMY
				new BehaviorSequence
				{
					std::vector<IBehavior*>
					{
						new BehaviorConditional{ BT_Conditions::HasBeenDamaged },
						new BehaviorAction{ BT_Actions::SearchEnemy }
					}
				},
				
				//CHECK IF STUCK
				new BehaviorSequence
				{
					std::vector<IBehavior*>
					{
						new BehaviorConditional{ BT_Conditions::IsStuck },
						new BehaviorAction{ BT_Actions::MoveStraightForward }
					}
				},
				//PICK UP ITEM
				new BehaviorSequence
				{
					std::vector<IBehavior*>
					{
						new BehaviorConditional{ BT_Conditions::IsOnItem },
						new BehaviorAction{ BT_Actions::PickUpItem }
					}
				},
				//GOING TO ITEM IN VISION
				new BehaviorSequence
				{
					std::vector<IBehavior*>
					{
						new BehaviorConditional{ BT_Conditions::IsItemInVision },
						new BehaviorAction{ BT_Actions::GoToItem }
					}
				},
				//ENTER HOUSE
				new BehaviorSequence
				{
					std::vector<IBehavior*>
					{
						new BehaviorConditional{ BT_Conditions::IsHouseInVision },
						new BehaviorAction{ BT_Actions::GoInHouse }
					}
				},
				//WANDER
				new BehaviorAction{ BT_Actions::Wander }
			}
		}
	);

	m_pDecisionMaking = pBehaviorTree;
}

Blackboard* Bot::CreateBlackboard()
{
	Blackboard* pBlackboard = new Blackboard();
	pBlackboard->AddData("Steering", static_cast<SteeringPlugin_Output*>(nullptr));
	pBlackboard->AddData("EntityInfoVector", &m_EntityInfoVector);
	pBlackboard->AddData("HouseInfoVector", &m_HouseInfoVector);
	pBlackboard->AddData("ExamInterface", m_IExamInterface);
	pBlackboard->AddData("VisitedHouseCenters", &m_VisitedHouseCenters);
	pBlackboard->AddData("HouseCentersToVisit", &m_HouseCentersToVisit);
	pBlackboard->AddData("TimeStuck", &m_TimeStuck);
	pBlackboard->AddData("DeltaTime", &m_DeltaTime);
	pBlackboard->AddData("WanderPointsVector", &m_WanderPointsVector);
	pBlackboard->AddData("HasFinishedWorldPatrol", &m_HasFinishedWorldPatrol);
	pBlackboard->AddData("IsRunning", &m_IsRunning);
	pBlackboard->AddData("TimeSinceLastPurgeSeen", &m_TimeSinceLastPurgeSeen);
	pBlackboard->AddData("TimeSinceLastPurgeSeen", &m_TimeSinceLastPurgeSeen);
	pBlackboard->AddData("PurgeFleeLocation", &m_PurgeFleeLocation);
	pBlackboard->AddData("TimeSpentSearching", &m_TimeSpentSearching);
	pBlackboard->AddData("ItemsToVisit", &m_ItemsToVisit);

	m_pBlackboard = pBlackboard;

	return pBlackboard;
}

void Bot::SetHouseInfoVector(const std::vector<HouseInfo>& houseInfoVector)
{
	//if (m_HouseInfoVector != houseInfoVector)
		m_HouseInfoVector = houseInfoVector;
}

void Bot::SetEntityInfoVector(const std::vector<EntityInfo>& entityInfoVector)
{
	//if (m_EntityInfoVector != entityInfoVector)
		m_EntityInfoVector = entityInfoVector;
}

void Bot::SetSteeringTarget(SteeringPlugin_Output* steering)
{
	m_pBlackboard->ChangeData("Steering", steering);
}

void Bot::Update(float dt)
{
	m_DeltaTime = dt;
	m_pDecisionMaking->Update(dt);
}
