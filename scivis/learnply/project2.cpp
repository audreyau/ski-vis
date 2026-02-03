#include "project2.h"
#include "project1.h"


#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "glError.h"
#include "GL/glew.h"
#include "GL/freeglut.h"

ScalarTopology::ScalarTopology(Polyhedron* poly) 
{
    m_poly = poly;

    above_below = std::vector<bool>(m_poly->nverts, false);

    findMm(m_poly, scalar_max, scalar_min);
    calcCriticals();
}

ScalarTopology::~ScalarTopology() 
{
}

void ScalarTopology::calcUserContour(double s) 
{   
	if (s <= scalar_min || s >= scalar_max) 
    {
        std::cout << "Scalar value out of range" << std::endl;
        return;
    }

    user_contour.clear();
    user_contour = calcContour(s);

    if (user_contour.empty())
        std::cout << "Contour is a single point." << std::endl;
    else
        std::cout << "Calculated contour at " << s << std::endl;
}

void ScalarTopology::calcSpacedContours(int N) 
{
    spaced_contours.clear();

    // divide the scalar range into N even intervals
    double step = (scalar_max - scalar_min) / (double)(N + 1);
    for (int i = 0; i < N; i++) 
    { 
        double s = scalar_min + (double)(i + 1) * step;
        spaced_contours.push_back(calcContour(s));
    }

	std::cout << "Calculated " << spaced_contours.size() << " contours." << std::endl;
}

void ScalarTopology::getQuadValues(Quad* quad, 
    double& x1, double& x2, 
    double& y1, double& y2, 
    double& f11, double& f12, double& f21, double& f22) 
{

    x1 = y1 = INFINITY;
	x2 = y2 = -INFINITY;
	for (int j = 0; j < 4; j++)
	{
        //quad->verts.size()
        //std::cout << "NULL VERTS QUAD " << j << " : " << (quad->verts[j] == nullptr) << std::endl;
        //std::cout << "QUAD " << j << " VALS : " << quad->verts[j]->x << ", " << quad->verts[j]->y << ", " << quad->verts[j]->scalar << std::endl;
		if (quad->verts[j]->x < x1)
			x1 = quad->verts[j]->x;
		if (quad->verts[j]->y < y1)
			y1 = quad->verts[j]->y;
		if (quad->verts[j]->x > x2)
			x2 = quad->verts[j]->x;
		if (quad->verts[j]->y > y2)
			y2 = quad->verts[j]->y;
	}
	for (int j = 0; j < 4; j++)
	{
		if (quad->verts[j]->x == x1 && quad->verts[j]->y == y1)
			f11 = quad->verts[j]->scalar;
		else if (quad->verts[j]->x == x1 && quad->verts[j]->y == y2)
			f12 = quad->verts[j]->scalar;
		else if (quad->verts[j]->x == x2 && quad->verts[j]->y == y1)
			f21 = quad->verts[j]->scalar;
		else
			f22 = quad->verts[j]->scalar;
	}

}

