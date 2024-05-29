#include "Render.h"
#include <future>
#include <sstream>
#include <iostream>

#include <windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>


#include "ObjLoader.h"
#include "MyOGL.h"

#include "Camera.h"
#include "Light.h"
#include "Primitives.h"

#include "GUItextRectangle.h"

bool textureMode = true;
bool lightMode = true;

#define PI 3.14159265
void bSort(double list[500][3], int listLength);
void CyrcleSort(double points[][3], double base_point[3], int n);


//класс для настройки камеры
class CustomCamera : public Camera
{
public:
	//дистанция камеры
	double camDist;
	//углы поворота камеры
	double fi1, fi2;

	
	//значния масеры по умолчанию
	CustomCamera()
	{
		camDist = 15;
		fi1 = 1;
		fi2 = 1;
	}

	
	//считает позицию камеры, исходя из углов поворота, вызывается движком
	void SetUpCamera()
	{
		//отвечает за поворот камеры мышкой
		lookPoint.setCoords(0, 0, 0);

		pos.setCoords(camDist*cos(fi2)*cos(fi1),
			camDist*cos(fi2)*sin(fi1),
			camDist*sin(fi2));

		if (cos(fi2) <= 0)
			normal.setCoords(0, 0, -1);
		else
			normal.setCoords(0, 0, 1);

		LookAt();
	}

	void CustomCamera::LookAt()
	{
		//функция настройки камеры
		gluLookAt(pos.X(), pos.Y(), pos.Z(), lookPoint.X(), lookPoint.Y(), lookPoint.Z(), normal.X(), normal.Y(), normal.Z());
	}



}  camera;   //создаем объект камеры


//Класс для настройки света
class CustomLight : public Light
{
public:
	CustomLight()
	{
		//начальная позиция света
		pos = Vector3(1, 1, 3);
	}

	
	//рисует сферу и линии под источником света, вызывается движком
	void  DrawLightGhismo()
	{
		glDisable(GL_LIGHTING);

		
		glColor3d(0.9, 0.8, 0);
		Sphere s;
		s.pos = pos;
		s.scale = s.scale*0.08;
		s.Show();
		
		if (OpenGL::isKeyPressed('G'))
		{
			glColor3d(0, 0, 0);
			//линия от источника света до окружности
			glBegin(GL_LINES);
			glVertex3d(pos.X(), pos.Y(), pos.Z());
			glVertex3d(pos.X(), pos.Y(), 0);
			glEnd();

			//рисуем окруность
			Circle c;
			c.pos.setCoords(pos.X(), pos.Y(), 0);
			c.scale = c.scale*1.5;
			c.Show();
		}

	}

	void SetUpLight()
	{
		GLfloat amb[] = { 0.2, 0.2, 0.2, 0 };
		GLfloat dif[] = { 1.0, 1.0, 1.0, 0 };
		GLfloat spec[] = { .7, .7, .7, 0 };
		GLfloat position[] = { pos.X(), pos.Y(), pos.Z(), 1. };

		// параметры источника света
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		// характеристики излучаемого света
		// фоновое освещение (рассеянный свет)
		glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
		// диффузная составляющая света
		glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
		// зеркально отражаемая составляющая света
		glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

		glEnable(GL_LIGHT0);
	}


} light;  //создаем источник света




//старые координаты мыши
int mouseX = 0, mouseY = 0;

void mouseEvent(OpenGL *ogl, int mX, int mY)
{
	int dx = mouseX - mX;
	int dy = mouseY - mY;
	mouseX = mX;
	mouseY = mY;

	//меняем углы камеры при нажатой левой кнопке мыши
	if (OpenGL::isKeyPressed(VK_RBUTTON))
	{
		camera.fi1 += 0.01*dx;
		camera.fi2 += -0.01*dy;
	}

	
	//двигаем свет по плоскости, в точку где мышь
	if (OpenGL::isKeyPressed('G') && !OpenGL::isKeyPressed(VK_LBUTTON))
	{
		LPPOINT POINT = new tagPOINT();
		GetCursorPos(POINT);
		ScreenToClient(ogl->getHwnd(), POINT);
		POINT->y = ogl->getHeight() - POINT->y;

		Ray r = camera.getLookRay(POINT->x, POINT->y);

		double z = light.pos.Z();

		double k = 0, x = 0, y = 0;
		if (r.direction.Z() == 0)
			k = 0;
		else
			k = (z - r.origin.Z()) / r.direction.Z();

		x = k*r.direction.X() + r.origin.X();
		y = k*r.direction.Y() + r.origin.Y();

		light.pos = Vector3(x, y, z);
	}

	if (OpenGL::isKeyPressed('G') && OpenGL::isKeyPressed(VK_LBUTTON))
	{
		light.pos = light.pos + Vector3(0, 0, 0.02*dy);
	}

	
}

