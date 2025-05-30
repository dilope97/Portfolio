#pragma once
#include "Core/Scene.h"
#include "Core/GameObject.h"
#include "Selectable/Selectable.h"
#include "Door.hpp"
#include "Grids/Grid.h"
#include "Graphics/Renderable.h"
#include "Interpolation/Interpolation.h"
#include "Core/Time.h"
#include "Squad/SquadController.h"
#include <thread>

using namespace QuimeraGameplay;
using namespace QuimeraEngine;

std::vector<std::list<Door*>> MakeVector()
{
	std::vector<std::list<Door*>> v;
	v.push_back(std::list<Door*>());
	v.push_back(std::list<Door*>());
	v.push_back(std::list<Door*>());
	return v;
}

std::vector<std::list<Door*>> Door::closedDoors = MakeVector();
std::vector<unsigned> Door::maxDoors{ 1,1,1 };
PlayerController* Door::PlCn = nullptr;

void Door::Close()
{
	ch = QuimeraEngine::AudioMgr::Get().PlaySound("sounds\\spatial_door.wav", SFX, 1.0f, &GetOwner()->mTransform.World().mPos, 0, 300000.f);
	opened = false;
	animation = true;
	//if (t < 1)
	//	t = 1 - t;
	//else
	t = 0;
	GameObject* obj = GetOwner();
	glm::vec3 vec = Door1->mTransform.Local().mSca;
	d1 = vec.x;
	vec = Door2->mTransform.Local().mSca;
	d2 = vec.x;

	mGrid->Put(GetOwner());
	closedDoors[group].push_back(this);

	while (closedDoors[group].size() > maxDoors[group])
	{
		closedDoors[group].front()->Open();
		//closedDoors[group].pop_front();
	}

	std::thread th([&]() { mGrid->MakeFlowField(mGrid->mflowObjective, mFlow, &done); });
	th.detach();
	opened = false;
	animation = true;
}

QuimeraGameplay::Door::~Door()
{
	//if (th)
	//{
	//	std::terminate();
	//}
	ch->setPaused(true);
	for (int i = 0; i < closedDoors.size(); i++)
	{
		closedDoors[i].clear();
	}

	PlCn = nullptr;
}

void Door::OnCreate()
{
	GameObject* obj = GetOwner();
	Space* space = Scene::Get().GetSpace("Space1");
	mGrid = space->FindObjectByName("Grid")->GetComp<Grid>();
	mSelect = obj->GetComp<Selectable>();
	if (mSelect == nullptr)
	{
		mSelect = new Selectable();
		obj->AddComp(mSelect);
	}
	obj->GetComp<ColliderComp>()->mGhost = true;

	std::vector<GameObject*> vec = obj->GetAllChildren();
	Door1 = vec[1]->GetAllChildren()[0];
	Door2 = vec[2]->GetAllChildren()[0];
	t = 1;
}

void Door::UpdateSquads()
{
	std::vector<SquadController*>& vec = SquadController::mSquads;
	for (int i = 0; i < vec.size(); i++)
	{
		/// Is the unit moving
		if (vec[i]->mPath.mPoints.size() /* && vec[i]->mPath.mPathTimer >= vec[i]->mPath.Duration()*/)
		{
			std::vector<glm::vec3>* path = mGrid->GetPath(vec[i]->GetOwner()->mTransform.World().mPos, vec[i]->mPath.mPoints.back().mPoint);
			vec[i]->mPath.mPoints.clear();
			vec[i]->mPath.mPathTimer = 0;
			for (int k = 0; k < path->size(); k++)
			{
				vec[i]->mPath.Push({ path->at(k).x,path->at(k).z });
			}
			//vec[i]->mPath.mPoints[0].mPoint = mOwner->mTransform.World().mPos;
		}
	}
}
void Door::Update()
{
	if (!animation)
	{
		if (mSelect->IsSelected())
		{
			if (PlCn != nullptr)
			{
				if (PlCn->SelMul() == false)
				{
					mSelect->SetSelected(false);
					if (opened)
						Close();
					else
						Open();
				}
			}
			else
			{
				PlCn = Scene::Get().GetMainSpace()->FindObjectByName("PlayerController")->GetComp<PlayerController>();
				if (PlCn->SelMul() == false)
				{
					mSelect->SetSelected(false);
					if (opened)
						Close();
					else
						Open();
				}
			}
		}
	}
	else
	{
		if (mSelect->IsSelected())
			mSelect->SetSelected(false);
	}

	//Animation
	if (animation)
	{
		t += 0.25 * (Time::Get().GetDeltaTime());

		if (opened)
		{
			glm::vec3 vec = Door1->mTransform.Local().mSca;
			Door1->mTransform.SetLocalScale(Interpolation::Lerp(glm::vec3(d1, vec.y, vec.z), glm::vec3(2.2f, vec.y, vec.z), t, Interpolation::MODE::Normal));
			vec = Door2->mTransform.Local().mSca;
			Door2->mTransform.SetLocalScale(Interpolation::Lerp(glm::vec3(d2, vec.y, vec.z), glm::vec3(2.2f, vec.y, vec.z), t, Interpolation::MODE::Normal));
		}
		else
		{
			glm::vec3 vec = Door1->mTransform.Local().mSca;
			Door1->mTransform.SetLocalScale(Interpolation::Lerp(glm::vec3(d1, vec.y, vec.z), glm::vec3(9.0f, vec.y, vec.z), t, Interpolation::MODE::Normal));
			vec = Door2->mTransform.Local().mSca;
			Door2->mTransform.SetLocalScale(Interpolation::Lerp(glm::vec3(d2, vec.y, vec.z), glm::vec3(9.0f, vec.y, vec.z), t, Interpolation::MODE::Normal));
		}

		if (t >= 1)
		{
			ch->setPaused(true);
			animation = false;
			if (!opened)
			{
				GetOwner()->GetComp<ColliderComp>()->mGhost = false;
			}
		}
	}

	if (done && t >= 1)
	{
		done = false;
		mGrid->CopyFlow(mFlow);
		UpdateSquads();
	}

}



void Door::Open()
{
	ch = QuimeraEngine::AudioMgr::Get().PlaySound("sounds\\spatial_door.wav", SFX, 1.0f, &GetOwner()->mTransform.World().mPos, 0, 300000.f);
	GameObject* obj = GetOwner();
	glm::vec3 vec;
	vec = Door1->mTransform.Local().mSca;
	d1 = vec.x;
	vec = Door2->mTransform.Local().mSca;
	d2 = vec.x;
	//obj->GetComp<Renderable>()->mColor = glm::vec4(1, 1, 1, 1);
	//if (t <= 1)
	//	t = 1 - t;
	//else
	t = 0;
	obj->GetComp<ColliderComp>()->mGhost = true;
	opened = true;
	animation = true;
	mGrid->Take(obj);
	std::thread th([&]() { mGrid->MakeFlowField(mGrid->mflowObjective, mFlow, &done); });
	th.detach();
	for (auto itr = closedDoors[group].begin(); itr != closedDoors[group].end(); itr++)
	{
		if (*itr == this)
		{
			closedDoors[group].erase(itr);
			break;
		}
	}
	animation = true;
	opened = true;
}