void ScalarTopology::calcCriticals() 
{
    // clear any saved criticals
    criticals.clear();
    saddle_map.clear();
    crit_contours.clear();

    // first loop through all the quads to find critical points
    // occuring in the interior of a quad
    for (int i = 0; i < m_poly->nquads; i++)
    {
        Quad* quad = m_poly->qlist[i];

        double x1, x2, y1, y2, f11, f12, f21, f22;
        getQuadValues(quad, x1, x2, y1, y2, f11, f12, f21, f22);

        // find coordinates of a potential critical point. (Lecture_08 slide 39)
        if (f11 - f21 - f12 + f22 == 0)
            continue;
        double x0, y0, f00;
        x0 = (x2 * f11 - x1 * f21 - x2 * f12 + x1 * f22) / (f11 - f21 - f12 + f22);
		y0 = (y2 * f11 - y2 * f21 - y1 * f12 + y1 * f22) / (f11 - f21 - f12 + f22);

        // check if the coordinates are outside the quad
        if (x0 < x1 || x0 > x2 || y0 < y1 || y0 > y2)
            continue;

        // use bilinear interpolation to get the scalar value at (x0, y0). (Lecture_08 slide 33)
        f00 = ((x2 - x0) * (y2 - y0) * f11 + (x0 - x1) * (y2 - y0) * f21 +
            (x2 - x0) * (y0 - y1) * f12 + (x0 - x1) * (y0 - y1) * f22) /
            ((x2 - x1) * (y2 - y1));

        // create a critical point
        CriticalPoint crit;
        crit.loc = icVector3(x0, y0, f00);

        // The Hessian matrix will help us classify the critical point (Lecture_08 slide 49)
        double b = (f11 - f21 - f12 + f22) / ((x2 - x1) * (y2 - y1));
        // The Hessian matrix has zeros on the diagonal, so the eigenvalues (given on Lecture_08 slide 50)
        // are -2b and +2b. We already know that (f11 - f21 - f12 + f22 != 0), so (b != 0).
        crit.type = CRIT_TYPE::SADDLE;

        criticals.push_back(crit);
        saddle_map.insert(std::make_pair(quad, crit));
    }

    // then loop through all the vertices to check for local mins and maxes
    for (int i = 0; i < m_poly->nverts; i++)
    {
        Vertex* vert = m_poly->vlist[i];
        bool min = true;
        bool max = true;

        // if a vertex has a scalar value less than or greater than
		// of its neighbors, it is a local min or max
		for (int j = 0; j < vert->nedges; j++)
		{
			Edge* edge = vert->edges[j];
			Vertex* other = (edge->verts[0] == vert) ? edge->verts[1] : edge->verts[0];
			if (vert->scalar <= other->scalar)
				max = false;
			else if (vert->scalar >= other->scalar)
				min = false;
		}

        if (!min && !max)
            continue;

        // create the new critical point
		CriticalPoint crit;
		crit.loc = icVector3(vert->x, vert->y, vert->scalar);
		if (min)
			crit.type = CRIT_TYPE::MINIMUM;
		else
			crit.type = CRIT_TYPE::MAXIMUM;

		criticals.push_back(crit);

        // calculate contours at each critical point
        for (const CriticalPoint& cp : criticals)
        {
            double s = cp.loc.z;
            crit_contours.push_back(calcContour(s));
        }

    }
}

