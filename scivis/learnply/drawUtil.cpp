#include "drawUtil.h"
#include "glError.h"
#include "GL/freeglut.h"

// A polyline is a list of (connected) line segments
typedef std::vector<LineSegment> PolyLine;


// Draws a single line segment
// width: width of the line segment
// R: red channel for the line color [0,1]
// B: blue channel for the line color [0,1]
// G: green channel of the line color [0,1]
void drawLineSegment(LineSegment ls, double width, float R, float G, float B)
{
	glDisable(GL_LIGHTING);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glLineWidth(width);

	glBegin(GL_LINES);
	glColor3f(R, G, B);
	glVertex3f(ls.start.x, ls.start.y, ls.start.z);
	glVertex3f(ls.end.x, ls.end.y, ls.end.z);
	glEnd();
}

// Draws a polyline assuming the end point and start point of consecutive segments is the same
// width: width of the line
// R: red channel for the line color [0,1]
// B: blue channel for the line color [0,1]
// G: green channel of the line color [0,1]
void drawPolyLine(PolyLine pl, double width, float R, float G, float B)
{
	glDisable(GL_LIGHTING);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glLineWidth(width);

	glBegin(GL_LINES);
	glColor3f(R, G, B);
	for (LineSegment const &ls : pl)
	{
		glVertex3f(ls.start.x, ls.start.y, ls.start.z);
		glVertex3f(ls.end.x, ls.end.y, ls.end.z);
	}
	glEnd();
}


// Draws a sphere centered on the given coordinates
// x, y, z are the center coordinates
// radius: radius of the sphere
// R: red channel for the dot color [0,1]
// B: blue channel for the dot color [0,1]
// G: green channel of the dot color [0,1]
void drawSphere(double x, double y, double z, double radius, float R, float G, float B)
{
	glDisable(GL_POLYGON_OFFSET_FILL);
	glEnable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glMatrixMode(GL_MODELVIEW);

	CHECK_GL_ERROR();

	GLfloat mat_diffuse[4];
	mat_diffuse[0] = R;
	mat_diffuse[1] = G;
	mat_diffuse[2] = B;
	mat_diffuse[3] = 1.0;

	CHECK_GL_ERROR();

	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);

	CHECK_GL_ERROR();

	GLUquadric* quadric = gluNewQuadric();
	glPushMatrix();

	glTranslated(x, y, z);

	glColor3f(R, G, B);
	gluSphere(quadric, radius, 16, 16);
	glPopMatrix();
	gluDeleteQuadric(quadric);
}


void dots_and_lines_example()
{
	// make polylines for linear, quadratic, and cubic functions
	PolyLine linear, quadratic, cubic;
	for (int x = -10; x < 10; x++)
	{
		double y_linear = (double)x;
		double y_quadratic = (double)x * (double)x / 10.0;
		double y_cubic = (double)x * (double)x * (double)x / 100.0;

		double x1 = x + 1;
		double y1_linear = (double)x1;
		double y1_quadratic = (double)x1 * (double)x1 / 10.0;
		double y1_cubic = (double)x1 * (double)x1 * (double)x1 / 100.0;

		LineSegment linear_seg = LineSegment(x, y_linear, 0, x1, y1_linear, 0);
		LineSegment quadratic_seg = LineSegment(x, y_quadratic, 0, x1, y1_quadratic, 0);
		LineSegment cubic_seg = LineSegment(x, y_cubic, 0, x1, y1_cubic, 0);

		linear.push_back(linear_seg);
		quadratic.push_back(quadratic_seg);
		cubic.push_back(cubic_seg);
	}

	drawPolyLine(linear);
	drawPolyLine(quadratic);
	drawPolyLine(cubic);

	// make dots along x and y axes
	for (int i = -10; i <= 10; i++)
	{
		drawSphere(i, 0, 0);
		drawSphere(0, i, 0);
	}
}