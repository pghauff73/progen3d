#include "Context.h"

#include <iostream>
#include <GL/gl.h>
#include <GL/glu.h>
//#include <GL/glut.h>

#include <algorithm>

extern void draw_box(glm::vec3 scale_vec,
                     glm::vec3 position_vec,
                     glm::vec3 pos,
                     int tex_index,
                     float texscale,
                     float alpha);

namespace {

constexpr int kPrimitiveVertexCount = 36;
constexpr int kVertexStride = 8;

enum class PrimitiveTransformMode {
	Default,
	CubeX,
	CubeY
};

struct PrimitiveTransformCache {
	PrimitiveTransformMode mode = PrimitiveTransformMode::Default;
	glm::mat4 primary_transform{1.0f};
	glm::mat4 secondary_transform{1.0f};
	glm::mat3 primary_normal_transform{1.0f};
	glm::mat3 secondary_normal_transform{1.0f};
};

PrimitiveTransformCache build_transform_cache(const Primitive *primitive, const Scope *scope)
{
	PrimitiveTransformCache cache;
	cache.primary_transform = scope->getTransform();
	cache.secondary_transform = scope->getTransform2();
	cache.primary_normal_transform =
		glm::transpose(glm::inverse(glm::mat3(cache.primary_transform)));
	cache.secondary_normal_transform =
		glm::transpose(glm::inverse(glm::mat3(cache.secondary_transform)));

	if (primitive->type == "CubeX") {
		cache.mode = PrimitiveTransformMode::CubeX;
	} else if (primitive->type == "CubeY") {
		cache.mode = PrimitiveTransformMode::CubeY;
	}

	return cache;
}

void append_primitive_vertices(const PrimitiveTransformCache &cache,
	                           float texscale,
	                           const GLfloat *vertex_data,
	                           std::vector<GLfloat> *buffer)
{
	const std::size_t base_offset = buffer->size();
	buffer->resize(base_offset + (kPrimitiveVertexCount * kVertexStride));

	for (int vertex_index = 0; vertex_index < kPrimitiveVertexCount; ++vertex_index) {
		const int source_offset = vertex_index * kVertexStride;
		const std::size_t destination_offset =
			base_offset + static_cast<std::size_t>(source_offset);

		glm::vec4 vertex(vertex_data[source_offset],
		                vertex_data[source_offset + 1],
		                vertex_data[source_offset + 2],
		                1.0f);

		const glm::mat4 *transform = &cache.primary_transform;
		const glm::mat3 *normal_transform = &cache.primary_normal_transform;

		if (cache.mode == PrimitiveTransformMode::CubeX) {
			vertex.x += 0.5f;
			if (vertex.x > 0.5f) {
				transform = &cache.secondary_transform;
				normal_transform = &cache.secondary_normal_transform;
			}
		} else if (cache.mode == PrimitiveTransformMode::CubeY) {
			vertex.z += 0.5f;
			if (vertex.z > 0.5f) {
				transform = &cache.secondary_transform;
				normal_transform = &cache.secondary_normal_transform;
			}
		} else if (vertex.y > 0.0f) {
			transform = &cache.secondary_transform;
			normal_transform = &cache.secondary_normal_transform;
		}

		vertex = (*transform) * vertex;
		(*buffer)[destination_offset] = vertex.x;
		(*buffer)[destination_offset + 1] = vertex.y;
		(*buffer)[destination_offset + 2] = vertex.z;

		const glm::vec3 normal = glm::normalize(
			(*normal_transform) *
			glm::vec3(vertex_data[source_offset + 3],
			          vertex_data[source_offset + 4],
			          vertex_data[source_offset + 5]));
		(*buffer)[destination_offset + 3] = normal.x;
		(*buffer)[destination_offset + 4] = normal.y;
		(*buffer)[destination_offset + 5] = normal.z;
		(*buffer)[destination_offset + 6] = (vertex_data[source_offset + 6] + 1.0f) * texscale;
		(*buffer)[destination_offset + 7] = (vertex_data[source_offset + 7] + 1.0f) * texscale;
	}
}

}

Primitive::Primitive(std::string type,GLuint texId, bool x,bool y,bool z){
this->texId=texId;

this->type=type;
this->x=x;
this->y=y;
this->z=z;


}

