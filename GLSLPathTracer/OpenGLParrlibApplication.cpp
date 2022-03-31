// OpenGLParrlibApplication.cpp : Defines the entry point for the console application.
//

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#define _CRT_SECURE_NO_WARNINGS


#include <math.h>
#include <string>
#include <experimental\filesystem>
#include <time.h>
#include <chrono>

#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include <parrlib/math/vector2f.h>

#include <parrlibgl/context.h>
#include <parrlibgl/util.h>
#include <parrlibgl/shader.h>
#include <parrlibgl/frameBufferobject.h>
#include <parrlibgl/debug.h>
#include <parrlibgl/debugmenu/debugmenu.h>

#include "Sphere.h"
#include "Plane.h"
#include "Intersection.h"
#include "Ray.h"

#include <fstream>

using namespace prb;

#define PI 3.141592

float toRadians(float angle) {
	return (angle * PI) / 180.0f;
}

float toDegrees(float theta) {
	return (theta * 180.0f) / PI;
}

Vector3f rotateX(Vector3f v, float angle) {
	angle = (float)toRadians(angle);

	float x = v.x;
	float y = v.y*cos(angle) - v.z*sin(angle);
	float z = v.y*sin(angle) + v.z*cos(angle);

	return Vector3f(x, y, z);
}

Vector3f rotateY(Vector3f v, float angle) {
	angle = (float)toRadians(angle);

	float x = v.x*cos(angle) + v.z*sin(angle);
	float y = v.y;
	float z = -v.x*sin(angle) + v.z*cos(angle);

	return Vector3f(x, y, z);
}

Vector3f rotateZ(Vector3f v, float angle) {
	angle = (float)toRadians(angle);

	float x = v.x*cos(angle) - v.y*sin(angle);
	float y = v.x*sin(angle) + v.y*cos(angle);
	float z = v.z;

	return Vector3f(x, y, z);
}

Vector3f rotate(Vector3f v, float x, float y, float z) {
	return rotateZ(rotateY(rotateX(v, x), y), z);
}

Vector3f rotate(Vector3f v, Vector3f r) {
	return rotate(v, r.x, r.y, r.z);
}

int numSp;
Sphere *sp;

int numPl;
Plane *pl;

Intersection intersects(Ray ray, Sphere sp) {
	// Check for a Negative Square Root
	Vector3f oc = sp.pos - ray.orig;
	float l = ray.dir.dot(oc);
	float det = pow(l, 2.0) - oc.dot(oc) + pow(sp.radius, 2.0);
	Intersection miss;
	miss.notNull = false;

	if (det < 0.0) return miss;

	// Find the Closer of Two Solutions
	float len = l - sqrt(det);
	if (len < 0.0) len = l + sqrt(det);
	if (len < 0.0) return miss;
	return Intersection(ray.orig + ray.dir*len, (ray.orig + ray.dir*len - sp.pos) / sp.radius, len, sp.mat);
}

Intersection intersects(Ray ray, Plane pl) {
	Intersection nll;
	nll.notNull = false;

	Vector3f norm = pl.normal*-1.0f;
	float denom = norm.dot(ray.dir);
	float t = 0.;
	if (denom > 0.0000001) {
		Vector3f p0l0 = pl.pos - ray.orig;
		t = p0l0.dot(norm) / denom;
		Vector3f point = ray.orig + ray.dir*t;
		return t >= 0. ? Intersection(point, norm*-1.0f, t, pl.mat) : nll;
	}

	return nll;
}

Intersection trace(Ray ray) {
	Intersection inters;
	float dist = 1e09;

	for (int i = 0; i < numPl; i++) {
		Intersection tInters = intersects(ray, pl[i]);
		if (tInters.notNull) {
			if (tInters.dist < dist) {
				inters = tInters;
				dist = tInters.dist;
			}
		}
	}

	for (int i = 0; i < numSp; i++) {
		Intersection tInters = intersects(ray, sp[i]);
		if (tInters.notNull) {
			if (tInters.dist < dist) {
				inters = tInters;
				dist = tInters.dist;
			}
		}
	}

	return inters;
}

bool loadScene(std::string fileName);
bool loadShot(std::string fileName);

bool saveScene(std::string fileName);
bool saveShot(std::string fileName);

std::string curScene, curShot;

void writeToFile(std::string fileName);

float ambientIntensity = 10.0f;
Vector3f ambient = Vector3f(0.0f, 0.0f, 0.0f);

