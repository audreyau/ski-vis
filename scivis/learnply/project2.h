#pragma once
#include "polyhedron.h"
#include "drawUtil.h"
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <iostream>
// glm!
#define GLM_FORCE_RADIANS
//#undef X
//#undef Y
//#undef Z
//#undef W

#include "glm/vec3.hpp"
#include "glm/common.hpp"
#include "glm/glm.hpp"

// Redefine the macros
//#define X 0
//#define Y 1
//#define Z 2
//#define W 3

enum CRIT_TYPE
{
    MINIMUM,
    MAXIMUM,
    SADDLE
};

enum COLOR_SCHEME
{
    SOLID,
    GRAYSCALE,
    BICOLOR,
    RAINBOW
};

struct CriticalPoint
{
    icVector3 loc;
    CRIT_TYPE type;
};

//ziv impl
struct ZPoint {
    glm::vec2 loc;
    double scalar;
    std::string getLine()
    {
        std::string r = "";
        std::ostringstream myString;
        myString <<  "x: " << loc.x <<  ", y: " << loc.y << ", scalar: " << scalar;
        return myString.str();
    }
};

//ziv impl
struct ZSegment {
    ZPoint point1; //lower x val
    ZPoint point2; //lower y val

    float gradient;

    float length;

    void print()
    {
        std::cout << "\t Segment point1 " << point1.getLine() << ", point2 " << point2.getLine() << ", gradient " << gradient << std::endl;
    }
};

// Piste Struct
struct Piste {

    std::string name;
    std::string difficulty;

    std::vector<ZSegment> segments;

    void print()
    {
        std::cout << "Piste " << name << " has following segments... " << std::endl;
        for (auto seg : segments)
        {
            seg.print();
        }

    }

};

struct CompareDistanceFunctor {
    icVector3 p1;
    CompareDistanceFunctor(const icVector3& point) : p1(point) {}

    bool operator()(const icVector3& a, const icVector3& b) const {
        double da = std::sqrt((a.x - p1.x) * (a.x - p1.x) + (a.y - p1.y) * (a.y - p1.y));
        double db = std::sqrt((b.x - p1.x) * (b.x - p1.x) + (b.y - p1.y) * (b.y - p1.y));
        return da < db;
    }
};


class ScalarTopology 
{
    private:
        Polyhedron* m_poly;
        double scalar_min;
        double scalar_max;

        std::unordered_map<Quad*, CriticalPoint> saddle_map;
        std::vector<CriticalPoint> criticals;
        std::vector<PolyLine> crit_contours;

        std::vector<bool> above_below;
        std::unordered_map<Edge*, icVector3> crossing_map;
        PolyLine user_contour;
        std::vector<PolyLine> spaced_contours;

    public:
        ScalarTopology(Polyhedron* poly);
        ~ScalarTopology();

        void calcUserContour(double s);
        void calcSpacedContours(int N);

        void drawCriticalPoints(bool use_height = false);
        void drawCritContours(bool use_height = false);
        void drawUserContours(bool use_height = false);
        void drawSpacedContours(bool use_height = false);

        void const printQuad(Quad* q);
        
        COLOR_SCHEME color_scheme = COLOR_SCHEME::SOLID;

        /* ADDED FOR FINAL PROJECT */
        double getScalarMin() const { return scalar_min; }
        double getScalarMax() const { return scalar_max; }
        std::vector<icVector3> calculateIntermediateScalars(const std::vector<icVector3>& pistePoints, Polyhedron* poly);
        // ziv implementation
        std::vector<ZSegment> zParsePisteAndFindCrossings(std::vector<ZPoint> zPistePoints, bool debug=false);
        double bilinearInterpolate(Quad* quad, float x0, float y0);
        // void getIntersectionEdge(Quad* quad, std::vector<Vertex*> verts, float yValueAtXEdge, bool p2XGrtrP1X, Vertex*& v1, Vertex*& v2);
        void getIntersectionEdge(Quad* quad, std::vector<Vertex*> verts, float yValueAtXEdge, bool p2XGrtrP1X, ZPoint p1, ZPoint p2, Vertex*& v1, Vertex*& v2);
        Quad* getQuadForPoint(ZPoint p);

        /* DECOMMISIONED */
        // std::vector<std::vector<ZPoint> >  drawDetailedPisteCoordinates(const std::vector<icVector3>& pistePoints);
        // std::vector<std::vector<ZPoint> >  parsePisteAndFindCrossings(const std::string& filename);
        // void printZSegmentValues (const std::vector<ZSegment>& skiSegments);
        // bool findLineSegmentIntersection(const icVector3& p1, const icVector3& p2, const icVector3& e1, const icVector3& e2, icVector3& intersection);

    private:
        void getQuadValues(Quad* quad, 
            double& x1, double& x2, 
            double& y1, double& y2, 
            double& f11, double& f12, double& f21, double& f22);
        void calcCriticals();
        PolyLine calcContour(double s);

        icVector3 getGrayscaleColor(double s);
        icVector3 getBiColor(double s);
        icVector3 getRainbowColor(double s);
        double getScaledHeight(double s);

};