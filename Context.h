#pragma once

#include "Scope.h"
#include "Mesh.h"

#include <fstream>
#include <stack>

#include <GL/gl.h>
#include <GL/glu.h>
//#include <GL/glut.h>
//#include <SOIL/SOIL.h>










class Primitive{
	public:
	Primitive(std::string type,GLuint texId_name,bool,bool,bool);
	
	
	void draw(Scope *scope,int texindex,int rotate,float texscale);
	
	
	std::string type;

	GLuint  texId;
	bool x=false,y=false,z=false;
	
};










class Context
{
public:
    Context();
    ~Context();
    void pushScope();
    Scope *popScope();
    void newScope();
    Scope *getCurrentScope();
    Mesh &getScene();
    GLuint loadTexture(GLuint texid);
    void draw();
    GLfloat *calc(const GLfloat *,int texindex);
    void PLY(const GLfloat *,const std::string &filename);
    void addPrimitive(std::string type,Scope *scope,int texindex,int rotate,float texscale);
    void genPrimitives();

    Scope *current_scope=NULL;
    std::stack<Scope *> scopes;
    Mesh scene;
    Primitive *Cube;
    Primitive *Cylinder;
    Primitive *Sphere;
    Primitive *CubeX;
    Primitive *CubeY;
    std::vector<GLuint> texids;
    std::vector<Primitive *> primitives;
    std::vector<Scope *> primitive_scopes;
    std::vector<int> texindexes;
    std::vector<int> rotates;
    std::vector<float> texscales;
    
    
    
};
