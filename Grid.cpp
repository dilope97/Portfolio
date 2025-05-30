#pragma once

#include "Physics/Collisions/Collider.h"
#include "Graphics/RenderPipeline.h"

#include "Core/GameObject.h"
#include "Graphics/Renderable.h"
#include "Graphics/ShapeLibrary/Shapes.h"
#include <Core/Seralization.h>
#include "Math/Geometry.h"
#include "Physics/Collisions/CollisionDetectors.h"
#include "Physics/Collisions/CollisionResolution.h"
#include <Enemy/Enemy.h>
#include "Grid.h"
#include "Math/Math.h"

#include <mutex>

#include <glm/gtx/norm.hpp>

std::mutex mtx;

using namespace QuimeraGameplay;

namespace QuimeraEngine
{
	void Cell::Occupie()
	{
		mOccupied = true;
	}
	void Cell::Desoccupie()
	{
		mOccupied = false;
	}
	bool Cell::isOcuppied() const
	{
		return mOccupied;
	}
	void Cell::SetPos(glm::vec2 vec)
	{
		mPos = vec;
	}
	glm::ivec2 Cell::GetPos()
	{
		return mPos;
	}

	void Grid::OnCreate()
	{
		MotherBrain::Get().SetGrid(this);

		mFreeNeighbour.resize(8);
		mFreeNeighbourPath.resize(8);

		debugGrid = false;
	}

	void Grid::CreateGrid(int sizeCell_, int MapSizeX, int MapSizeY)
	{
		ShapeGenerator shapes;
		Mesh* CellMesh = new Mesh(shapes.GenerateSphere(8));
		Shader* CellShader = ResourceManager::Get().GetResource<Shader>("shaders\\noLight.shader", ResourceType::SHADER);
		mSizeCell = sizeCell_;
		mWidth = (MapSizeX / mSizeCell) + 1;
		mHeight = (MapSizeY / mSizeCell) + 1;
		int number = mHeight * mWidth;
		mCells.reserve(number);
		int x = -1;
		int y = 0;

		for (int i = 0; i < number; i++)
		{
			Cell* var = new Cell;
			x++;
			if (x == mWidth)
			{
				x = 0;
				y++;
			}
			var->SetPos(glm::vec2(x, y));

			mCells.push_back(var);
		}
	}

	void Grid::Insert(ColliderComp* aabb)
	{
		for (int i = 0; i < mCells.size(); i++)
		{
			glm::ivec2 pos = mCells[i]->GetPos();
			pos *= mSizeCell;

			Transform3D obb1(aabb->mPos, glm::abs(aabb->mSca), aabb->TransformWorld().mRot);
			Transform3D obb2(glm::vec3(pos.x, 0.f, pos.y), glm::vec3(mSizeCell, 0.f, mSizeCell), glm::vec3(0.f, 0.f, 0.f));

			if (!mCells[i]->isOcuppied() && OBBVsOBBRes(obb1, obb2, nullptr))
			{
				mCells[i]->Occupie();
				//std::cout << "occupied" << std::endl;
			}
		}
	}

	void Grid::Free(ColliderComp* aabb)
	{
		for (int i = 0; i < mCells.size(); i++)
		{
			glm::ivec2 Pos = mCells[i]->GetPos();
			glm::ivec2 min(Pos.x * mSizeCell, Pos.y * mSizeCell);
			glm::ivec2 max(Pos.x * mSizeCell + mSizeCell, Pos.y * mSizeCell + mSizeCell);

			AABB cell;
			cell.SetMinMax(glm::vec3(min.x - mSizeCell / 2.f, 0, min.y - mSizeCell / 2.f), glm::vec3(max.x - mSizeCell / 2.f, 0.0f, max.y - mSizeCell / 2.f));

			glm::vec3 pos = aabb->mPos;
			glm::vec3 sca = aabb->mSca;

			AABB objAabb;
			objAabb.SetPosSca(pos, sca);

			if (mCells[i]->isOcuppied() && AABBVsAABB(cell, objAabb))
			{
				mCells[i]->Desoccupie();
			}
		}
	}
	Grid::Grid()
	{
		gridId = std::distance(std::filesystem::directory_iterator("data/binary/"), std::filesystem::directory_iterator{}) + 1;
		mRestart = false;
	}
	Grid::~Grid()
	{
		mRestart = true;
		Clear();
	}