void mouseWheelEvent(OpenGL *ogl, int delta)
{

	if (delta < 0 && camera.camDist <= 1)
		return;
	if (delta > 0 && camera.camDist >= 100)
		return;

	camera.camDist += 0.01*delta;

}


bool FrogJump = false;
void ChangeTexture();
void keyDownEvent(OpenGL *ogl, int key)
{
	if (key == 'L')
	{
		lightMode = !lightMode;
	}

	if (key == 'T')
	{
		textureMode = !textureMode;
	}

	if (key == 'R')
	{
		camera.fi1 = 1;
		camera.fi2 = 1;
		camera.camDist = 15;

		light.pos = Vector3(1, 1, 3);
	}

	if (key == 'F')
	{
		light.pos = camera.pos;
	}

	if (key == ' ') {
		FrogJump = true;
	}
}



void keyUpEvent(OpenGL *ogl, int key)
{
	
}

GLuint texId, texId2, texRoc, texSakura, texFrog;



ObjFile tree, frog;

//выполняется перед первым рендером
void initRender(OpenGL* ogl)
{
	//настройка текстур

	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	//настройка режима наложения текстур
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	//включаем текстуры
	glEnable(GL_TEXTURE_2D);


	//массив трехбайтных элементов  (R G B)
	RGBTRIPLE* texarray;

	//массив символов, (высота*ширина*4      4, потомучто   выше, мы указали использовать по 4 байта на пиксель текстуры - R G B A)
	char* texCharArray;
	int texW, texH;
	OpenGL::LoadBMP("water1.bmp", &texW, &texH, &texarray);
	OpenGL::RGBtoChar(texarray, texW, texH, &texCharArray);



	//генерируем ИД для текстуры
	glGenTextures(1, &texId);
	//биндим айдишник, все что будет происходить с текстурой, будте происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texId);

	//загружаем текстуру в видеопямять, в оперативке нам больше  она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray);

	//отчистка памяти
	free(texCharArray);
	free(texarray);

	//наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	RGBTRIPLE* texarray2;
	char* texCharArray2;
	int texW2, texH2;
	OpenGL::LoadBMP("grass2.bmp", &texW2, &texH2, &texarray2);
	OpenGL::RGBtoChar(texarray2, texW2, texH2, &texCharArray2);

	// Генерируем ИД для второй текстуры
	glGenTextures(1, &texId2);
	// Биндим айдишник, все что будет происходить с текстурой, будет происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texId2);

	// Загружаем текстуру в видеопамять, в оперативке нам больше она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW2, texH2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray2);

	// Отчистка памяти
	free(texCharArray2);
	free(texarray2);

	// Наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//3 

	RGBTRIPLE* texarray3;
	char* texCharArray3;
	int texW3, texH3;
	OpenGL::LoadBMP("stone1.bmp", &texW3, &texH3, &texarray3);
	OpenGL::RGBtoChar(texarray3, texW3, texH3, &texCharArray3);

	// Генерируем ИД для второй текстуры
	glGenTextures(1, &texRoc);
	// Биндим айдишник, все что будет происходить с текстурой, будет происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texRoc);

	// Загружаем текстуру в видеопамять, в оперативке нам больше она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW3, texH3, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray3);

	// Отчистка памяти
	free(texCharArray3);
	free(texarray3);

	// Наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//4
	RGBTRIPLE* texarray4;
	char* texCharArray4;
	int texW4, texH4;
	OpenGL::LoadBMP("plaki.bmp", &texW4, &texH4, &texarray4);
	OpenGL::RGBtoChar(texarray4, texW4, texH4, &texCharArray4);

	// Генерируем ИД для второй текстуры
	glGenTextures(1, &texSakura);
	// Биндим айдишник, все что будет происходить с текстурой, будет происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texSakura);

	// Загружаем текстуру в видеопамять, в оперативке нам больше она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW4, texH4, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray4);

	// Отчистка памяти
	free(texCharArray4);
	free(texarray4);

	// Наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


	//5

	RGBTRIPLE* texarray5;
	char* texCharArray5;
	int texW5, texH5;
	OpenGL::LoadBMP("frog.bmp", &texW5, &texH5, &texarray5);
	OpenGL::RGBtoChar(texarray5, texW5, texH5, &texCharArray5);

	// Генерируем ИД для второй текстуры
	glGenTextures(1, &texFrog);
	// Биндим айдишник, все что будет происходить с текстурой, будет происходить по этому ИД
	glBindTexture(GL_TEXTURE_2D, texFrog);

	// Загружаем текстуру в видеопамять, в оперативке нам больше она не нужна
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW5, texH5, 0, GL_RGBA, GL_UNSIGNED_BYTE, texCharArray5);

	// Отчистка памяти
	free(texCharArray5);
	free(texarray5);

	// Наводим шмон
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	loadModel("dada.obj", &tree);
	loadModel("frog.obj", &frog);

	//камеру и свет привязываем к "движку"
	ogl->mainCamera = &camera;
	ogl->mainLight = &light;

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// устранение ступенчатости для линий
	glEnable(GL_LINE_SMOOTH);


	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);

	camera.fi1 = -1.3;
	camera.fi2 = 0.8;
}


