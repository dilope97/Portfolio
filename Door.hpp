#pragma once
#include <thread>
#include <Player/PlayerController.h>
#include "Audio/Audio.h"

namespace QuimeraEngine
{
	class Grid;
	class GameObject;
	class FlowCell;
}
namespace QuimeraGameplay
{
	class Selectable;


	class Door : public QuimeraEngine::LogicComp
	{
	public:
		Door() : mSelect(nullptr), mGrid(nullptr), opened(true), animation(false), Door1(nullptr), Door2(nullptr), t(0), d1(0), d2(0), group(0), done(false), reset(false) {}
		~Door();
		virtual void OnCreate();
		virtual void Update();

		void Close();
		void Open();
		void UpdateSquads();

	private:
		QuimeraEngine::Grid* mGrid;
		Selectable* mSelect;
		bool opened;
		bool animation;
		unsigned group;


		QuimeraEngine::GameObject* Door1;
		float d1;
		QuimeraEngine::GameObject* Door2;
		float d2;

		std::vector<QuimeraEngine::FlowCell> mFlow;
		float t;
		bool done;
		bool reset;

		static std::vector<std::list<Door*>> closedDoors;
		static std::vector<unsigned> maxDoors;
		static PlayerController* PlCn;
		FMOD::Channel* ch;
	};
}