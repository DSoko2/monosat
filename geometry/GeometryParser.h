/*
 * graph_parser.h
 *
 *  Created on: 2013-06-08
 *      Author: sam
 */

#ifndef GEOMETRY_PARSER_H_
#define GEOMETRY_PARSER_H_


#include <stdio.h>
#include <gmpxx.h>
#include "utils/ParseUtils.h"
#include "graph/GraphParser.h"
#include "geometry/GeometryTypes.h"
#include "core/SolverTypes.h"
#include "geometry/GeometryTheory.h"
#include "core/Dimacs.h"
#include "core/Config.h"
#include "mtl/Vec.h"
#include <vector>

namespace Minisat {


// GEOMETRY Parser:
template<class B, class Solver, class T=double>
class GeometryParser:public Parser<B,Solver>{
	//vec<GeometryTheorySolver<1,mpq_class>*> space_1D;
	GeometryTheorySolver<2,T> *space_2D=nullptr;
	//vec<GeometryTheorySolver<3,double>*> space_3D;
	vec<char> tmp_str;
	struct ParsePoint{
		Var var;
		vec<T> position;
	};

	vec<vec<ParsePoint>> pointsets;

	struct ConvexHullArea{
		int pointsetID;
		T area;
		Var v;
	};

	vec<ConvexHullArea> convex_hull_areas;

/*
	struct ConvexHullPointContained{
		int pointsetID;
		ParsePoint point;

	};

	vec<ConvexHullPointContained> convex_hull_point_containments;
*/

/*	struct ConvexHullLineIntersection{
		int pointsetID;
		Var var;
		ParsePoint p;
		ParsePoint q;
	};

	vec<ConvexHullLineIntersection> convex_hull_line_intersections;*/

	struct ConvexHullPolygonIntersection{
		int pointsetID;
		bool inclusive;
		Var var;
		vec<ParsePoint> points;
	};

	vec<ConvexHullPolygonIntersection> convex_hull_polygon_intersections;

	struct ConvexHullPoint{
		int pointsetID;
		Var pointVar;
		Var pointOnHull;
	};
	vec<ConvexHullPoint> convex_hull_points;

	struct ConvexHullsIntersection{
		int pointsetID1;
		int pointsetID2;
		Var var;
		bool inclusive;
	};
	vec<ConvexHullsIntersection> convex_hulls_intersect;

	struct PolygonInfo{
		int id;
		int dimension;
	};

	vec<PolygonInfo> polygons;

	struct ConstantPolygon{
		int pointsetID;
		vec<ParsePoint> points;
	};

	vec<ConstantPolygon> constant_polygons;

	struct ConstantRectangle{
		//An axis alligned rectangle defined by two points (top left, bottom right)
		int polygonID;

		ParsePoint a;
		ParsePoint b;
	};

	vec<ConstantRectangle> constant_rectangles;

	struct PolygonOperation{
		int pointsetID;
		int pointsetID1;
		int pointsetID2;

		PolygonOperationType operation;
	};
	vec<PolygonOperation> polygon_operations;
	struct PolygonIntersection{

		int polygonID1;
		int polygonID2;
		bool inclusive;
		Var v;
	};
	vec<PolygonIntersection> polygon_intersections;

	struct PolygonAreaLT{

		int polygonID1;
		bool inclusive;
		Var v;
	};


	vec<PolygonAreaLT> polygon_area_lt;

	struct ConditionalPolygon{
		int pointsetID;
		int pointsetID1;
		int pointsetID2;
		Lit conditional;
	};

	vec<ConditionalPolygon> conditional_polygons;

	void parsePoint(B &in, int d, ParsePoint & point){
		//for now, points are given as 32-bit integers. This is less convenient than, say, floats, but makes parsing much easier.
		//in the future, we could also support inputting arbitrary precision integers... but for now, we aren't going to.

		//is there a better way to read in an arbitrary-sized vector of points?
		for(int i = 0;i<d;i++){

			int n = parseInt(in);
			//double p = parseDouble(in,tmp_str);
			point.position.push((T)n);
		}
	}