PolyLine ScalarTopology::calcContour(double s) 
{   
    // loop through all vertices and classify them as above or below s
	for (int i = 0; i < m_poly->nverts; i++)
	{
		if (m_poly->vlist[i]->scalar < s)
			above_below[i] = false;
		else
			above_below[i] = true;
	}

    // loop through all edges to find crossing points
    crossing_map.clear();
    for (int i = 0; i < m_poly->nedges; i++)
    {
        Edge* edge = m_poly->elist[i];
        Vertex* v1 = edge->verts[0];
        Vertex* v2 = edge->verts[1];

        // if one endpoint is above s and one is below, we have a crossing
        if (above_below[v1->index] != above_below[v2->index])
        {
            // use linear interpolation to find the crossing point (Lecture_08 slide 51)
			double a = (s - v1->scalar) / (v2->scalar - v1->scalar);
			double cx = v1->x + a * (v2->x - v1->x);
			double cy = v1->y + a * (v2->y - v1->y);
			double cz = s;

            icVector3 cross = icVector3(cx, cy, cz);
            crossing_map.insert(std::make_pair(edge, cross));
        }
    }

    // loop through all the quads and connect crossing points
    PolyLine contour;
    for (int i = 0; i < m_poly->nquads; i++)
    {
        Quad* quad = m_poly->qlist[i];
        std::vector<icVector3> cross_points;

        // get saved crossing points
		for (int j = 0; j < 4; j++)
		{
			Edge* edge = quad->edges[j];
			if (crossing_map.find(edge) != crossing_map.end())
				cross_points.push_back(crossing_map.at(edge));
		}

        // If there are only two crossing points, connect them with a line segment.
		// If there are 4 crossing points, we need to be more careful.
		if (cross_points.size() == 2)
		{
			LineSegment ls(cross_points[0], cross_points[1]);
			contour.push_back(ls);
		}
        else if (cross_points.size() == 4)
        {
            // if the quad contains a saddle point at the same height as the contour,
			// connect each crossing point to the saddle. Otherwise, we try connecting each pair of
			// crossing points and see which combinations give us line segment midpoints closest to the contour
			bool saddle_on_contour = false;
			CriticalPoint* saddle = NULL;
			if (saddle_map.find(quad) != saddle_map.end())
			{
				saddle = &saddle_map.at(quad);
				if (saddle->loc.z == s)
					saddle_on_contour = true;
			}

            if (saddle_on_contour)
			{
				icVector3 sloc = saddle_map.at(quad).loc;
				contour.push_back(LineSegment(cross_points[0], sloc));
				contour.push_back(LineSegment(cross_points[1], sloc));
				contour.push_back(LineSegment(cross_points[2], sloc));
				contour.push_back(LineSegment(cross_points[3], sloc));
			}
            else
            {
                // all possible line segments between adjacent crossing points
				LineSegment ls01(cross_points[0], cross_points[1]);
				LineSegment ls23(cross_points[2], cross_points[3]);
				LineSegment ls12(cross_points[1], cross_points[2]);
				LineSegment ls30(cross_points[3], cross_points[0]);

                // line segment candidate midpoints
                icVector3 m01 = ls01.midpoint();
                icVector3 m30 = ls30.midpoint();

                // use bilinear interpolation to get the scalar values at the midpoints
				double x1, x2, y1, y2, f11, f12, f21, f22;
				getQuadValues(quad, x1, x2, y1, y2, f11, f12, f21, f22);
				double s01 = ((x2 - m01.x) * (y2 - m01.y) * f11 + (m01.x - x1) * (y2 - m01.y) * f21 +
							  (x2 - m01.x) * (m01.y - y1) * f12 + (m01.x - x1) * (m01.y - y1) * f22) /
							 ((x2 - x1) * (y2 - y1));
				double s30 = ((x2 - m30.x) * (y2 - m30.y) * f11 + (m30.x - x1) * (y2 - m30.y) * f21 +
							  (x2 - m30.x) * (m30.y - y1) * f12 + (m30.x - x1) * (m30.y - y1) * f22) /
							 ((x2 - x1) * (y2 - y1));

                // if we have a saddle, check which sides of the saddle the contour passes through
				if (saddle)
				{
					double fs = saddle->loc.z;
					if ((s > fs && s01 > fs) || (s < fs && s01 < fs))
					{
						contour.push_back(ls01);
						contour.push_back(ls23);
					}
					else
					{
						contour.push_back(ls12);
						contour.push_back(ls30);
					}
				}
                // if there is no saddle, check which midpoints are closer to the contour values
                else
                {
                    if (abs(s - s01) < abs(s - s30))
                    {
                        contour.push_back(ls01);
                        contour.push_back(ls23);
                    }
                    else
                    {
                        contour.push_back(ls12);
                        contour.push_back(ls30);
                    }
                }
            }
        }
    }

    return contour;
}

void ScalarTopology::drawCriticalPoints(bool use_height) 
{
    for (const CriticalPoint &cp : criticals)
    {
        icVector3 loc = cp.loc;
        if (use_height)
            loc.z = getScaledHeight(loc.z);
        else
            loc.z = 0.0;

       	switch (cp.type)
		{
		case CRIT_TYPE::MAXIMUM:
			drawSphere(loc.x, loc.y, loc.z, 0.1, 1.0, 0.0, 0.0);
			break;
		case CRIT_TYPE::MINIMUM:
			drawSphere(loc.x, loc.y, loc.z, 0.1, 0.0, 0.0, 1.0);
			break;
		case CRIT_TYPE::SADDLE:
			drawSphere(loc.x, loc.y, loc.z, 0.1, 1.0, 0.0, 1.0);
			break;
		default:
			break;
		}   
    }
}