Vector3f camPos = Vector3f(0.0f,1.0f,-4.0f);
Vector3f rot = Vector3f(0.0f,0.0f,0.0f);
float sens = 1.0f;
float fovMult = 0.7f;

float aperture = 0.0f;
float focusDist = 3.5f;
float focusInterval = 0.0f;

Vector3f up = Vector3f(0.0f, 1.0f, 0.0f);
Vector3f defForward = Vector3f(0.0f, 0.0f, 1.0f);
float speed = 2.0f;
float rSpeed = speed;

Shader pathShader;
Shader accumShader;

FrameBufferObject fbo;
bool sceneChanged = false;

int spp = 16384; //2^15;//2^14;
int curSpp = 0;

int bounces = 2;

int sppRate = 1;

bool is_number(const std::string& s) {
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](char c) { std::locale loc;  return !std::isdigit(c, loc); }) == s.end();
}

void init() {
	//spp = pow(2, 18);

	pathShader = Shader("pathTracer.vert", "pathTracer.frag");
	accumShader = Shader("assets/shaders/standard.vert", "accumShader.frag");
	
	debugmenu::addConsoleCommand(L"listscenes", L"list all scenes available.", {},
	[&](std::vector<std::wstring> args) -> std::wstring {
		std::wstring result = L"";
		std::wstring path = L"scenes/";
		int sceneN = 0;
		for (const auto & entry : std::experimental::filesystem::directory_iterator(path)) {
			std::wstring scene = stru::path(entry.path())[-1];
			//scene = scene.substr(scene.find('\\')+1);
			//scene = scene.substr(0, scene.find('.'));
			result += stru::fmtw(sceneN, ": ", scene, "\n");

			sceneN++;
		}

		return result;
	});

	debugmenu::addConsoleCommand(L"loadscene", L"load a scene.", {
		{ L"scene", L"the scene to load (can be a number)." }
	},
	[&](std::vector<std::wstring> args) -> std::wstring {
		if (args.size() == 0) { return L"error: please specify a scene"; }

		std::wstring toLoad = args[0];

		if (is_number(stru::tostr(toLoad))) {
			std::wstring path = L"scenes/";

			int iter = 0;
			bool found = false;
			for (const auto & entry : std::experimental::filesystem::directory_iterator(path)) {
				if (iter >= stru::toi(toLoad)) { 
					found = true; 
					toLoad = ((std::wstring)entry.path()).substr(((std::wstring)entry.path()).find(L'\\')+1);
					toLoad = toLoad.substr(0, toLoad.find('.'));
					break; 
				}
				iter++;
			}

			if (!found) return stru::fmtw("error: scene n.", args[0], " could not be found");
		}

		if (!loadScene(stru::tostr(toLoad))) return stru::fmtw("error: scene '", toLoad, "' could not be loaded");

		return L"";
	});

	debugmenu::addConsoleCommand(L"getinfo", L"prints the current rendering settings.", {},
	[&](std::vector<std::wstring> args) -> std::wstring {
		return stru::fmtw(
			"res x: ", cst::resx(), "\n",
			"res y: ", cst::resy(), "\n",
			"spp rate: ", sppRate, "\n",
			"total spp: ", spp, "\n",
			"focus distance: ", focusDist, "\n",
			"focus interval: ", focusInterval, "\n",
			"fov multiplier: ", fovMult, "\n"
		);
	});

	debugmenu::addConsoleCommand(L"setmaxspp", L"sets how many samples per pixel to accumulate over this image.", {
		{ L"spp rate", L"the amount of samples (number)." }
	},
	[&](std::vector<std::wstring> args) -> std::wstring {
		if (args.size() == 0) return L"error: please specify a quantity";
		if (!is_number(stru::tostr(args[0]).c_str())) return L"error: you must specify a number";
		if (stru::toi(args[0]) <= 0) return L"error: please specify a quantity greater than 0";

		spp = stru::toi(args[0]);

		return L"";
	});

	debugmenu::addConsoleCommand(L"setspprate", L"sets how many samples to take every frame.", {
		{ L"spp rate", L"the amount of samples to take every frame (number)." }
	},
	[&](std::vector<std::wstring> args) -> std::wstring {
		if (args.size() == 0) return L"error: please specify a quantity";
		if (!is_number(stru::tostr(args[0]).c_str())) return L"error: you must specify a number";
		if (stru::toi(args[0]) <= 0) return L"error: please specify a quantity greater than 0";

		sppRate = stru::toi(args[0]);

		return L"";
	});

	//loadScene("illumplane.scene");

	//numSp = 6;
	//sp = new Sphere[numSp];

	//int iter = 0;
	//sp[iter++] = Sphere(Vector3f(2.0f, 1.0f, 0.0f), 1.0f, Material(Vector3f(0.9f,0.1f,0.1f), 0.2f, 0.0f, 0.0f));
	//sp[iter++] = Sphere(Vector3f(-2.0f, 1.0f, 0.0f), 1.0f, Material(Vector3f(0.1f, 0.1f, 0.9f), 0.0f, 0.0f, 0.0f));
	//sp[iter++] = Sphere(Vector3f(0.7f, 4.3f, 0.0f), 2.5f, Material(Vector3f(0.9f, 0.9f, 0.9f), 0.0f, 0.0f, 0.0f));

	////sp[iter++] = Sphere(Vector3f(0.0f, -50.8f, 0.0f), 50.0f, Material(Vector4f(0.9f, 0.9f, 0.9f, 1.0f), 0.0f, 0.0f, 0));
	////sp[iter++] = Sphere(Vector3f(80.8f, 0.0f, 0.0f), 50.0f, Material(Vector4f(0.9f, 0.9f, 0.9f, 1.0f), 0.0f, 0.0f, 0));
	////sp[iter++] = Sphere(Vector3f(-80.8f, 0.0f, 0.0f), 50.0f, Material(Vector4f(0.9f, 0.9f, 0.9f, 1.0f), 0.0f, 0.0f, 0));

	//sp[iter++] = Sphere(Vector3f(0.0f, 5.0f, 0.0f), 2.0f, Material(Vector3f(0.9f, 0.9f, 0.9f), 0.0f, 0.0f, 1000.0f));

	//numPl = 6;
	//pl = new Plane[numPl];

	//iter = 0;
	//pl[iter++] = Plane(Vector3f(0.0f,0.0f,0.0f), Vector3f(0.0f,1.0f,0.0f), Material(Vector3f(0.9f,0.9f,0.9f), 0.0f, 0.0f, 0.0f));
	//pl[iter++] = Plane(Vector3f(5.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f), Material(Vector3f(0.1f, 0.9f, 0.1f), 0.0f, 0.0f, 0.0f));
	//pl[iter++] = Plane(Vector3f(-5.0f, 0.0f, 0.0f), Vector3f(1.0f, 0.0f, 0.0f), Material(Vector3f(0.9f, 0.1f, 0.1f), 0.0f, 0.0f, 0.0f));
	//pl[iter++] = Plane(Vector3f(0.0f, 0.0f, 5.0f), Vector3f(0.0f, 0.0f, -1.0f), Material(Vector3f(0.1f, 0.1f, 0.9f), 0.0f, 0.0f, 0.0f));
	//pl[iter++] = Plane(Vector3f(0.0f, 5.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f), Material(Vector3f(0.9f, 0.9f, 0.9f), 0.0f, 0.0f, 0.0f));
	//pl[iter++] = Plane(Vector3f(0.0f, 0.0f, -5.0f), Vector3f(0.0f, 0.0f, 1.0f), Material(Vector3f(0.9f, 0.9f, 0.9f), 0.0f, 0.0f, 0.0f));

	fbo = FrameBufferObject(cst::resx(), cst::resy(), GL_NEAREST, GL_NEAREST, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	//fbo = FrameBufferObject(constants::resx(), constants::resy());
	
	loadShot("illumplane");
	//loadShot("simple2");

	Context::setClearColor(vec4(1.0f,1.0f,1.0f,1.0f));
}

void resetGraphics() {
	sceneChanged = true;
	curSpp = 0;
}

void update() {
	/*sp[3].pos.x = sin(Time::time)*3.0f;
	sp[3].pos.z = cos(Time::time)*3.0f;

	sp[4].pos.x = sin(Time::time+3.14f)*3.0f;
	sp[4].pos.z = cos(Time::time+3.14f)*3.0f;*/

	Vector3f forward = rotate(defForward, rot);

	Vector3f right = up.cross(forward).normalized();

	if (input::getKey(GLFW_KEY_LEFT_SHIFT)) {
		rSpeed = speed * 8.0f;
	}
	else {
		rSpeed = speed;
	}

	if (input::getKey(GLFW_KEY_W)) {
		camPos += forward * rSpeed*gtime::deltaTime;
		resetGraphics();
	}
	if (input::getKey(GLFW_KEY_S)) {
		camPos -= forward * rSpeed* gtime::deltaTime;
		resetGraphics();
	}

	if (input::getKey(GLFW_KEY_A)) {
		camPos -= right * rSpeed* gtime::deltaTime;
		resetGraphics();
	}
	if (input::getKey(GLFW_KEY_D)) {
		camPos += right * rSpeed* gtime::deltaTime;
		resetGraphics();
	}

	if (input::getKey(GLFW_KEY_SPACE)) {
		camPos += up * rSpeed* gtime::deltaTime;
		resetGraphics();
	}
	if (input::getKey(GLFW_KEY_C)) {
		camPos -= up * rSpeed* gtime::deltaTime;
		resetGraphics();
	}

	if (input::getKeyDown(GLFW_KEY_E)) {
		bounces++;
		resetGraphics();
	}
	if (input::getKeyDown(GLFW_KEY_Q)) {
		bounces--;
		resetGraphics();
	}

	if (input::getKey(GLFW_KEY_X)) {
		aperture += input::getMouseDelta().x;
		resetGraphics();
	}

	if (input::getKey(GLFW_KEY_V)) {
		focusInterval += input::getMouseDelta().x;
		resetGraphics();
	}

	focusInterval = max(0.0f, min(focusInterval, focusDist - 0.01f));

	if (input::getKeyDown(GLFW_KEY_R)) {
		aperture = 0.0f;
		resetGraphics();
	}

	if (input::getKeyDown(GLFW_KEY_G)) {
		//writeToFile("sample");

		GLfloat* data = new GLfloat[fbo.sizex() * fbo.sizey() * 3];

		fbo.bind();
			glReadPixels(0, 0, fbo.sizex(), fbo.sizey(), GL_RGB, GL_FLOAT, data);
		fbo.unbind();

		for (int i = 0; i < fbo.sizex() * fbo.sizey() * 3; i++) data[i] /= (float)curSpp;
		
		std::time_t t = std::time(0);   // get time now
		std::tm* now = std::localtime(&t);
		auto p = std::chrono::system_clock::now();

		util::writeToBMP(stru::fmt("sample_", now->tm_year + 1900, "_", now->tm_mon + 1, "_", now->tm_mday, "_", time(0)%(24*60*60), ".bmp"), data, fbo.size());
	}

	if (input::getCursorInputMode() == GLFW_CURSOR_NORMAL) {
		if(input::getMouse(1)) {
			Vector2f mouseCoords = input::getMousePos();
			Ray ray = Ray(camPos, rotate(Vector3f(mouseCoords.x, mouseCoords.y, fovMult).normalized(), rot));

			Intersection inters = trace(ray);
			if (inters.notNull) {
				focusDist = inters.dist;

				resetGraphics();
			}
		}
	}

	if (input::getKeyDown(GLFW_KEY_Z)) {
		input::setCursorInputMode(input::getCursorInputMode() == GLFW_CURSOR_NORMAL ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}

	int scroll = input::getMouseScrollWheel();
	if (scroll > 0) {
		fovMult *= 2.0f;
		resetGraphics();
	}
	else if (scroll < 0) {
		fovMult /= 2.0f;
		resetGraphics();
	}

	if (input::getCursorInputMode() == GLFW_CURSOR_DISABLED) {
		Vector2f mouseDelta = input::getMouseDeltaConst() * sens / std::fmax(fovMult, 1.f);

		if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
			rot.y += mouseDelta.x;
			rot.x += mouseDelta.y;
			resetGraphics();
		}
	}

	if (curSpp >= spp)	prc::framerateCap = 30;
	else				prc::framerateCap = 0;
}

void draw() {
	float aspectCorrect = (float)constants::resy() / (float)constants::resx();
	float aspectCorrect2 = (float)constants::resx() / (float)constants::resy();

	prc::setClearColor(vColor4f::black);

	fbo.bind();

	pathShader.use();
	pathShader.setUniform("iTime", (float)gtime::time);
	pathShader.setUniform("iResolution", fbo.size());

	pathShader.setUniform("ambientIntensity", ambientIntensity);
	pathShader.setUniform("ambient", ambient);

	pathShader.setUniform("camPos", camPos);
	pathShader.setUniform("rot", rot);
	pathShader.setUniform("fovMult", fovMult);

	pathShader.setUniform("aperture", aperture);
	pathShader.setUniform("focusDist", focusDist);
	pathShader.setUniform("focusInterval", focusInterval);

	pathShader.setUniform("curSpp", curSpp);
	pathShader.setUniform("spp", spp);
	pathShader.setUniform("sppRate", sppRate);
	pathShader.setUniform("bounces", bounces);
	pathShader.setUniform("tex", 0);

	pathShader.setUniform("numSpheres", numSp);
	for (int i = 0; i < numSp; i++) {
		std::string s = ("sp[" + std::to_string(i) + "].pos");

		pathShader.setUniform(("sp[" + std::to_string(i) + "].pos").c_str(), sp[i].pos);
		pathShader.setUniform(("sp[" + std::to_string(i) + "].radius").c_str(), sp[i].radius);
		pathShader.setUniform(("sp[" + std::to_string(i) + "].mat.albedo").c_str(), sp[i].mat.albedo);
		pathShader.setUniform(("sp[" + std::to_string(i) + "].mat.metallic").c_str(), sp[i].mat.metallic);
		pathShader.setUniform(("sp[" + std::to_string(i) + "].mat.roughness").c_str(), sp[i].mat.roughness);
		pathShader.setUniform(("sp[" + std::to_string(i) + "].mat.emit").c_str(), sp[i].mat.emit);
	}

	pathShader.setUniform("numPlanes", numPl);
	for (int i = 0; i < numPl; i++) {
		std::string s = ("pl[" + std::to_string(i) + "].pos");

		pathShader.setUniform(("pl[" + std::to_string(i) + "].pos").c_str(), pl[i].pos);
		pathShader.setUniform(("pl[" + std::to_string(i) + "].normal").c_str(), pl[i].normal);
		pathShader.setUniform(("pl[" + std::to_string(i) + "].mat.albedo").c_str(), pl[i].mat.albedo);
		pathShader.setUniform(("pl[" + std::to_string(i) + "].mat.metallic").c_str(), pl[i].mat.metallic);
		pathShader.setUniform(("pl[" + std::to_string(i) + "].mat.roughness").c_str(), pl[i].mat.roughness);
		pathShader.setUniform(("pl[" + std::to_string(i) + "].mat.emit").c_str(), pl[i].mat.emit);
	}

	util::bindTexture(0, fbo.getPrimaryColorAttachment());
	util::setColor(vc4::white);
	util::drawQuad(aabb2s(2.f));

	pathShader.release();

	fbo.unbind();

	if (curSpp < spp) {
		curSpp += sppRate;
	}

	accumShader.use();
	accumShader.setUniform("iTime", (float)gtime::time);
	accumShader.setUniform("iResolution", fbo.size());
	accumShader.setUniform("fboRes", fbo.size());
	accumShader.setUniform("curSpp", curSpp);
	accumShader.setUniform("tex", 0);
	

	util::bindTexture(0, fbo.getPrimaryColorAttachment());
	//util::drawTexture(fbo.getPrimaryColorAttachment(), mat3(1.f) * pmat3::scale(cst::resbb().size().aspectmaxv()));
	util::drawTexture(fbo.getPrimaryColorAttachment(), util::getResbbNDCAspect(), mat3(1.f));

	accumShader.release();

	deb::rtPrintln("SPP: " + std::to_string(curSpp) + "/" + std::to_string(spp));
	deb::rtPrintln("Bounces: " + std::to_string(bounces));
	deb::rtPrintln("Sizes: " + cst::res().toString() + " " + fbo.size().toString());
	//deb::rtPrintln("Focus Inverval: " + std::to_string(focusInterval));
}

void resize() {
	resetGraphics();
	fbo.resize(cst::res());
}

int main(){
	//cst::res({ 800,600 });
	//prc::setscmode(0);
	Context::setup({ &init, &update, &draw, NULL, &resize, &resize, &resize });
}

int loadInt(std::ifstream &f) {
	std::string t;
	f >> t;
	std::cout << t << "\n";
	return atoi(t.c_str());
}

float loadFloat(std::ifstream &f) {
	std::string t;
	f >> t;
	std::cout << t << "\n";
	return atof(t.c_str());
}

Vector3f loadVec3(std::ifstream &f) {
	float x = loadFloat(f);
	float y = loadFloat(f);
	float z = loadFloat(f);

	Vector3f vec = Vector3f(x, y, z);
	std::cout << vec.toString() << "\n";
	return vec;
}

Material loadMaterial(std::ifstream &f) {
	Vector3f albedo = loadVec3(f);
	float metallic = loadFloat(f);
	float roughness = loadFloat(f);
	float emit = loadFloat(f);

	return Material(albedo, metallic, roughness, emit);
}

bool loadScene(std::string fileName) {
	std::ifstream file;
	file.open(("scenes/" + fileName + ".scene"), std::ios::in);
	if (!file) return 0;

	resetGraphics();

	curScene = fileName;

	numSp=0;
	delete[] sp;

	numPl=0;
	delete[] pl;

	ambient = loadVec3(file);
	ambientIntensity = loadFloat(file);

	file >> numSp;

	sp = new Sphere[numSp];
	for (int i = 0; i < numSp; i++) {
		Vector3f pos = loadVec3(file);
		float radius = loadFloat(file);

		sp[i] = Sphere(pos, radius, loadMaterial(file));
	}

	file >> numPl;

	pl = new Plane[numPl];
	for (int i = 0; i < numPl; i++) {
		Vector3f pos = loadVec3(file);
		Vector3f normal = loadVec3(file);

		pl[i] = Plane(pos, normal, loadMaterial(file));
	}

	file.close();

	return 1;
}

bool loadShot(std::string fileName) {
	curShot = fileName;

	std::string sceneName;

	std::ifstream file;
	file.open("shots/" + fileName + ".shot", std::ios::in);
	if (!file) return 0;

	resetGraphics();

	file >> sceneName;

	//Constants::WIDTH = loadInt(file);
	//Constants::HEIGHT = loadInt(file);
	//MyContext::scalingMode = MyContext::SCALING_MODE::NON_INTEGER_RATIO;
	int x, y;
	x = loadInt(file);
	y = loadInt(file);
	//MyContext::changeResolution(x, y);
	//resize();

	spp = loadInt(file);
	bounces = loadInt(file);

	camPos = loadVec3(file);
	rot = loadVec3(file);

	fovMult = loadFloat(file);
	aperture = loadFloat(file);
	focusDist = loadFloat(file);
	focusInterval = loadFloat(file);

	file.close();

	loadScene(sceneName);

	return 1;
}

void saveVec3(Vector3f vec, std::ofstream &f) {
	f << vec.x << " " << vec.y << " " << vec.z;
}

bool saveScene(std::string fileName) {
	return 1;
}

bool saveShot(std::string fileName) {
	std::ofstream of;
	of.open("shots/" + fileName, std::ios::out);
	if (!of) return 0;
	
	of << curScene << "\n\n";
	
	of << constants::resx() << " " << constants::resy() << "\n";
	of << spp << "\n";
	of << bounces << "\n\n";

	saveVec3(camPos, of);
	of << "\n";

	saveVec3(rot, of);
	of << "\n\n";

	of << fovMult << "\n";
	of << aperture << "\n";
	of << focusDist << "\n";
	of << focusInterval << "\n";

	of.close();

	return 1;
}

float clamp(float f, float fmin, float fmax) {
	return min(max(f, fmin), fmax);
}

void writeToFile(std::string fileName) {
	GLfloat *data = new GLfloat[constants::resx() *constants::resy() *3];

	fbo.bind();

		glReadPixels(0,0, constants::resx(), constants::resy(), GL_RGB, GL_FLOAT, data);

	fbo.unbind();

	fileName += ".ppm";

	std::ofstream file;
	file.open(fileName, std::ios::out);

	file << "P3\n";
	file << constants::resx() << "\n" << constants::resy() << "\n";
	file << "255\n\n";
	
	//for (int i = 0; i < Constants::WIDTH*Constants::HEIGHT*3; i+=3) {
		//std::cout << (clamp(data[i]/curSpp*255.0f, 0.0f, 255.0f)) << " " << (clamp(data[i+1] / curSpp * 255.0f, 0.0f, 255.0f)) << " " << (clamp(data[i+2] / curSpp * 255.0f, 0.0f, 255.0f)) << "\n";
	for (int y = fbo.sizey() - 1; y >= 0; y--) {
		for (int x = 0; x < fbo.sizex(); x++) {
			int i = y * fbo.sizex() * 3 + x * 3;
			file << (int)(clamp(data[i] / curSpp * 255.0f, 0.0f, 255.0f)) << " " << (int)(clamp(data[i + 1] / curSpp * 255.0f, 0.0f, 255.0f)) << " " << (int)(clamp(data[i + 2] / curSpp * 255.0f, 0.0f, 255.0f)) << "\n";
		}
	}
	//}

	file.close();
}