double Search_delta_time() {
	static auto end_render = std::chrono::steady_clock::now();
	auto cur_time = std::chrono::steady_clock::now();
	auto deltatime = cur_time - end_render;
	double delta = 1.0 * std::chrono::duration_cast<std::chrono::microseconds>(deltatime).count() / 1000000;
	end_render = cur_time;
	return delta;
}

double Cur_Time() {
	auto cur_time = std::chrono::steady_clock::now();
	double current_time = std::chrono::duration_cast<std::chrono::microseconds>(cur_time.time_since_epoch()).count() / 1000000.0;
	return current_time;
}

inline double f(double p1, double p2, double p3, double t)
{
	return p1 * (1 - t) * (1 - t) + 2 * p2 * t * (1 - t) + p3 * t * t; //посчитаная формула
}

inline double Beze3f(double p1, double p2, double p3, double p4, double t) {
	return pow(1 - t, 3) * p1 + 3 * t * pow(1 - t, 2) * p2 + 3 * pow(t, 2) * (1 - t) * p3 + pow(t, 3) * p4;
}

void Bezie3(double P1[3], double P2[3], double P3[3], double P4[3]) {
	double P[3];
	glColor3d(1, 0, 0);
	glBegin(GL_LINE_STRIP);
	glVertex3dv(P1);
	glVertex3dv(P2);
	glVertex3dv(P3);
	glVertex3dv(P4);
	glEnd();
	glLineWidth(3); //ширина линии
	glColor3d(0, 0, 0);
	glBegin(GL_LINE_STRIP);
	for (double t = 0; t <= 1.0001; t += 0.01)
	{
		double P[3];
		P[0] = Beze3f(P1[0], P2[0], P3[0], P4[0], t);
		P[1] = Beze3f(P1[1], P2[1], P3[1], P4[1], t);
		P[2] = Beze3f(P1[2], P2[2], P3[2], P4[2], t);
		glVertex3dv(P); //Рисуем точку P
	}
	glEnd();
	glLineWidth(1);
	glColor3d(1, 0, 0);

	glPointSize(10);
	glBegin(GL_POINTS);
	glVertex3dv(P1);
	glVertex3dv(P2);
	glVertex3dv(P3);
	glVertex3dv(P4);
	glEnd();
	glColor3d(0, 0, 0);

}

