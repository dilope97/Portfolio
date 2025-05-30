#pragma once
#include <vector>
#include "Core/ResourceManager.h"
#include "LogicSystem.h"
#include <queue>
#include <glm/glm.hpp>
#include <thread>
namespace QuimeraEngine
{

	class ColliderComp;
	class GameObject;

	
	class FlowCell
	{
	public:
		FlowCell() : mDir(glm::vec2(0)), mCost(0) {};
		glm::vec2 mDir;
		float mCost;
	};

	class Cell
	{
	public:
		Cell() : mOccupied(false), mPos{} {}
		void Occupie();
		void Desoccupie();

		bool isOcuppied() const;
		glm::ivec2 GetPos();
		void SetPos(glm::vec2 vec);

		bool mOccupied;
	private:
		glm::ivec2 mPos;
	};

	class ListCell
	{
	public:
		ListCell(Cell* cell, float cost) : mCell(cell), mCost(cost) {}
		Cell* mCell;
		float mCost;
	};

	class PathCell
	{
	public:
		PathCell(Cell* cell, PathCell* prev, float cost_) : mCell(cell), mPrev(prev), cost(cost_) {}
		Cell* mCell;
		PathCell* mPrev;

		float cost;
	};

	class Path
	{
		
		void Clear();
	public:
		~Path();
		std::vector<Cell*> road;

		std::vector<PathCell*> openList;
		std::vector<PathCell*> closedList;
	};

	class Grid : public LogicComp
	{
		std::vector<Cell*> mCells;
		std::vector<FlowCell> mFlow;
		int mWidth;
		int mHeight;
		int mSizeCell;
	public:
		void OnCreate() override;
		void CreateGrid(int sizeCell_, int MapSizeX, int MapSizeY);
		void Insert(ColliderComp* aabb);
		void Free(ColliderComp* aabb);
		Grid();
		~Grid();
		void Put(GameObject* pObj);
		void Take(GameObject* pObj);
		void Fill(const std::vector<GameObject*> pObjs);
		void Unfill(const std::vector<GameObject*> pObjs);
		bool Check(float posX, float posY);
		bool CheckNeigbourOccupie(float posX, float posY);
		void Update() override;
		void Clear();
		void Desocupy();

		int GetCellSize() const;
		int GetWidth() const;
		int GetHeight() const;

		Cell* GetCell(glm::vec2 pos);
		void DrawPath(Path* path);

		void Flow(Cell* pDestiny, std::vector<FlowCell>& field, float pParentCost, std::queue<ListCell>& list);
		bool CheckValid(int index, glm::vec2 dir);

		void LoadGrid(const std::string& pFilename);
		void SaveGrid(const std::string& pFilename) const;

		void StreamRead(const nlohmann::json& j) override;
		void StreamWrite(nlohmann::json& j) const override;

		int mEditSizeCell;
		int mEditMapSizeX;
		int mEditMapSizeY;
		bool debugGrid;

		bool mRestart;
		int gridId;
		glm::vec2 mflowObjective;
		std::vector<Cell*> mFreeNeighbour;
		std::vector<Cell*> mFreeNeighbourPath;
	public:
		int CheckNeighbour(Cell* cell);
		int CheckNeighbourPath(Cell* cell);

		std::thread mThread;

		Path* FindPath(glm::vec2 start, glm::vec2 end);
		std::vector<glm::vec3>* GetPath(glm::vec3 pStart, glm::vec3 pEnd);
		void GetFlowField(glm::vec2 pDestiny);
		void MakeFlowField(glm::vec2 pDestiny, std::vector<FlowCell>& FlowF, bool * done = nullptr);
		void CopyFlow(std::vector<FlowCell>& FlowF);
		glm::vec2 GetDir(glm::vec2 pos) const;
		glm::vec3 GetDir(glm::vec3 pos) const;
		bool VisionRay(glm::vec2 Origin, glm::vec2 End);
	};
}
