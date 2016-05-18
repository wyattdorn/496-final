// 
// Michael Shafae
// mshafae at fullerton.edu
// 
// The Utah teapot floating in space. Camera can be rotated up/down and 
// left right. The camera's transformations are defined/implemented in
// transformations.h/cpp. 
//
// This demonstration program requires that you provide your own GFXMath.h.
//
// $Id: camera_control_glfw.cpp 5625 2015-03-26 06:18:52Z mshafae $
//

#include <cstdlib>
#include <cstdio>
#include <sys/time.h>

#include "GLFWApp.h"
#include "GFXMath.h"
#include "GFXExtra.h"

#include "GLSLShader.h"
#include "transformations.h"
#include "PlyModel.h"

#include "SceneObj.h"
#include "BBox.h"
#include "SceneGraph.h"

Vec3 endPoint = Vec3(0.0f,0.0f,0.0f);
Vec3 startPoint = endPoint;
float distToCenter = 5;
float pi = 3.14159f;


void msglVersion(void){
  fprintf(stderr, "OpenGL Version Information:\n");
  fprintf(stderr, "\tVendor: %s\n", glGetString(GL_VENDOR));
  fprintf(stderr, "\tRenderer: %s\n", glGetString(GL_RENDERER));
  fprintf(stderr, "\tOpenGL Version: %s\n", glGetString(GL_VERSION));
  fprintf(stderr, "\tGLSL Version: %s\n",
          glGetString(GL_SHADING_LANGUAGE_VERSION)); 
}

void drawSphere(float radius, int slices, int stacks){
	GLUquadricObj *quadObj;
	quadObj = gluNewQuadric();
	assert(quadObj);
	/*
	gluQuadricDrawStyle(quadObj, GLU_FILL);
	gluQuadricNormals(quadObj, GLU_SMOOTH);
	*/
	gluQuadricDrawStyle(quadObj, GLU_LINE);
	gluQuadricNormals(quadObj, GLU_SMOOTH);
	gluSphere(quadObj, radius, slices, stacks);
}

class CameraControlApp : public GLFWApp{
private:
  FaceList* bunnyModel;
  float rotationDelta;

	SceneGraph myGraph;

  Vec3 centerPosition;
  Vec3 eyePosition;
  Vec3 upVector;

	Vec3 leftNorm;
	Vec3 rightNorm;
	Vec3 bottomNorm;
	Vec3 topNorm;

  Mat4 modelViewMatrix;
  Mat4 projectionMatrix;
  Mat4 normalMatrix;
  
  GLSLProgram shaderProgram;

  Vec4 light0;
  Vec4 light1; 

  // Variables to set uniform params for lighting fragment shader 
  unsigned int uModelViewMatrix; 
  unsigned int uProjectionMatrix; 
  unsigned int uNormalMatrix; 
  unsigned int uLight0_position;
  unsigned int uLight0_color;
  unsigned int uLight1_position;
  unsigned int uLight1_color;
  unsigned int uAmbient;
  unsigned int uDiffuse;
  unsigned int uSpecular;
  unsigned int uShininess;
  
public:
  CameraControlApp(int argc, char* argv[]) :
    GLFWApp(argc, argv, std::string("Camera Control").c_str( ), 500, 500){ }
  
  void initCenterPosition( ){
    centerPosition = Vec3(0.0, 0.0, 0.0);
  }
  
  void initEyePosition( ){
    eyePosition = Vec3(0.0, 0.0, distToCenter);
  }

  void initUpVector( ){
    upVector = Vec3(0.0, 1.0, 0.0);
  }

  void initRotationDelta( ){
    rotationDelta = 5.0;
  }
  
