#include "GFXMath.h"
#include "PlyModel.h"
#include "SceneObj.h"
#include "BBox.h"
#include <cmath>

const int numObj = 4;

#ifndef Included_SceneGraph_H
#define Included_SceneGraph_H

class SceneGraph{
	public:
	//basic data structure to act as scene graph
	SceneObj *root;
	SceneObj myObjs[numObj];
	BBox worldBB;

	int showBB; //keep track of which bounding volume to show
	bool boolBB; //A switch used when toggling bounding volumes on/off
	bool hitFlag; //keep track of weather the pick() hit a model

	void init();

	void updatePly();

	bool cullIt(Vec3 centerPosition, Vec3 eyePosition, Vec3 upVector, Vec3 midPoint, Mat4 modelViewMatrix);

	void draw(Vec3 centerPosition, Vec3 eyePosition, Vec3 upVector, Mat4 modelViewMatrix);

	void update(Vec3 centerPosition, Vec3 eyePosition, Vec3 upVector, Mat4 modelViewMatrix);

	float distance(Vec3 a, Vec3 b);

	void testPar();

	void translate(SceneObj *s, float n);

	void drawSphere(float radius, int slices, int stacks, double x, double y, double z);
};
#endif