	void Grid::Put(GameObject* pObj)
	{
		ColliderComp* col;
		col = pObj->GetComp<ColliderComp>(nullptr);
		Insert(col);
	}

	void Grid::Take(GameObject* pObj)
	{
		ColliderComp* col;
		col = pObj->GetComp<ColliderComp>(nullptr);
		Free(col);
	}

	void Grid::Fill(const std::vector<GameObject*> pObjs)
	{
		ColliderComp* col;
		for (auto& obj : pObjs)
		{
			col = obj->GetComp<ColliderComp>(nullptr);
			if (col != nullptr)
			{
				Insert(col);
			}
		}
	}

	void Grid::Unfill(const std::vector<GameObject*> pObjs)
	{
		ColliderComp* col;
		for (auto& obj : pObjs)
		{
			col = obj->GetComp<ColliderComp>(nullptr);
			if (col != nullptr)
			{
				Free(col);
			}
		}
	}

	bool Grid::Check(float posX, float posY)
	{
		glm::vec2 posGrid = glm::vec2(posX / mSizeCell, posY / mSizeCell);

		if (posGrid.x < 0 || posGrid.y < 0)
			return false;

		int index = posGrid.x + posGrid.y * mWidth;

		return mCells[index]->isOcuppied();
	}

	bool Grid::CheckNeigbourOccupie(float posX, float posY)
	{
		if (posX <= 0 || posY <= 0) {
			return true;
		}

		glm::ivec2 posGrid = glm::vec2(posX / mSizeCell, posY / mSizeCell);
		for (int x = -1; x < 2; x++)
		{
			for (int y = -1; y < 2; y++)
			{
				if (x == 0 && y == 0)
					continue;

				unsigned size = posGrid.x + x + (posGrid.y + y) * mWidth;
				if (size > mCells.size())
				{
					return false;
				}

				return mCells[size]->isOcuppied();
			}
		}

		return false;
	}

	void Grid::Update()
	{
		if (debugGrid)
		{
			for (int i = 0; i < mCells.size(); i++)
			{
				glm::vec4 color = {1,1,1,1};
			
				if (mCells[i]->isOcuppied())
				{	
					color = { 1,0,0,1 };
				}
			
				else
				{
					if(mFlow.size() > 0)
					{
						color = { glm::normalize(mFlow[i].mDir), 0 ,1 };
						color += 1.f;
						color /= 2.f;
					}
				}
			
				RenderPipeline::Get().DrawDebugAabb(glm::ivec3(mCells[i]->GetPos().x, 1, mCells[i]->GetPos().y) * mSizeCell, { mSizeCell,0,mSizeCell }, color);
			}
		}
	}

	Cell* Grid::GetCell(glm::vec2 pos)
	{
		glm::vec2 posGrid = glm::vec2((pos.x+mSizeCell/2) / mSizeCell, (pos.y+mSizeCell/2) / mSizeCell);

		glm::clamp(posGrid, glm::vec2(0), glm::vec2(mWidth, mHeight));

		int indexX = posGrid.x;
		int indexY = posGrid.y;

		if (indexX < 0 || indexY < 0 || indexX >= mWidth || indexY >= mHeight)
			return nullptr;

		return mCells[indexX + indexY * mWidth];
	}