void DrawSq() {
	// Координаты вершин куба
	double A[] = { 0.0, 0.0, 0.0 };
	double B[] = { 0.0, 0.1, 0.0 };
	double C[] = { 0.1, 0.1, 0.0 };
	double D[] = { 0.1, 0.0, 0.0 };

	double A1[] = { 0.0, 0.0, 0.1 };
	double B1[] = { 0.0, 0.1, 0.1 };
	double C1[] = { 0.1, 0.1, 0.1 };
	double D1[] = { 0.1, 0.0, 0.1 };

	// Привязываем текстуру
	glBindTexture(GL_TEXTURE_2D, texId);

	glBegin(GL_QUADS);
	// ВЕРХ
	glNormal3d(0.0, 0.0, -1.0);
	glTexCoord2d(0, 0); glVertex3dv(A);
	glTexCoord2d(0, 1); glVertex3dv(B);
	glTexCoord2d(1, 1); glVertex3dv(C);
	glTexCoord2d(1, 0); glVertex3dv(D);

	// НИЗ
	glNormal3d(0.0, 0.0, 1.0);
	glTexCoord2d(0, 0); glVertex3dv(A1);
	glTexCoord2d(0, 1); glVertex3dv(B1);
	glTexCoord2d(1, 1); glVertex3dv(C1);
	glTexCoord2d(1, 0); glVertex3dv(D1);

	// СТЕНЫ
	// Передняя
	glNormal3d(0.0, -1.0, 0.0);
	glTexCoord2d(0, 0); glVertex3dv(A);
	glTexCoord2d(0, 1); glVertex3dv(A1);
	glTexCoord2d(1, 1); glVertex3dv(D1);
	glTexCoord2d(1, 0); glVertex3dv(D);

	// Задняя
	glNormal3d(0.0, 1.0, 0.0);
	glTexCoord2d(0, 0); glVertex3dv(B);
	glTexCoord2d(0, 1); glVertex3dv(C);
	glTexCoord2d(1, 1); glVertex3dv(C1);
	glTexCoord2d(1, 0); glVertex3dv(B1);

	// Левая
	glNormal3d(-1.0, 0.0, 0.0);
	glTexCoord2d(0, 0); glVertex3dv(A);
	glTexCoord2d(0, 1); glVertex3dv(B);
	glTexCoord2d(1, 1); glVertex3dv(B1);
	glTexCoord2d(1, 0); glVertex3dv(A1);

	// Правая
	glNormal3d(1.0, 0.0, 0.0);
	glTexCoord2d(0, 0); glVertex3dv(D);
	glTexCoord2d(0, 1); glVertex3dv(C);
	glTexCoord2d(1, 1); glVertex3dv(C1);
	glTexCoord2d(1, 0); glVertex3dv(D1);

	glEnd();

	// Отключаем текстуру
	glBindTexture(GL_TEXTURE_2D, 0);
}

void ActionBizie3(double P1[3], double P2[3], double P3[3], double P4[3], double t_m) {
	int numCubes = 50;  // Количество кубиков
	double interval = 0.02;  // Интервал временного сдвига

	for (int i = 0; i < numCubes; i++) {
		// Рассчитаем временной параметр для текущего кубика
		double temp_t = fmod(t_m + i * interval, 1);

		glPushMatrix();
		double point1[] = { Beze3f(P1[0], P2[0], P3[0], P4[0], temp_t),
							Beze3f(P1[1], P2[1], P3[1], P4[1], temp_t),
							Beze3f(P1[2], P2[2], P3[2], P4[2], temp_t) };

		glTranslated(point1[0], point1[1], point1[2]);
		glColor3d(0.0, 0.5, 1.0);
		DrawSq();
		glPopMatrix();
	}
}


void DrawFrog();
void ActionFrog(double P1[3], double P2[3], double P3[3], double P4[3], double t_m, bool un = false) {
	glPushMatrix();
	double point1[] = { Beze3f(P1[0], P2[0], P3[0], P4[0], t_m),
						Beze3f(P1[1], P2[1], P3[1], P4[1],t_m),
						Beze3f(P1[2], P2[2], P3[2], P4[2], t_m) };

	glTranslated(point1[0], point1[1], point1[2]);
	if (un) {
		glRotated(180, 0, 0, 1);
	}
	glColor3d(0.0, 0.5, 1.0);
	DrawFrog();
	glPopMatrix();
}

