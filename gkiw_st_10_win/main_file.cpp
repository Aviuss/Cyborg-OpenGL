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
	float val = 0;


	void updateControlLoop(float deltaTime) {
		angle_x += speed_x * deltaTime; //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
		angle_y += speed_y * deltaTime; //Zwiększ/zmniejsz kąt obrotu na podstawie prędkości i czasu jaki upłynał od poprzedniej klatki
		val += val_speed * deltaTime;
	}

	float speed_x = 0;
	float speed_y = 0;

	float val_speed = 0;
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
		glUniform4f(sp->u("lp"), 0, 0, -6, 1);
		glUniform1i(sp->u("textureMap0"), 0); //drawScene

		glEnableVertexAttribArray(sp->a("vertex"));  //Włącz przesyłanie danych do atrybutu vertex
		glVertexAttribPointer(sp->a("vertex"), 4, GL_FLOAT, false, 0, vertices); //Wskaż tablicę z danymi dla atrybutu vertex

		//glEnableVertexAttribArray(sp->a("color"));  //Włącz przesyłanie danych do atrybutu color
		//glVertexAttribPointer(sp->a("color"), 4, GL_FLOAT, false, 0, colors); //Wskaż tablicę z danymi dla atrybutu color

		glEnableVertexAttribArray(sp->a("normal"));  //Włącz przesyłanie danych do atrybutu normal
		glVertexAttribPointer(sp->a("normal"), 4, GL_FLOAT, false, 0, normals); //Wskaż tablicę z danymi dla atrybutu normal

		glEnableVertexAttribArray(sp->a("texCoord0"));
		glVertexAttribPointer(sp->a("texCoord0"),
			2, GL_FLOAT, false, 0, texCoords);//odpowiednia tablica


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, tex1);


		glDrawArrays(GL_TRIANGLES, 0, vertexCount); //Narysuj obiekt

		glDisableVertexAttribArray(sp->a("vertex"));  //Wyłącz przesyłanie danych do atrybutu vertex
		//glDisableVertexAttribArray(sp->a("color"));  //Wyłącz przesyłanie danych do atrybutu color
		glDisableVertexAttribArray(sp->a("normal"));  //Wyłącz przesyłanie danych do atrybutu normal
		glDisableVertexAttribArray(sp->a("texCoord0"));

	}

	inline void applyTransform(glm::mat4& originMatrix) {
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
};

struct RobotStructure; // tylko deklaracja

struct RobotJointAngles {
	//torso
	float head_rot = PI / 7;
	float torso_rot = PI/10;

	// left arm
	float left_shoulder_forward = PI/5;
	float left_shoulder_side = PI / 4;
	float left_shoulder_rot = PI / 2;
	float left_elbow = PI / 2;
	float left_hand = PI / 3;

	//right arm
	float right_shoulder_forward = PI / 5;
	float right_shoulder_side = PI / 4;
	float right_shoulder_rot = 0;
	float right_elbow = PI/2;
	float right_hand = PI / 3;

	// left leg
	float left_leg_forward = PI / 4;
	float left_leg_rot = -PI / 10;
	float left_leg2_forward = -PI/10;
	float left_foot = PI/10;


	// right leg
	float right_leg_forward = -PI / 4;
	float right_leg_rot = PI / 10;
	float right_leg2_forward = -PI / 4;
	float right_foot = -PI / 2;

	void applyAngles(RobotStructure& robot);
};


struct RobotStructure {
	glm::vec3 position = glm::vec3(0,0,0);
	glm::vec3 rotation = glm::vec3(0, 0, PI);

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

	RobotJointAngles currentPosition;