	void Grid::DrawPath(Path* path)
	{
		for (int it = path->road.size()-1; it >=0; it--)
		{
			RenderPipeline::Get().DrawDebugAabb(glm::ivec3(path->road[it]->GetPos().x, 0, path->road[it]->GetPos().y) * mSizeCell, { mSizeCell,0,mSizeCell }, {0,1,0,1});
		}
	}

	void Grid::StreamRead(const nlohmann::json& j)
	{
		auto grid = j.find("gridId");
		std::string filename;
		if (grid != j.end())
		{
			gridId = *grid;

			filename = std::to_string(gridId);
		}
		else
		{
			gridId = std::distance(std::filesystem::directory_iterator("data/binary/"), std::filesystem::directory_iterator{}) + 1;

			filename = std::to_string(gridId);
		}

		// read the position, if it exists
		auto it = j.find("Flowposition");

		if (it != j.end())
		{
			glm::vec2 newPos;
			(*it) >> newPos;
			this->mflowObjective = newPos;
		}

		LoadGrid(filename);
	}

	void Grid::StreamWrite(nlohmann::json& j) const
	{
		std::string filename = std::to_string(gridId);

		SaveGrid(filename);

		j["gridId"] = gridId;

		j["Flowposition"] << mflowObjective;
	}

	int Grid::CheckNeighbour(Cell* cell)
	{
		glm::ivec2 pos = cell->GetPos();
		int insertedItems = 0;
		for (int x = -1; x < 2; x++)
		{
			Cell* Neighbour = nullptr;
			for (int y = -1; y < 2; y++)
			{
				if (x == 0 && y == 0)
					continue;

				glm::ivec2 posCheck = { pos.x + x, pos.y + y };

				if (posCheck.x < 0 || posCheck.y < 0 || posCheck.x >= mWidth || posCheck.y >= mHeight)
					continue;
				else
				{
					Neighbour = mCells[posCheck.x + posCheck.y * mWidth];
					if (Neighbour && Neighbour->isOcuppied() == false)
					{
						mFreeNeighbour[insertedItems++] = Neighbour;
					}
				}
			}
		}

		return insertedItems;
	}

	int Grid::CheckNeighbourPath(Cell* cell)
	{
		glm::ivec2 pos = cell->GetPos();
		int insertedItems = 0;
		for (int x = -1; x < 2; x++)
		{
			Cell* Neighbour = nullptr;
			for (int y = -1; y < 2; y++)
			{
				if (x == 0 && y == 0)
					continue;

				glm::ivec2 posCheck = { pos.x + x, pos.y + y };

				if (posCheck.x < 0 || posCheck.y < 0 || posCheck.x >= mWidth || posCheck.y >= mHeight)
					continue;
				else
				{
					Neighbour = mCells[posCheck.x + posCheck.y * mWidth];
					if (Neighbour->isOcuppied() == false)
						mFreeNeighbourPath[insertedItems++] = Neighbour;
				}
			}
		}

		return insertedItems;
	}

	float CostBetweenCells(Cell* one, Cell* two)
	{
		glm::vec2 cost = one->GetPos() - two->GetPos();
		return glm::length(cost);
		float dx = abs(one->GetPos().x - two->GetPos().x);
		float dy = abs(one->GetPos().y - two->GetPos().y);
		float distance = 1 * (dx + dy) + (sqrt(2) - 2 * 1) * glm::min(dx, dy);
	}

	PathCell* FindOption(Cell* Cell, std::vector<PathCell*>& list)
	{
		for (int i = 0; i < list.size(); i++)
		{
			if (list[i]->mCell == Cell)
				return list[i];
		}

		return nullptr;
	}

	void BuildPath(Path& path, PathCell* destiny)
	{
		path.road.push_back(destiny->mCell);
		if (destiny->mPrev)
			BuildPath(path, destiny->mPrev);
	}

	bool Find(std::vector<PathCell*>& list, Cell const* cell)
	{
		for (int i = 0; i < list.size(); i++)
		{
			if (list[i]->mCell == cell)
			{
				return true;
			}
		}

		return false;
	}