void Primitive::draw(Scope *scope,int tex,float alpha,float scaletex){
	
	

 /*glEnable(GL_TEXTURE_2D);
glBindTexture(GL_TEXTURE_2D,tex);


	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();


	
	glTranslatef(scope->position[0],scope->position[1],scope->position[2]);
	
	glScalef(scope->size.x,scope->size.y,scope->size.z);
	
	*/
	
	
	if(type=="Sphere"){	
	//GLUquadric *qobj = gluNewQuadric();
	//gluQuadricTexture(qobj, GL_TRUE);
	//gluSphere(qobj, 0.5, 20, 20);
	//gluDeleteQuadric(qobj);
	}
	else if(type=="Cylinder"){
	
	
	//glTranslatef(0.0f,1.0f,0.0f);	
	//glRotatef(90,1,0,0);	
	//GLUquadric *qobj = gluNewQuadric();
	//gluQuadricTexture(qobj, GL_TRUE);
	//gluCylinder(qobj,0.5f,0.5f,1.0f,32,32);
	//gluDeleteQuadric(qobj);
		
	
		
	}
	else if(type=="Cube" || type=="CubeX" || type=="CubeY"){
	
	//std::cout<<"draw"<<std::endl;
	
	
	//float y_val=0.0f,x_val=0.0f,z_val=0.0f;
	//if(y)y_val=0.5f;
	//if(x)x_val=0.5f;
	//if(x)z_val=0.5f;
	//glTranslatef(x_val,y_val,z_val);
	
	//glScalef(0.5f,0.5f,0.5f);
	
	
	float SCALE=0.5f;
	
	float x=fabs(scope->size.x);
	float z=fabs(scope->size.z);
	float y=fabs(scope->size.y);
	
	float X=scope->position.x;
	float Z=scope->position.z;
	float Y=scope->position.y;
	
	if(type=="Cube")
	draw_box(glm::vec3(x,y,z),glm::vec3(X,Y,Z),glm::vec3(0,0,0),tex,scaletex,alpha);
	else if(type=="CubeX")
	draw_box(glm::vec3(x,y,z),glm::vec3(X,Y,Z),glm::vec3(0.5,0,0),tex,scaletex,alpha);
	else if(type=="CubeY")
	draw_box(glm::vec3(x,y,z),glm::vec3(X,Y,Z),glm::vec3(0,0,0.5),tex,scaletex,alpha);
	
	/*
	
	glBegin(GL_QUADS);                
        
        //std::cout<<"*";
        // Top face (y = 1.0f)
										  // Define vertices in counter-clockwise (CCW) order with normal pointing out
		//glColor3f(0.0f, 1.0f, 0.0f);     // Green
		glNormal3f(0.0f,1.0f,0.0f);
		glTexCoord2f(x,-z);
		glVertex3f(1.0f, 1.0f, -1.0f);
		
		glNormal3f(0.0f,1.0f,0.0f);
		glTexCoord2f(-x,-z);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		
		glNormal3f(0.0f,1.0f,0.0f);
		glTexCoord2f(-x,z);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		
		glNormal3f(0.0f,1.0f,0.0f);
		glTexCoord2f(x,z);
		glVertex3f(1.0f, 1.0f, 1.0f);



		// Bottom face (y = -1.0f)

		//glColor3f(1.0f, 0.5f, 0.0f);     // Orange
		glNormal3f(0.0f,-1.0f,0.0f);
		
		
		glTexCoord2f(x,z);
		glVertex3f(1.0f, -1.0f, 1.0f);
		glTexCoord2f(-x,z);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(-x,-z);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(x,-z);
		glVertex3f(1.0f, -1.0f, -1.0f);

		// Front face  (z = 1.0f)
		//glColor3f(1.0f, 0.0f, 0.0f);     // Red
		glNormal3f(0.0f,0.0f,1.0f);
		
		
		glTexCoord2f(x,y);
		glVertex3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(-x,y);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(-x,-y);
		glVertex3f(-1.0f, -1.0f, 1.0f);
		glTexCoord2f(x,-y);
		glVertex3f(1.0f, -1.0f, 1.0f);

		// Back face (z = -1.0f)
		//glColor3f(1.0f, 1.0f, 0.0f);     // Yellow
		glNormal3f(0.0f,0.0f,-1.0f);
		
		
		glTexCoord2f(x,-y);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glTexCoord2f(-x,-y);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(-x,y);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(x,y);
		glVertex3f(1.0f, 1.0f, -1.0f);

		// Left face (x = -1.0f)
		//glColor3f(0.0f, 0.0f, 1.0f);     // Blue
		glNormal3f(-1.0f,0.0f,0.0f);
		
		glTexCoord2f(y,z);
		glVertex3f(-1.0f, 1.0f, 1.0f);
		glTexCoord2f(y,-z);
		glVertex3f(-1.0f, 1.0f, -1.0f);
		glTexCoord2f(-y,-z);
		glVertex3f(-1.0f, -1.0f, -1.0f);
		glTexCoord2f(-y,z);
		glVertex3f(-1.0f, -1.0f, 1.0f);

		// Right face (x = 1.0f)
		//glColor3f(1.0f, 0.0f, 1.0f);     // Magenta
		glNormal3f(1.0f,0.0f,0.0f);
		
		glTexCoord2f(y,-z);
		glVertex3f(1.0f, 1.0f, -1.0f);
		
		glTexCoord2f(y,z);
		glVertex3f(1.0f, 1.0f, 1.0f);
		
		glTexCoord2f(-y,z);
		glVertex3f(1.0f, -1.0f, 1.0f);
		
		glTexCoord2f(-y,-z);
		glVertex3f(1.0f, -1.0f, -1.0f);
		glEnd();  // End of drawing color-cube*/
	}
	//glPopMatrix();
	
	 //glDisable(GL_TEXTURE_2D);
}



