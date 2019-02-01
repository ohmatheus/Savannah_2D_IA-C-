#include "stdafx.h"

#include "GridScene.h"
#include "Game.h"
#include "RenderSystem.h"
#include "GridEntity.h"
#include "MeshData.h"

#include "Spawner.h"

#include "AntelopeSM.h"
#include "LionSM.h"

//----------------------------------------------------------

GridScene::GridScene(Game *game)
:	Super()
,	m_Game(game)
,	m_GridEntity(nullptr)
{
	_CreateScene();
}

//----------------------------------------------------------

GridScene::~GridScene()
{
	if (m_LionStateMachine != nullptr)
		delete m_LionStateMachine;
	if (m_AntelopeStateMachine != nullptr)
		delete m_AntelopeStateMachine;
}

//----------------------------------------------------------

IEntity		*GridScene::GetFlagsEntity(ETeam team)
{
	return m_Flags[team];
};

//----------------------------------------------------------

void		GridScene::PreUpdate(float dt)
{
	// Here precompute one time every each needed information for state machine
	// Saves a huge amount of time regarding state machine's conditions that do not need to recalculate this.

	const std::vector<GridEntity*>	&lions = m_Spawners[LION]->Entities();
	const std::vector<GridEntity*>	&antelopes = m_Spawners[ANTELOPE]->Entities();

	// pre-pre-update to know if one needs to die
	for (int i = 0; i < antelopes.size(); i++)
	{
		antelopes[i]->m_StateMachineAttr.m_FriendsNextToMe = 0;
		if (antelopes[i]->Health() <= 0.f)
			antelopes[i]->Die();
	}

	for (int i = 0; i < lions.size(); i++)
	{
		lions[i]->m_StateMachineAttr.m_FriendsNextToMe = 0;
		if (lions[i]->Health() <= 0.f)
			lions[i]->Die();
	}

	for (int i = 0; i < antelopes.size(); i++)
	{
		GridEntity		*antelopeA = antelopes[i];

		if (!antelopeA->IsActive())
			continue;
		const glm::vec3	&positionA = antelopeA->Position();

		for (int j = i + 1; j < antelopes.size(); j++)
		{
			// find the nearest friend
			GridEntity	*antelopeB = antelopes[j];
			if (!antelopeB->IsActive())
				continue;

			const glm::vec3	&positionB = antelopeB->Position();
			float localDistance = glm::length(positionA - positionB);

			if (localDistance < 5.f) // friends next to me
			{
				antelopeA->m_StateMachineAttr.m_FriendsNextToMe++;
				antelopeB->m_StateMachineAttr.m_FriendsNextToMe++;
			}

			if (localDistance < 0.2f) // special event to avoid entities overlap
			{
				const glm::vec3 arbitraryAntiOverlapVector = glm::vec3(0.5f, 0.5f, 0.f);
				antelopeB->SetPosition(positionB + arbitraryAntiOverlapVector * dt);
				antelopeA->SetPosition(positionA - arbitraryAntiOverlapVector * dt);
			}

			GridEntity	*antelopeAC = antelopeA->m_StateMachineAttr.m_NearestFriend;
			if (antelopeAC != nullptr)
			{
				const float distanceOldNearest = glm::length(positionA - antelopeAC->Position());
				if (localDistance < distanceOldNearest || !antelopeAC->IsActive())
					antelopeA->m_StateMachineAttr.m_NearestFriend = antelopeB;
				else
					antelopeA->m_StateMachineAttr.m_NearestFriend = antelopeAC;
			}
			else
				antelopeA->m_StateMachineAttr.m_NearestFriend = antelopeB;

			GridEntity	*antelopeBD = antelopeB->m_StateMachineAttr.m_NearestFriend;
			if (antelopeBD != nullptr)
			{
				const float distanceOldNearest = glm::length(positionB - antelopeBD->Position());
				if (localDistance < distanceOldNearest || !antelopeBD->IsActive())
					antelopeB->m_StateMachineAttr.m_NearestFriend = antelopeA;
				else 
					antelopeB->m_StateMachineAttr.m_NearestFriend = antelopeBD;
			}
			else
				antelopeB->m_StateMachineAttr.m_NearestFriend = antelopeA;
		}

		for (int j = 0; j < lions.size(); j++)
		{
			// find the nearest ennemy
			GridEntity		*lionB = lions[j];

			if (!lionB->IsActive())
				continue;

			const glm::vec3	&positionB = lionB->Position();
			float localDistance = glm::length(positionA - positionB);

			if (localDistance < 2.5f && antelopeA->IsActive()) // attack range
			{
				antelopeA->Hit(lionB->Dps() * dt);
				lionB->Hit(antelopeA->Dps() * dt);
				// die at next frame
			}

			GridEntity	*lionAC = antelopeA->m_StateMachineAttr.m_NearestEnemy;
			if (lionAC != nullptr)
			{
				const float distanceOldNearest = glm::length(positionA - lionAC->Position());
				if (localDistance < distanceOldNearest || !lionAC->IsActive())
					antelopeA->m_StateMachineAttr.m_NearestEnemy = lionB;
				else
					antelopeA->m_StateMachineAttr.m_NearestEnemy = lionAC;
			}
			else
				antelopeA->m_StateMachineAttr.m_NearestEnemy = lionB;

			GridEntity	*antelopeBD = lionB->m_StateMachineAttr.m_NearestEnemy;
			if (antelopeBD != nullptr)
			{
				const float distanceOldNearest = glm::length(positionB - antelopeBD->Position());
				if (localDistance < distanceOldNearest || !antelopeBD->IsActive())
					lionB->m_StateMachineAttr.m_NearestEnemy = antelopeA;
				else
					lionB->m_StateMachineAttr.m_NearestEnemy = antelopeBD;
			}
			else
				lionB->m_StateMachineAttr.m_NearestEnemy = antelopeA;
		}
	}

	for (int i = 0; i < lions.size(); i++)
	{
		GridEntity		*lionA = lions[i];
		const glm::vec3	&positionA = lionA->Position();

		if (!lionA->IsActive())
			continue;

		for (int j = 0; j < lions.size(); j++)
		{
			// find the nearest friend
			GridEntity		*lionB = lions[j];
			if (!lionB->IsActive())
				continue;

			const glm::vec3	&positionB = lionB->Position();
			float localDistance = glm::length(positionA - positionB);

			if (localDistance < 5.f) // friends next to me
			{
				lionA->m_StateMachineAttr.m_FriendsNextToMe++;
				lionB->m_StateMachineAttr.m_FriendsNextToMe++;
			}

			GridEntity	*lionAC = lionA->m_StateMachineAttr.m_NearestFriend;
			if (lionAC != nullptr)
			{
				const float distanceOldNearest = glm::length(positionA - lionAC->Position());
				if (localDistance < distanceOldNearest || !lionAC->IsActive())
					lionA->m_StateMachineAttr.m_NearestFriend = lionB;
				else
					lionA->m_StateMachineAttr.m_NearestFriend = lionAC;
			}
			else
				lionA->m_StateMachineAttr.m_NearestFriend = lionB;

			GridEntity	*lionBD = lionB->m_StateMachineAttr.m_NearestFriend;
			if (lionBD != nullptr)
			{
				const float distanceOldNearest = glm::length(positionB - lionBD->Position());
				if (localDistance < distanceOldNearest || !lionBD->IsActive())
					lionB->m_StateMachineAttr.m_NearestFriend = lionA;
				else
					lionB->m_StateMachineAttr.m_NearestFriend = lionBD;
			}
			else
				lionB->m_StateMachineAttr.m_NearestFriend = lionA;
		}
	}
}

