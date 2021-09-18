#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <string>

GLuint game2_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > game2_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("game2.pnct"));
	game2_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > game2_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("game2.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = game2_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = game2_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

PlayMode::PlayMode() : scene(*game2_scene) {
	
	// random device
	std::random_device r;
	mt.seed(r());

	// get the game objects transform
	for (auto &transform : scene.transforms)
	{
		std::string objName = transform.name.substr(0, transform.name.find("."));
		uint32_t index = static_cast<uint32_t>(std::atoi(&transform.name[transform.name.size() - 1]));

		if (objName == "Box_0")
		{
			coffins[index].box = &transform;
			coffins[index].box_base_rotation = transform.rotation;
			coffins[index].box_range_rotation[0] = (transform.rotation * glm::angleAxis(
					glm::radians(-15.0f),
					glm::vec3(0.0f, 1.0f, 0.0f))).y;
			coffins[index].box_range_rotation[1] = (transform.rotation * glm::angleAxis(
					glm::radians(15.0f),
					glm::vec3(0.0f, 1.0f, 0.0f))).y;
		}
		else if (objName == "Cover_0")
		{
			coffins[index].cover = &transform;
			coffins[index].cover_base_rotation = transform.rotation;
			coffins[index].cover->rotation = coffins[index].cover_base_rotation * glm::angleAxis(
					glm::radians(55.0f),
					glm::vec3(0.0f, 1.0f, 0.0f)); 
		}
		else if (objName == "Ghost_0_0")
		{
			coffins[index].lives[0] = &transform;
			coffins[index].live_base_scale = transform.scale.x;
		}
		else if (objName == "Ghost_0_1")
		{
			coffins[index].lives[1] = &transform;
		}
		else if (objName == "Ghost_0_2")
		{
			coffins[index].lives[2] = &transform;
		}
		else if (objName == "CrossSword_0")
		{
			crosses[index].cross = &transform;
			crosses[index].base_position = transform.position;
			crosses[index].target_position = transform.position;
			crosses[index].target_position.z -= 6.2f;
		}
	}

	// get the camera
	camera = &scene.cameras.front();
	light = &scene.lights.front();

	// set start game state 
	coffins[current_coffin].cover->rotation = coffins[current_coffin].cover_base_rotation *
		glm::angleAxis(
			glm::radians(55.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
	coffins[next_coffin].isClosed = true;
		
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		size_t row = current_coffin / 3;
		size_t col = current_coffin % 3;
		if (evt.key.keysym.sym == SDLK_a) {
			if (col == 0) col = 2;
			else if (col > 0) col -= 1;
		} else if (evt.key.keysym.sym == SDLK_d) {
			if (col == 2) col = 0;
			else if (col < 2) col += 1;
		} else if (evt.key.keysym.sym == SDLK_w) {
			if (row == 0) row = 2;
			else if (row > 0) row -= 1;
		} else if (evt.key.keysym.sym == SDLK_s) {
			if (row == 2) row = 0;
			else if (row < 2) row += 1;
		}
		next_coffin = 3 * row + col;
		return true;
	}

	return false;
}

void PlayMode::update(float elapsed) {

	if (isEnd) return;

	if (isSetting > 0.0f)
	{
		isSetting -= elapsed;
		current_interval -= elapsed;
		for (Coffin &coffin: coffins)
		{
			coffin.cover->rotation = coffin.cover->rotation * glm::angleAxis(
				glm::radians(-0.5f),
				glm::vec3(0.0f, 1.0f, 0.0f));;
			if (coffin.cover->rotation.y < coffin.cover_base_rotation.y)
			{
				coffin.cover->rotation = coffin.cover_base_rotation;
			}
		}
		return;
	}
	
	// set game state
	if (remain_time > 0.0f)
	{
		remain_time -= elapsed;
		if (remain_time < 60.0f && triggerCrossInterval > 1.5f)
		{
			triggerCrossInterval = 3.0f * (remain_time / 60.0f);
		}
	} 
	
	else if (!isEnd)
	{
		// set the game to end
		isEnd = true;
		remain_time = 0.0f;
		coffins[current_coffin].cover->rotation = coffins[current_coffin].cover_base_rotation;
		coffins[current_coffin].isClosed = false;

		// set the final score
		for (Coffin &coffin: coffins)
		{
			coffin.box->rotation = coffin.box_base_rotation;
			if (coffin.hitTimes == 3) continue;
			remain_coffins ++;
		}
		score = 100 * remain_lives + 300 * remain_coffins;
		
		return;		
	}

	// change selected coffin
	if (next_coffin != current_coffin)
	{
		// close next coffin
		coffins[next_coffin].cover->rotation = coffins[next_coffin].cover_base_rotation *
		glm::angleAxis(
			glm::radians(55.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
		coffins[next_coffin].isClosed = true;
		
		// open current coffin
		coffins[current_coffin].cover->rotation = coffins[current_coffin].cover_base_rotation;
		coffins[current_coffin].isClosed = false;
		
		current_coffin = next_coffin;
	}

	// set random cross move
	current_interval -= elapsed;
	if (current_interval < 0.0f)
	{
		uint8_t temp_index;
		do {
			static std::uniform_int_distribution<int> distrib(0, crosses.size());
			temp_index = static_cast<uint8_t>(distrib(mt)); 
		} while (crosses[temp_index].isMoving);
		crosses[temp_index].isMoving = true;
		current_interval = triggerCrossInterval;
	}
	
	// move cross and detect collision
	for (Cross &cross: crosses)
	{
		// the cross move to the edges
		if (cross.isMoving)
		{
			cross.cross->position.z += cross.movingSpeed;
			if (cross.cross->position.z < cross.target_position.z || cross.cross->position.z > cross.base_position.z)
			{
				cross.movingSpeed *= -1.0f;
			}

			if (cross.cross->position.z > cross.base_position.z)
			{
				cross.isMoving = false;
				cross.cross->position.z = cross.base_position.z;
			}
		}
		
		size_t index = &cross - crosses.begin();
		
		// the cross hit the coffin
		if (cross.cross->position.z < cross.target_position.z && !coffins[index].isClosed &&
			coffins[index].hitTimes < coffins[index].lives.size())
		{
			coffins[index].lives[coffins[index].hitTimes]->scale *= 0.9f;
			coffins[index].hitTimes += 1;
			remain_lives --;
		}

		// the box animation
		if (cross.isMoving && cross.movingSpeed < 0.0f && !coffins[index].isClosed)
		{
			float approach = (cross.cross->position.z - cross.target_position.z) / (cross.base_position.z - cross.target_position.z);
			coffins[index].box->rotation = coffins[index].box->rotation * glm::angleAxis(
					glm::radians(coffins[index].box_rotation_speed * (1.f - approach) * 2),
					glm::vec3(0.0f, 1.0f, 0.0f));
			if (coffins[index].box->rotation.y >= coffins[index].box_range_rotation[1] || 
				coffins[index].box->rotation.y <= coffins[index].box_range_rotation[0])
			{
				coffins[index].box_rotation_speed *= -1.0f;
			}
		}
		else
		{
			coffins[index].box->rotation = coffins[index].box_base_rotation;
			if (coffins[index].box_rotation_speed < 0.0f) coffins[index].box_rotation_speed *= -1.0f;
		}
		
		// the lives animation trigger
		for (Scene::Transform * live: coffins[index].lives)
		{	
			// die animation
			if (live->scale.x < coffins[index].live_base_scale && live->scale.x > 0.0f)
			{
				live->rotation = live->rotation * glm::angleAxis(
					glm::radians(10.0f),
					glm::vec3(0.0f, 0.0f, 1.0f));
				live->scale -= glm::vec3(0.01f);
				if (live->scale.x < 0.f) live->scale = glm::vec3(0.0f);
			}
		}
		
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		uint8_t minute = remain_time / 60.0f;
		std::string minute_string = std::to_string(minute);
		if (minute < 10) minute_string = "0" + minute_string;
		uint8_t second = remain_time - minute * 60;
		std::string second_string = std::to_string(second);
		if (second < 10) second_string = "0" + second_string;
		std::string infor = isEnd ? std::to_string(score) : minute_string + ":" + second_string;
		std::string title = isEnd ? "Score" : "";
		std::string explain = isEnd ? std::to_string(remain_lives) + " skulls left, resurrect " + 
			std::to_string(remain_coffins) + " vampires" : "Use WASD to cover the coffins";
		if (isSetting > 0.0f) explain = "";
		


		constexpr float H = 0.5f;
		constexpr float h = 0.2f;
		constexpr float h1 = 0.15f;
		lines.draw_text(infor,
			glm::vec3(-aspect + 4.8f * H, -1.0 + 0.2f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(title,
			glm::vec3(-aspect + 6.0f * H, -1.0 + 1.5f * H, 0.0),
			glm::vec3(h, 0.0f, 0.0f), glm::vec3(0.0f, h, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(explain,
				glm::vec3(-aspect + 0.2f * h1, -1.0 + 0.2f * h1, 0.0),
				glm::vec3(h1, 0.0f, 0.0f), glm::vec3(0.0f, h1, 0.0f),
				glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(infor,
			glm::vec3(-aspect + 4.8f * H + ofs, -1.0 + 0.2f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		lines.draw_text(title,
			glm::vec3(-aspect + 6.0f * H + ofs, -1.0 + 1.5f * H + ofs, 0.0),
			glm::vec3(h, 0.0f, 0.0f), glm::vec3(0.0f, h, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		lines.draw_text(infor,
			glm::vec3(-aspect + 4.8f * H + 2 * ofs, -1.0 + 0.2f * H + 2 * ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));
	}
}