void Context::addPrimitive(std::string type,Scope *scope,int texindex, float alpha, float texscale){
	
	
	
	
	if(type=="Cube"){
		//std::cout<<"Adding Primitive "<<Cube->type<<std::endl;
		primitives.push_back(Cube);
		}
	
	else if(type=="CubeX"){
		//std::cout<<"Adding Primitive "<<Cube->type<<std::endl;
		primitives.push_back(CubeX);
		}
	else if(type=="CubeY"){
		//std::cout<<"Adding Primitive "<<Cube->type<<std::endl;
		primitives.push_back(CubeY);
		}
	else if(type=="Cylinder"){
		primitives.push_back(Cylinder);
		//std::cout<<"Adding Primitive "<<Cylinder->type<<std::endl;
		}
	else if(type=="Sphere") {
		primitives.push_back(Sphere);
		//std::cout<<"Adding Primitive "<<Sphere->type<<std::endl;
		}
	else {
		//std::cout<<"primitive not added!"<<std::endl;
		return;
	}
	
	glm::vec3 pos=scope->getPosition();
	glm::vec3 size=scope->getSize();
	
	//std::cout<<pos.x<<","<<pos.y<<","<<pos.z<<std::endl;
	//std::cout<<size.x<<","<<size.y<<","<<size.z<<std::endl;
	//std::cout<<texindex<<std::endl;
	primitive_scopes.push_back(new Scope(scope));
	texindexes.push_back(texindex);
	alphas.push_back(alpha);
	texscales.push_back(texscale);
	
	
}

GLfloat *Context::calc(const GLfloat *vertex_data,int tex_index,int *out_count){
	if(out_count!=NULL)*out_count=0;
	if(tex_index<0)return NULL;

	std::vector<std::vector<GLfloat>> buffers;
	std::vector<int> counts;
	buildTextureBuffers(vertex_data,
	                   static_cast<std::size_t>(tex_index + 1),
	                   &buffers,
	                   &counts);
	if(static_cast<std::size_t>(tex_index) >= buffers.size() ||
	   static_cast<std::size_t>(tex_index) >= counts.size()){
		return NULL;
	}
	if(out_count!=NULL)*out_count=counts[tex_index];
	if(buffers[tex_index].empty()){
		return NULL;
	}

	GLfloat *vertex_buffer = new GLfloat[buffers[tex_index].size()];
	std::copy(buffers[tex_index].begin(), buffers[tex_index].end(), vertex_buffer);
	return vertex_buffer;	
}

void Context::buildTextureBuffers(const GLfloat *vertex_data,
	                              std::size_t texture_count,
	                              std::vector<std::vector<GLfloat>> *buffers,
	                              std::vector<int> *counts) const
{
	if (buffers == NULL || counts == NULL) {
		return;
	}

	buffers->assign(texture_count, {});
	counts->assign(texture_count, 0);

	for (std::size_t primitive_index = 0; primitive_index < texindexes.size(); ++primitive_index) {
		const int tex_index = texindexes[primitive_index];
		if (tex_index < 0 || static_cast<std::size_t>(tex_index) >= texture_count) {
			continue;
		}
		++(*counts)[tex_index];
	}

	for (std::size_t texture_index = 0; texture_index < texture_count; ++texture_index) {
		(*buffers)[texture_index].reserve(
			static_cast<std::size_t>((*counts)[texture_index]) *
			kPrimitiveVertexCount *
			kVertexStride);
	}

	const std::size_t primitive_count = std::min(
		std::min(primitives.size(), primitive_scopes.size()),
		std::min(texindexes.size(), texscales.size()));
	for (std::size_t primitive_index = 0; primitive_index < primitive_count; ++primitive_index) {
		const int tex_index = texindexes[primitive_index];
		if (tex_index < 0 || static_cast<std::size_t>(tex_index) >= texture_count) {
			continue;
		}

		const PrimitiveTransformCache cache =
			build_transform_cache(primitives[primitive_index], primitive_scopes[primitive_index]);
		append_primitive_vertices(cache,
		                         texscales[primitive_index],
		                         vertex_data,
		                         &(*buffers)[tex_index]);
	}
}