void computeNormal(double* a, double* b, double* c, double* normal) {
	double u[3], v[3];

	// U = B - A
	u[0] = b[0] - a[0];
	u[1] = b[1] - a[1];
	u[2] = b[2] - a[2];

	// V = C - A
	v[0] = c[0] - a[0];
	v[1] = c[1] - a[1];
	v[2] = c[2] - a[2];

	// N = U x V
	normal[0] = u[1] * v[2] - u[2] * v[1];
	normal[1] = u[2] * v[0] - u[0] * v[2];
	normal[2] = u[0] * v[1] - u[1] * v[0];

	// Normalize the normal
	double length = sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
	if (length != 0) {
		normal[0] /= length;
		normal[1] /= length;
		normal[2] /= length;
	}
}

void DrawMountain(double height, double width, double width2) {
	// Размеры
	double A[] = { 0, 0, 0 };
	double B[] = { 0, width, 0 };
	double C[] = { width2, width, 0 };
	double D[] = { width2, 0, 0 };

	double A1[] = { 0, 0, height };
	double B1[] = { 0, width, height };
	double C1[] = { width2, width, height };
	double D1[] = { width2, 0, height };


	double half_w = width / 1.5;

	double A2[] = { -half_w, 0, 0 };
	double B2[] = { -half_w, width, 0 };

	double C2[] = { width2 + half_w, width, 0 };
	double D2[] = { width2 + half_w, 0, 0 };

	double normal[3];

	// Нижняя поверхность
	glBegin(GL_QUADS);
	glNormal3d(0.0, 0.0, -1.0);
	glVertex3dv(A);
	glVertex3dv(B);
	glVertex3dv(C);
	glVertex3dv(D);
	glEnd();


	glBindTexture(GL_TEXTURE_2D, texId);
	// Верхняя поверхность
	glBegin(GL_QUADS);
	glNormal3d(0.0, 0.0, 1.0);
	glTexCoord2d(0, 0);
	glVertex3dv(A1);
	glTexCoord2d(0, 1);
	glVertex3dv(B1);
	glTexCoord2d(1, 0);
	glVertex3dv(C1);
	glTexCoord2d(1, 1);
	glVertex3dv(D1);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);

	glBindTexture(GL_TEXTURE_2D, texRoc);
	// Левая наклонная грань
	computeNormal(A2, B2, B1, normal);
	glBegin(GL_QUADS);
	glNormal3d(-normal[0], normal[1], normal[2]);
	glTexCoord2d(0, 0);
	glVertex3dv(A2);
	glTexCoord2d(0, 1);
	glVertex3dv(B2);
	glTexCoord2d(1, 0);
	glVertex3dv(B1);
	glTexCoord2d(1, 1);
	glVertex3dv(A1);
	glEnd();


	// Правая наклонная грань
	computeNormal(C2, D2, D1, normal);
	glBegin(GL_QUADS);
	glNormal3d(-normal[0], normal[1], normal[2]);
	glTexCoord2d(0, 0);
	glVertex3dv(C2);
	glTexCoord2d(0, 1);
	glVertex3dv(D2);
	glTexCoord2d(1, 0);
	glVertex3dv(D1);
	glTexCoord2d(1, 1);
	glVertex3dv(C1);
	glEnd();


	// Левая вертикальная грань
	computeNormal(C2, C1, B1, normal);
	glBegin(GL_POLYGON);
	glNormal3d(normal[0], -normal[1], normal[2]);
	glTexCoord2d(0, 0);
	glVertex3dv(C2);
	glTexCoord2d(1, 0.25);
	glVertex3dv(C1);
	glTexCoord2d(1, 0.5);
	glVertex3dv(B1);
	glTexCoord2d(0, 1);
	glVertex3dv(B2);
	glEnd();


	// Правая вертикальная грань
	computeNormal(D2, D1, A1, normal);
	glBegin(GL_POLYGON);
	glNormal3d(normal[0],normal[1],normal[2]);
	glTexCoord2d(0, 0);
	glVertex3dv(D2);
	glTexCoord2d(1, 0.25);
	glVertex3dv(D1);
	glTexCoord2d(1, 0.5);
	glVertex3dv(A1);
	glTexCoord2d(0, 1);
	glVertex3dv(A2);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
}