	void ClosestWay(Path& path, Cell* destiny, Grid* grid)
	{
		//Find the new Node to visit
		//Maybe we need new but not currently because its a placeholder
		PathCell* shortest = nullptr;
		float totalCost = FLT_MAX;

		//Look in the open list for the least costly cell
		for (int i = 0; i < path.openList.size(); i++)
		{
			//Heuristic with destiny
			//glm::vec2 number = path.openList[i]->mCell->GetPos();			
			float distance = CostBetweenCells(path.openList[i]->mCell, destiny);
			if ((path.openList[i]->cost + distance) < totalCost)
			{
				shortest = path.openList[i];
				totalCost = path.openList[i]->cost + distance;
			}
		}

		//Push the "closest" cell into the closed list
		path.closedList.push_back(shortest);
		//Erase the "Closest" cell from open list
		path.openList.erase(std::remove(path.openList.begin(), path.openList.end(), shortest), path.openList.end());

		//Check the 8 cells around it
		int neighbourNumber = grid->CheckNeighbourPath(shortest->mCell);
		int num = 0;

		for (int i = 0; i < neighbourNumber; i++)
		{
			Cell* cell = grid->mFreeNeighbourPath[i];
			//If one of the neighbours is destiny we finished
			if (cell == destiny)
			{
				PathCell celda = PathCell(destiny, shortest, CostBetweenCells(destiny, shortest->mCell));
				BuildPath(path, &celda);
				return;
			}
			float newCost = CostBetweenCells(cell, shortest->mCell) + shortest->cost;

			//Check if the current Cell has already been visited
			PathCell* closedOption = FindOption(cell, path.closedList);
			if (closedOption)
			{
				num++;
				//If the cell takes less cost to get here now replace it
				if (newCost < closedOption->cost)
				{
					closedOption->cost = newCost;
					closedOption->mPrev = shortest;
				}
				continue;
			}

			//We need to check if a cell is already in the open list
			PathCell* duplicate = FindOption(cell, path.openList);

			if (duplicate)
			{
				if (newCost < duplicate->cost)
				{
					duplicate->cost = newCost;
					duplicate->mPrev = shortest;
					num++;
				}
			}
			else
			{
				num++;
				path.openList.push_back(new PathCell(cell, shortest, newCost));
			}
		}

		return;
	}

	Path* Grid::FindPath(glm::vec2 start, glm::vec2 end)
	{
		std::queue<PathCell*> list;
		Path * camino = new Path;
		Cell* startCell = GetCell(start);
		Cell* destinyCell = GetCell(end);

		if (!startCell || !destinyCell)
			return camino;
		if (startCell == destinyCell)
			return camino;
		if (destinyCell->isOcuppied())
			return camino;
		
		PathCell* insert = new PathCell(startCell, nullptr, 0);
		//camino->closedList.push_back(insert);

		//Check the 8 cells around it

		int neighbourNumber = CheckNeighbourPath(startCell);
		for (int i = 0; i < neighbourNumber; i++)
		{
			Cell* cell = mFreeNeighbourPath[i];
			camino->openList.push_back(new PathCell(mFreeNeighbourPath[i], insert, CostBetweenCells(cell, startCell)));
		}

		//Cell* ending = std::find(camino.openList.begin, camino.openList.end, destinyCell);

		if (Find(camino->openList, destinyCell))
		{
			PathCell zelda = PathCell(destinyCell, insert, CostBetweenCells(destinyCell, insert->mCell));
			BuildPath(*camino, &zelda);
			return camino;
		}

 		while (camino->road.size() == 0)
		{			
			ClosestWay(*camino, destinyCell, this);
		}
		
		return camino;
	}