void Context::PLY(const GLfloat *vertex_data,const std::string &filename){
	std::ofstream fout(filename);
	    fout << "ply\n";
	    
    fout << "format binary_little_endian 1.0\n";
    
    
		fout << "element vertex "<<primitives.size()*36<<"\n";
		fout << "property float x\n";
		fout << "property float y\n";
		fout << "property float z\n";
		fout << "element face "<<primitives.size()*12<<"\n";
		fout << "property list uchar int vertex_index\n";
	
    fout << "end_header\n";
    int vertex_counter=0;
	for(int i=0;i<primitives.size();i++){
			
			
			
			for (int j=0;j<36;j++){
				glm::vec4 v1(0,0,0,1.0);
				
				
				v1.x=vertex_data[j*8];
				v1.y=vertex_data[j*8+1];
				v1.z=vertex_data[j*8+2];
				
				
				glm::mat4 transform;
				if(primitives[i]->type=="CubeX"){
					
					if(v1.x>0.0){
						v1.x=v1.x+0.5;
						transform=primitive_scopes[i]->getTransform2();
						
			
					}
					else {
						v1.x=v1.x+0.5;
						transform=primitive_scopes[i]->getTransform();
					}
				}
				else if(primitives[i]->type=="CubeY"){
					
					if(v1.z>0.0){
						v1.z=v1.z+0.5;
						transform=primitive_scopes[i]->getTransform2();
						
			
					}
					else {
						v1.z=v1.z+0.5;
						transform=primitive_scopes[i]->getTransform();
					}
				}
				else {
					if(v1.y>0.0){
						transform=primitive_scopes[i]->getTransform2();
						
			
					}
					else {
						transform=primitive_scopes[i]->getTransform();
					}
				}
				
				
				v1=transform*v1;
				fout.write(reinterpret_cast<char*>(&v1.x), sizeof(float));
				fout.write(reinterpret_cast<char*>(&v1.y), sizeof(float));
				fout.write(reinterpret_cast<char*>(&v1.z), sizeof(float));
				vertex_counter++;
				
				
		}
	}
	for(int i=0;i<primitives.size()*36;i+=3){
						unsigned char n=3;
					    int i0 = i;
						int i1 = i+1;
						int i2 = i+2;
						//std::cout<<i0<<" "<<i1<<" "<<i2<<std::endl;
					   fout.write(reinterpret_cast<char*>(&n), sizeof(unsigned char));
					   fout.write(reinterpret_cast<char*>(&i0), sizeof(int));
					   fout.write(reinterpret_cast<char*>(&i1), sizeof(int));
					   fout.write(reinterpret_cast<char*>(&i2), sizeof(int));
	}
	fout.close();
}




void Context::draw(){
	
	for(int i=0;i<primitives.size();i++){
		
		
		primitives[i]->draw(primitive_scopes[i],texindexes[i],alphas[i],texscales[i]);
	}
	
}



GLuint Context::loadTexture(GLuint tex){
	
		

	texids.push_back(tex);
	return tex;
}



void Context::genPrimitives(){
Cube=new Primitive("Cube",0,false,true,false);
CubeX=new Primitive("CubeX",0,true,true,false);
CubeY=new Primitive("CubeY",0,false,true,true);
	//Cylinder=new Primitive("Cylinder",0,false,false,false);
	//Sphere=new Primitive("Sphere",0,false,false,false);
}

Context::Context()

{
	
	
	primitives.clear();
	texids.clear();
	texindexes.clear();
	
	if(current_scope==NULL)current_scope=new Scope();
    
}

Context::~Context()
{
	delete Cube;
	delete CubeX;
	delete CubeY;

		for(int i=0;i<primitive_scopes.size();i++){
		
		delete primitive_scopes[i];
	}

}
void Context::newScope(){
	
	
	scopes.push(current_scope);
	glm::vec3 pos=current_scope->getPosition();//preserve position not scale
	current_scope=new Scope();
	current_scope->setPosition(pos);
}
void Context::pushScope() 
{
    if(current_scope!=NULL){
		scopes.push(current_scope);
		current_scope=new Scope(current_scope);//copy consructor
	}
	else {
		current_scope=new Scope();
    }
}

Scope *Context::popScope() 

{

	if(current_scope!=NULL)delete current_scope;
    current_scope=scopes.top();
    scopes.pop();
    
    return current_scope;

}

Scope *Context::getCurrentScope()
{
    
    return current_scope;
}

Mesh &Context::getScene()
{
    return scene;
}