  bool begin( ){
    msglError( );
    initCenterPosition( );
    initEyePosition( );
    initUpVector( );
    initRotationDelta( );
	myGraph.init();

    // Load the bunny
    bunnyModel = readPlyModel("trico.ply");

	//initialize frustum plane normals
	leftNorm = Vec3(-1,0,0.5);
	rightNorm = Vec3(1,0,0.5);
	bottomNorm = Vec3(0,1,0.5);
	topNorm = Vec3(0,-1,0.5);

    // Load the shader program
    const char* vertexShaderSource = "blinn_phong.vert.glsl";
    const char* fragmentShaderSource = "blinn_phong.frag.glsl";
    FragmentShader fragmentShader(fragmentShaderSource);
    VertexShader vertexShader(vertexShaderSource);
    shaderProgram.attach(vertexShader);
    shaderProgram.attach(fragmentShader);
    shaderProgram.link( );
    shaderProgram.activate( );
    
    printf("Shader program built from %s and %s.\n",
           vertexShaderSource, fragmentShaderSource);
    if( shaderProgram.isActive( ) ){
      printf("Shader program is loaded and active with id %d.\n", shaderProgram.id( ) );
    }else{
      printf("Shader program did not load and activate correctly. Exiting.");
      exit(1);
    }

    // Set up uniform variables
    uModelViewMatrix = glGetUniformLocation(shaderProgram.id( ),
                                            "modelViewMatrix");
    uProjectionMatrix = glGetUniformLocation(shaderProgram.id( ),
                                             "projectionMatrix");
    uNormalMatrix = glGetUniformLocation(shaderProgram.id( ),
                                         "normalMatrix");
    uLight0_position = glGetUniformLocation(shaderProgram.id( ),
                                            "light0_position");
    uLight0_color = glGetUniformLocation(shaderProgram.id( ),
                                         "light0_color");
    uLight1_position = glGetUniformLocation(shaderProgram.id( ),
                                            "light1_position");
    uLight1_color = glGetUniformLocation(shaderProgram.id( ),
                                         "light1_color");
    uAmbient = glGetUniformLocation(shaderProgram.id( ), "ambient");
    uDiffuse = glGetUniformLocation(shaderProgram.id( ), "diffuse");
    uSpecular = glGetUniformLocation(shaderProgram.id( ), "specular");
    uShininess = glGetUniformLocation(shaderProgram.id( ), "shininess");
    
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    msglVersion( );
    
    return !msglError( );
  }
  
  bool end( ){
    windowShouldClose( );
    return true;
  }

	void isInFrustum(){
		Vec3 point;
		for(int x = 1; x < numObj; x++){
			point = Vec3(myGraph.myObjs[x].FL->center[0], myGraph.myObjs[x].FL->center[1], myGraph.myObjs[x].FL->center[2]);
			float distLeft	= dot(leftNorm, (point - eyePosition));
			float distRight	= dot(rightNorm, (point - eyePosition));
			float distTop	= dot(topNorm, (point - eyePosition));
			float distBot	= dot(bottomNorm, (point - eyePosition));
			if(distLeft>0 || distRight>0 || distTop>0 || distBot>0){
				myGraph.myObjs[x].draw = false;
			}else{
				myGraph.myObjs[x].draw = true;
			}
		}
		//fprintf(stderr, "left/right/top/bottom: %f, %f, %f, %f.\n", distLeft, distRight, distTop, distBot );
		//if(distLeft && distRight && distTop && distBot){
		//return true;
	}

	/*
	float dist_Point_to_Plane( Vec3 P, Vec3 B){
		//plane = vec4
		Vec3 planeNormals[4];
		planeNormals[0] = Vec3(0,1,1); //bottom
		planeNormals[1] = Vec3(-1,0,1);
		planeNormals[2] = Vec3(0,1,-1);
		planeNormals[3] = Vec3(1,0,1);
		float    sb, sn, sd;

		sn = -dot( planeNormals[0], (P - eyePosition));
		sd = dot(planeNormals[0], planeNormals[0]);
		sb = sn / sd;

		*B = P + sb * planeNormals[0];
		return 1;//normalize(P - B);
	}
		*/
  