	void readPoint(B& in, Solver& S ) {

		int pointsetID = parseInt(in);

		int v = parseInt(in);
		int d = parseInt(in);
		pointsets.growTo(pointsetID+1);
		pointsets[pointsetID].push();
		//stringstream ss(in);
		ParsePoint & point = pointsets[pointsetID].last();
		parsePoint(in,d,point);

		point.var = v-1;
		while ( point.var >= S.nVars()) S.newVar();
	}

void readConvexHullArea(B& in, Solver& S) {

    //hull_area_lt pointsetID area var
	int pointset = parseInt(in); //ID of the hull
	int var = parseInt(in)-1;
	stringstream ss(in);
	T area;
	ss>>area;
	//T area =  parseDouble(in,tmp_str);
	convex_hull_areas.push({pointset,area,var});
}
void readConvexHullIntersectsPolygon(B& in, Solver& S, bool inclusive){
	//convex_hull_intersects_polygon pointsetID var NumPoints D p1 p2 ... pD q1 q2 ... qD ...

	convex_hull_polygon_intersections.push();
	convex_hull_polygon_intersections.last().inclusive=inclusive;

	int pointsetID = parseInt(in);
	int v = parseInt(in)-1;
	convex_hull_polygon_intersections.last().pointsetID = pointsetID;
	convex_hull_polygon_intersections.last().var = v;
	int numPoints = parseInt(in);

	int d = parseInt(in);
	for(int i = 0;i<numPoints;i++){
		convex_hull_polygon_intersections.last().points.push();
		ParsePoint & p = convex_hull_polygon_intersections.last().points.last();
		parsePoint(in,d,p);
	}
}
/*void readConvexHullIntersectsLine(B& in, Solver& S){
	//convex_hull_intersects_line pointsetID var D p1 p2 ... qD q1 q2 ... qD

	convex_hull_line_intersections.push();
	ParsePoint & p = convex_hull_line_intersections.last().p;
	ParsePoint & q = convex_hull_line_intersections.last().q;
	int pointsetID = parseInt(in);
	int v = parseInt(in)-1;
	convex_hull_line_intersections.last().pointsetID = pointsetID;
	convex_hull_line_intersections.last().var = v;
	int d = parseInt(in);
	parsePoint(in,d,p);
	parsePoint(in,d,q);

	//stringstream ss(in);
	for(int i = 0;i<d;i++){
		//skipWhitespace(in);
		long n = parseInt(in);
		//double p = parseDouble(in,tmp_str);
		point.position.push((T)n);
	}
	for(int i = 0;i<d;i++){

		ss>>p;
		point.position.push(p);
	}



}*/
/*
void readConvexHullPointContained(B& in, Solver& S) {

    //hull_point_contained pointsetID var D p1 p2 ... pD

    convex_hull_point_containments.push();
    ParsePoint & point = convex_hull_point_containments.last().point;
	int pointsetID = parseInt(in);
	int v = parseInt(in)-1;
	int d = parseInt(in);
	parsePoint(in,d,point);

	//stringstream ss(in);
	for(int i = 0;i<d;i++){
		//skipWhitespace(in);
		long n = parseInt(in);
		//double p = parseDouble(in,tmp_str);
		point.position.push((T)n);
	}
	for(int i = 0;i<d;i++){

		ss>>p;
		point.position.push(p);
	}

	point.var = v;
	convex_hull_point_containments.last().pointsetID = pointsetID;
}*/
void readConvexHullPointOnHull(B& in, Solver& S) {

    //point_on_hull pointsetID pointVar var

	int pointsetID = parseInt(in);
	int pointVar = parseInt(in)-1;
	int var = parseInt(in)-1;
	convex_hull_points.push({pointsetID,pointVar, var});
}


void readConvexHullsIntersect(B& in, Solver& S, bool inclusive) {

    //hulls_intersect pointsetID1 pointsetID2 var
	//v is true iff the two convex hulls intersect
	int pointsetID1 = parseInt(in);
	int pointsetID2 = parseInt(in);
	int v = parseInt(in)-1;
	convex_hulls_intersect.push({pointsetID1,pointsetID2,v,inclusive});


}

void read_rectangle(B& in, Solver& S) {
	constant_rectangles.push();

	int polygonID = parseInt(in);
/*	if(polygons.contains(polygonID)){
		fprintf(stderr,"PolygonID %d redefined, aborting\n",polygonID);
		exit(1);
	}*/


	constant_rectangles.last().pointsetID=polygonID;
	int d = parseInt(in);
	//constant_rectangles.last().width = parseInt(in);
	//constant_rectangles.last().height = parseInt(in);
	parsePoint(in,d, constant_rectangles.last().a);
	parsePoint(in,d, constant_rectangles.last().b);
	polygons.push({polygonID,d});

}

void read_polygon_intersection(B& in, Solver& S, bool inclusive){
	int polygonID1 = parseInt(in);
	int polygonID2 = parseInt(in);
	int v = parseInt(in)-1;
	polygon_intersections.push({polygonID1,polygonID2, inclusive,v});
}
void read_polygon_area_lt(B& in, Solver& S, bool inclusive){
	int polygonID = parseInt(in);
	int v = parseInt(in)-1;
	polygon_area_lt.push({polygonID, inclusive,v});
}
void read_polygon(B& in, Solver& S) {
	constant_polygons.push();

	int polygonID = parseInt(in);
	constant_polygons.last().pointsetID=polygonID;

	int numPoints = parseInt(in);
	int d = parseInt(in);
	for(int i = 0;i<numPoints;i++){
		constant_polygons.last().points.push();
		ParsePoint & p = convex_hull_polygon_intersections.last().points.last();
		parsePoint(in,d,p);
	}

	int v = parseInt(in)-1;

}


void read_polygon_operation(B& in, Solver& S) {
    //hulls_intersect pointsetID1 pointsetID2 var
	//v is true iff the two convex hulls intersect
	int pointsetID = parseInt(in);
	int pointsetID1 = parseInt(in);
	int pointsetID2 = parseInt(in);
	PolygonOperationType op;
	if(match(in,"union")){
		op=PolygonOperationType::op_union;
	}else if(match(in,"intersect")){
		op=PolygonOperationType::op_intersect;
	}else if(match(in,"difference")){
		op=PolygonOperationType::op_difference;
	}else if(match(in,"minkowski_sum")){
		op=PolygonOperationType::op_minkowski_sum;
	}else{
		fprintf(stderr,"Unrecognized polygon operation %s, aborting\n",*in);
		exit(1);
	}

	int v = parseInt(in)-1;
	polygon_operations.push({pointsetID,pointsetID1,pointsetID2,op});
}


void read_conditional_polygon(B& in, Solver& S) {
	int pointsetID = parseInt(in);
	int pointsetID1 = parseInt(in);
	int pointsetID2 = parseInt(in);

	int v = parseInt(in);
	bool sign;
	int var;
	if(v<0){
		sign=true;
		var = (-v)-1;
	}else{
		sign=false;
		var = v-1;
	}
	while(S.nVars()<=var){
		S.newVar();
	}
	conditional_polygons.push({pointsetID,pointsetID1,pointsetID2,mkLit(var,sign)});
}

public:

 bool parseLine(B& in, Solver& S){

		skipWhitespace(in);
		if (*in == EOF){
			return false;
		}else if (match(in,"convex_hull_area_gt")){
			readConvexHullArea(in,S);
		}/*else if (match(in,"convex_hull_containment")){
			readConvexHullPointContained(in,S);
		}else if (match(in,"convex_hull_intersects_line")){
			readConvexHullIntersectsLine(in,S);
		}*/
		else if (match(in,"convex_hull_collides_polygon")){
			readConvexHullIntersectsPolygon(in,S,true);
		}else if (match(in,"convex_hull_intersects_polygon")){
			readConvexHullIntersectsPolygon(in,S,false);
		}else if (match(in,"point_on_convex_hull")){
			readConvexHullPointOnHull(in,S);
		}else if (match(in,"convex_hulls_intersect")){
			readConvexHullsIntersect(in,S,false);
		}else if (match(in,"convex_hulls_collide")){
			readConvexHullsIntersect(in,S,true);
		}else if (match(in,"rectangle")){
			read_rectangle(in,S);
		}else if (match(in,"overlap")){
			read_polygon_intersection(in,S,true);
		}else if (match(in,"intersect")){
			read_polygon_intersection(in,S,false);
		}else if (match(in,"area_lt")){
			read_polygon_area_lt(in,S,false);
		}else if (match(in,"area_leq")){
			read_polygon_area_lt(in,S,true);
		}else if (match(in,"constant_polygon")){
			read_polygon(in,S);
		}else if (match(in,"conditional_polygon")){
			read_conditional_polygon(in,S);
		}else if (match(in,"polygon_operation")){
			read_polygon_operation(in,S);
		}else if (match(in,"heightmap_volume")){

		}else if (match(in, "euclidian_steiner_tree_weight")){

		}else if (match(in, "rectilinear_steiner_tree_weight")){

		}else if (match(in,"point")){
			//add a point to a point set
			readPoint(in,S);

		}else{
			return false;
		}
		return true;
 }

