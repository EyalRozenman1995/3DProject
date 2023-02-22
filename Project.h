#pragma once
#include <cmath>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include "resource.h"

using namespace std;

// forward declartaion
array<uchar, 3> getColorFromMap(int X, int Y);

struct vertex
{
	GLfloat x, y, z;
	vertex(GLfloat x, GLfloat y, GLfloat z) : x(x), y(y), z(z) {}
};

struct position
{
	int row, col;
	position(int row, int col) : row(row), col(col) {}
};

struct color
{
	GLfloat R, G, B;
	color(GLfloat r, GLfloat g, GLfloat b) : R(r), G(g), B(b) {}
};

struct Triangle
{
	position pos;
	int id;
	vertex a, b, c;
	bool paintByHeight = true;
	color specificColor = color(0, 0, 0);

	Triangle(position pos, int id, vertex a, vertex b, vertex c)
		: pos(pos), id(id), a(a), b(b), c(c) {};

	void render()
	{
		/*
			simply uses openGL functrions to render the triangle defined by 3 points
		*/

		glBegin(GL_TRIANGLES);
		setColorByHeight(a);
		glVertex3f(a.x, a.y, a.z);

		setColorByHeight(b);
		glVertex3f(b.x, b.y, b.z);

		setColorByHeight(c);
		glVertex3f(c.x, c.y, c.z);
		glEnd();

		// draw black outline
		glBegin(GL_LINE_LOOP);
		glColor4f(0.0, 0.0, 0.0, 0.5);
		glVertex3f(a.x, a.y, a.z);
		glVertex3f(b.x, b.y, b.z);
		glVertex3f(c.x, c.y, c.z);
		glEnd();

	}

	void pickingColor()
	{
		/*
			taken from openGL docmentation
			icreats a unique color for the triangle
			the color is a function of the triangle id
		*/
		int R = (id & 0x000000FF) >> 0;
		int G = (id & 0x0000FF00) >> 8;
		int B = (id & 0x00FF0000) >> 16;

		glColor3ub(R, G, B);

		// render the triangle with its unique color
		glBegin(GL_TRIANGLES);
		glVertex3f(a.x, a.y, a.z);
		glVertex3f(b.x, b.y, b.z);
		glVertex3f(c.x, c.y, c.z);
		glEnd();
	}

	static int getTriangleID(int r, int g, int b)
	{
		/*
			get id from unique color
		*/
		int i = (r << 0) | (g << 8) | (b << 16);
		return i;
	}

	void setColorByHeight(vertex v)
	{
		/*
			take the olor in the inmput vertex from its position in the map
			and set the next render color to it
		*/

		if (!paintByHeight)
		{
			glColor3f(specificColor.R, specificColor.G, specificColor.B);
			return;
		}

		array<uchar, 3> realRGBValues = getColorFromMap(v.x, v.z);
		glColor3ub(realRGBValues[0], realRGBValues[1], realRGBValues[2]);
	}

	void changeColor(GLfloat R, GLfloat G, GLfloat B)
	{
		/*
			allow choosing non default color (used in picking)
		*/
		paintByHeight = false;
		specificColor.R = R;
		specificColor.G = G;
		specificColor.B = B;
	}

};

struct Raindrop
{
	float spawnY;
	float x, y, z;
	float speedFactor;
	float gravityAcceleration;
	float velocity;

	static array<float, 3> RandomRainColor() {
		float r = rand() / (float)RAND_MAX;
		float g = rand() / (float)RAND_MAX;
		float b = rand() / (float)RAND_MAX;
		// RGB values are random weighted to generate more gray-blue colors
		// so we only get reasonable rain drops color
		return { float(r * 0.2 + 0.3), float(g * 0.2 + 0.5), float(b * 0.2 + 0.8) };
	}
};