void ScalarTopology::drawCritContours(bool use_height) 
{
    if (crit_contours.empty())
        return;

    for (int i = 0; i < criticals.size(); i++)
	{
		CRIT_TYPE type = criticals[i].type;
		double s = criticals[i].loc.z;
		PolyLine contour = crit_contours[i];

		if (contour.empty())
			continue;

		if (use_height)
			s = getScaledHeight(s);
		else
			s = 0.0;
		for (int i = 0; i < contour.size(); i++)
		{
			contour.at(i).start.z = s;
			contour.at(i).end.z = s;
		}

		switch (type)
		{
		case CRIT_TYPE::MAXIMUM:
			//drawPolyLine(contour, 1.5, 1.0, 0.0, 0.0);
			break;
		case CRIT_TYPE::MINIMUM:
			//drawPolyLine(contour, 1.5, 0.0, 0.0, 1.0);
			break;
		case CRIT_TYPE::SADDLE:
			// drawPolyLine(contour, 1.5, 0.4, 0.4, 0.4);
			drawPolyLine(contour, 0.5, 0.6, 0.6, 0.6);	// width and color of the contours
			break;
		default:
			break;
		}
	}
}

void ScalarTopology::drawUserContours(bool use_height) 
{
    if (user_contour.empty())
		return;

	PolyLine contour = user_contour;
	double s = contour[0].start.z;
	
	if (use_height)
		s = getScaledHeight(s);
	else
		s = 0.0;
	for (int i = 0; i < contour.size(); i++)
	{
		contour.at(i).start.z = s;
		contour.at(i).end.z = s;
	}

	drawPolyLine(contour, 2.0, 1.0, 1.0, 1.0);
}

void ScalarTopology::drawSpacedContours(bool use_height) 
{
    if (spaced_contours.empty())
		return;

	icVector3 color (0.4, 0.4, 0.4);
	for (PolyLine contour : spaced_contours)
	{
		if (contour.empty())
			continue;

		double s = contour[0].start.z;
		if (color_scheme == COLOR_SCHEME::GRAYSCALE)
			color = getGrayscaleColor(s);
		else if (color_scheme == COLOR_SCHEME::BICOLOR)
			color = getBiColor(s);
		else if (color_scheme == COLOR_SCHEME::RAINBOW)
			color = getRainbowColor(s);

		if (use_height)
			s = getScaledHeight(s);
		else
			s = 0.0;
		for (int i = 0; i < contour.size(); i++)
		{
			contour.at(i).start.z = s;
			contour.at(i).end.z = s;
		}

		drawPolyLine(contour, 1.0, color.x, color.y, color.z);
		
	}
}

icVector3 ScalarTopology::getGrayscaleColor(double s)
{
    double gray = (s - scalar_min) / (scalar_max - scalar_min);
    return icVector3(gray, gray, gray);
}

icVector3 ScalarTopology::getBiColor(double s)
{
    icVector3 c1(0.0, 0.0, 1.0);
	icVector3 c2(0.0, 1.0, 0.0);

	double left = (s - scalar_min) / (scalar_max - scalar_min);
	double right = (scalar_max - s) / (scalar_max - scalar_min);

	return c1 * left + c2 * right;
}

icVector3 ScalarTopology::getRainbowColor(double s)
{
    icVector3 c1(0.0, 0.0, 1.0);
	icVector3 c2(1.0, 0.0, 0.0);
	icVector3 HSVc1, HSVc2;
	RGBtoHSV(HSVc1, c1);
	RGBtoHSV(HSVc2, c2);

	double left = (s - scalar_min) / (scalar_max - scalar_min);
	double right = (scalar_max - s) / (scalar_max - scalar_min);

	icVector3 HSVc = HSVc1 * left + HSVc2 * right;
	icVector3 c;
	HSVtoRGB(HSVc, c);

	return c;
}

double ScalarTopology::getScaledHeight(double s)
{
	double new_s = (s - scalar_min);	// changed from 	double new_s = (s - scalar_min) / (scalar_max - scalar_min);
	return new_s;						// changed from 	return new_s * 10;
}

/********* FINAL PROJECT FUNCTIONS *********************************************************************************************************************/

std::vector<Vertex*> vecFromQuad(Quad* q) {

    std::vector<Vertex*> newVerts;

    for (auto v : q->verts)
    {
        newVerts.push_back(v);
    }
    return newVerts;

}