void DrawPlatform() {
	double height = 0.5;
	double size = 10.0;

	// Верхняя поверхность (разделенная на две части для наложения разных текстур)
	glBindTexture(GL_TEXTURE_2D, texId); // Привязываем первую текстуру (голубая часть)
	glColor3d(0.0, 0.0, 0.5); // Голубой цвет
	glBegin(GL_QUADS);
	glNormal3d(0.0, 1.0, 0.0); // Нормаль направлена вверх
	glTexCoord2d(0, 0); glVertex3d(0, height, -size);
	glTexCoord2d(1, 0); glVertex3d(size, height, -size);
	glTexCoord2d(1, 1); glVertex3d(size, height, size);
	glTexCoord2d(0, 1); glVertex3d(0, height, size);
	glEnd();

	glBindTexture(GL_TEXTURE_2D, texId2); // Привязываем вторую текстуру (зеленая часть)
	glColor3d(0.0, 1.0, 0.0); // Зеленый цвет
	glBegin(GL_QUADS);
	glNormal3d(0.0, 1.0, 0.0); // Нормаль направлена вверх
	glTexCoord2d(0, 0); glVertex3d(-size, height, -size);
	glTexCoord2d(1, 0); glVertex3d(0, height, -size);
	glTexCoord2d(1, 1); glVertex3d(0, height, size);
	glTexCoord2d(0, 1); glVertex3d(-size, height, size);
	glEnd();

	// Нижняя поверхность
	glBindTexture(GL_TEXTURE_2D, texId2); // Привязываем текстуру
	glColor3d(0.5, 0.5, 0.5); // Серый цвет для нижней поверхности
	glBegin(GL_QUADS);
	glNormal3d(0.0, -1.0, 0.0);
	glTexCoord2d(0, 0); glVertex3d(-size, 0.0, -size);
	glTexCoord2d(1, 0); glVertex3d(size, 0.0, -size);
	glTexCoord2d(1, 1); glVertex3d(size, 0.0, size);
	glTexCoord2d(0, 1); glVertex3d(-size, 0.0, size);
	glEnd();

	// Передняя грань
	glBindTexture(GL_TEXTURE_2D, texId); // Привязываем текстуру
	glBegin(GL_QUADS);
	glNormal3d(0.0, 0.0, -1.0);
	glTexCoord2d(0, 0); glVertex3d(-size, 0.0, -size);
	glTexCoord2d(1, 0); glVertex3d(size, 0.0, -size);
	glTexCoord2d(1, 1); glVertex3d(size, height, -size);
	glTexCoord2d(0, 1); glVertex3d(-size, height, -size);
	glEnd();

	// Задняя грань
	glBindTexture(GL_TEXTURE_2D, texId); // Привязываем текстуру
	glBegin(GL_QUADS);
	glNormal3d(0.0, 0.0, 1.0);
	glTexCoord2d(0, 0); glVertex3d(-size, 0.0, size);
	glTexCoord2d(1, 0); glVertex3d(size, 0.0, size);
	glTexCoord2d(1, 1); glVertex3d(size, height, size);
	glTexCoord2d(0, 1); glVertex3d(-size, height, size);
	glEnd();

	// Левая грань
	glBindTexture(GL_TEXTURE_2D, texId); // Привязываем текстуру
	glBegin(GL_QUADS);
	glNormal3d(-1.0, 0.0, 0.0);
	glTexCoord2d(0, 0); glVertex3d(-size, 0.0, -size);
	glTexCoord2d(1, 0); glVertex3d(-size, 0.0, size);
	glTexCoord2d(1, 1); glVertex3d(-size, height, size);
	glTexCoord2d(0, 1); glVertex3d(-size, height, -size);
	glEnd();

	// Правая грань
	glBindTexture(GL_TEXTURE_2D, texId); // Привязываем текстуру
	glBegin(GL_QUADS);
	glNormal3d(1.0, 0.0, 0.0);
	glTexCoord2d(0, 0); glVertex3d(size, 0.0, -size);
	glTexCoord2d(1, 0); glVertex3d(size, 0.0, size);
	glTexCoord2d(1, 1); glVertex3d(size, height, size);
	glTexCoord2d(0, 1); glVertex3d(size, height, -size);
	glEnd();

	// Отключаем текстуру
	glBindTexture(GL_TEXTURE_2D, 0);
}


double t_jump = 0;
bool frog_statr = true;

void DrawFrog() {
	double p = 0.06f;




	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, texFrog);
	glRotated(90, 1, 0, 0);
	//glTranslated(1, 0.5, 1);
	glScaled(p, p, p);
	frog.DrawObj();
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 0);


}