//----------------------------------------------------------

GridEntity	*GridScene::AddEntity(ETeam type, const glm::vec3 &position, bool isActive)
{
	glm::vec4	lionColor = glm::vec4(0.8f, 0.5f, 0.f, 1.f);
	glm::vec4	antelopeColor = glm::vec4(0.8f, 0.25f, 0.f, 1.f);

	GridEntity	*entity = new GridEntity("Default name", type, isActive);
	entity->SetColor(type == LION ? lionColor : antelopeColor);
	entity->SetMeshName(type == LION ? "Rectangle" : "Triangle");
	entity->SetShaderName("DefaultShader");
	entity->SetPosition(position + glm::vec3(0.f, 0.f, 0.1f));

	entity->ChangeStateNode(type == LION ? m_LionStateMachine->Root() : m_AntelopeStateMachine->Root());
	m_GridEntity->AddChild(entity);

	m_Entities.push_back(entity);

	return entity;
}

//----------------------------------------------------------

void	GridScene::_CreateScene()
{
	m_AntelopeStateMachine = new StateMachine::AntelopeStateMachine(this);
	m_LionStateMachine = new StateMachine::LionStateMachine(this);

	m_Dps[LION] = 10.f;
	m_Dps[ANTELOPE] = 2.6f;

	_GenerateAndAddGrid(100, 60);

	// flags
	{
		GridEntity	*entity = new GridEntity("Lion Flag", LION);
		entity->SetColor(glm::vec4(0.f, 1.f, 0.7f, 1.f));
		entity->SetMeshName("Diamond");
		entity->SetShaderName("DefaultShader");
		entity->SetPosition(glm::vec3(-40.f, 20.f, 0.1f));

		m_GridEntity->AddChild(entity);
		m_Entities.push_back(entity);
		m_Flags[LION] = entity;
	}
	{
		GridEntity	*entity = new GridEntity("Antelope Flag", ANTELOPE);
		entity->SetColor(glm::vec4(1.f, 0.f, 0.f, 1.f));
		entity->SetMeshName("Diamond");
		entity->SetShaderName("DefaultShader");
		entity->SetPosition(glm::vec3(40.f, 20.f, 0.1f));

		m_GridEntity->AddChild(entity);
		m_Entities.push_back(entity);
		m_Flags[ANTELOPE] = entity;
	}

	for (int i = 0; i < ETeam::_NONE; i++)
	{
		m_Spawners[i] = new GridSpawner((ETeam)i, this, (ETeam)i == LION ? 75 : 300);
		m_Entities.push_back(m_Spawners[i]);
		m_GridEntity->AddChild(m_Spawners[i]);

		m_Spawners[i]->SetColor((ETeam)i == LION ? glm::vec4(0.f, 0.7f, 0.1f, 1.f) : glm::vec4(0.5f, 0.f, 0.7f, 1.f));
		m_Spawners[i]->SetMeshName("Rectangle");
		m_Spawners[i]->SetShaderName("DefaultShader");
		m_Spawners[i]->SetPosition((ETeam)i == LION ? glm::vec3(-40.f, -20.f, 0.1f) : glm::vec3(40.f, -20.f, 0.1f));
		m_Spawners[i]->SetScale(3.f);

		m_Spawners[i]->SetDps(m_Dps[(ETeam)i]);
		m_Spawners[i]->OnSceneStart(); // TODO move that when be able to copy scene
	}
}

//----------------------------------------------------------

StateMachine::StateNode		*GridScene::GetStateMachineRoot(ETeam team)
{
	if (team == LION)
		return m_LionStateMachine->Root();
	else
		return m_AntelopeStateMachine->Root();
}

//----------------------------------------------------------

void	GridScene::_GenerateAndAddGrid(int xSubdiv, int ySubdiv)
{
	RenderSystem	*renderSystem = m_Game->GetRenderSystem();
	assert(renderSystem != nullptr);

	const std::string meshName = renderSystem->GenrateGridMesh(1.f, xSubdiv, ySubdiv);

	GridEntity *entity = new GridEntity("Grid", _NONE);
	entity->SetColor(glm::vec4(0.5f, 0.5f, 0.5f, 1.f));
	entity->SetMeshName(meshName);
	entity->SetShaderName("DefaultShader");
	entity->SetPosition(glm::vec3(0.f, 0.f, 0.f));
	m_Entities.push_back(entity);
	m_GridEntity = entity;
}

//----------------------------------------------------------