// based on project 2 code
double ScalarTopology::bilinearInterpolate(Quad* quad, float x0, float y0)
{

    double x1, x2, y1, y2, f11, f12, f21, f22;
    ScalarTopology::getQuadValues(quad, x1, x2, y1, y2, f11, f12, f21, f22);
    //std::cout << "retrieved quad vals" << std::endl;
    // use bilinear interpolation to get the scalar value at (x0, y0). (Lecture_08 slide 33)
    double f00 = ((x2 - x0) * (y2 - y0) * f11 + (x0 - x1) * (y2 - y0) * f21 +
        (x2 - x0) * (y0 - y1) * f12 + (x0 - x1) * (y0 - y1) * f22) /
        ((x2 - x1) * (y2 - y1));

    return f00;

}

Quad* ScalarTopology::getQuadForPoint(ZPoint p)
{

    std::vector<Vertex*> nVerts;

    for (int i = 0; i < m_poly->nquads; i++)
    {
        Quad* quad = m_poly->qlist[i];
        nVerts = vecFromQuad(quad);

        std::sort(nVerts.begin(), nVerts.end(), [](const Vertex* a, const  Vertex* b) {
            return a->x < b->x;
            });

        if (nVerts[0]->x <= p.loc.x && nVerts[3]->x >= p.loc.x)
        {
            std::sort(nVerts.begin(), nVerts.end(), [](const Vertex* a, const  Vertex* b) {
                return a->y < b->y;
            });


            if (nVerts[0]->y <= p.loc.y && nVerts[3]->y >= p.loc.y)
            {

                return quad;
            }

        }

    }

    return nullptr;


}

/*

When we calculate the y value at the right hand side
vertices, does the y value
1. Go through the top edge
--------/-x
       /  |
      /   |  
     /    |
    X     |
          |
----------x

2. Go through the side edge
---------x
         /
        /|
       / |
      X  |
         |
---------x

3. Go through the bottom edge
---------x
         |
         |
         |
   X     |
    \    |
-----\---x


verts must be sorted by low to high X value!
Last 2 args are vertices to be populated for appropriate edge
*/
// void ScalarTopology::getIntersectionEdge(Quad* quad, std::vector<Vertex*> verts, float yValueAtXEdge, bool p2XGrtrP1X, Vertex*& v1, Vertex*& v2)
void ScalarTopology::getIntersectionEdge(Quad* quad, std::vector<Vertex*> verts, float yValueAtXEdge, bool p2XGrtrP1X, ZPoint p1, ZPoint p2, Vertex*& v1, Vertex*& v2)
{

    Vertex* highYV;
    Vertex* lowYV;
    if (p2XGrtrP1X)
    {
        if (verts[2]->y > verts[3]->y)
        {
            highYV = verts[2];
            lowYV = verts[3];
        }
        else {
            highYV = verts[3];
            lowYV = verts[2];
        }
    }
    else {
        if (verts[0]->y > verts[1]->y)
        {
            highYV = verts[0];
            lowYV = verts[1];
        }
        else {
            highYV = verts[1];
            lowYV = verts[0];
        }
    }

    // handle same x val case



    if (p1.loc.x == p2.loc.x)
    {
        if (p1.loc.y < p2.loc.y)
        {
            v1 = highYV;

            if (verts[0]->y >= highYV->y)
            {
                v2 = verts[0];
            }

            else
            {
                v2 = verts[1];
            }
            return;
        }

        else {
            v1 = lowYV;

            if (verts[0]->y <= lowYV->y)
            {
                v2 = verts[0];
            }
            else
            {
                v2 = verts[1];
            }

            return;
        }
    }

    if (yValueAtXEdge > highYV->y)
    {
        // get top edge;

        v1 = highYV;
        if (p2XGrtrP1X) {
            if (verts[0]->y >= highYV->y)
            {
                v2 = verts[0];
            }
            else
            {
                v2 = verts[1];
            }
        }
        else {
            if (verts[2]->y >= highYV->y)
            {
                v2 = verts[2];
            }
            else
            {
                v2 = verts[3];
            }
        }


    }
    else if ( yValueAtXEdge <= highYV->y  && yValueAtXEdge > lowYV->y )
    {
        // side edge
        
        v1 = highYV;
        v2 = lowYV;

    }
    else {
        bool lessThan = yValueAtXEdge <= lowYV->y;

        v1 = lowYV;
        if (p2XGrtrP1X) {
            if (verts[0]->y <= lowYV->y)
            {
                v2 = verts[0];
            }
            else
            {
                v2 = verts[1];
            }
        }
        else {
            if (verts[2]->y <= lowYV->y)
            {
                v2 = verts[2];
            }
            else
            {
                v2 = verts[3];
            }
        }


    }



}