  bool pick(int x, int y, FaceList *fl){
		bool result = false;
    		float center[3];
		for(int i = 0; i < 3; i++){
			center[i] = fl->center[i];
		}
		Point3 cc(center);
		BoundingSphere boundingSphere(cc, fl->radius);
		GLViewPort vp;
		    /*******
		     * With the double unproject technique
		     */
		    // origin at the lower left corner; flip the y
		int flipped_y = vp.height( ) - y - 1;
		    
		Vec3 nearWinCoord(x, flipped_y, 0);
		Vec3 farWinCoord(x, flipped_y, 1);
		Vec3 nearObjCoord, farObjCoord;
		/*int viewport[4];
		glGetIntegerv( GL_VIEWPORT, viewport ); 
		double mv[16], proj[16];
		for(int i = 0; i < 4; i++){
		      for(int j = 0; j < 4; j++){
			mv[(i*4)+j] = modelViewMatrix(i, j);
			proj[(i*4)+j] = projectionMatrix(i, j);
		      }
		    }
		    double a, b, c, d, e, f;*/
		    //_msUnProject(x, y, 0.0, mv, proj, viewport, &a, &b, &c);
		if(!unproject(nearWinCoord, projectionMatrix, modelViewMatrix, vp, nearObjCoord)){
			std::cerr << "Something is wrong, the omega of the unprojected winCoord is zero." << std::endl;
			assert(false);
		}
		    //_msUnProject(x, y, 1.0, mv, proj, viewport, &d, &e, &f);
		    if(!unproject(farWinCoord, projectionMatrix, modelViewMatrix, vp, farObjCoord)){
		      std::cerr << "Something is wrong, the omega of the unprojected winCoord is zero." << std::endl;
		      assert(false);
		    }
		    /*std::cerr << nearWinCoord << " " << nearObjCoord << " " << a << " " << b << " " << c << std::endl;
		    std::cerr << farWinCoord << " " << farObjCoord << " " << d << " " << e << " " << f << std::endl;*/
		    Vec3 direction1 = farObjCoord - nearObjCoord;
		    Point3 rayOrigin1(eyePosition);
		    Ray r1(rayOrigin1, direction1);
		    if( boundingSphere.intersectWith(r1) ){
		      //std::cerr << "Intersection" << std::endl;
			result = true;
		    }else{
		      //std::cerr << "No intersection" << std::endl;
		    }
		    /*
		     * End double unproject technique
		     ******/
		    
		    /******
		     * Ray in eye coordinates
		     */
		    int window_y = (vp.height( ) - y) - vp.height( )/2;
		    double norm_y = double(window_y)/double(vp.height( )/2);
		    int window_x = x - vp.width( )/2;
		    double norm_x = double(window_x)/double(vp.width( )/2);
		    double near_height = atan(degreesToRadians(25.0));
		    float _y = near_height * norm_y;
		    float _x = near_height * vp.aspect( ) * norm_x;
		    
		    Mat4 modelViewInverse = modelViewMatrix.inverse( );
		    Vec4 ray_origin(0.f, 0.f, 0.f, 1.f);
		    // near distance (3rd param) is from the perspective call,
		    // it should really come from a camera object.
		    Vec4 ray_direction(_x, _y, -1.f, 0.f);
		    ray_origin = modelViewInverse * ray_origin;
		    ray_direction = modelViewInverse * ray_direction;
		    
		    //std::cerr << ray_origin << std::endl;
		    //std::cerr << ray_direction << std::endl;
		    Point3 rayOrigin2 = Point3(ray_origin);
		    Vec3 direction2 = ray_direction.xyz( );
		    Ray r2(rayOrigin2, direction2);
		    //std::cerr << r2 << std::endl;
		    if( boundingSphere.intersectWith(r2) ){
		      //std::cerr << "Intersection again" << std::endl;
		    }else{
		      //std::cerr << "No intersection again" << std::endl;
		    }
		    
		   /*
		    * End ray in eye coordinates
		    ******/
		return result;
	}