	std::vector<glm::vec3> * Grid::GetPath(glm::vec3 pStart, glm::vec3 pEnd)
	{
		Path* path;
		if (GetDir(pEnd) != glm::vec3(0))
			path = FindPath(glm::vec2(pStart.x, pStart.z), glm::vec2(pEnd.x, pEnd.z));
		else
			path = new Path;
		
		if (path->road.size() == 0)
			path->road.push_back(GetCell(glm::vec2(pStart.x, pStart.z)));

		std::vector<glm::vec3>* vector = new std::vector<glm::vec3>();
		
		for (int it = path->road.size() - 1; it >= 0; it--)
		{
			glm::ivec2 temp = path->road[it]->GetPos() * mSizeCell;
			vector->push_back(glm::vec3(temp.x, 0, temp.y));
		}

		delete(path);
		return vector;
	}

	void Grid::Clear()
	{
		for (int i = 0; i < mCells.size(); i++)
		{
			delete mCells[i];
			mCells[i] = nullptr;
		}

		mCells.clear();
	}

	void Grid::Desocupy()
	{
		for (int i = 0; i < mCells.size(); i++)
		{
			mCells[i]->Desoccupie();
		}
	}

	Path::~Path()
	{
		Clear();
	}

	void Path::Clear()
	{
		for (int i = 0; i < openList.size(); i++)
		{
			delete openList[i];
			openList[i] = nullptr;
		}
		
		for (int i = 0; i < closedList.size(); i++)
		{		
			delete closedList[i];
			closedList[i] = nullptr;
		}
	}

	bool Grid::CheckValid(int index, glm::vec2 dir)
	{
		if (Dot(dir, dir) > 1)
		{
			int x = dir.x;
			int y = dir.y;

			if (mCells[index + dir.x]->isOcuppied())
				return false;
			if (mCells[index + dir.y*mWidth]->isOcuppied())
				return false;
		}
		return true;
	}

	int Grid::GetCellSize() const
	{
		return mSizeCell;
	}

	int Grid::GetWidth() const
	{
		return mWidth;
	}

	int Grid::GetHeight() const
	{
		return mHeight;
	}

	void Grid::LoadGrid(const std::string& pFilename)
	{
		std::string filepath = "data/binary/";
		filepath.append(pFilename);

		std::ifstream fp(filepath, std::ios::binary);
		if (fp.is_open() && fp.good())
		{
			fp.read((char*)&mSizeCell, sizeof(int));
			fp.read((char*)&mWidth, sizeof(int));
			fp.read((char*)&mHeight, sizeof(int));

			int MapSizeX = (mWidth - 1) * mSizeCell;
			int MapSizeY = (mHeight - 1) * mSizeCell;

			this->CreateGrid(mSizeCell, MapSizeX, MapSizeY);

			for (auto& cell : mCells)
			{
				fp.read((char*)&cell->mOccupied, sizeof(bool));
			}
		}
	}

	void Grid::SaveGrid(const std::string& pFilename) const
	{
		std::string filepath = "data/binary/";
		filepath.append(pFilename);
		std::ofstream fp(filepath, std::ofstream::binary);
		if (fp.is_open() && fp.good())
		{
			fp.write((char*)&mSizeCell, sizeof(int));
			fp.write((char*)&mWidth, sizeof(int));
			fp.write((char*)&mHeight, sizeof(int));

			for (auto& cell : mCells)
			{
				fp.write((char*)&cell->mOccupied, sizeof(bool));
			}
		}
	}