 void implementConstraints(Solver & S){
	 //build point sets in their appropriate spaces.
	 //for now, we only support up to 3 dimensions
	 vec<int> pointsetDim;
	 for(int i = 0;i<pointsets.size();i++){
		 vec<ParsePoint> & pointset = pointsets[i];
		 if(pointset.size()==0)
			 continue;
		 ParsePoint & firstP = pointset[0];
		 int D = firstP.position.size();

		 if(D==1){
			/* space_1D.growTo(i+1,nullptr);
			 if(!space_1D[i]){
				 space_1D[i] = new GeometryTheorySolver<1,double>(&S);
				 S.addTheory(space_1D[i]);
			 }*/
		 }else if (D==2){

			 if(!space_2D){
				 space_2D = new GeometryTheorySolver<2,T>(&S);
				 S.addTheory(space_2D);
			 }
		 }/*else if (D==3){
			 space_3D.growTo(i+1,nullptr);
			 if(!space_3D[i]){
				 space_3D[i] = new GeometryTheorySolver<3,double>(&S);
				 S.addTheory(space_3D[i]);
			 }
		 }*/else{

			 fprintf(stderr,"Only points of dimension 1 and 2 currently supported (found point %d of dimension %d), aborting\n", i,D);
			 exit(2);
		 }

		 pointsetDim.growTo(i+1,-1);
		 pointsetDim[i]=D;
		 for(ParsePoint & p:pointset){
			 if(p.position.size()!=D){
				 fprintf(stderr,"All points in a pointset must have the same dimensionality\n");
				 exit(2);
			 }
			 if(D==1){
				/* Point<1,double> pnt(p.position);
				 space_1D[i]->newPoint(pnt,p.var);*/
			 }else if(D==2){
				 Point<2,T> pnt(p.position);
				 space_2D->newPoint(i,pnt,p.var);
			 }/*else if(D==3){
				 Point<3,double> pnt(p.position);
				 space_3D[i]->newPoint(pnt,p.var);
			 }*/else{
				 assert(false);
			 }
		 }
	 }



	 for (auto & c: convex_hull_areas){
		 if(c.pointsetID>=pointsetDim.size() || c.pointsetID<0 || pointsetDim[c.pointsetID]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID);
		 }
		 if(c.pointsetID<0 || c.pointsetID>=pointsetDim.size() || pointsetDim[c.pointsetID]<0){
			 fprintf(stderr,"Pointset %d is undefined, aborting!",c.pointsetID);
			 exit(2);
		 }
		 int D = pointsetDim[c.pointsetID];

		 if(D==1){
			// space_1D[c.pointsetID]->convexHullArea(c.area,c.v);
		 }else if(D==2){
			 space_2D->convexHullArea(c.pointsetID,c.area,c.v);
		 }/*else if(D==3){
			 space_3D[c.pointsetID]->convexHullArea(c.area,c.v);
		 }*/else{
			 assert(false);
		 }

	 }
	 for (auto & c: convex_hull_polygon_intersections){
	 		 if(c.pointsetID>=pointsetDim.size() || c.pointsetID<0 || pointsetDim[c.pointsetID]<0){
	 			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID);
	 		 }
	 		 if(c.pointsetID<0 || c.pointsetID>=pointsetDim.size() || pointsetDim[c.pointsetID]<0){
	 			 fprintf(stderr,"Pointset %d is undefined, aborting!",c.pointsetID);
	 			 exit(2);
	 		 }
	 		 int D = pointsetDim[c.pointsetID];