void const ScalarTopology::printQuad(Quad* q)  {
    std::cout << "Printing Quad Index: " << q->index << std::endl;
    for (int i = 0; i < 4; i++)
    {

        std::cout << "\t x: " << q->verts[i]->x << ", y: " << q->verts[i]->y << ", scalr: " << q->verts[i]->scalar << std::endl;

    }

}

// ziv's implementation
std::vector<ZSegment> ScalarTopology::zParsePisteAndFindCrossings(std::vector<ZPoint> zPistePoints, bool debug) {

    // REMOVING BECAUSE THIS MESSES UP THE VIZ!
    // points go from lower x to higher x!
    
    //std::sort(zPistePoints.begin(), zPistePoints.end(), [](const ZPoint& a, const ZPoint& b) {
    //    return a.loc.x < b.loc.x;
    //    });

    std::vector<ZSegment> segments = {};

    for (size_t i = 0; i < zPistePoints.size() - 1; i++) 
    {
        ZPoint p1 = zPistePoints[i];
        ZPoint p2 = zPistePoints[i + 1];

        float slope = (p2.loc.y - p1.loc.y) / (p2.loc.x - p1.loc.x);

        // what if Y vals are the same -> Inf slope
        bool sameYPistes = p2.loc.x == p2.loc.x;

        // is x val of p2 > x val p1?
        bool p2XGrtrP1X = p2.loc.x > p1.loc.x;

        
        if (debug)
        {
            std::cout << "FIRST PISTE PT: (" << p1.loc.x << ", " << p1.loc.y << ")" << std::endl;
            std::cout << "SECOND PISTE PT: (" << p2.loc.x << ", " << p2.loc.y << ")" << std::endl;
            std::cout << "Slope b/w : " << slope << std::endl;
        }
        Quad* currQuad = getQuadForPoint(p1);
        if (debug)
        {
            std::cout << "QUAD FOR FIRST PISTE PT IS NULL? " << (currQuad == nullptr) << std::endl;
            if (currQuad != nullptr)
                printQuad(currQuad);
        }

        if (currQuad == nullptr)
        {
            std::cout << "Could not find Quad for Piste Point #" << i << " (" << p1.loc.x << ", " << p1.loc.y << ")";
           // continue;
            return segments;
        }

        Quad* endQuad = getQuadForPoint(p2);
        if (debug)
        {
            std::cout << "QUAD FOR SECOND PISTE PT IS NULL? " << (endQuad == nullptr) << std::endl;

        }

        if (endQuad == nullptr)
        {
            std::cout << "Could not find Quad for Piste Point #" << i + 1 << " (" << p2.loc.x << ", " << p2.loc.y << ")";
            //continue;
            return segments;
        }


        std::vector<Vertex*> nVerts;
    

        double scalarAtP1 = bilinearInterpolate(currQuad, p1.loc.x, p1.loc.y);
        p1.scalar = scalarAtP1;
        // are p1 and p2 points in the same quad?
        if (currQuad == endQuad)
        {
            if (debug)
            {
                std::cout << "P1 and P2 points are in the same quad" << std::endl;

            }

            ZSegment seg;

            double scalarAtP2 = bilinearInterpolate(currQuad, p2.loc.x, p2.loc.y);
            p2.scalar = scalarAtP2;

            seg.point1 = p1;
            seg.point2 = p2;

            float len = glm::distance(seg.point1.loc, seg.point2.loc);
            float gradient = (seg.point2.scalar - seg.point1.scalar) / len;

            seg.gradient = gradient;

            segments.push_back(seg);

            // get next points
            continue;

        }



        if (debug)
        {
            std::cout << "SCALAR AT p1:  " << scalarAtP1 << std::endl;

        }
        
        bool reachedP2 = false;

        ZPoint currPoint = p1;

        while (!reachedP2)
        {

            nVerts = vecFromQuad(currQuad);

            // sort by x value
            std::sort(nVerts.begin(), nVerts.end(), [](const Vertex* a, const  Vertex* b) {
                return a->x < b->x;
            });

            float xDiff;
            if (p2XGrtrP1X)
            {
                xDiff = (nVerts[3]->x - currPoint.loc.x);
            }
            else
            {
                xDiff = (nVerts[0]->x - currPoint.loc.x);
            }

            // what's the value at the x extreme of this quad
            float yValueAtXEdge = (xDiff * slope) + currPoint.loc.y;

            
            if (debug)
            {
                if (p2XGrtrP1X)
                {
                    std::cout << "Vertex 3 (" << nVerts[3]->x << ", " << nVerts[3]->y << ")" << std::endl;
                }
                else
                {
                    std::cout << "Vertex 0 (" << nVerts[0]->x << ", " << nVerts[0]->y << ")" << std::endl;
                }

                std::cout << "XDIFF to " << xDiff << std::endl;
                std::cout << "Y VAl at edge: " << yValueAtXEdge << std::endl;

            }
            Vertex* v1;
            Vertex* v2;

            // getIntersectionEdge(currQuad, nVerts, yValueAtXEdge, p2XGrtrP1X, v1, v2);
            getIntersectionEdge(currQuad, nVerts, yValueAtXEdge, p2XGrtrP1X,p1,p2, v1, v2);

            if (debug)
            {
                std::cout << "V1 PTR: (" << v1->x << ", " << v1->y << ")" << std::endl;
                std::cout << "V2 PTR: (" << v2->x << ", " << v2->y << ")" << std::endl;
            }
            float intersectionX, intersectionY;

            if (v1->y == v2->y)
            {
                intersectionY = v1->y;

                float xInterval = (intersectionY - p1.loc.y) / slope;
                intersectionX = p1.loc.x + xInterval;

            }
            else
            {

                intersectionX = v1->x;
                intersectionY = yValueAtXEdge;

            }

            ZPoint newIntersectionPoint;
            newIntersectionPoint.loc = glm::vec2(intersectionX, intersectionY);

            if (debug)
            {
                std::cout << "INTERSECTION POINT: (" << intersectionX << ", " << intersectionY << ")" << std::endl;
            }
            double scalarAtIntersection = bilinearInterpolate(currQuad, intersectionX, intersectionY);
            newIntersectionPoint.scalar = scalarAtIntersection;

            if (debug)
            {
                std::cout << "SCALAR AT INTERSECTION: " << scalarAtIntersection << std::endl;
            }
            ZSegment seg;

            seg.point1 = currPoint;
            seg.point2 = newIntersectionPoint;

            float len = glm::distance(seg.point1.loc, seg.point2.loc);
            float gradient = (seg.point2.scalar - seg.point1.scalar) / len;

            seg.gradient = gradient;

            segments.push_back(seg);

            currQuad = m_poly->find_common_edge(currQuad, v1, v2);

            // we have reached the edge of the mesh even though the piste doesn't end
            if (currQuad == nullptr)
            {
                std::cout << "COULD NOT FIND COMMON EDGE FOR VERTICES v1:  (" << v1->x << ", " << v1->y << "), (" << v2->x << ", " << v2->y << ")" << std::endl;
                reachedP2 = true;
                return segments;

            }
            else if (debug)
            {


            }

            currPoint = newIntersectionPoint;

            // is it p2 quad?
            if (currQuad == endQuad)
            {
                if (debug)
                {
                    std::cout << "P2 QUAD ENCOUNTERED" << std::endl;
                }
                reachedP2 = true;
                double scalarAtP2 = bilinearInterpolate(currQuad, p2.loc.x, p2.loc.y);
                p2.scalar = scalarAtP2;
                ZSegment lastSeg;
                lastSeg.point1 = newIntersectionPoint;
                lastSeg.point2 = p2;

                float lastGradient = (lastSeg.point2.scalar - lastSeg.point1.scalar) / glm::distance(lastSeg.point1.loc, lastSeg.point2.loc);

                lastSeg.gradient = lastGradient;

                segments.push_back(lastSeg);
            }
            else {

                printQuad(currQuad);

            }
        }
    }

    return segments;
}