void Render(OpenGL *ogl)
{

	static double t_max = 1;
	static double t_max_next = 0;
	static bool flag_tmax = true;

	//настройка времени
	double delta_time = Search_delta_time();
	double go = delta_time / 5;
	//t_max сама по себе изменяется от 0 до 1 постепенно от кадра к кадру
		t_max -= go;
		t_max_next = t_max - go;
		if (t_max < 0) {
			t_max = 1;
			flag_tmax = true;
		}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	if (textureMode)
		glEnable(GL_TEXTURE_2D);

	if (lightMode)
		glEnable(GL_LIGHTING);


	//альфаналожение
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	//настройка материала
	GLfloat amb[] = { 0.2, 0.2, 0.1, 1. };
	GLfloat dif[] = { 0.4, 0.65, 0.5, 1. };
	GLfloat spec[] = { 0.9, 0.8, 0.3, 1. };
	GLfloat sh = 0.1f * 256;


	//фоновая
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	//дифузная
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	//зеркальная
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec); \
		//размер блика
		glMaterialf(GL_FRONT, GL_SHININESS, sh);

	//чтоб было красиво, без квадратиков (сглаживание освещения)
	glShadeModel(GL_SMOOTH);
	//==================================
// Начальная точка водопада (внизу)
	double b2P0[] = { -1.5, 0, 0.5 }; // Уровень земли

	// Контрольная точка для начального подъема
	double b2P1[] = { 2, 0, 0 }; // Немного вверх и назад

	// Контрольная точка для среднего подъема
	double b2P2[] = { 3, 0, 5 }; // Ещё выше и немного в сторону

	// Конечная точка водопада (вверху)
	double b2P3[] = { 4, 0,6 }; // Высоко вверх


	double b1P0[] = { -1, 0, 0.6}; // Уровень земли

	// Контрольная точка для начального подъема
	double b1P1[] = { 2, 0, 0.4 }; // Немного вверх и назад

	// Контрольная точка для среднего подъема
	double b1P2[] = { 3, 0, 4 }; // Ещё выше и немного в сторону

	// Конечная точка водопада (вверху)
	double b1P3[] = { 4, 0,6 }; // Высоко вверх



	double b3P0[] = { 1, -2, 0.5 }; 

	double b3P1[] = { 1, 1, 2 }; 

	double b3P2[] = { 1, 3, 3 }; 

	double b3P3[] = { 1, 4,0.5 }; 


	for (double i = 0; i < 1; i += 0.1) {
		//b2P0[0] += i;
		b1P0[1] += 0.2;

		//b2P1[0] += i;
		b1P1[1] += 0.2;

		//b2P2[0] += i;
		b1P2[1] += 0.2;

		// b2P3[0] += i;
		b1P3[1] += 0.2;

		ActionBizie3(b1P0, b1P1, b1P2, b1P3, t_max);
	}


	for (double i = 0; i < 1; i+=0.1) {
		//b2P0[0] += i;
		b2P0[1] += 0.2;

		//b2P1[0] += i;
		b2P1[1] += 0.2;

		//b2P2[0] += i;
		b2P2[1] += 0.2;

		// b2P3[0] += i;
		b2P3[1] += 0.2;

		ActionBizie3(b2P0, b2P1, b2P2, b2P3, t_max);
	}


	if (FrogJump) {
		if (!frog_statr) {
			t_jump -= 0.1;
			if (t_jump <= 0) {
				frog_statr = true;
				FrogJump = false;
				t_jump = 0;
				ActionFrog(b3P0, b3P1, b3P2, b3P3, t_jump,true);
			}
			else {
				ActionFrog(b3P0, b3P1, b3P2, b3P3, t_jump,true);
			}
		}
		else {
			t_jump += 0.1;
			if (t_jump >= 1) {
				frog_statr = false;
				FrogJump = false;
				t_jump = 1;
				ActionFrog(b3P0, b3P1, b3P2, b3P3, t_jump,true);
			}
			else {
				ActionFrog(b3P0, b3P1, b3P2, b3P3, t_jump);
			}
			
		}
	}
	else {
		if (t_jump >= 1) {
			ActionFrog(b3P0, b3P1, b3P2, b3P3, t_jump,true);
		}
		else {
			ActionFrog(b3P0, b3P1, b3P2, b3P3, t_jump);
		}
		
	}


	
	glPushMatrix();

	glRotated(90, 1, 0, 0);
	glRotated(180, 0, 1, 0);
	DrawPlatform();
	glPopMatrix();



	glPushMatrix();
	glRotated(90, 0, 0, 1);
	glTranslated(0.2, -7, 0);
	DrawMountain(6, 3, 2.1);
	glPopMatrix();

	glPushMatrix();
	glRotated(90, 0, 0, 1);
	glTranslated(0, -7.5, 0);
	DrawMountain(7, 2, 3);
	glPopMatrix();

	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, texSakura);
	glTranslated(3, -5.5, 0.2);
	double p = 0.001f;
	glScalef(p, p, p);
	glRotated(90, 1, 0, 0);
	tree.DrawObj();
	glBindTexture(GL_TEXTURE_2D, 0);
	glPopMatrix();

	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, texSakura);
	glTranslated(3, 6, 0.2);
	double p1 = 0.0008f;
	glScalef(p1, p1, p1);
	glRotated(90, 1, 0, 0);
	tree.DrawObj();
	glBindTexture(GL_TEXTURE_2D, 0);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, 0);

	glPushMatrix();

	
	glPopMatrix();

	glMatrixMode(GL_PROJECTION);	//Делаем активной матрицу проекций. 
	                                //(всек матричные операции, будут ее видоизменять.)
	glPushMatrix();   //сохраняем текущую матрицу проецирования (которая описывает перспективную проекцию) в стек 				    
	glLoadIdentity();	  //Загружаем единичную матрицу
	glOrtho(0, ogl->getWidth(), 0, ogl->getHeight(), 0, 1);	 //врубаем режим ортогональной проекции

	glMatrixMode(GL_MODELVIEW);		//переключаемся на модел-вью матрицу
	glPushMatrix();			  //сохраняем текущую матрицу в стек (положение камеры, фактически)
	glLoadIdentity();		  //сбрасываем ее в дефолт

	glDisable(GL_LIGHTING);



	GuiTextRectangle rec;		   //классик моего авторства для удобной работы с рендером текста.
	rec.setSize(300, 150);
	rec.setPosition(10, ogl->getHeight() - 150 - 10);


	std::stringstream ss;
	ss << "T - вкл/выкл текстур" << std::endl;
	ss << "L - вкл/выкл освещение" << std::endl;
	ss << "F - Свет из камеры" << std::endl;
	ss << "G - двигать свет по горизонтали" << std::endl;
	ss << "Пробел - лягушка прыгнет" << std::endl;
	ss << "G+ЛКМ двигать свет по вертекали" << std::endl;
	ss << "Коорд. света: (" << light.pos.X() << ", " << light.pos.Y() << ", " << light.pos.Z() << ")" << std::endl;
	ss << "Коорд. камеры: (" << camera.pos.X() << ", " << camera.pos.Y() << ", " << camera.pos.Z() << ")" << std::endl;
	ss << "Параметры камеры: R="  << camera.camDist << ", fi1=" << camera.fi1 << ", fi2=" << camera.fi2 << std::endl;
	
	rec.setText(ss.str().c_str());
	rec.Draw();

	glMatrixMode(GL_PROJECTION);	  //восстанавливаем матрицы проекции и модел-вью обратьно из стека.
	glPopMatrix();


	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

}



double angle(double base[3], double point[3]) {
	return atan2(point[1] - base[1], point[0] - base[0]);
}

void CyrcleSort(double points[][3], double base_point[3], int n) {
	for (int i = 1; i < n; ++i) {
		int j = i;
		while (j > 0 && angle(base_point, points[j - 1]) > angle(base_point, points[j])) {
			std::swap(points[j], points[j - 1]);
			--j;
		}
	}
}

void bSort(double list[500][3], int listLength)
{
	while (listLength--)
	{
		bool swapped = false;

		for (int i = 0; i < listLength; i++)
		{

			if (list[i][1] < list[i + 1][1] && list[i][0] < list[i + 1][0])
			{
				std::swap(list[i], list[i + 1]);
				swapped = true;
			}
		}

		if (swapped == false)
			break;
	}
}