	void Grid::Flow(Cell* pDestiny, std::vector<FlowCell>& field, float pParentCost, std::queue<ListCell>& list)
	{
		int neighbourNumber = CheckNeighbour(pDestiny);
		for (int i = 0; i < neighbourNumber; i++)
		{
			Cell* cell = mFreeNeighbour[i];
			glm::ivec2 pos = cell->GetPos();
			int index = pos.x + pos.y * mWidth;

			float& cost = field[index].mCost;

			if (cost == -1)
				continue;

			if (!CheckValid(index, pDestiny->GetPos() - pos))
				continue;

			if (cost == 0)
			{
				glm::ivec2 newDir = pDestiny->GetPos() - pos;
				field[index].mDir = newDir;
				cost = pParentCost + Dot(newDir, newDir);

				ListCell c(cell, cost);
				list.push(c);
			}
			else
			{

				glm::ivec2 newVec = pDestiny->GetPos() - pos;
				float newCost = pParentCost + Dot(newVec, newVec);
				if (newCost < cost)
				{
					field[index].mDir = newVec;
					cost = newCost;
					ListCell c(cell, cost);
					list.push(c);
					//Flow(cell, field, field[index].mCost);
				}
			}
		}
	}

	void Grid::GetFlowField(glm::vec2 pDestiny)
	{
		this->mflowObjective = pDestiny;
		Cell* destiny = GetCell(pDestiny);

		if (destiny && !destiny->isOcuppied())
		{
			std::vector<FlowCell> FlowField;
			for (int i = 0; i < mCells.size(); i++)
				FlowField.push_back(FlowCell());

			glm::ivec2 pos = destiny->GetPos();
			int index = pos.x + pos.y * mWidth;
			//Set a cost of - to the objective to identify it
			FlowField[index].mCost = -1;
			std::queue<ListCell> list;
			
			ListCell c(destiny, 0);
			list.push(c);
			
			while (!list.empty())
			{
				ListCell visita = list.front();
				list.pop();
				Flow(visita.mCell, FlowField, visita.mCost, list);
			}
			mFlow = FlowField;
		}
		else
			mFlow = std::vector<FlowCell>();
	}


	void Grid::MakeFlowField(glm::vec2 pDestiny, std::vector<FlowCell>& FlowF, bool * done)
	{
		mtx.lock();

		if (done != nullptr)
		{
			*done = false;
		}

		this->mflowObjective = pDestiny;
		Cell* destiny = GetCell(pDestiny);

		if (destiny && !destiny->isOcuppied())
		{
			std::vector<FlowCell> FlowField;
			for (int i = 0; i < mCells.size(); i++)
				FlowField.push_back(FlowCell());

			glm::ivec2 pos = destiny->GetPos();
			int index = pos.x + pos.y * mWidth;
			//Set a cost of - to the objective to identify it
			FlowField[index].mCost = -1;
			std::queue<ListCell> list;

			ListCell c(destiny, 0);
			list.push(c);

			while (!list.empty())
			{
				if (mRestart)
				{
					mtx.unlock();
					return;
				}
				ListCell visita = list.front();
				list.pop();
				Flow(visita.mCell, FlowField, visita.mCost, list);
			}
			
			FlowF = FlowField;
		}
		else
			FlowF = std::vector<FlowCell>();

		if (done != nullptr)
		{
			*done = true;
		}

		mtx.unlock();
	}

	void Grid::CopyFlow(std::vector<FlowCell>& FlowF)
	{
		std::swap(mFlow, FlowF);
		return;
	}

	glm::vec2 Grid::GetDir(glm::vec2 pos) const
	{
		glm::vec2 posGrid = glm::vec2(pos.x / mSizeCell, pos.y / mSizeCell);

		int indexX = posGrid.x;
		int indexY = posGrid.y;

		if (indexX < 0 || indexY < 0 || indexX >= mWidth || indexY >= mHeight)
			return glm::vec2(0.f);

		if (mFlow.size() > 0)
			return mFlow[indexX + indexY * mWidth].mDir;
		else
			return glm::vec2(0.f);
	}

	glm::vec3 Grid::GetDir(glm::vec3 pos) const
	{
		glm::vec2 dir = GetDir(pos.xz);
		return glm::vec3(dir.x, 0, dir.y);
	}

	bool Grid::VisionRay(glm::vec2 Origin, glm::vec2 End)
	{
		return false;
	}
}