	void directKinematicsLogic() {
		currentPosition.applyAngles(*this);

		glm::mat4 robotWholeMatrix = glm::translate(glm::mat4(1.0f), position);
		robotWholeMatrix = glm::rotate(robotWholeMatrix, rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
		robotWholeMatrix = glm::rotate(robotWholeMatrix, rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
		robotWholeMatrix = glm::rotate(robotWholeMatrix, rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

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

RobotStructure robotStructure;
Model3D terrain;

void loadModel(Model3D& model, std::string fileName) {

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(fileName,
		aiProcess_Triangulate |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
		return;
	}

	std::vector<float> vertices_local;
	std::vector<float> normals_local;
	std::vector<float> texCoords_local;
	
	for (int meshId = 0; meshId < scene->mNumMeshes; meshId ++) {
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

	model.vertexCount = vertices_local.size()/4;
	return;
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
    if (action==GLFW_PRESS) {
        if (key==GLFW_KEY_LEFT) inputControlData.speed_x=-PI/2;
        if (key==GLFW_KEY_RIGHT) inputControlData.speed_x=PI/2;
        if (key==GLFW_KEY_UP) inputControlData.speed_y=PI/2;
        if (key==GLFW_KEY_DOWN) inputControlData.speed_y=-PI/2;
		if (key == GLFW_KEY_1) inputControlData.val_speed = -0.5;
		if (key == GLFW_KEY_2) inputControlData.val_speed = 0.5;
    }
    if (action==GLFW_RELEASE) {
        if (key==GLFW_KEY_LEFT) inputControlData.speed_x=0;
        if (key==GLFW_KEY_RIGHT) inputControlData.speed_x=0;
        if (key==GLFW_KEY_UP) inputControlData.speed_y=0;
        if (key==GLFW_KEY_DOWN) inputControlData.speed_y=0;
		if (key == GLFW_KEY_1) inputControlData.val_speed = 0;
		if (key == GLFW_KEY_2) inputControlData.val_speed = 0;
    }
}

void windowResizeCallback(GLFWwindow* window,int width,int height) {
    if (height==0) return;
    aspectRatio=(float)width/(float)height;
    glViewport(0,0,width,height);
}

//Procedura inicjująca
void initOpenGLProgram(GLFWwindow* window) {
	//************Tutaj umieszczaj kod, który należy wykonać raz, na początku programu************
	glClearColor(0,0,0,1);
	glEnable(GL_DEPTH_TEST);
	glfwSetWindowSizeCallback(window,windowResizeCallback);
	glfwSetKeyCallback(window,keyCallback);

	sp=new ShaderProgram("v_simplest.glsl",NULL,"f_simplest.glsl");
	tex0 = readTexture("red_brick_diff_4k.png");
	tex1 = readTexture("sky.png");


	loadRobot(robotStructure);
	loadModel(terrain, "terrain.fbx");
}


//Zwolnienie zasobów zajętych przez program
void freeOpenGLProgram(GLFWwindow* window) {
    //************Tutaj umieszczaj kod, który należy wykonać po zakończeniu pętli głównej************

    delete sp;
}

void userInputMove(RobotStructure& robot) {
	robot.torso.localEulerRotation.x = inputControlData.angle_y;
	robot.torso.localEulerRotation.z = inputControlData.angle_x;

	std::cout << inputControlData.val << std::endl;
	robot.position.y = -20*inputControlData.val;
}


//Procedura rysująca zawartość sceny
void drawScene(GLFWwindow* window, RobotStructure& robot) {
	//************Tutaj umieszczaj kod rysujący obraz******************l
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	userInputMove(robot);
	robot.directKinematicsLogic();

	glm::mat4 V=glm::lookAt(
         glm::vec3(0, 20+robot.position.y, 15),
         robot.position,
         glm::vec3(0.0f,-1.0f,0.0f)); //Wylicz macierz widoku

    glm::mat4 P=glm::perspective(50.0f*PI/180.0f, aspectRatio, 0.01f, 500.0f); //Wylicz macierz rzutowania
    

	terrain.drawOpenGL(P, V);
	robot.drawWholeRobot(P, V);

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
        glfwSetTime(0); //Zeruj timer
		drawScene(window, robotStructure); //Wykonaj procedurę rysującą
		glfwPollEvents(); //Wykonaj procedury callback w zalezności od zdarzeń jakie zaszły.
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window); //Usuń kontekst OpenGL i okno
	glfwTerminate(); //Zwolnij zasoby zajęte przez GLFW
	exit(EXIT_SUCCESS);
}