	 		 if(D==1){

	 			// space_1D[c.pointsetID]->convexHullContains(pnt, c.point.var);
	 		 }else if(D==2){

	 			 std::vector<Point<2,T> > p2;
	 			 for(ParsePoint &p:c.points){
	 				p2.push_back(p.position);
	 			 }
	 			 space_2D->convexHullIntersectsPolygon(c.pointsetID,p2, c.var,c.inclusive);
	 		 }else if(D==3){

	 		 }else{
	 			 assert(false);
	 		 }
	 	 }
/*	 for (auto & c: convex_hull_point_containments){
		 if(c.pointsetID>=pointsetDim.size() || c.pointsetID<0 || pointsetDim[c.pointsetID]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID);
		 }
		 int D = pointsetDim[c.pointsetID];

		 if(D==1){
			 Point<1,T> pnt(c.point.position);
			// space_1D[c.pointsetID]->convexHullContains(pnt, c.point.var);
		 }else if(D==2){
			 Point<2,T> pnt(c.point.position);
			 space_2D->convexHullContains(c.pointsetID,pnt, c.point.var);
		 }else if(D==3){
			 Point<3,double> pnt(c.point.position);
			 space_3D[c.pointsetID]->convexHullContains(pnt, c.point.var);
		 }else{
			 assert(false);
		 }
	 }*/

/*	 for (auto & c:convex_hull_line_intersections){
		 if(c.pointsetID>=pointsetDim.size() || c.pointsetID<0 || pointsetDim[c.pointsetID]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID);
		 }
		 int D = pointsetDim[c.pointsetID];

		 if(D==1){
			// Point<1,T> pnt(c.point.position);
			// space_1D[c.pointsetID]->convexHullContains(pnt, c.point.var);
		 }else if(D==2){
			 Point<2,T> p(c.p.position);
			 Point<2,T> q(c.q.position);
			 space_2D->convexHullIntersectsLine(c.pointsetID,p,q,c.var);
		 }else if(D==3){
			 Point<3,double> pnt(c.point.position);
			 space_3D[c.pointsetID]->convexHullContains(pnt, c.point.var);
		 }else{
			 assert(false);
		 }
	 }*/