  bool render( ){
    Vec4 light0_position(0.0, 5.0, 10.0, 1.0);
    Vec4 light1_position(0,5,-10,1);
    const Vec4 light0_specular(0.6,0.3,0,1);
    const Vec4 light1_specular(0,0.3,0.6,1);
    // specular on teapot
    const Vec4 one(1,1,1,1);
    // diffuse on teapot
    const Vec4 medium(0.5,0.5,0.5,1);
    // ambient on teapot
    const Vec4 small(0.2,0.2,0.2,1);
    // shininess of teapot
    const Vec1 high(100);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Vec2i w = windowSize( );
    double ratio = double(w[0]) / double(w[1]);

    projectionMatrix = perspective(50.0, ratio, 1.0, 25.0);

    modelViewMatrix = lookat(eyePosition, centerPosition, upVector);

    normalMatrix = modelViewMatrix.inverse().transpose( );

    // Set light & material properties for the teapot;
    // lights are transformed by  current modelview matrix
    // such that they are positioned correctly in the scene.
    light0 = modelViewMatrix * light0_position;
    light1 = modelViewMatrix * light1_position;
    
    glUniformMatrix4fv(uModelViewMatrix, 1, false, modelViewMatrix);
    glUniformMatrix4fv(uProjectionMatrix, 1, false, projectionMatrix);
    glUniformMatrix4fv(uNormalMatrix, 1, false, normalMatrix);
    glUniform4fv(uLight0_position, 1, light0); 
    glUniform4fv(uLight0_color, 1, light0_specular); 
    glUniform4fv(uLight1_position, 1, light1); 
    glUniform4fv(uLight1_color, 1, light1_specular); 

    glUniform4fv(uAmbient, 1, small); 
    glUniform4fv(uDiffuse, 1, medium); 
    glUniform4fv(uSpecular, 1, one); 
    glUniform1fv(uShininess, 1, high); 

	glBegin(GL_QUADS);
    /* Front sky */
    glNormal3f( -1.0f,  0.0f,   1.0f);
    glVertex3f( 12.0f,  0.0f, -12.0f);
    glNormal3f( -1.0f, -1.0f,   1.0f);
    glVertex3f( 12.0f, 12.0f, -12.0f);
    glNormal3f(  1.0f, -1.0f,   1.0f);
    glVertex3f(-12.0f, 12.0f, -12.0f);
    glNormal3f(  1.0f,  0.0f,   1.0f);
    glVertex3f(-12.0f,  0.0f, -12.0f);
    /* Rear sky */
    glNormal3f(  1.0f,  0.0f,  -1.0f);
    glVertex3f(-12.0f,  0.0f,  12.0f);
    glNormal3f(  1.0f, -1.0f,  -1.0f);
    glVertex3f(-12.0f, 12.0f,  12.0f);
    glNormal3f( -1.0f, -1.0f,  -1.0f);
    glVertex3f( 12.0f, 12.0f,  12.0f);
    glNormal3f( -1.0f,  0.0f,  -1.0f);
    glVertex3f( 12.0f,  0.0f,  12.0f);
    /* Left sky */
    glNormal3f(  1.0f,  0.0f,   1.0f);
    glVertex3f(-12.0f,  0.0f, -12.0f);
    glNormal3f(  1.0f, -1.0f,   1.0f);
    glVertex3f(-12.0f, 12.0f, -12.0f);
    glNormal3f(  1.0f, -1.0f,  -1.0f);
    glVertex3f(-12.0f, 12.0f,  12.0f);
    glNormal3f(  1.0f,  0.0f,  -1.0f);
    glVertex3f(-12.0f,  0.0f,  12.0f);
    /* Right sky */
    glNormal3f( -1.0f,  0.0f,  -1.0f);
    glVertex3f( 12.0f,  0.0f,  12.0f);
    glNormal3f( -1.0f, -1.0f,  -1.0f);
    glVertex3f( 12.0f, 12.0f,  12.0f);
    glNormal3f( -1.0f, -1.0f,   1.0f);
    glVertex3f( 12.0f, 12.0f, -12.0f);
    glNormal3f( -1.0f,  0.0f,   1.0f);
    glVertex3f( 12.0f,  0.0f, -12.0f);
    /* Top sky */
    glNormal3f( -1.0f, -1.0f,   1.0f);
    glVertex3f( 12.0f, 12.0f, -12.0f);
    glNormal3f( -1.0f, -1.0f,  -1.0f);
    glVertex3f( 12.0f, 12.0f,  12.0f);
    glNormal3f(  1.0f, -1.0f,  -1.0f);
    glVertex3f(-12.0f, 12.0f,  12.0f);
    glNormal3f(  1.0f, -1.0f,   1.0f);
    glVertex3f(-12.0f, 12.0f, -12.0f);
    glEnd();
	
	glLineWidth(1.0f);
	
	isInFrustum();
    
	//Show bounding sphere
	if(myGraph.showBB>=0 && myGraph.boolBB == true){
		myGraph.drawBoundingSphere(Vec3(myGraph.myObjs[myGraph.showBB].FL->center[0],
						myGraph.myObjs[myGraph.showBB].FL->center[1],
						myGraph.myObjs[myGraph.showBB].FL->center[2]),
						myGraph.myObjs[myGraph.showBB].FL->radius);
	}

	if(isKeyPressed('1')){
		isInFrustum();
	}
	if(isKeyPressed('A')){
		myGraph.myObjs[1].FL->translate(0,0.1,0);
	}
	if(isKeyPressed('S')){
		myGraph.myObjs[1].FL->translate(0,-0.1,0);
	}

	if(isKeyPressed('B')){
		fprintf(stderr, "Center %f, %f, %f \n", centerPosition[0], centerPosition[1], centerPosition[2]);
		fprintf(stderr, "eye %f, %f, %f \n", eyePosition[0], eyePosition[1], eyePosition[2]);
		//fprintf(stderr, "Right %f, %f, %f \n", eyePosition[0], eyePosition[1], eyePosition[2]);
	}

    if(isKeyPressed('Q')){
      end( );      
    }else if(isKeyPressed(GLFW_KEY_EQUAL)){
      rotationDelta += 1.0;
      printf( "Rotation delta set to %g\n", rotationDelta );
    }else if(isKeyPressed(GLFW_KEY_MINUS)){
      rotationDelta -= 1.0;
      printf( "Rotation delta set to %g\n", rotationDelta );
    }else if(isKeyPressed('R')){
      initEyePosition( );
      initUpVector( );
      initRotationDelta( );
      printf("Eye position, up vector and rotation delta reset.\n");
    }else if(isKeyPressed(GLFW_KEY_LEFT)){
	Vec3 right = normalize(cross(centerPosition-eyePosition, upVector));
	eyePosition-=right/5.0f;
	centerPosition-=(right/5.0f);
    }else if(isKeyPressed(GLFW_KEY_RIGHT)){
	Vec3 right = normalize(cross(centerPosition-eyePosition, upVector));
	eyePosition+=right/5.0f;
	centerPosition+=(right/5.0f);
    }else if(isKeyPressed(GLFW_KEY_UP)){
	eyePosition+=upVector/5.0f;
	centerPosition+=(upVector/5.0f);
    }else if(isKeyPressed(GLFW_KEY_DOWN)){
	eyePosition-=upVector/5.0f;
	centerPosition-=(upVector/5.0f);
    }
	if(mouseButtonFlags( ) == GLFWApp::MOUSE_BUTTON_LEFT){
		//printf("mouse left button\n");
		Vec2 mousePosition = mouseCurrentPosition( );
		//std::cout << "Mouse position: " << mousePosition << std::endl;
		for(int x=1; x<numObj; x++){
			if(pick(mousePosition[0], mousePosition[1], myGraph.myObjs[x].FL)){
				printf("Intersect with %s!\n", myGraph.myObjs[x].name.c_str());
				myGraph.showBB = x;
				break;
			}
			else{
				myGraph.showBB = -1;
			}
		}
	}
	if (isKeyPressed(GLFW_KEY_LEFT_SHIFT) && mouseButtonFlags( ) == GLFWApp::MOUSE_BUTTON_LEFT){
		
	}
	if (isKeyPressed(GLFW_KEY_LEFT_CONTROL) && mouseButtonFlags( ) == GLFWApp::MOUSE_BUTTON_LEFT){
		Vec2 mousePosition = mouseCurrentPosition( );
		//fprintf(stderr, "Trackball engaged!\n");
		startPoint = normalize(endPoint);
		endPoint = normalize(Vec3( -(mousePosition[0]-250)/(250), (mousePosition[1]-250)/(250), 0 ) + eyePosition);

		Vec3 startVec = normalize(centerPosition-startPoint);
		Vec3 endVec = normalize(centerPosition-endPoint);

		float angle = dot(endVec, startVec);
		Vec3 axis = cross(startVec, endVec);
		//axis[2] = 0;

		angle*=2.0f;

		float delta = sin(angle / 2);
		float gamma = cos(angle / 2);
		Mat4 Q_bar = { 	gamma,			delta * axis[2],	-delta * axis[1],	delta * axis[0]
				,-delta * axis[2],	gamma,			delta * axis[0],	delta * axis[1]
				, delta * axis[1],	-delta * axis[0],	gamma,			delta * axis[2]
				,-delta * axis[0],	-delta * axis[1],	-delta * axis[2],	gamma};

		Mat4 Q = { 	gamma,			delta * axis[2],	-delta * axis[1],	-delta * axis[0]
				,-delta * axis[2],	gamma,			delta * axis[0],	-delta * axis[1]
				, delta * axis[1],	-delta * axis[0],	gamma,			-delta * axis[2]
				, delta * axis[0],	delta * axis[1],	delta * axis[2],	gamma};
		Mat4 temp = Q_bar * Q;

		//rotate frustum//
		Vec4 left4 = Vec4(leftNorm[0], leftNorm[1], leftNorm[2], 0);
		Vec4 right4 = Vec4(rightNorm[0], rightNorm[1], rightNorm[2], 0);
		Vec4 top4 = Vec4(topNorm[0], topNorm[1], topNorm[2], 0);
		Vec4 bot4 = Vec4(bottomNorm[0], bottomNorm[1], bottomNorm[2], 0);
		left4 = temp * left4;
		right4 = temp * right4;
		top4 = temp * top4;
		bot4 = temp * bot4;
		leftNorm = normalize(Vec3(left4[0], left4[1], left4[2]));
		rightNorm = normalize(Vec3(right4[0], right4[1], right4[2]));
		topNorm = normalize(Vec3(top4[0], top4[1], top4[2]));
		bottomNorm = normalize(Vec3(bot4[0], bot4[1], bot4[2]));
		
		Vec4 gaze = normalize(Vec4(centerPosition[0]-eyePosition[0], centerPosition[1]-eyePosition[1], centerPosition[2]-eyePosition[2], 0));
		gaze = temp * gaze;
		centerPosition = (eyePosition + Vec3(gaze[0], gaze[1], gaze[2]));
		fprintf(stderr, "Center is: %f, %f, %f \n", centerPosition[0], centerPosition[1], centerPosition[2]);
		Vec3 right = (cross(Vec3(gaze[0], gaze[1], gaze[2]), upVector));
		right[1]=0.0f;
		
		upVector = normalize(cross(right, Vec3(gaze[0], gaze[1], gaze[2])));
	}
	Vec2 mousePosition = mouseCurrentPosition();
	endPoint = Vec3( -(mousePosition[0]-250)/(250) , (mousePosition[1]-250)/(250), 0 ) + eyePosition;
	myGraph.update(centerPosition, eyePosition, upVector, modelViewMatrix);

	

    return !msglError( );
  }
    
};


int main(int argc, char* argv[]){
  CameraControlApp app(argc, argv);
  return app();
}

