#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <array>
#include <random>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// game state
	std::mt19937 mt;
	uint8_t current_coffin = 4;
	uint8_t next_coffin = 4;
	float triggerCrossInterval = 3.0f;
	float current_interval = 3.0f;
	float originalTime = 90.0f;
	float remain_time = originalTime;
	uint8_t remain_lives = 27;
	uint8_t remain_coffins = 0;
	uint32_t score = 0;
	bool isEnd = false;
	float isSetting = 3.0f;

	
	//game components
	struct Coffin {
		Coffin(){
			isSelected = false;
			isClosed = false;
			hitTimes = 0;
			box = nullptr;
			cover = nullptr;
			lives[0] = nullptr;
			lives[1] = nullptr;
			lives[2] = nullptr;
		};
		bool isSelected;
		bool isClosed;
		uint8_t hitTimes;
		Scene::Transform *box;
		Scene::Transform *cover;
		std::array<Scene::Transform *, 3> lives;
		glm::quat cover_base_rotation;
		glm::quat box_base_rotation;
		std::array<float,2>  box_range_rotation;
		float box_rotation_speed = 3.0f;
		float live_base_scale;
	};

	struct Cross {
		Cross(){
			isMoving = false;
			movingSpeed = -0.025f;
			cross = nullptr;
		}
		bool isMoving;
		float movingSpeed;
		Scene::Transform *cross;
		glm::vec3 base_position;
		glm::vec3 target_position;
	};

    std::array<Coffin,9> coffins;
	std::array<Cross,9>  crosses;

	
	//camera:
	Scene::Camera *camera = nullptr;

	//light:
	Scene::Light *light = nullptr;

};