	 for (auto & c: convex_hull_points){
		 if(c.pointsetID>=pointsetDim.size() || c.pointsetID<0 || pointsetDim[c.pointsetID]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID);
		 }
		 if(c.pointsetID<0 || c.pointsetID>=pointsetDim.size() || pointsetDim[c.pointsetID]<0){
			 fprintf(stderr,"Pointset %d is undefined, aborting!",c.pointsetID);
			 exit(2);
		 }
		 printf("point_on_convex_hull not currently supported, aborting.\n");
		 exit(1);
		/* int D = pointsetDim[c.pointsetID];

		 if(D==1){
			 //space_1D[c.pointsetID]->convexHullContains(c.pointVar,c.pointOnHull);
		 }else if(D==2){
			 if(!S.hasTheory(c.pointVar)){
				 fprintf(stderr,"Variable %d is not a point variable, aborting!",c.pointVar);
				 exit(2);
			 }
			 Var theoryVar = S.getTheoryVar(c.pointVar);
			 int pointID = space_2D->getPointID(theoryVar);
			 int pointset = space_2D->getPointset(pointID);
			 assert(pointset == c.pointsetID);
			 int pointIndex = space_2D->getPointsetIndex(pointID);

			 assert(pointIndex>=0);
			 space_2D->pointOnHull(c.pointsetID,pointIndex,c.pointOnHull);
		 }else if(D==3){
			 space_3D[c.pointsetID]->convexHullContains(c.pointVar,c.pointOnHull);
		 }else{
			 assert(false);
		 }*/

	 }

	 for (auto & c:convex_hulls_intersect){
		 if(c.pointsetID1 < 0 || c.pointsetID1>=pointsetDim.size() || c.pointsetID1<0 || pointsetDim[c.pointsetID1]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID1);
			 exit(2);
		 }
		 if(c.pointsetID2< 0 || c.pointsetID2>=pointsetDim.size() || c.pointsetID2<0 || pointsetDim[c.pointsetID2]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID2);
			 exit(2);
		 }
		 int D = pointsetDim[c.pointsetID1];

		 if( pointsetDim[c.pointsetID2] != D){
			 fprintf(stderr,"Cannot intersect convex hulls in different dimensions (%d has dimension %d, while %d has dimension %d)\n",c.pointsetID1,D, c.pointsetID2, pointsetDim[c.pointsetID2]);
		 }
		 if(D==1){

		 }else if(D==2){

			 space_2D->convexHullsIntersect(c.pointsetID1,c.pointsetID2,c.var, c.inclusive);
		 }else{
			 assert(false);
		 }


	 }
	 for (auto & c:constant_rectangles){
		 if(c.polygonID < 0 || c.polygonID>=pointsetDim.size() || c.polygonID<0 || pointsetDim[c.polygonID]<0){
			 fprintf(stderr,"Bad polygonID %d\n", c.polygonID);
			 exit(2);
		 }

		 vec<ParsePoint> points;

		 int D = pointsetDim[c.polygonID];

		 if(D==1){

		 }else if(D==2){
			 Point<2,T> a=c.a.position;
			 Point<2,T> b=c.b.position;
			 space_2D->constant_rectangle(c.pointsetID,a,b);
			 //space_2D->constant_rectangle(c.pointsetID,c.width,c.height,center);
		 }else{
			 assert(false);
		 }
	 }


	 for (auto & c:constant_polygons){
		 if(c.pointsetID < 0 || c.pointsetID>=pointsetDim.size() || c.pointsetID<0 || pointsetDim[c.pointsetID]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID);
			 exit(2);
		 }
		 int pointsetID;
		 vec<ParsePoint> points;

		 int D = pointsetDim[c.pointsetID];

		 if(D==1){

		 }else if(D==2){
			 std::vector<Point<2,T> > polygon;
			 for(ParsePoint &p:c.points){
				 polygon.push_back(p.position);
			 }
			 space_2D->constant_polygon (c.pointsetID,polygon);
		 }else{
			 assert(false);
		 }
	 }

	 for (auto & c:conditional_polygons){
		 if(c.pointsetID < 0 || c.pointsetID>=pointsetDim.size() || c.pointsetID<0 || pointsetDim[c.pointsetID]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID);
			 exit(2);
		 }
		 if(c.pointsetID1 < 0 || c.pointsetID1>=pointsetDim.size() || c.pointsetID1<0 || pointsetDim[c.pointsetID1]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID1);
			 exit(2);
		 }
		 if(c.pointsetID2< 0 || c.pointsetID2>=pointsetDim.size() || c.pointsetID2<0 || pointsetDim[c.pointsetID2]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID2);
			 exit(2);
		 }

		 int D = pointsetDim[c.pointsetID];

		 if( pointsetDim[c.pointsetID1] != D){
			 fprintf(stderr,"Cannot intersect convex hulls in different dimensions (%d has dimension %d, while %d has dimension %d)\n",c.pointsetID,D, c.pointsetID1, pointsetDim[c.pointsetID1]);
		 }

		 if( pointsetDim[c.pointsetID2] != D){
			 fprintf(stderr,"Cannot intersect convex hulls in different dimensions (%d has dimension %d, while %d has dimension %d)\n",c.pointsetID,D, c.pointsetID2, pointsetDim[c.pointsetID2]);
		 }
		 if(D==1){

		 }else if(D==2){
			 space_2D->conditional_polygon(c.pointsetID,c.pointsetID1,c.pointsetID2,c.conditional);
		 }else{
			 assert(false);
		 }
	 }

	 for (auto & c:polygon_operations){
		 if(c.pointsetID < 0 || c.pointsetID>=pointsetDim.size() || c.pointsetID<0 || pointsetDim[c.pointsetID]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID);
			 exit(2);
		 }
		 if(c.pointsetID1 < 0 || c.pointsetID1>=pointsetDim.size() || c.pointsetID1<0 || pointsetDim[c.pointsetID1]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID1);
			 exit(2);
		 }
		 if(c.pointsetID2< 0 || c.pointsetID2>=pointsetDim.size() || c.pointsetID2<0 || pointsetDim[c.pointsetID2]<0){
			 fprintf(stderr,"Bad pointsetID %d\n", c.pointsetID2);
			 exit(2);
		 }

		 int D = pointsetDim[c.pointsetID];

		 if( pointsetDim[c.pointsetID1] != D){
			 fprintf(stderr,"Cannot intersect convex hulls in different dimensions (%d has dimension %d, while %d has dimension %d)\n",c.pointsetID,D, c.pointsetID1, pointsetDim[c.pointsetID1]);
		 }

		 if( pointsetDim[c.pointsetID2] != D){
			 fprintf(stderr,"Cannot intersect convex hulls in different dimensions (%d has dimension %d, while %d has dimension %d)\n",c.pointsetID,D, c.pointsetID2, pointsetDim[c.pointsetID2]);
		 }
		 if(D==1){

		 }else if(D==2){
			 space_2D->polygon_operation(c.pointsetID,c.pointsetID1,c.pointsetID2,c.operation);
		 }else{
			 assert(false);
		 }
	 }


	 //now that we have defined all the polygons, build predicates

	 for (auto & c:polygon_intersections){
		 if(c.polygonID1 < 0 || c.polygonID1>=pointsetDim.size() || c.polygonID1<0 || pointsetDim[c.polygonID1]<0){
			 fprintf(stderr,"Bad polygonID %d\n", c.polygonID1);
			 exit(2);
		 }
		 if(c.polygonID2 < 0 || c.polygonID2>=pointsetDim.size() || c.polygonID2<0 || pointsetDim[c.polygonID2]<0){
			 fprintf(stderr,"Bad polygonID %d\n", c.polygonID2);
			 exit(2);
		 }

		 int pointsetID;
		 vec<ParsePoint> points;

		 int D = pointsetDim[c.polygonID1];

		 if( pointsetDim[c.polygonID2] != D){
			 fprintf(stderr,"Cannot intersect convex hulls in different dimensions (%d has dimension %d, while %d has dimension %d)\n",c.polygonID1,D, c.polygonID2, pointsetDim[c.polygonID2]);
		 }
		 if(D==1){

		 }else if(D==2){
			 space_2D->polygon_intersects(c.polygonID1,c.polygonID2,c.inclusive,c.v);
		 }else{
			 assert(false);
		 }
	 }



 }

};

//=================================================================================================
};



#endif /* GRAPH_PARSER_H_ */
