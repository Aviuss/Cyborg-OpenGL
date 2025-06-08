/*
Niniejszy program jest wolnym oprogramowaniem; możesz go
rozprowadzać dalej i / lub modyfikować na warunkach Powszechnej
Licencji Publicznej GNU, wydanej przez Fundację Wolnego
Oprogramowania - według wersji 2 tej Licencji lub(według twojego
wyboru) którejś z późniejszych wersji.

Niniejszy program rozpowszechniany jest z nadzieją, iż będzie on
użyteczny - jednak BEZ JAKIEJKOLWIEK GWARANCJI, nawet domyślnej
gwarancji PRZYDATNOŚCI HANDLOWEJ albo PRZYDATNOŚCI DO OKREŚLONYCH
ZASTOSOWAŃ.W celu uzyskania bliższych informacji sięgnij do
Powszechnej Licencji Publicznej GNU.

Z pewnością wraz z niniejszym programem otrzymałeś też egzemplarz
Powszechnej Licencji Publicznej GNU(GNU General Public License);
jeśli nie - napisz do Free Software Foundation, Inc., 59 Temple
Place, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define ARMY_SIZE 20

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include "lodepng.h"
#include "shaderprogram.h"
#include "myCube.h"
#include "myTeapot.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags
#include <iostream>


float aspectRatio=1;

ShaderProgram *sp;

GLuint tex0;
GLuint tex1;
GLuint tex2;
GLuint tex3;
GLuint tex4;
GLuint tex5;

float* fshift = new float[1];

enum whichStep { // dla uzytku ruszajacego sie robota
	LEFT_LEG_FORWARD,
	LEFT_LEG_BACKWARD,
	RIGHT_LEG_FORWARD,
	RIGHT_LEG_BACKWARD
};

enum direction {
	NONE,
	FORWARD,
	BACKWARD
};

enum walkingAnimation {
	BEND_KNEE,
	LEG_FORWARD,
	LEG_BACKWARD,
	PUT_LEG,
	STEP
};

enum walkingBackwardsAnimation {
	LEGS_SPLIT,
	LEGS_TOGETHER
};

enum turningAroundAnimation {
	LEG_UP,
	LEG_DOWN
};

enum typeOfControl {
	ANIMATION,
	MANUAL
};

GLuint readTexture(const char* filename) {
	GLuint tex;
	glActiveTexture(GL_TEXTURE0);
	//Wczytanie do pamięci komputera
	std::vector<unsigned char> image; //Alokuj wektor do wczytania obrazka
	unsigned width, height; //Zmienne do których wczytamy wymiary obrazka
	//Wczytaj obrazek
	unsigned error = lodepng::decode(image, width, height, filename);
	//Import do pamięci karty graficznej
	glGenTextures(1, &tex); //Zainicjuj jeden uchwyt
	glBindTexture(GL_TEXTURE_2D, tex); //Uaktywnij uchwyt
	//Wczytaj obrazek do pamięci KG skojarzonej z uchwytem
	glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, (unsigned char*)image.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return tex;
}

struct ControlData {
	
	float angle_x = 0;
	float angle_y = 0;
	float angle_z = 0;
	float left_shoulder_angle = 0;
	float left_elbow_angle = 0;
	float left_hand_angle = 0;
	float right_shoulder_angle = 0;
	float right_elbow_angle = 0;
	float right_hand_angle = 0;
	float head_angle = 0;
	float val = 0;
	float rot_angle = 0;
	float zoom = 0;
	float maxSwing = PI / 6;
	float stepDelta = PI/5;
	float moveSpeed = 0.75f;
	float lerpPosition = 1.0f;

	void updateControlLoop(float deltaTime) {
		angle_x += speed_x * deltaTime; //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
		angle_y += speed_y * deltaTime; //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
		angle_z = speed_z * deltaTime;
		val += val_speed * deltaTime;
		rot_angle += rot_speed * deltaTime;
		zoom += zoom_speed * deltaTime;
		stepDelta = PI/5 * deltaTime;
		lerpPosition += deltaTime/moveSpeed;
		if (lerpPosition > 1.0f) lerpPosition = 1.0f;
		//czesci ciala
		right_shoulder_angle += right_shoulder_speed * deltaTime;
		right_elbow_angle += right_elbow_speed * deltaTime;
		right_hand_angle += right_hand_speed * deltaTime;
		left_shoulder_angle += left_shoulder_speed * deltaTime;
		left_elbow_angle += left_elbow_speed * deltaTime;
		left_hand_angle += left_hand_speed * deltaTime;
		head_angle += head_speed * deltaTime;

		if (shift_modifier_refresh > 0) {
			shift_modifier_refresh -= deltaTime;
		}
		if (ctrl_modifier_refresh > 0) {
			ctrl_modifier_refresh -= deltaTime;
		}

		if (shift_modifier_refresh <= 0) {
			right_shoulder_speed = 0;
			right_elbow_speed = 0;
			right_hand_speed = 0;
			right_shoulder_speed = 0;
			right_elbow_speed = 0;
			right_hand_speed = 0;
		}
		if (ctrl_modifier_refresh <= 0) {
			left_shoulder_speed = 0;
			left_elbow_speed = 0;
			left_hand_speed = 0;
			left_shoulder_speed = 0;
			left_elbow_speed = 0;
			left_hand_speed = 0;
		}
	}

	float speed_x = 0;
	float speed_y = 0;
	float speed_z = 0;
	float rot_speed = 0;
	float val_speed = 0;
	float zoom_speed = 0;
	float left_shoulder_speed = 0;
	float left_elbow_speed = 0;
	float left_hand_speed = 0;
	float right_shoulder_speed = 0;
	float right_elbow_speed = 0;
	float right_hand_speed = 0;
	float head_speed = 0;

	int shift_modifier_refresh = 0;
	int ctrl_modifier_refresh = 0;
	float refresh_value = 2;
};
ControlData inputControlData;


struct Model3D {
	float* vertices;
	float* normals;
	float* texCoords;
	int vertexCount = 0;
	glm::mat4 localMatrix = glm::mat4(1.0f);
	glm::vec3 localTransform = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 localEulerRotation = glm::vec3(0.0f, 0.0f, 0.0f);


	GLuint* texture_diff = nullptr;
	GLuint* texture_metal = nullptr;
	GLuint* texture_mask = nullptr;
	
	void printModelToCOUT() {
		std::cout << "Model3D:\n";

		std::cout << "Vertices:\n";
		for (int i = 0; i < vertexCount * 4; i += 4) {
			std::cout << "  [" << i / 4 << "]: (" << vertices[i] << ", "
				<< vertices[i + 1] << ", "
				<< vertices[i + 2] << ", "
				<< vertices[i + 3] << ")\n";
		}

		std::cout << "Normals:\n";
		for (int i = 0; i < vertexCount * 4; i += 4) {
			std::cout << "  [" << i / 4 << "]: ("
				<< normals[i] << ", "
				<< normals[i + 1] << ", "
				<< normals[i + 2] << ", "
				<< normals[i + 3] << ")\n";
		}

		std::cout << "Texture Coords:\n";
		for (int i = 0; i < vertexCount * 2; i += 2) {
			std::cout << "  [" << i / 2 << "]: ("
				<< texCoords[i] << ", "
				<< texCoords[i + 1] << ")\n";
		}

		std::cout << "Total Vertices: " << vertexCount << "\n";
	}

	void drawOpenGL(glm::mat4& P, glm::mat4& V) {

		sp->use();//Aktywacja programu cieniującego	
		//Przeslij parametry programu cieniującego do karty graficznej
		glUniformMatrix4fv(sp->u("P"), 1, false, glm::value_ptr(P));
		glUniformMatrix4fv(sp->u("V"), 1, false, glm::value_ptr(V));
		glUniformMatrix4fv(sp->u("M"), 1, false, glm::value_ptr(localMatrix));
		glUniform4f(sp->u("lp1"), 0, 100, 20, 1);
		glUniform4f(sp->u("lp2"), -40, 0, 15, 1);
		glUniform1f(sp->u("fshift"), *fshift);

		if (texture_diff != nullptr) {
			glUniform1i(sp->u("textureMap0"), 0); //drawScene
		}
		if (texture_metal != nullptr) {
			glUniform1i(sp->u("textureMap1"), 1); //drawScene
		}
		if (texture_mask != nullptr) {
			glUniform1i(sp->u("textureMap2"), 2); //drawScene
		}


		glEnableVertexAttribArray(sp->a("vertex"));  //Włącz przesyłanie danych do atrybutu vertex
		glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, vertices); //Wskaż tablicę z danymi dla atrybutu vertex

		//glEnableVertexAttribArray(sp->a("fshift"));  //Włącz przesyłanie danych do atrybutu vertex
		//glVertexAttribPointer(sp->a("fshift"), 1, GL_FLOAT, false, 0, fshift); //Wskaż tablicę z danymi dla atrybutu vertex


		//glEnableVertexAttribArray(sp->a("color"));  //Włącz przesyłanie danych do atrybutu color
		//glVertexAttribPointer(sp->a("color"), 4, GL_FLOAT, false, 0, colors); //Wskaż tablicę z danymi dla atrybutu color

		glEnableVertexAttribArray(sp->a("normal"));  //Włącz przesyłanie danych do atrybutu normal
		glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, normals); //Wskaż tablicę z danymi dla atrybutu normal

		glEnableVertexAttribArray(sp->a("texCoord0"));
		glVertexAttribPointer(sp->a("texCoord0"),
			2, GL_FLOAT, false, 0, texCoords);//odpowiednia tablica


		if (texture_diff != nullptr) {
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, *texture_diff);
		}
		if (texture_metal != nullptr) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, *texture_metal);
		}
		if (texture_mask != nullptr) {
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, *texture_mask);
		}


		glDrawArrays(GL_TRIANGLES, 0, vertexCount); //Narysuj obiekt

		glDisableVertexAttribArray(sp->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
		//glDisableVertexAttribArray(sp->a("color"));  //Wyłącz przesyłanie danych do atrybutu color
		glDisableVertexAttribArray(sp->a("normal"));  //Wyłącz przesyłanie danych do atrybutu normal
		glDisableVertexAttribArray(sp->a("texCoord0"));
		//glDisableVertexAttribArray(sp->a("fshift"));  //Włącz przesyłanie danych do atrybutu vertex


	}

	inline void applyTransform(glm::mat4 originMatrix) {
		localMatrix = glm::translate(originMatrix, localTransform);
	}


	inline void applyRotation(std::string order = "XYZ") {
		for (int i = 0; i < order.size(); i++) {
			switch (order[i]) {
			case 'X':
				localMatrix = glm::rotate(localMatrix, localEulerRotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case 'Y':
				localMatrix = glm::rotate(localMatrix, localEulerRotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			case 'Z':
				localMatrix = glm::rotate(localMatrix, localEulerRotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
				break;
			}
		}
	}

	void setTextures(GLuint* _texture_diff, GLuint* _texture_metal, GLuint* _texture_mask) {
		texture_diff = _texture_diff;
		texture_metal = _texture_metal;
		texture_mask = _texture_mask;
	}

};

struct RobotStructure; // tylko deklaracja

struct RobotJointAngles {
	// robot position
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 rotation = glm::vec3(0, 0, 0);
	glm::vec3 targetPosition = glm::vec3(0, 0, 0); //docelowa pozycja po ruchu
	float positionLerpProgress = 0.0f; //zmienna do interpolacji pozycji

	//torso
	float head_rot = 0;
	float torso_rot = 0;

	// left arm
	float left_shoulder_forward = 0;
	float left_shoulder_side = 0;
	float left_shoulder_rot = 0;
	float left_elbow = 0;
	float left_hand = 0;

	//right arm
	float right_shoulder_forward = 0;
	float right_shoulder_side = 0;
	float right_shoulder_rot = 0;
	float right_elbow = 0;
	float right_hand = 0;

	// left leg
	float left_leg_forward = 0;
	float left_leg_rot = 0;
	float left_leg2_forward = 0;
	float left_foot = 0;


	// right leg
	float right_leg_forward = 0;
	float right_leg_rot = 0;
	float right_leg2_forward = 0;
	float right_foot = 0;

	void applyAngles(RobotStructure& robot);

	RobotJointAngles& operator=(const RobotJointAngles& other) {
		if (this != &other) {
			std::memcpy(this, &other, sizeof(RobotJointAngles));
		}
		return *this;
	}

	RobotJointAngles() {};

	RobotJointAngles(glm::vec3 _position, glm::vec3 _rotation, float _head_rot, float _torso_rot, float _left_shoulder_forward, float _left_shoulder_side,
		float _left_shoulder_rot, float _left_elbow, float _left_hand, float _right_shoulder_forward, float _right_shoulder_side,
		float _right_shoulder_rot, float _right_elbow, float _right_hand, float _left_leg_forward, float _left_leg_rot,
		float _left_leg2_forward, float _left_foot, float _right_leg_forward, float _right_leg_rot, float _right_leg2_forward,
		float _right_foot) {
		position = _position;
		rotation = _rotation;

		//torso
		head_rot = _head_rot;
		torso_rot = _torso_rot;

		// left arm
		left_shoulder_forward = _left_shoulder_forward;
		left_shoulder_side = _left_shoulder_side;
		left_shoulder_rot = _left_shoulder_rot;
		left_elbow = _left_elbow;
		left_hand = _left_hand;

		//right arm
		right_shoulder_forward = _right_shoulder_forward;
		right_shoulder_side = _right_shoulder_side;
		right_shoulder_rot = _right_shoulder_rot;
		right_elbow = _right_elbow;
		right_hand = _right_hand;

		// left leg
		left_leg_forward = _left_leg_forward;
		left_leg_rot = _left_leg_rot;
		left_leg2_forward = _left_leg2_forward;
		left_foot = _left_foot;


		// right leg
		right_leg_forward = _right_leg_forward;
		right_leg_rot = _right_leg_rot;
		right_leg2_forward = _right_leg2_forward;
		right_foot = _right_foot;

	}
};

struct RobotStructure {
	Model3D head;
	Model3D torso;
	Model3D torsoJoint;
	Model3D legsHolder;

	Model3D leftLeg;
	Model3D leftLeg2;
	Model3D leftFoot;

	Model3D rightLeg;
	Model3D rightLeg2;
	Model3D rightFoot;

	Model3D leftArm;
	Model3D leftArm2;
	Model3D leftHand;

	Model3D rightArm;
	Model3D rightArm2;
	Model3D rightHand;
	glm::vec4 pointingVector;

	RobotJointAngles currentKeyframe;


	GLuint* texture_diff = nullptr;
	GLuint* texture_metal = nullptr;
	GLuint* texture_mask = nullptr;

	bool isMarching = false;
	bool isMarchingAnimationFinished = true;
	bool isForwardAnimationFinished = true;
	bool isBackwardAnimationFinished = true;
	bool isTurningAnimationFinished = true;

	whichStep step = LEFT_LEG_FORWARD;
	direction direction = NONE;
	walkingAnimation walkingPhase = BEND_KNEE;
	walkingBackwardsAnimation backwardPhase = LEGS_SPLIT;
	turningAroundAnimation turningPhase = LEG_UP;
	typeOfControl control = MANUAL;

	void directKinematicsLogic() {
		currentKeyframe.applyAngles(*this);

		glm::mat4 robotWholeMatrix = glm::translate(glm::mat4(1.0f), currentKeyframe.position)
			* glm::rotate(glm::mat4(1.0f), currentKeyframe.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), currentKeyframe.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), currentKeyframe.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

		torso.applyTransform(robotWholeMatrix);
		torso.applyRotation();

		head.applyTransform(torso.localMatrix);
		head.applyRotation();

		torsoJoint.applyTransform(torso.localMatrix);
		torsoJoint.applyRotation();

		legsHolder.applyTransform(torso.localMatrix);
		legsHolder.applyRotation();

		leftLeg.applyTransform(legsHolder.localMatrix);
		leftLeg.applyRotation();

		leftLeg2.applyTransform(leftLeg.localMatrix);
		leftLeg2.applyRotation();

		leftFoot.applyTransform(leftLeg2.localMatrix);
		leftFoot.applyRotation();

		rightLeg.applyTransform(legsHolder.localMatrix);
		rightLeg.applyRotation();

		rightLeg2.applyTransform(rightLeg.localMatrix);
		rightLeg2.applyRotation();

		rightFoot.applyTransform(rightLeg2.localMatrix);
		rightFoot.applyRotation();

		leftArm.applyTransform(torso.localMatrix);
		leftArm.applyRotation();

		leftArm2.applyTransform(leftArm.localMatrix);
		leftArm2.applyRotation();

		leftHand.applyTransform(leftArm2.localMatrix);
		leftHand.applyRotation();

		rightArm.applyTransform(torso.localMatrix);
		rightArm.applyRotation();

		rightArm2.applyTransform(rightArm.localMatrix);
		rightArm2.applyRotation();

		rightHand.applyTransform(rightArm2.localMatrix);
		rightHand.applyRotation();
	}

	inline void drawWholeRobot(glm::mat4& P, glm::mat4& V) {
		head.drawOpenGL(P, V);
		torso.drawOpenGL(P, V);
		torsoJoint.drawOpenGL(P, V);
		legsHolder.drawOpenGL(P, V);

		leftLeg.drawOpenGL(P, V);
		leftLeg2.drawOpenGL(P, V);
		leftFoot.drawOpenGL(P, V);

		rightLeg.drawOpenGL(P, V);
		rightLeg2.drawOpenGL(P, V);
		rightFoot.drawOpenGL(P, V);

		leftArm.drawOpenGL(P, V);
		leftArm2.drawOpenGL(P, V);
		leftHand.drawOpenGL(P, V);

		rightArm.drawOpenGL(P, V);
		rightArm2.drawOpenGL(P, V);
		rightHand.drawOpenGL(P, V);
	}

	void changeKeyframe(RobotStructure &robot, RobotJointAngles newKeyframe) {
		robot.currentKeyframe = newKeyframe;
	}


	void changePosition(RobotStructure& robot) {
		robot.currentKeyframe.position = glm::mix(
			robot.currentKeyframe.position,
			robot.currentKeyframe.targetPosition,
			inputControlData.lerpPosition
		);
		//std::cout << "Lerp: " << inputControlData.lerpPosition << std::endl;
		//std::cout << "Marching? " << robot.isMarching << std::endl;
		//std::cout << "CurrentPos: " << robot.currentKeyframe.position.x << " " << robot.currentKeyframe.position.y << " " << robot.currentKeyframe.position.z << " " << std::endl;
		//std::cout << "TargetPos: " << robot.currentKeyframe.targetPosition.x << " " << robot.currentKeyframe.targetPosition.y << " " << robot.currentKeyframe.targetPosition.z << " " << std::endl;
		
		if (inputControlData.lerpPosition >= 1.0f) {
			inputControlData.lerpPosition = 1.0f;
			robot.currentKeyframe.position = robot.currentKeyframe.targetPosition;
		}
	}

	void turn(RobotStructure& robot, float angle) {
		RobotJointAngles newKeyframe = robot.currentKeyframe;
		newKeyframe.rotation.z += angle;
		changeKeyframe(robot, newKeyframe);
	}

	void calculatePointingVector() {
		pointingVector =
			glm::rotate(glm::mat4(1.0f), currentKeyframe.rotation.z, glm::vec3(0, 0, 1))
			* glm::vec4(0, -1, 0, 0);
	}

	void moveOneStep(RobotStructure& robot, float stepLength) {  
		robot.calculatePointingVector();
		robot.currentKeyframe.targetPosition = robot.currentKeyframe.position + glm::vec3(robot.pointingVector) * stepLength;

		inputControlData.lerpPosition = 0.0f;
	}

	void incrementWalkingPhase(RobotStructure& robot) {
		switch (robot.walkingPhase) {
		case BEND_KNEE:
			robot.walkingPhase = LEG_FORWARD;
			break;
		case LEG_FORWARD:
			robot.walkingPhase = LEG_BACKWARD;
			break;
		case LEG_BACKWARD:
			robot.walkingPhase = PUT_LEG;
			break;
		case PUT_LEG:
			robot.walkingPhase = STEP;
			break;
		case STEP:
			robot.walkingPhase = BEND_KNEE;
		default:
			robot.walkingPhase = BEND_KNEE;
		}
	}

	void resetPhases(RobotStructure& robot) {
		if (robot.step != LEFT_LEG_FORWARD) robot.step = LEFT_LEG_FORWARD;
		if (robot.direction != NONE) robot.direction = NONE;
		if (robot.walkingPhase != BEND_KNEE) robot.walkingPhase = BEND_KNEE;
		if (robot.backwardPhase != LEGS_SPLIT) robot.backwardPhase = LEGS_SPLIT;
		if (robot.turningPhase != LEG_UP) robot.turningPhase = LEG_UP;
	}

	void idleLegPosition(RobotStructure& robot, RobotJointAngles& keyframe) {
		//robot.resetPhases(robot);
		RobotJointAngles newKeyframe = keyframe;

		if (keyframe.left_leg_forward != 0) newKeyframe.left_leg_forward = 0;
		if (keyframe.left_leg2_forward != 0) newKeyframe.left_leg2_forward = 0;
		if (keyframe.left_foot != 0) newKeyframe.left_foot = 0;
		if (keyframe.right_leg_forward != 0) newKeyframe.right_leg_forward = 0;
		if (keyframe.right_leg2_forward != 0) newKeyframe.right_leg2_forward = 0;
		if (keyframe.right_foot != 0) newKeyframe.right_foot = 0;

		robot.changeKeyframe(robot, newKeyframe);
	}

	inline void initRobotValues() {
		torso.localTransform = glm::vec3(0, 0, 5);
		head.localTransform = glm::vec3(0, 0, 1.46724);
		torsoJoint.localTransform = glm::vec3(0.0f, 0.0f, -1.0f);
		legsHolder.localTransform = glm::vec3(0.0f, 0.0f, -2.0f);

		leftLeg.localTransform = glm::vec3(-0.699591, 0.0f, -0.366832);
		leftLeg2.localTransform = glm::vec3(-0.28473, 0.0f, -1.58215);
		leftFoot.localTransform = glm::vec3(0.30515, 0, -2.29);

		rightLeg.localTransform = glm::vec3(0.699591, 0.0f, -0.366832);
		rightLeg2.localTransform = glm::vec3(0.28473, 0.0f, -1.58215);
		rightFoot.localTransform = glm::vec3(-0.30515, 0, -2.29);

		leftArm.localTransform = glm::vec3(-2.22455, -0.359883, 0.333555);
		leftArm2.localTransform = glm::vec3(-0.0724277, -0.0241681, -2.04295);
		leftHand.localTransform = glm::vec3(0.0832711, 0, -1.66374);

		rightArm.localTransform = glm::vec3(2.22455, -0.359883, 0.333555);
		rightArm2.localTransform = glm::vec3(0.0724277, -0.0241681, -2.04295);
		rightHand.localTransform = glm::vec3(-0.0832711, 0, -1.66374);

		torso.setTextures(texture_diff, texture_metal, texture_mask);
		head.setTextures(texture_diff, texture_metal, texture_mask);
		torsoJoint.setTextures(texture_diff, texture_metal, texture_mask);
		legsHolder.setTextures(texture_diff, texture_metal, texture_mask);

		leftLeg.setTextures(texture_diff, texture_metal, texture_mask);
		leftLeg2.setTextures(texture_diff, texture_metal, texture_mask);
		leftFoot.setTextures(texture_diff, texture_metal, texture_mask);

		rightLeg.setTextures(texture_diff, texture_metal, texture_mask);
		rightLeg2.setTextures(texture_diff, texture_metal, texture_mask);
		rightFoot.setTextures(texture_diff, texture_metal, texture_mask);

		leftArm.setTextures(texture_diff, texture_metal, texture_mask);
		leftArm2.setTextures(texture_diff, texture_metal, texture_mask);
		leftHand.setTextures(texture_diff, texture_metal, texture_mask);

		rightArm.setTextures(texture_diff, texture_metal, texture_mask);
		rightArm2.setTextures(texture_diff, texture_metal, texture_mask);
		rightHand.setTextures(texture_diff, texture_metal, texture_mask);
	}

};

void RobotJointAngles::applyAngles(RobotStructure &robot) {
	robot.head.localEulerRotation.z = -head_rot;
	robot.torso.localEulerRotation.z = -torso_rot;
	robot.legsHolder.localEulerRotation.z = torso_rot;

	
	robot.leftArm.localEulerRotation.x = -left_shoulder_forward;
	robot.leftArm.localEulerRotation.y = left_shoulder_side;
	robot.leftArm.localEulerRotation.z = left_shoulder_rot;
	robot.leftArm2.localEulerRotation.x = -left_elbow;
	robot.leftHand.localEulerRotation.y = -left_hand;


	robot.rightArm.localEulerRotation.x = -right_shoulder_forward;
	robot.rightArm.localEulerRotation.y = -right_shoulder_side;
	robot.rightArm.localEulerRotation.z = -right_shoulder_rot;
	robot.rightArm2.localEulerRotation.x = -right_elbow;
	robot.rightHand.localEulerRotation.y = right_hand;


	robot.leftLeg.localEulerRotation.x = -left_leg_forward;
	robot.leftLeg.localEulerRotation.z = left_leg_rot;
	robot.leftLeg2.localEulerRotation.x = -left_leg2_forward;
	robot.leftFoot.localEulerRotation.x = -left_foot;


	robot.rightLeg.localEulerRotation.x = -right_leg_forward;
	robot.rightLeg.localEulerRotation.z = right_leg_rot;
	robot.rightLeg2.localEulerRotation.x = -right_leg2_forward;
	robot.rightFoot.localEulerRotation.x = -right_foot;
}

void moveLimbsMarch(RobotStructure& robot, RobotJointAngles& keyframe) {
	float maxSwing = inputControlData.maxSwing;
	float stepDelta = inputControlData.stepDelta;
	robot.control = ANIMATION;

	if (robot.step == LEFT_LEG_FORWARD) {
		if (keyframe.left_leg_forward < maxSwing) {
			keyframe.left_leg_forward += stepDelta;
			keyframe.right_shoulder_forward += stepDelta;
		}
		else {
			robot.step = LEFT_LEG_BACKWARD;
		}
	}
	else if (robot.step == LEFT_LEG_BACKWARD) {
		if (keyframe.left_leg_forward > 0) {
			keyframe.left_leg_forward -= stepDelta;
			keyframe.right_shoulder_forward -= stepDelta;
		}
		else {
			robot.step = RIGHT_LEG_FORWARD;
			robot.moveOneStep(robot, 3.0f);
			robot.isMarchingAnimationFinished = true;
			robot.control = MANUAL;
		}
	}
	else if (robot.step == RIGHT_LEG_FORWARD) {
		if (keyframe.right_leg_forward < maxSwing) {
			keyframe.right_leg_forward += stepDelta;
			keyframe.left_shoulder_forward += stepDelta;
		}
		else {
			robot.step = RIGHT_LEG_BACKWARD;
		}
	}
	else {
		if (keyframe.right_leg_forward > 0) {
			keyframe.right_leg_forward -= stepDelta;
			keyframe.left_shoulder_forward -= stepDelta;
		}
		else {
			robot.step = LEFT_LEG_FORWARD;
			robot.moveOneStep(robot, 3.0f);
			robot.isMarchingAnimationFinished = true;
			robot.control = MANUAL;
		}
	}

	robot.changeKeyframe(robot, keyframe);
	robot.changePosition(robot);
}

void marchForward(RobotStructure& robot, RobotJointAngles& keyframe) {
	moveLimbsMarch(robot, keyframe);
}

void forwardStepAnimation(RobotStructure& robot, RobotJointAngles& keyframe) {
	float maxSwing = inputControlData.maxSwing + PI/5;
	float stepDelta = inputControlData.stepDelta + PI/45;
	float stepLength = 5.0f;
	RobotJointAngles newKeyframe;
	robot.control = ANIMATION;

	if (robot.step == LEFT_LEG_FORWARD || robot.step == LEFT_LEG_BACKWARD) {
		if (robot.walkingPhase == BEND_KNEE) {
			keyframe.left_leg2_forward -= stepDelta;
			keyframe.left_foot += stepDelta;
			if (keyframe.left_leg2_forward <= -maxSwing) robot.incrementWalkingPhase(robot);
		}
		else if (robot.walkingPhase == LEG_FORWARD) {
			keyframe.left_leg_forward += stepDelta;
			keyframe.left_foot -= stepDelta;
			if (keyframe.left_leg_forward >= maxSwing) robot.incrementWalkingPhase(robot);
		}
		else if (robot.walkingPhase == LEG_BACKWARD) {
			keyframe.right_leg_forward -= stepDelta;
			keyframe.right_foot += stepDelta;
			if (keyframe.right_leg_forward <= -maxSwing / 2) robot.incrementWalkingPhase(robot);
		}
		else if (robot.walkingPhase == PUT_LEG) {
			keyframe.left_leg2_forward += stepDelta;
			keyframe.left_leg_forward -= stepDelta;
			if (inputControlData.lerpPosition >= 1.0f) robot.moveOneStep(robot, stepLength);
			if (keyframe.right_leg_forward <= 0) {
				keyframe.right_leg_forward += stepDelta;
				keyframe.right_foot -= stepDelta;
			}
			if (keyframe.left_leg2_forward >= 0) robot.incrementWalkingPhase(robot);
		}
		else {
			robot.step = RIGHT_LEG_FORWARD;
			robot.incrementWalkingPhase(robot);
			robot.isForwardAnimationFinished = true;
			robot.control = MANUAL;
			robot.idleLegPosition(robot, robot.currentKeyframe);
		}
		//kolano -> miednica -> kolano sie prostuje -> pochylam sie na noge -> dosuwam cialo do nogi
	}
	else if (robot.step == RIGHT_LEG_FORWARD || robot.step == RIGHT_LEG_BACKWARD) {
		if (robot.walkingPhase == BEND_KNEE) {
			keyframe.right_leg2_forward -= stepDelta;
			keyframe.right_foot += stepDelta;
			if (keyframe.right_leg2_forward <= -maxSwing) robot.incrementWalkingPhase(robot);
			printf("prawa noga bend_knee\n");
		}
		else if (robot.walkingPhase == LEG_FORWARD) {
			keyframe.right_leg_forward += stepDelta;
			keyframe.right_foot -= stepDelta;
			if (keyframe.right_leg_forward >= maxSwing) robot.incrementWalkingPhase(robot);
			printf("prawa noga leg_forward\n");
		}
		else if (robot.walkingPhase == LEG_BACKWARD) {
			keyframe.left_leg_forward -= stepDelta;
			keyframe.left_foot += stepDelta;
			if (keyframe.left_leg_forward <= -maxSwing / 2) robot.incrementWalkingPhase(robot);
			printf("prawa noga leg_backward\n");
		}
		else if (robot.walkingPhase == PUT_LEG) {
			keyframe.right_leg2_forward += stepDelta;
			keyframe.right_leg_forward -= stepDelta;
			robot.moveOneStep(robot, stepLength);
			if (inputControlData.lerpPosition >= 1.0f) robot.moveOneStep(robot, stepLength);
			if (keyframe.left_leg_forward <= 0) {
				keyframe.left_leg_forward += stepDelta;
				keyframe.left_foot -= stepDelta;
			}
			if (keyframe.right_leg2_forward >= 0) robot.incrementWalkingPhase(robot);
			printf("prawa noga put_leg\n");
		}
		else {
			robot.step = LEFT_LEG_FORWARD;
			robot.incrementWalkingPhase(robot);
			robot.isForwardAnimationFinished = true;
			robot.control = MANUAL;
			robot.idleLegPosition(robot, robot.currentKeyframe);
		}
	}

	newKeyframe = keyframe;
	robot.changeKeyframe(robot, newKeyframe);
}

void backwardStepAnimation(RobotStructure& robot, RobotJointAngles& keyframe) {
	float maxSwing = inputControlData.maxSwing;
	float stepDelta = inputControlData.stepDelta + PI / 45;
	float stepLength = 4.0f;
	RobotJointAngles newKeyframe;
	robot.control = ANIMATION;

	if (robot.step == LEFT_LEG_BACKWARD || robot.step == LEFT_LEG_FORWARD) {
		if (robot.backwardPhase == LEGS_SPLIT) {
			keyframe.left_leg_forward -= stepDelta;
			keyframe.left_foot += stepDelta / 2;
			keyframe.right_leg_forward += stepDelta / 2;
			keyframe.right_foot -= stepDelta / 2;

			if (keyframe.left_leg_forward <= -maxSwing) robot.backwardPhase = LEGS_TOGETHER;
		}
		else if (robot.backwardPhase == LEGS_TOGETHER) {
			if (inputControlData.lerpPosition >= 1.0f) robot.moveOneStep(robot, -stepLength);
			keyframe.left_leg_forward += stepDelta;
			keyframe.left_foot -= stepDelta / 2;
			keyframe.right_leg_forward -= stepDelta / 2;
			keyframe.right_foot += stepDelta / 2;

			if (keyframe.left_leg_forward >= 0) {
				robot.backwardPhase = LEGS_SPLIT;
				robot.step = RIGHT_LEG_BACKWARD;
				robot.isBackwardAnimationFinished = true;
				robot.control = MANUAL;
				robot.idleLegPosition(robot, robot.currentKeyframe);
			}
		}
	}
	else if (robot.step == RIGHT_LEG_BACKWARD || robot.step == RIGHT_LEG_FORWARD) {
		if (robot.backwardPhase == LEGS_SPLIT) {
			keyframe.right_leg_forward -= stepDelta;
			keyframe.right_foot += stepDelta / 2;
			keyframe.left_leg_forward += stepDelta / 2;
			keyframe.left_foot -= stepDelta / 2;

			if (keyframe.right_leg_forward <= -maxSwing) robot.backwardPhase = LEGS_TOGETHER;
		}
		else if (robot.backwardPhase == LEGS_TOGETHER) {
			if (inputControlData.lerpPosition >= 1.0f) robot.moveOneStep(robot, -stepLength);
			keyframe.right_leg_forward += stepDelta;
			keyframe.right_foot -= stepDelta / 2;
			keyframe.left_leg_forward -= stepDelta / 2;
			keyframe.left_foot += stepDelta / 2;

			if (keyframe.right_leg_forward >= 0) {
				robot.backwardPhase = LEGS_SPLIT;
				robot.step = LEFT_LEG_BACKWARD;
				robot.isBackwardAnimationFinished = true;
				robot.control = MANUAL;
				robot.idleLegPosition(robot, robot.currentKeyframe);
			}
		}
	}

	newKeyframe = keyframe;
	robot.changeKeyframe(robot, newKeyframe);
}

void turningAnimation(RobotStructure& robot, RobotJointAngles& keyframe) {
	float maxSwing = inputControlData.maxSwing/2;
	float stepDelta = inputControlData.stepDelta + PI / 45;
	RobotJointAngles newKeyframe;
	robot.control = ANIMATION;

	if (robot.step == LEFT_LEG_BACKWARD || robot.step == LEFT_LEG_FORWARD) {
		if (robot.turningPhase == LEG_UP) {
			keyframe.left_leg_forward += stepDelta;
			keyframe.left_leg2_forward -= stepDelta / 2;

			if (keyframe.left_leg_forward >= maxSwing) robot.turningPhase = LEG_DOWN;
		}
		else if (robot.turningPhase == LEG_DOWN) {
			keyframe.left_leg_forward -= stepDelta;
			keyframe.left_leg2_forward += stepDelta / 2;

			if (keyframe.left_leg_forward <= 0) {
				robot.turningPhase = LEG_UP;
				robot.step = RIGHT_LEG_FORWARD;
				robot.isTurningAnimationFinished = true;
				robot.control = MANUAL;
				robot.idleLegPosition(robot, robot.currentKeyframe);
			}

			robot.turn(robot, inputControlData.angle_z/stepDelta);
		}
	}
	else if (robot.step == RIGHT_LEG_BACKWARD || robot.step == RIGHT_LEG_FORWARD) {
		if (robot.turningPhase == LEG_UP) {
			keyframe.right_leg_forward += stepDelta;
			keyframe.right_leg2_forward -= stepDelta / 2;

			if (keyframe.right_leg_forward >= maxSwing) robot.turningPhase = LEG_DOWN;
		}
		else if (robot.turningPhase == LEG_DOWN) {
			keyframe.right_leg_forward -= stepDelta;
			keyframe.right_leg2_forward += stepDelta / 2;

			if (keyframe.right_leg_forward <= 0) {
				//robot.turn(robot, inputControlData.angle_z);
				robot.turningPhase = LEG_UP;
				robot.step = LEFT_LEG_FORWARD;
				robot.isTurningAnimationFinished = true;
				robot.control = MANUAL;
				robot.idleLegPosition(robot, robot.currentKeyframe);
			}

			robot.turn(robot, inputControlData.angle_z / stepDelta);
		}
	}

	newKeyframe = keyframe;
	robot.changeKeyframe(robot, newKeyframe);
}

RobotStructure mainRobot;
RobotStructure robotArmy[ARMY_SIZE];
Model3D terrain;

void loadModel(Model3D& model, std::string fileName) {

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName,
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace |
		aiProcess_GenSmoothNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	std::vector<float> vertices_local;
	std::vector<float> normals_local;
	std::vector<float> texCoords_local;

	for (int meshId = 0; meshId < scene->mNumMeshes; meshId++) {
		aiMesh* mesh = scene->mMeshes[meshId];

		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			aiVector3D pos = mesh->mVertices[i];
			aiVector3D normal = mesh->mNormals[i];
			aiVector3D texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0f, 0.0f, 0.0f);

			vertices_local.push_back(pos.x);
			vertices_local.push_back(pos.y);
			vertices_local.push_back(pos.z);
			vertices_local.push_back(1.0f);
			normals_local.push_back(normal.x);
			normals_local.push_back(normal.y);
			normals_local.push_back(normal.z);
			normals_local.push_back(0.0f);
			texCoords_local.push_back(texCoord.x);
			texCoords_local.push_back(texCoord.y);
		}

	}


	model.vertices = new float[vertices_local.size()];
	std::copy(vertices_local.begin(), vertices_local.end(), model.vertices);

	model.normals = new float[normals_local.size()];
	std::copy(normals_local.begin(), normals_local.end(), model.normals);

	model.texCoords = new float[texCoords_local.size()];
	std::copy(texCoords_local.begin(), texCoords_local.end(), model.texCoords);

	model.vertexCount = vertices_local.size() / 4;
	return;
}

void armyMarch() {
	for (int i = 0; i < ARMY_SIZE; i++) {
		robotArmy[i].isMarching = true;
		robotArmy[i].currentKeyframe.targetPosition = robotArmy[i].currentKeyframe.position;
	}
}

void loadRobot(RobotStructure& robot) {
	loadModel(robot.torso, "robot_torso.fbx");
	loadModel(robot.head, "robot_head.fbx");
	loadModel(robot.torsoJoint, "robot_torso_joint.fbx");
	loadModel(robot.legsHolder, "robot_legs_holder.fbx");
	loadModel(robot.leftLeg, "robot_left_leg.fbx");
	loadModel(robot.leftLeg2, "robot_left_leg2.fbx");
	loadModel(robot.rightLeg, "robot_right_leg.fbx");
	loadModel(robot.rightLeg2, "robot_right_leg2.fbx");
	loadModel(robot.rightFoot, "robot_right_foot.fbx");
	loadModel(robot.leftFoot, "robot_left_foot.fbx");
	loadModel(robot.leftArm, "robot_left_arm.fbx");
	loadModel(robot.leftArm2, "robot_left_arm2.fbx");
	loadModel(robot.rightArm, "robot_right_arm.fbx");
	loadModel(robot.rightArm2, "robot_right_arm2.fbx");
	loadModel(robot.leftHand, "robot_left_hand.fbx");
	loadModel(robot.rightHand, "robot_right_hand.fbx");


	robot.initRobotValues();
}

//Procedura obsługi błędów
void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void keyCallback(GLFWwindow* window,int key,int scancode,int action,int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key == GLFW_KEY_LEFT) inputControlData.speed_x = -PI / 2;
		if (key == GLFW_KEY_RIGHT) inputControlData.speed_x = PI / 2;
		if (key == GLFW_KEY_UP) inputControlData.speed_y = PI / 2;
		if (key == GLFW_KEY_DOWN) inputControlData.speed_y = -PI / 2;
		if (key == GLFW_KEY_1) inputControlData.val_speed = -0.5;
		if (key == GLFW_KEY_2) inputControlData.val_speed = 0.5;
		if (key == GLFW_KEY_E) inputControlData.rot_speed = PI;
		if (key == GLFW_KEY_Q) inputControlData.rot_speed = -PI;
		if (key == GLFW_KEY_4) inputControlData.zoom_speed = PI;
		if (key == GLFW_KEY_5) inputControlData.zoom_speed = -PI;
		if (key == GLFW_KEY_W) mainRobot.direction = FORWARD;
		if (key == GLFW_KEY_S) mainRobot.direction = BACKWARD;
		if (key == GLFW_KEY_A) inputControlData.speed_z = PI / 70;
		if (key == GLFW_KEY_D) inputControlData.speed_z = -PI / 70;
		if (key == GLFW_KEY_I) armyMarch();
		// shift odpowiada za kontrolowanie prawej strony ciala - bark, lokiec, dlon
		if (mods & GLFW_MOD_SHIFT) {
			if (key == GLFW_KEY_R) inputControlData.right_shoulder_speed = -PI / 10;
			if (key == GLFW_KEY_T) inputControlData.right_elbow_speed = -PI / 10;
			if (key == GLFW_KEY_V) inputControlData.right_hand_speed = -PI / 10;
			if (key == GLFW_KEY_Z) inputControlData.right_shoulder_speed = PI / 10;
			if (key == GLFW_KEY_X) inputControlData.right_elbow_speed = PI / 10;
			if (key == GLFW_KEY_C) inputControlData.right_hand_speed = PI / 10;
			inputControlData.shift_modifier_refresh = inputControlData.refresh_value;
		}
		// control odpowiada za kontrolowanie lewej strony ciala
		if (mods & GLFW_MOD_CONTROL) {
			if (key == GLFW_KEY_R) inputControlData.left_shoulder_speed = PI / 10;
			if (key == GLFW_KEY_T) inputControlData.left_elbow_speed = PI / 10;
			if (key == GLFW_KEY_V) inputControlData.left_hand_speed = PI / 10;
			if (key == GLFW_KEY_Z) inputControlData.left_shoulder_speed = -PI / 10;
			if (key == GLFW_KEY_X) inputControlData.left_elbow_speed = -PI / 10;
			if (key == GLFW_KEY_C) inputControlData.left_hand_speed = -PI / 10;
			inputControlData.ctrl_modifier_refresh = inputControlData.refresh_value;
		}
		if (key == GLFW_KEY_Y) inputControlData.head_speed = PI / 20;
		if (key == GLFW_KEY_U) inputControlData.head_speed = -PI / 20;
		if (key == GLFW_KEY_3) mainRobot.isMarching = true;
    }
    if (action==GLFW_RELEASE) {
        if (key==GLFW_KEY_LEFT) inputControlData.speed_x=0;
        if (key==GLFW_KEY_RIGHT) inputControlData.speed_x=0;
        if (key==GLFW_KEY_UP) inputControlData.speed_y=0;
        if (key==GLFW_KEY_DOWN) inputControlData.speed_y=0;
		if (key == GLFW_KEY_A) inputControlData.speed_z = 0;
		if (key == GLFW_KEY_D) inputControlData.speed_z = 0;
		if (key == GLFW_KEY_1) inputControlData.val_speed = 0;
		if (key == GLFW_KEY_2) inputControlData.val_speed = 0;
		if (key == GLFW_KEY_Q) inputControlData.rot_speed = 0;
		if (key == GLFW_KEY_E) inputControlData.rot_speed = 0;
		if (key == GLFW_KEY_4) inputControlData.zoom_speed = 0;
		if (key == GLFW_KEY_5) inputControlData.zoom_speed = 0;
		if (key == GLFW_KEY_W) mainRobot.direction = NONE;
		if (key == GLFW_KEY_S) mainRobot.direction = NONE;
		if (key == GLFW_KEY_3) mainRobot.isMarching = false;
		if (mods & GLFW_MOD_SHIFT) {
			if (key == GLFW_KEY_R) inputControlData.right_shoulder_speed = 0;
			if (key == GLFW_KEY_T) inputControlData.right_elbow_speed = 0;
			if (key == GLFW_KEY_V) inputControlData.right_hand_speed = 0;
			if (key == GLFW_KEY_Z) inputControlData.right_shoulder_speed = 0;
			if (key == GLFW_KEY_X) inputControlData.right_elbow_speed = 0;
			if (key == GLFW_KEY_C) inputControlData.right_hand_speed = 0;
			inputControlData.shift_modifier_refresh = 0;
		}
		if (mods & GLFW_MOD_CONTROL) {
			if (key == GLFW_KEY_R) inputControlData.left_shoulder_speed = 0;
			if (key == GLFW_KEY_T) inputControlData.left_elbow_speed = 0;
			if (key == GLFW_KEY_V) inputControlData.left_hand_speed = 0;
			if (key == GLFW_KEY_Z) inputControlData.left_shoulder_speed = 0;
			if (key == GLFW_KEY_X) inputControlData.left_elbow_speed = 0;
			if (key == GLFW_KEY_C) inputControlData.left_hand_speed = 0;
			inputControlData.shift_modifier_refresh = 0;
		}
		if (key == GLFW_KEY_Y) inputControlData.head_speed = 0;
		if (key == GLFW_KEY_U) inputControlData.head_speed = 0;
    }
}

void windowResizeCallback(GLFWwindow* window,int width,int height) {
    if (height==0) return;
    aspectRatio=(float)width/(float)height;
    glViewport(0,0,width,height);
}



RobotJointAngles keyframe1(glm::vec3(0, 0, 3), glm::vec3(0, 0, PI), 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);


//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który należy wykonać raz, na początku programu************
	glClearColor(0,0,0,1);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window,windowResizeCallback);
	glfwSetKeyCallback(window,keyCallback);

	sp=new ShaderProgram("v_simplest.glsl",NULL,"f_simplest.glsl");
	tex0 = readTexture("rusty_metal_04_diff_4k.png");
	tex1 = readTexture("rusty_metal_04_metal_4k.png");
	tex2 = readTexture("green effect mask.png");
	tex3 = readTexture("TerrazzoSlab028/TerrazzoSlab028_COL_2K_METALNESS.png");
	tex4 = readTexture("TerrazzoSlab028/TerrazzoSlab028_ROUGHNESS_2K_METALNESS.png");
	tex5 = readTexture("black_pixel.png");




	mainRobot.currentKeyframe = keyframe1;
	mainRobot.texture_diff = &tex0;
	mainRobot.texture_metal = &tex1;
	mainRobot.texture_mask = &tex2;
	loadRobot(mainRobot);

	for (int i = 0; i < sizeof(robotArmy) / sizeof(robotArmy[0]); i++) {
		robotArmy[i].head.vertices = mainRobot.head.vertices;
		robotArmy[i].head.normals = mainRobot.head.normals;
		robotArmy[i].head.texCoords = mainRobot.head.texCoords;
		robotArmy[i].head.vertexCount = mainRobot.head.vertexCount;
		robotArmy[i].head.setTextures(mainRobot.head.texture_diff, mainRobot.head.texture_metal, mainRobot.head.texture_mask);

		robotArmy[i].torso.vertices = mainRobot.torso.vertices;
		robotArmy[i].torso.normals = mainRobot.torso.normals;
		robotArmy[i].torso.texCoords = mainRobot.torso.texCoords;
		robotArmy[i].torso.vertexCount = mainRobot.torso.vertexCount;
		robotArmy[i].torso.setTextures(mainRobot.torso.texture_diff, mainRobot.torso.texture_metal, mainRobot.torso.texture_mask);

		robotArmy[i].leftArm.vertices = mainRobot.leftArm.vertices;
		robotArmy[i].leftArm.normals = mainRobot.leftArm.normals;
		robotArmy[i].leftArm.texCoords = mainRobot.leftArm.texCoords;
		robotArmy[i].leftArm.vertexCount = mainRobot.leftArm.vertexCount;
		robotArmy[i].leftArm.setTextures(mainRobot.leftArm.texture_diff, mainRobot.leftArm.texture_metal, mainRobot.leftArm.texture_mask);

		robotArmy[i].leftArm2.vertices = mainRobot.leftArm2.vertices;
		robotArmy[i].leftArm2.normals = mainRobot.leftArm2.normals;
		robotArmy[i].leftArm2.texCoords = mainRobot.leftArm2.texCoords;
		robotArmy[i].leftArm2.vertexCount = mainRobot.leftArm2.vertexCount;
		robotArmy[i].leftArm2.setTextures(mainRobot.leftArm2.texture_diff, mainRobot.leftArm2.texture_metal, mainRobot.leftArm2.texture_mask);

		robotArmy[i].rightArm.vertices = mainRobot.rightArm.vertices;
		robotArmy[i].rightArm.normals = mainRobot.rightArm.normals;
		robotArmy[i].rightArm.texCoords = mainRobot.rightArm.texCoords;
		robotArmy[i].rightArm.vertexCount = mainRobot.rightArm.vertexCount;
		robotArmy[i].rightArm.setTextures(mainRobot.rightArm.texture_diff, mainRobot.rightArm.texture_metal, mainRobot.rightArm.texture_mask);

		robotArmy[i].rightArm2.vertices = mainRobot.rightArm2.vertices;
		robotArmy[i].rightArm2.normals = mainRobot.rightArm2.normals;
		robotArmy[i].rightArm2.texCoords = mainRobot.rightArm2.texCoords;
		robotArmy[i].rightArm2.vertexCount = mainRobot.rightArm2.vertexCount;
		robotArmy[i].rightArm2.setTextures(mainRobot.rightArm2.texture_diff, mainRobot.rightArm2.texture_metal, mainRobot.rightArm2.texture_mask);



		robotArmy[i].leftLeg.vertices = mainRobot.leftLeg.vertices;
		robotArmy[i].leftLeg.normals = mainRobot.leftLeg.normals;
		robotArmy[i].leftLeg.texCoords = mainRobot.leftLeg.texCoords;
		robotArmy[i].leftLeg.vertexCount = mainRobot.leftLeg.vertexCount;
		robotArmy[i].leftLeg.setTextures(mainRobot.leftLeg.texture_diff, mainRobot.leftLeg.texture_metal, mainRobot.leftLeg.texture_mask);

		robotArmy[i].leftLeg2.vertices = mainRobot.leftLeg2.vertices;
		robotArmy[i].leftLeg2.normals = mainRobot.leftLeg2.normals;
		robotArmy[i].leftLeg2.texCoords = mainRobot.leftLeg2.texCoords;
		robotArmy[i].leftLeg2.vertexCount = mainRobot.leftLeg2.vertexCount;
		robotArmy[i].leftLeg2.setTextures(mainRobot.leftLeg2.texture_diff, mainRobot.leftLeg2.texture_metal, mainRobot.leftLeg2.texture_mask);

		robotArmy[i].rightLeg.vertices = mainRobot.rightLeg.vertices;
		robotArmy[i].rightLeg.normals = mainRobot.rightLeg.normals;
		robotArmy[i].rightLeg.texCoords = mainRobot.rightLeg.texCoords;
		robotArmy[i].rightLeg.vertexCount = mainRobot.rightLeg.vertexCount;
		robotArmy[i].rightLeg.setTextures(mainRobot.rightLeg.texture_diff, mainRobot.rightLeg.texture_metal, mainRobot.rightLeg.texture_mask);

		robotArmy[i].rightLeg2.vertices = mainRobot.rightLeg2.vertices;
		robotArmy[i].rightLeg2.normals = mainRobot.rightLeg2.normals;
		robotArmy[i].rightLeg2.texCoords = mainRobot.rightLeg2.texCoords;
		robotArmy[i].rightLeg2.vertexCount = mainRobot.rightLeg2.vertexCount;
		robotArmy[i].rightLeg2.setTextures(mainRobot.rightLeg2.texture_diff, mainRobot.rightLeg2.texture_metal, mainRobot.rightLeg2.texture_mask);


		robotArmy[i].currentKeyframe = keyframe1;

		robotArmy[i].initRobotValues();

		int row = i / 5;
		int col = i % 5;
		robotArmy[i].currentKeyframe.position = glm::vec3(row * 10 - 10, col * 10 + 30, -10);
	}

	loadModel(terrain, "terrain2.fbx");
	terrain.texture_diff = &tex3;
	terrain.texture_metal = &tex4;
	terrain.texture_mask = &tex5;
	terrain.localTransform.z = -3;

	*fshift = 0;
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
    //************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************

    delete sp;
}

void armyMarchControl(RobotStructure& robot) {
	if (robot.isMarching) {
		robot.isMarchingAnimationFinished = false;
	}

	if (robot.isMarching || !robot.isMarchingAnimationFinished) marchForward(robot, robot.currentKeyframe);
}



void userInputMove(RobotStructure& robot) {
	//CZESCI CIALA

	if (robot.control == MANUAL) {
		std::cout << "manual control " << std::endl;
		robot.currentKeyframe.right_shoulder_forward = inputControlData.right_shoulder_angle;
		robot.currentKeyframe.right_elbow = inputControlData.right_elbow_angle;
		robot.currentKeyframe.right_hand = inputControlData.right_hand_angle;
		robot.currentKeyframe.left_shoulder_forward = inputControlData.left_shoulder_angle;
		robot.currentKeyframe.left_elbow = inputControlData.left_elbow_angle;
		robot.currentKeyframe.left_hand = inputControlData.left_hand_angle;
		robot.currentKeyframe.head_rot = inputControlData.head_angle;
	}

	if (robot.isMarching || !robot.isMarchingAnimationFinished) {
		robot.isMarchingAnimationFinished = false;
		marchForward(robot, robot.currentKeyframe);
	
	} else if (inputControlData.speed_z != 0 || !robot.isTurningAnimationFinished) {
		robot.isTurningAnimationFinished = false;
		turningAnimation(robot, robot.currentKeyframe);

	} else if (robot.direction == FORWARD || !robot.isForwardAnimationFinished) {
		robot.isForwardAnimationFinished = false;
		forwardStepAnimation(robot, robot.currentKeyframe);

	} else if (robot.direction == BACKWARD || !robot.isBackwardAnimationFinished) {
		robot.isBackwardAnimationFinished = false;
		backwardStepAnimation(robot, robot.currentKeyframe);

	}

		
	if (inputControlData.lerpPosition < 1.0f) robot.changePosition(robot);
}


//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window, RobotStructure& robot) {
	//************Tutaj umieszczaj kod rysujący obraz******************l
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int radius = 23 - inputControlData.zoom;

	userInputMove(robot);

	robot.directKinematicsLogic();
	
	for (int i = 0; i < sizeof(robotArmy) / sizeof(robotArmy[0]); i++) {
		armyMarchControl(robotArmy[i]);
		robotArmy[i].directKinematicsLogic();
	}


	// obliczenie wektora wskazujacego
	robot.calculatePointingVector();

	glm::vec4 camera_back_off =
		glm::rotate(glm::mat4(1.0f), inputControlData.rot_angle+robot.currentKeyframe.rotation.z, glm::vec3(0, 0, 1))
		* glm::vec4(0, -1, 0, 0);

	glm::vec3 cameraPosition = robot.currentKeyframe.position
		- glm::vec3(camera_back_off.x * 20, camera_back_off.y * 20, 0) //danie kamery z tyłu roboa
		+ glm::vec3(0, 0, 15); // danie kamery wyżej robota

	// generowanie wektora "up"
	glm::vec3 cameraDirectionLook = glm::normalize(robot.currentKeyframe.position - cameraPosition);
	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 0.0f, 1.0f), cameraDirectionLook));
	glm::vec3 up = glm::normalize(glm::cross(cameraDirectionLook, right));
	glm::mat4 V = glm::lookAt(
		cameraPosition,
		robot.currentKeyframe.position,
		up); //Wylicz macierz widoku

    glm::mat4 P=glm::perspective(50.0f*PI/180.0f, aspectRatio, 0.01f, 500.0f); //Wylicz macierz rzutowania
    
	terrain.applyTransform(glm::mat4(1.0f));
	terrain.drawOpenGL(P, V);
	robot.drawWholeRobot(P, V);

	for (int i = 0; i < sizeof(robotArmy) / sizeof(robotArmy[0]); i++) {
		robotArmy[i].drawWholeRobot(P, V);
	}

    glfwSwapBuffers(window); //Przerzuć tylny bufor na przedni
}


int main(void)
{
	GLFWwindow* window; //Wskaźnik na obiekt reprezentujący okno
	glfwSetErrorCallback(error_callback);//Zarejestruj procedurę obsługi błędów
	
	if (!glfwInit()) { //Zainicjuj bibliotekę GLFW
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}
	window = glfwCreateWindow(500, 500, "OpenGL", NULL, NULL);  //Utwórz okno 500x500 o tytule "OpenGL" i kontekst OpenGL.
	if (!window) //Jeżeli okna nie udało się utworzyć, to zamknij program
	{
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window); //Od tego momentu kontekst okna staje się aktywny i polecenia OpenGL będą dotyczyć właśnie jego.
	glfwSwapInterval(1); //Czekaj na 1 powrót plamki przed pokazaniem ukrytego bufora

	if (glewInit() != GLEW_OK) { //Zainicjuj bibliotekę GLEW
		fprintf(stderr, "Nie można zainicjować GLEW.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window); //Operacje inicjujące

	//Główna pętla
	glfwSetTime(0); //Zeruj timer
	while (!glfwWindowShouldClose(window)) //Tak długo jak okno nie powinno zostać zamknięte
	{
		inputControlData.updateControlLoop(glfwGetTime());
		*fshift += glfwGetTime();
		glfwSetTime(0); //Zeruj timer
		drawScene(window, mainRobot); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}
