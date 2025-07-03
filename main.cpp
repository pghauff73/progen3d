#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <list>
#include <string>
#include <unordered_map>
#include <stack>
#include <vector>
#include <filesystem>
#include <glob.h>
#include <random>


#include <stdio.h>
#include <bits/stdc++.h>
#include <termios.h>
#include <cstdlib>
#include <ctime>
#include <algorithm>



#include <epoxy/gl.h>
#include <GL/gl.h>
#include <GL/glu.h>

	

#include <stdio.h> /*standard i/o*/
#include <unistd.h> /*FOR USING FORK for at a time send and receive messages*/ 
#include <malloc.h> /*FOR MEMORY ALLOCATION */
#include <string.h> /*using fgets funtions for geting input from user*/
#include <sys/socket.h> /*for creating sockets*/
#include <resolv.h> /*server to find out the runner's IP address*/ 
#include <netdb.h> /*definitions for network database operations */
#include <openssl/ssl.h> /*using openssl function's and certificates and configuring them*/
#include <openssl/err.h> /* helps in finding out openssl errors*/
#include <unistd.h>  /*FOR USING FORK for at a time send and receive messages*/ 



#define FAIL    -1 /*for error output == -1 */
#define BUFFER  1024  /*buffer for reading messages*/




std::string hostname="xxx.xxx.xxx.xxx";
std::string portnum="pppp";

#include <gtk/gtk.h>


//#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"

#include "PLYWriter.h"
#include "Context.h"
#include "Scope.h"
#include "grammar.h"
#include "Solution.h"




#define STB_IMAGE_IMPLEMENTATION
#include "stbimage/stb_image.h"

//#include "hex_planet.h"
#include "textures.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


                                                   char appname[]="ProGen3d 0.95c";                //////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace cv;


std::mt19937_64 rng;





//===========================================================================================================================

//												 Globals 

//===========================================================================================================================

Grammar *grammar;

std::string username,password;


static int SCREEN_WIDTH=1600;
static int SCREEN_HEIGHT=1200;

std::string grammar_filename("./test.grammar");

extern std::vector<Variable *> variable_list;
extern std::vector<Variable *> full_variable_list;


static GtkWidget *window;
static GtkWidget *notebook,*layout,*drawing_area2,*layout3,*layout4;
static GtkWidget *colorchooser1;
static GtkWidget *gl_area;
static GtkTextBuffer *mybuffer,*errorbuffer;
static GtkWidget *view;
static GtkWidget *file_label;
static GtkWidget *scale_slider3,*scale_slider4;

char const * filterPatterns[1] = { "*.grammar"  };

float angle_view=0.0f;
float scale_global=0.3f;
float elevation_view=0.0f;

std::vector<int> texture_list;
std::vector<float> texture_list_alpha;
std::vector<std::string> texture_filenames;
bool loggedin=false;

bool play=true;

void display( void );
GtkWidget *labels[10000];
GtkWidget *ruleslabel[1000];
bool setup_textures=true;
bool setup_vars=true;
bool setup_rules=true;
GtkWidget *frames[140];
GtkBox *token_box[140];
GtkComboBoxText *combo1[140],*combo2[140],*combo3[140];
GtkComboBoxText *token_label[140];
GtkWidget *token_button[140];
GtkWidget *token_button_add[140];
GtkWidget *token_button_add_movedown[140];
GtkWidget *token_button_add_moveup[140];
int img_counter=0;
GtkWidget *button_remove_img[50];
GtkWidget *button_moveup_img[50];
GtkWidget *button_movedown_img[50];
GtkWidget *scale_slider_alpha[50];
GtkWidget *layout2;
GtkWidget *genlabel1;
GtkWidget *genlabel2;
GtkWidget *gencombo1;
GtkWidget *gencombo2;
GtkWidget *genlabelR1;
GtkWidget *gencomboR1;
GtkWidget *genlabelG1;
GtkWidget *gencomboG1;
GtkWidget *genlabelB1;
GtkWidget *gencomboB1;
 GtkWidget *entry1,*entry2;
 
 GtkWidget *box6,*box,*paned,*vpaned,*box3,*box4,*box5,*box8,*box7,*box9,*box10,*box11;
 
 GtkActionBar *actionbar,*actionbar3,*actionbar7,*actionbar5,*actionbar6;
   GtkWidget *runbutton,*newbutton;
  GtkWidget *savebutton;
  GtkWidget *plybutton;
  GtkWidget *playbutton3;
  GtkWidget *stopbutton3;
  GtkWidget *texture_filechooser;

 GtkActionBar *actionbar2,*actionbar4;
char *rulenames[1000];
GtkWidget *view2,*viewerror;

GtkTextBuffer *buffer2;

//GtkWidget *images[30];

std::vector<GtkWidget *> images;

int frame_counter=0;
bool init_buffer=false;
GtkAdjustment * vadj;

  GtkWidget *rulescombo;

GtkFileChooserNative *native;
GtkFileChooser *chooser;
void generate_widgets();
void activate_add(GtkWidget *item) ;

GdkPixbuf *pix=NULL;
std::vector<GdkPixbuf *> pixbuf;
std::vector<unsigned char *> pixsbuffer;


cv::Mat image;		
GtkWidget *drawing_area;
int mx[30],my[30],mouse_index=0,mousex,mousey;
Point p1,p2;
bool mouse_pressed=false;
std::vector<std::vector<Point>> squares;
std::vector<Point2f> perspective_points;
int draw2width,draw2height;
void txt_changed (GtkTextBuffer *textbuffer,gpointer       user_data);

using glm::mat4;
using glm::vec3;
using glm::lookAt;
using glm::perspective;
using glm::rotate;
using glm::translate;
using glm::scale;

const GLchar  *VERTEX_SOURCE =
"#version 330\n"
"in vec3 position;\n"
"in vec3 normal;\n"
"in vec2 texture;\n"
"out vec3 fN;\n"
"out vec3 fV;\n"
"out vec3 fL;\n"
"out vec2 tex_coord;\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"uniform vec3 lightposition;\n"
//"uniform vec3 pos;\n"
//"uniform vec3 scale_vec;\n"
"void main(){\n"
"       tex_coord.x=texture.x;	\n"
"       tex_coord.y=texture.y;	\n"
//"    fN = (model*view*vec4(normal,1.0)).xyz ;\n"
"    fN = (view*vec4(normal,1.0)).xyz ;\n"
//"    fV = - (model*view*vec4(position+pos, 1.0)).xyz;\n"
"    fV = - (view*vec4(position, 1.0)).xyz;\n"
//"    fL = lightposition.xyz - (model*view*vec4(position+pos, 1.0)).xyz ;\n"
"    fL = (view*vec4(lightposition-position, 1.0)).xyz ;\n"
//"    gl_Position = projection * view *(   (model * vec4(position+pos, 1.0)) );\n"
"    gl_Position = projection * view *model*vec4(position, 1.0);\n"
"}\n";






const GLchar *FRAGMENT_SOURCE2 =
"#version 330\n"
"in vec3 fN ;\n"
"in vec3 fL ;\n"
"in vec3 fV ;\n"
"in vec2 tex_coord;\n"
"uniform vec4 ambientproduct, diffuseproduct, specularproduct ;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform vec3 lightposition ;\n"
"uniform float shinyness ;\n"
"uniform sampler2D texture1;\n"
"uniform float alpha;\n"
"void main(){vec3 N = normalize(fN) ;\n"
"vec3 V = normalize(fV) ;\n"
"vec3 L = normalize(fL) ;\n"
"vec3 H = normalize( L + V ) ;\n"
"vec4 ambient = ambientproduct ;\n"
"float Kd = max(dot(L, N), 0.0) ;\n"
"vec4 diffuse = Kd*diffuseproduct ;\n"
"float Ks = pow(max(dot(N, H), 0.0), shinyness) ;\n"
"vec4 specular = Ks*specularproduct ;\n"
"if( dot(L, N) < 0.0 )specular = vec4(0.0, 0.0, 0.0, 1.0) ;\n"
//"if(alpha!=1.0)gl_FragColor = texture(texture1, tex_coord) ;\n"
" gl_FragColor = texture(texture1, tex_coord) *vec4((ambient.xyz + diffuse.xyz + specular.xyz),alpha);\n"
"}\n"; 


/* the GtkGLArea widget */


/* The object we are drawing */


long current_frame = 0.0;
long delta_time = 0.0;
GDateTime *last_frame;
int dt = 0;

static GLuint position_buffer,position_buffer1[50];
static GLuint program;
static GLuint vao,vao1[50];
int tex_count[50];






void errorout(std::string error_str){
	GtkTextMark* mark;
	GtkTextIter end;
	
	error_str+="\n";
	gtk_text_buffer_get_end_iter(errorbuffer,&end);
	gtk_text_buffer_insert(errorbuffer,&end,error_str.c_str(),error_str.length());
	
	
	gtk_text_buffer_get_end_iter(errorbuffer,&end);
	mark = gtk_text_buffer_create_mark    (errorbuffer, NULL, &end, 1);
   gtk_text_view_scroll_to_mark((GtkTextView *)viewerror, mark, 0.0, 0, 0.0, 1.0);
   gtk_text_buffer_delete_mark (errorbuffer, mark);
}

std::vector<std::string> breakup_into_lines(std::string input,std::string delimiter){
		std::vector<std::string> output;
	
		int pos=-1;
		
		while( (pos = input.find(delimiter))!=-1){
	
			std::string str=input.substr(0, pos);
			
			input.erase(0, pos + delimiter.length());
		
		    output.push_back(str);
		}
		output.push_back(input);
		
		
		
		return output;
}



mat4 model = mat4(1.0);

static const GLfloat vertex_data[] = {
  
  //Y
  
  1.0, -1.0, -1.0, 0.0, -1.0, 0.0, 1.0,-1.0,
    1.0, -1.0, 1.0, 0.0, -1.0, 0.0,  1.0, 1.0,
  -1.0, -1.0, 1.0, 0.0, -1.0, 0.0,-1.0, 1.0,
  
  
    -1.0, -1.0, 1.0, 0.0, -1.0, 0.0,-1.0, 1.0, 
  -1.0, -1.0,-1.0, 0.0, -1.0, 0.0,-1.0,-1.0,
  1.0, -1.0, -1.0, 0.0, -1.0, 0.0, 1.0,-1.0,
  

  -1.0, 1.0, 1.0, 0.0, 1.0, 0.0, -1.0,1.0,
   1.0, 1.0, 1.0, 0.0, 1.0, 0.0, 1.0,1.0,
   1.0, 1.0, -1.0, 0.0, 1.0, 0.0, 1.0,-1.0,
   
1.0, 1.0, -1.0, 0.0, 1.0, 0.0, 1.0,-1.0,
  -1.0, 1.0,-1.0, 0.0, 1.0, 0.0, -1.0,-1.0,
-1.0, 1.0, 1.0, 0.0, 1.0, 0.0, -1.0,1.0,  





 //X 
  -1.0, 1.0, -1.0, -1.0, 0.0, 0.0, -1.0,1.0,
  -1.0, -1.0,-1.0, -1.0, 0.0, 0.0, -1.0,-1.0,
  -1.0, -1.0, 1.0, -1.0, 0.0, 0.0, 1.0,-1.0,//cw
  
  
-1.0, -1.0, 1.0, -1.0, 0.0, 0.0, 1.0,-1.0,
  -1.0, 1.0, 1.0, -1.0, 0.0, 0.0, 1.0,1.0,
-1.0, 1.0, -1.0, -1.0, 0.0, 0.0, -1.0,1.0,//cw
  
  1.0, -1.0, 1.0,1.0, 0.0, 0.0, 1.0,-1.0,
  1.0, -1.0,-1.0,1.0, 0.0, 0.0, -1.0,-1.0,
  1.0, 1.0, -1.0,1.0, 0.0, 0.0,  -1.0,1.0,//ccw
  
      
  1.0, 1.0,-1.0 ,1.0, 0.0, 0.0, -1.0,1.0,
  1.0,  1.0, 1.0 ,1.0, 0.0, 0.0, 1.0,1.0,//ccw
 1.0, -1.0, 1.0 ,1.0, 0.0, 0.0,1.0,-1.0,

//Z
  
  
  
  -1.0, 1.0, 1.0, 0.0, 0.0, 1.0, -1.0,1.0,
  -1.0, -1.0,1.0, 0.0, 0.0, 1.0, -1.0,-1.0,
  1.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0,-1.0,

  
  
  1.0, -1.0, 1.0, 0.0, 0.0, 1.0, 1.0,-1.0,
  1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0,1.0,
  -1.0, 1.0, 1.0, 0.0, 0.0, 1.0, -1.0,1.0,
  
  
  
  -1.0, 1.0, -1.0, 0.0, 0.0, -1.0, -1.0, 1.0,
  1.0,  1.0, -1.0, 0.0, 0.0, -1.0,  1.0, 1.0,
  1.0, -1.0, -1.0, 0.0, 0.0, -1.0,  1.0,-1.0,
  
  
  1.0, -1.0, -1.0, 0.0, 0.0, -1.0,   1.0,-1.0,
  -1.0, -1.0, -1.0, 0.0, 0.0, -1.0, -1.0,-1.0,
  -1.0, 1.0, -1.0, 0.0, 0.0, -1.0,  -1.0,1.0
};



int OpenConnection(const char *hostname, int port)

{
	   int sd;
	struct hostent *host;
	struct sockaddr_in addr;   /*creating the sockets*/

	if ( (host = gethostbyname(hostname)) == NULL )
	{
		perror(hostname);
		abort();
	}
	sd = socket(PF_INET, SOCK_STREAM, 0);   /* setting the connection as tcp it creates endpoint for connection */
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = *(long*)(host->h_addr);
	if ( connect(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )  /*initiate a connection on a socket*/
	{
		close(sd);
		//perror(hostname);
		//abort();
		return -1;
	}
return sd;
}

SSL_CTX* InitCTX(void)     /*creating and setting up ssl context structure*/
{
    const SSL_METHOD *method;
	SSL_CTX *ctx;
	
	OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
	SSL_load_error_strings();   /* Bring in and register error messages */
	method = SSLv23_client_method();//TLSv1_2_client_method();  /* Create new client-method instance */
	ctx = SSL_CTX_new(method);   /* Create new context */
	if ( ctx == NULL )
	{
		//ERR_print_errors_fp(stderr);
		//abort();
		return NULL;
	}

return ctx;
}
void LoadCertificates(SSL_CTX* ctx, std::string CertFile, std::string KeyFile)   /* to load a certificate into an SSL_CTX structure*/
{	/* set the local certificate from CertFile */
	if ( SSL_CTX_use_certificate_file(ctx, CertFile.c_str(), SSL_FILETYPE_PEM) <= 0 )	{
		std::stringstream ss;
			ss<< "use certificate failed "<<CertFile<<std::endl;
			errorout(ss.str());
			//exit(0);
	}	
	/* set the private key from KeyFile (may be the same as CertFile) */
	//std::string passphrase="########";
	//SSL_CTX_set_default_passwd_cb_userdata(ctx,(void *)passphrase.c_str());
	//if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile.c_str(), SSL_FILETYPE_PEM) <= 0 )	{	
		//std::cout<< "use privatekey failed"<<std::endl;
	//exit(0);
	//}	/* verify private key */
	//if ( !SSL_CTX_check_private_key(ctx) )	{
	//		std::cout<< "Private key does not match the public certificate"<<std::endl;
	//	//	exit(0);
	
	//}

}
void ShowCerts(SSL* ssl)  /*show the ceritficates to server and match them but here we are not using any client certificate*/

{   X509 *cert;
	char *line;
	
	cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
	if ( cert != NULL )
	{	
		std::stringstream ss;
		ss<<"Server certificates:"<<std::endl;
		line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
		ss<<"Subject: "<<line<<std::endl;
		free(line);       /* free the malloc'ed string */
		line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
		ss<<"Issuer: "<<line<<std::endl;
		free(line);       /* free the malloc'ed string */
		X509_free(cert);     /* free the malloc'ed certificate copy */
		errorout(ss.str());
	}
	else
		errorout("Info: No client certificates configured.");

}


int generateTextureFromBuf(unsigned char *buf)
{
	GLuint mTexture;
    glGenTextures(1 , &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);// Bind our 2D texture so that following set up will be applied

    //Set texture wrapping parameter
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    //Set texture Filtering parameter
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);


    //Generate the image
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB , 128 , 128, 0, GL_RGB, GL_UNSIGNED_BYTE, buf);
    //glGenerateMipmap(GL_TEXTURE_2D);
    
    
    
    
    glBindTexture(GL_TEXTURE_2D,0); //Unbind 2D textures
    
    return mTexture;

}


int generateTexture(const char * filename)
{
	GLuint mTexture;
    glGenTextures(1 , &mTexture);
    glBindTexture(GL_TEXTURE_2D, mTexture);// Bind our 2D texture so that following set up will be applied

    //Set texture wrapping parameter
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

    //Set texture Filtering parameter
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

    //Load the image
    int picWidth,picHeight,n;
    unsigned char* image = stbi_load(filename, &picWidth, &picHeight, &n,STBI_rgb_alpha);
    if (image == NULL ) {
		std::stringstream ss;
        ss<<"Failed to load image: "<<stbi_failure_reason()<<std::endl;
        errorout(ss.str());
    }
    //Generate the image
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA , picWidth , picHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    //glGenerateMipmap(GL_TEXTURE_2D);
    
    //texture_data_list.push_back(image);
    
   // std::cout<<"Image START#####"<<picWidth<<":"<<picHeight<<":"<<picWidth*picHeight*3<<"#####################"<<std::endl;
   // for(int i=0;i<picWidth*picHeight*3;i++)std::cout<<(unsigned int)image[i]<<",";
   // std::cout<<"Image END##########################"<<std::endl;

    stbi_image_free(image);// Free the reference to the image
    glBindTexture(GL_TEXTURE_2D,0); //Unbind 2D textures
    
    return mTexture;

}



std::vector<std::string> globVector(const std::string& pattern){
	try{
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));
    
     int return_value,count=0;
    while((return_value=glob(pattern.c_str(),GLOB_TILDE,NULL,&glob_result))!=0){
		 
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		if(count++>30){
			throw(1);
			
		}
	}
    
    
    std::vector<std::string> files;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        files.push_back(std::string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    return files;
    
}catch(...){
	errorout("glob error");
	std::vector<std::string> out;
	return out;
}
}


void   setup(){
  
    
    errorout("setup");

    
   

	srand(time(NULL));
    //std::cout << std::fixed << std::showpoint;
	std::cout.precision(2);
	
    grammar=new Grammar(grammar_filename.c_str());
    
    
    
		grammar->addContext();
	
	
	errorout("Read files...");
	
	
if(loggedin==true){
	SSL_CTX *ctx;
	int client;
	SSL *ssl;
	char buf[65536];
	char input[BUFFER];
	int bytes;
	std::string writeout;
	

ctx = InitCTX();
LoadCertificates(ctx, "mycert.pem", "mycertkey.pem");
	client= OpenConnection(hostname.c_str(), atoi(portnum.c_str()));   /*converting ascii port to interger */
	ssl = SSL_new(ctx);      /* create new SSL connection state */
	SSL_set_fd(ssl, client);    /* attach the socket descriptor */
	if ( SSL_connect(ssl) != FAIL )   /* perform the connection */
	{    
	
		std::stringstream ss;
		ss<<"Connected with encryption "<< SSL_get_cipher(ssl)<<std::endl;
		errorout(ss.str());
		ShowCerts(ssl);
		/* get any certs */


	
			
			
			SSL_write(ssl, username.c_str(), strlen(username.c_str()));   /* encrypt & send message */
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			SSL_write(ssl, password.c_str(), strlen(password.c_str()));   /* encrypt & send message */
		
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			buf[bytes]=0;
			std::string OK=buf;
			//if(OK=="DOWN"){
				/*writeout="TEXTURE";
				std::cout<<writeout<<std::endl;
				SSL_write(ssl, writeout.c_str(), strlen(writeout.c_str()));
				img_counter=0;
				
				while(1){
					
					
					unsigned char ubuf[65536];
					int counter=0;
					
					int dec1,dec2;
					std::string bufferstr="";
					for(int i=0;i<20;i++){
					bytes = SSL_read(ssl, buf ,sizeof(buf));
						if(bytes>0){
							buf[bytes]=0;
						}
						bufferstr+=buf;
					}
					if(bufferstr=="OK" || bufferstr.size()<30 )break;
					
					std::vector<std::string> strdata=breakup_into_lines(bufferstr,",");
					for(int i=0;i<strdata.size();i++){
						//std::cout<<strdata[i]<<" ";
						ubuf[i]=(uint8_t)atoi(strdata[i].c_str());
					}
					if(strdata.size()<2)break;
					std::cout<<strdata.size()<<std::endl;
					
					int texid;
					
					if(strdata.size()>=128*128*3-15 && strdata.size()<=128*128*3+1 ){
						texid=generateTextureFromBuf((unsigned char *)ubuf);
						texture_list.push_back(texid);
						
						//cv::Mat newtexture(128,128, CV_8UC4);
						
						cv::Mat newtexture2(128,128,CV_8UC3);
						memcpy(newtexture2.data,ubuf,128*128*3);
						cv::cvtColor(newtexture2, newtexture2, cv::COLOR_BGR2RGB);
						
						
						//cv::imwrite(("texture"+std::to_string(img_counter)+".png").c_str(),newtexture);
						//images[img_counter]=gtk_image_new_from_file(("texture"+std::to_string(img_counter)+".png").c_str());
						pixsbuffer[img_counter]=(unsigned char *)malloc(128*128*3);
						memcpy(pixsbuffer[img_counter],ubuf,128*128*3);
						
						pixbufs[img_counter]=gdk_pixbuf_new_from_data((const guchar*)pixsbuffer[img_counter],GDK_COLORSPACE_RGB,FALSE,8,newtexture2.cols,newtexture2.rows,newtexture2.step ,NULL,NULL);
						img_counter++;
						gtk_widget_queue_draw_area(drawing_area2,0,0, draw2width,draw2height);
					}
					
				}*/
			//}
			 if(OK=="UP"){
								std::ifstream fin(grammar_filename.c_str());
								if (!fin.is_open()) {
									std::cout << "E: Could not open file " << grammar_filename << std::endl;
								}
								std::string fulltext;
								std::string line;
								while(std::getline(fin, line) ) {
								  fulltext+=line;
								}
								fin.close();
								SSL_write(ssl, fulltext.c_str(), strlen(fulltext.c_str())); 
								
							
					
			}
		
	}
	SSL_free(ssl);        /* release connection state */
	close(client);         /* close socket */
	SSL_CTX_free(ctx);        /* release */
	
}
else{
	std::vector<std::string> files = globVector("/snap/progen3d/current/textures/*.png");
	texture_filenames=files;
	std::stringstream ss;
	
	ss<<"files: "<<files.size()<<std::endl;
	errorout(ss.str());

	
	
	
	int texid;
	
	int counter=0;
    for (int i=0;i<files.size();i++){
		//std::cout<<counter;
		counter++;
		texid=generateTexture(files[i].c_str());
		texture_list.push_back(texid);
		texture_list_alpha.push_back(1.0);
		 GError* err = NULL;
		 cv::Mat newtexture = cv::imread(files[i].c_str(), cv::IMREAD_COLOR);
		 cv::resize(newtexture, newtexture,cv::Size(128, 128),0,0, cv::INTER_LINEAR);
			
		
			cv::cvtColor(newtexture, newtexture, cv::COLOR_BGR2RGB);
		pixsbuffer.push_back((unsigned char *)malloc(128*128*3));
		memcpy(pixsbuffer[pixsbuffer.size()-1],newtexture.data,128*128*3);
		pixbuf.push_back(gdk_pixbuf_new_from_data((const guchar*)pixsbuffer[pixsbuffer.size()-1],GDK_COLORSPACE_RGB,FALSE,8,newtexture.rows,newtexture.cols,newtexture.step,NULL,NULL));
		images.push_back(gtk_image_new_from_pixbuf(pixbuf[pixbuf.size()-1]));
		
	//pixbufs[img_counter] = gdk_pixbuf_new_from_file(files[i].c_str(), &err);
	img_counter++;
	    
	}
}
	/*std::vector<std::string> files2 = globVector("./*.jpg");
	
	
	std::cout<<"files: "<<files2.size()<<std::endl;
	
	
    for (int i=0;i<files2.size();i++){
		std::cout<<counter;
		counter++;
		GLuint texid=generateTexture(files2[i].c_str());
		texture_list.push_back(texid);
	
	}*/
	
	for(int i=0;i<texture_list.size();i++){
					grammar->context->loadTexture(texture_list[i]);
			
		}
		
	//back_texture=load_texture("paper.png");	
		errorout("generate primitives");
		grammar->context->genPrimitives();
		
		grammar->generateGeometry();
		errorout("Finished generating geometry...");


gtk_label_set_text((GtkLabel *)file_label,(grammar_filename+"    tokens: "+std::to_string(grammar->tokens_new.size())).c_str());

init_buffer=false;
txt_changed(mybuffer,NULL);

//grammar->context->getScene().calc_normals();
 float mydata[36*8];
    for(int i=0;i<36*8;i++){
		mydata[i]=vertex_data[i]; 
	}
	
	
	for(int i=0;i<36;i++){
		for(int j=0;j<8;j++){
			if(j==0 || j==2)mydata[i*8+j]*=0.5f;
			if(j==1){
				if(mydata[i*8+j]<0)mydata[i*8+j]=0.0f;
			}
		}
	}
	
	std::stringstream ss;
	
	ss<<"texture list size"<<texture_list.size()<<std::endl;
	errorout(ss.str());
	
	for(int i=0;i<texture_list.size();i++){
		
		float *my_vertex_data=grammar->context->calc(mydata,i);
		//tex_count[i]=texcount;
		
		  glGenVertexArrays (1, &vao1[i]);
		  glBindVertexArray (vao1[i]);

		  /* This is the buffer that holds the vertices */
		  glGenBuffers (1, &position_buffer1[i]);
		  glBindBuffer (GL_ARRAY_BUFFER, position_buffer1[i]);
		  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*36*8*tex_count[i],my_vertex_data,GL_DYNAMIC_DRAW);
		  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		  glEnableVertexAttribArray (0);
		  glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		  glEnableVertexAttribArray (1);
		   // texture coord attribute
		  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		  glEnableVertexAttribArray(2);
		  glBindBuffer (GL_ARRAY_BUFFER, 0);
		  delete my_vertex_data;
	  

	}



}
  
  
  


















/* Create and compile a shader */
static GLuint
create_shader (int  type)
{
  GLuint shader;
  int status;
  shader = glCreateShader (type);
  if (type== GL_FRAGMENT_SHADER){
    glShaderSource (shader, 1, &FRAGMENT_SOURCE2, NULL);
  }
  if (type== GL_VERTEX_SHADER){
	  //std::cout<<VERTEX_SOURCE<<std::endl;
    glShaderSource (shader, 1, &VERTEX_SOURCE, NULL);
  }
  glCompileShader (shader);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &status);
  if (status == GL_FALSE)
  {
    int log_len;
    char *buffer;
    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &log_len);
    buffer = (char*)g_malloc (log_len + 1);
    glGetShaderInfoLog (shader, log_len, NULL, buffer);
    g_warning ("Compile failure in %s shader:\n%s",
               type == GL_VERTEX_SHADER ? "vertex" : "fragment",
               buffer);
    g_free (buffer);
    glDeleteShader (shader);
    return 0;
  }

  return shader;
}

static void
realize (GtkWidget *widget)
{
  //GdkGLContext *context;
  gtk_gl_area_make_current (GTK_GL_AREA (widget));
  if (gtk_gl_area_get_error (GTK_GL_AREA (widget)) != NULL)
    return;
  //context = gtk_gl_area_get_context (GTK_GL_AREA (widget));
   gtk_gl_area_attach_buffers(GTK_GL_AREA (widget));
  gtk_gl_area_set_auto_render (GTK_GL_AREA (widget),TRUE);
  setup();
     
      std::string text="";
  for(int i=0;i<grammar->lines.size();i++)text+=grammar->lines[i]+"\n";
  

  gtk_text_buffer_set_text (mybuffer,text.c_str(),text.length());
     
    float mydata[36*8];
    for(int i=0;i<36*8;i++){
		mydata[i]=vertex_data[i]; 
	}
	
	
	for(int i=0;i<36;i++){
		for(int j=0;j<8;j++){
			if(j==0 || j==2)mydata[i*8+j]*=0.5f;
			if(j==1){
				if(mydata[i*8+j]<0)mydata[i*8+j]=0.0f;
			}
		}
	}
	
	
   
   /* We only use one VAO, so we always keep it bound */
  glGenVertexArrays (1, &vao);
  glBindVertexArray (vao);

  /* This is the buffer that holds the vertices */
  glGenBuffers (1, &position_buffer);
  glBindBuffer (GL_ARRAY_BUFFER, position_buffer);
  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*36*8,mydata,GL_STATIC_DRAW);
  glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray (0);
  glVertexAttribPointer (1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray (1);
   // texture coord attribute
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glBindBuffer (GL_ARRAY_BUFFER, 0);

  GLuint vertex, fragment;
  int status;
  vertex = create_shader (GL_VERTEX_SHADER);

  if (vertex == 0)
    {
      return;
    }

  fragment = create_shader (GL_FRAGMENT_SHADER);

  if (fragment == 0)
    {
      glDeleteShader (vertex);
      return;
    }

  program = glCreateProgram ();
  glAttachShader (program, vertex);
  glAttachShader (program, fragment);

  glLinkProgram (program);

  glGetProgramiv (program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
  {
    int log_len;
    char *buffer;

    glGetProgramiv (program, GL_INFO_LOG_LENGTH, &log_len);

    buffer = (char*)g_malloc (log_len + 1);
    glGetProgramInfoLog (program, log_len, NULL, buffer);

    g_warning ("Linking failure:\n%s", buffer);

    g_free (buffer);

    glDeleteProgram (program);
    program = 0;

    glDeleteShader (vertex);
    glDeleteShader (fragment);

    return;
  }

  glDetachShader (program, vertex);
  glDetachShader (program, fragment);

glEnable(GL_DEPTH_TEST);
//glClearDepth(1.0);
glDepthFunc(GL_LESS);
//glDisable(GL_CULL_FACE);
//glCullFace(GL_BACK_AND_FRONT);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);   
//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);



  
  last_frame = g_date_time_new_now_local();  
current_frame = g_date_time_get_microsecond(last_frame);
  
}
/* We should tear down the state when unrealizing */
static void
unrealize (GtkWidget *widget)
{
  gtk_gl_area_make_current (GTK_GL_AREA (widget));

  if (gtk_gl_area_get_error (GTK_GL_AREA (widget)) != NULL)
    return;
	for(int i=0;i<texture_list.size();i++){
		glDeleteBuffers (1, &position_buffer1[i]);
	}
  glDeleteBuffers (1, &position_buffer);
  glDeleteProgram (program);
}

int GLAREA_WIDTH;
int GLAREA_HEIGHT;


void
 resize_glarea (GtkGLArea *area,
               int        width,
               int        height,
               gpointer   user_data){
				   
				   GLAREA_WIDTH=width;
				GLAREA_HEIGHT=height;
				   
				   
			   }


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//                      CUBE


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void draw_box(float angle_cube,vec3 scale_vec,vec3 position_vec,vec3 pos,int tex_index,float texscale)
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_list[tex_index]);
  /* Use our shaders */
  glUseProgram (program);
  glUniform1i(glGetUniformLocation(program, "texture1"), 0);
 
 
 
 position_vec=vec3(position_vec.x*scale_global,position_vec.y*scale_global,position_vec.z*scale_global);
 
  model = glm::mat4(1.0);
  model = rotate(model, angle_cube, vec3(0,1,0));
  model = translate(model,position_vec);
  vec3 scale_vec2=vec3(scale_vec.x*scale_global,scale_vec.y*scale_global,scale_vec.z*scale_global);
  model = scale(model,scale_vec2);
  
  
  scale_vec=vec3(scale_vec.x*texscale,scale_vec.y*texscale,scale_vec.z*texscale);
  
  
  //vec3 pos(0,0,0);
  
  //model2 = rotate(model2, angle_view, vec3(0,1,0));
  //pos2=model2*pos2;
  //pos=vec3(pos2.x,pos2.y,pos2.z);
  
  
  glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);
  
  
 
  
  vec3 position = vec3(5.0*cos(angle_view*M_PI/180.0),0,5.0*sin(angle_view*M_PI/180.0));
  
  vec3 up = vec3(0,-1,0);
  mat4 view = lookAt(position, vec3(0.0,0.0,0.0), up);
  glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);
  
  mat4 projection = perspective(43.0, double(GLAREA_WIDTH)/double(GLAREA_HEIGHT), 0.01, 100.0);
  glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);
  
  vec3 lightposition=vec3(1.1,15.7,-1.3);
  glUniform3fv(glGetUniformLocation(program,"lightposition"),1,&lightposition[0]);
  
  //pos=position_vec;
  glUniform3fv(glGetUniformLocation(program,"pos"),1,&pos[0]);
  glUniform3fv(glGetUniformLocation(program,"scale_vec"),1,&scale_vec[0]);  
  
  glm::vec4 ambientproduct=glm::vec4(0.7,0.7,0.7,1.0);
  glUniform4fv(glGetUniformLocation(program,"ambientproduct"),1,&ambientproduct[0]);
  
  glm::vec4 diffuseproduct=glm::vec4(0.5,0.5,0.5,1.0);
  glUniform4fv(glGetUniformLocation(program,"diffuseproduct"),1,&diffuseproduct[0]);
  
  glm::vec4 specularproduct=glm::vec4(0.3,0.3,0.3,1.0);
  glUniform4fv(glGetUniformLocation(program,"specularproduct"),1,&specularproduct[0]);
  float shinyness=128.0f;
  glUniform1f(glGetUniformLocation(program,"shinyness"),shinyness);
  
  
  glBindVertexArray(vao);
  /* Use the vertices in our buffer */

  /* Draw the three vertices as a triangle */
  glDrawArrays (GL_TRIANGLES, 0, 36);

  /* We finished using the buffers and program */
  glBindVertexArray(0);
  glDisableVertexAttribArray (0);
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glUseProgram (0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//                      BUFFER


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void draw_buffer(float angle_cube,vec3 scale_vec,vec3 position_vec,vec3 pos,int tex_index,float texscale)
{
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_list[tex_index]);
  /* Use our shaders */
  glUseProgram (program);
  glUniform1i(glGetUniformLocation(program, "texture1"), 0);
 
 if(angle_cube==0){
	 glDepthFunc(GL_LESS);
 }
 else {
	 glDepthFunc(GL_GREATER);
 }
 
 position_vec=vec3(position_vec.x*scale_global,position_vec.y*scale_global,position_vec.z*scale_global);
 
  model = glm::mat4(1.0);
  //model = rotate(model, angle_cube, vec3(0,1,0));
  //model = translate(model,position_vec);
  vec3 scale_vec2=vec3(scale_vec.x*scale_global,scale_vec.y*scale_global,scale_vec.z*scale_global);
  model = scale(model,scale_vec2);
  
  
  scale_vec=vec3(scale_vec.x*texscale,scale_vec.y*texscale,scale_vec.z*texscale);
  
  
  //vec3 pos(0,0,0);
  
  //model2 = rotate(model2, angle_view, vec3(0,1,0));
  //pos2=model2*pos2;
  //pos=vec3(pos2.x,pos2.y,pos2.z);
  
  
  glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, &model[0][0]);
  
  
 
  
  vec3 position = vec3(5.0*cos(angle_view*M_PI/180.0),5.0*cos(elevation_view*M_PI/180.0),5.0*sin(angle_view*M_PI/180.0));
  
  vec3 up = vec3(0,-1,0);
  mat4 view = lookAt(position, vec3(0.0,0.0,0.0), up);
  glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, &view[0][0]);
  
  mat4 projection = perspective(43.0, double(GLAREA_WIDTH)/double(GLAREA_HEIGHT), 0.01, 100.0);
  glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, &projection[0][0]);
  
  vec3 lightposition=vec3(1.1*scale_global,-60.7*scale_global,-1.3*scale_global);
  glUniform3fv(glGetUniformLocation(program,"lightposition"),1,&lightposition[0]);
  
  //pos=position_vec;
  //glUniform3fv(glGetUniformLocation(program,"pos"),1,&pos[0]);
  //glUniform3fv(glGetUniformLocation(program,"scale_vec"),1,&scale_vec[0]);  
  
  glm::vec4 ambientproduct=glm::vec4(0.8,0.8,0.8,1.0);
  glUniform4fv(glGetUniformLocation(program,"ambientproduct"),1,&ambientproduct[0]);
  
  glm::vec4 diffuseproduct=glm::vec4(0.5,0.5,0.5,1.0);
  glUniform4fv(glGetUniformLocation(program,"diffuseproduct"),1,&diffuseproduct[0]);
  
  glm::vec4 specularproduct=glm::vec4(0.1,0.1,0.1,1.0);
  glUniform4fv(glGetUniformLocation(program,"specularproduct"),1,&specularproduct[0]);
  float shinyness=16.0f;
  glUniform1f(glGetUniformLocation(program,"shinyness"),shinyness);
  glUniform1f(glGetUniformLocation(program,"alpha"),texture_list_alpha[tex_index]);
  
  
  glBindVertexArray(vao1[tex_index]);
  /* Use the vertices in our buffer */

  /* Draw the three vertices as a triangle */
  glDrawArrays (GL_TRIANGLES, 0, 36*tex_count[tex_index]);

  /* We finished using the buffers and program */
  glBindVertexArray(0);
  glDisableVertexAttribArray (0);
  glBindBuffer (GL_ARRAY_BUFFER, 0);
  glUseProgram (0);
}






/////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//              RENDER


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
gboolean
render (GtkGLArea    *area,
        GdkGLContext *context)
{
gdk_gl_context_make_current (context);
GDateTime *date_time;



  date_time = g_date_time_new_now_local();  
  current_frame = g_date_time_get_microsecond(date_time);
  delta_time = g_date_time_difference(date_time, last_frame) / 1000;
  last_frame = date_time;

  if (gtk_gl_area_get_error (area) != NULL)
    return FALSE;
 if(play)angle_view+=(float)delta_time/1000.0;
  if(angle_view>360.0)angle_view=0.0;
  
  
  /* Clear the viewport */
  glClearColor (1.0, 1.0, 1.0, 1.0);
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  /* Draw our object */
  //draw_box (angle_view,vec3(0.5,1.0,0.5),vec3(0.0,0.0,0.0),0);
  //draw_box (delta_time,vec3(1.0,0.1,1.0),vec3(0.0,0.0,0.6),1);
  
  //grammar->context->draw();
  for(int i=0;i<texture_list.size();i++){
		if(tex_count[i]>0)draw_buffer(0,vec3(1,1,1),vec3(0,0,0),vec3(0,0,0),i,1);
	}
  
//display();
//glFlush ();
  gtk_gl_area_queue_render (area);
return TRUE;
}






////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//             MAIN



/////////////////////////////////////////////////////////////////////////////////////////////////////////////////





  void save_to_file(const char *filename){
   std::ofstream fout(filename);
    if (!fout.is_open()) {
        std::cerr << "E: Could not open file for saving" << filename << std::endl;
        return;
    }
    
    for(int i=0;i<grammar->lines.size();i++){
		fout<<grammar->lines[i]<<std::endl;
	}	
fout.close();
}








////////////////////////////////////////////////////////////////////////////////////////////////////////////////



void upload_fulltext(std::string fulltext){
SSL_CTX *ctx;
	int client;
	SSL *ssl;
	char buf[10024];
	char input[BUFFER];
	int bytes;


ctx = InitCTX();
LoadCertificates(ctx, "mycert.pem", "mycertkey.pem");
	client= OpenConnection(hostname.c_str(), atoi(portnum.c_str()));   /*converting ascii port to interger */
	ssl = SSL_new(ctx);      /* create new SSL connection state */
	SSL_set_fd(ssl, client);    /* attach the socket descriptor */
	if ( SSL_connect(ssl) != FAIL )   /* perform the connection */
	{    
	
		std::stringstream ss;
		ss<<"Connected with encryption "<< SSL_get_cipher(ssl)<<std::endl;
		errorout(ss.str());
		ShowCerts(ssl);
		/* get any certs */


		while(1){
			
			
			SSL_write(ssl, username.c_str(), strlen(username.c_str()));   /* encrypt & send message */
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			SSL_write(ssl, password.c_str(), strlen(password.c_str()));   /* encrypt & send message */
		
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			buf[bytes]=0;
			std::string OK=buf;
			
			if(OK=="UP" ) {
				SSL_write(ssl, fulltext.c_str(), strlen(fulltext.c_str())); 
				break;
			}
			else break;

		}
	}
	SSL_free(ssl);        /* release connection state */
	close(client);         /* close socket */
	SSL_CTX_free(ctx);        /* release */
	
}
void new_activate(GtkButton *item) {
	
	

   GList *children, *iter;

children = gtk_container_get_children(GTK_CONTAINER(layout3));
for(iter = children; iter != NULL; iter = g_list_next(iter))
 gtk_widget_destroy(GTK_WIDGET(iter->data));
g_list_free(children);


	
	gtk_text_buffer_set_text(mybuffer,"",0);
	
	 grammar->lines.clear();
	 grammar->rule_list.clear();
	 variable_list.clear();
 
 
      
 
	
	

setup_vars=true;
setup_rules=true;
	
	generate_widgets();
	
}

void activate(GtkButton *item) {
try{
   GList *children, *iter;

children = gtk_container_get_children(GTK_CONTAINER(layout3));
for(iter = children; iter != NULL; iter = g_list_next(iter))
gtk_widget_destroy(GTK_WIDGET(iter->data));
g_list_free(children);

	GtkTextIter start, end;
		gtk_text_buffer_get_start_iter(mybuffer, &start);
	gtk_text_buffer_get_end_iter(mybuffer, &end);
	
	std::string fulltext=(std::string)gtk_text_buffer_get_text(mybuffer, &start, &end, FALSE);
	
	if(loggedin==true)upload_fulltext(fulltext);
	 grammar->lines=breakup_into_lines(fulltext,"\n");
 
 
      grammar->Reread();    
    
    
		grammar->addContext();

 
	
	

setup_vars=true;
//setup_rules=true;
	
	generate_widgets();
	

		errorout("generate primitives");
		grammar->context->genPrimitives();
		
		grammar->generateGeometry();
		errorout("Finished generating geometry...");
		init_buffer=false;


/*
gtk_container_remove((GtkContainer *)actionbar2,rulescombo);
//g_free(rulescombo);
rulescombo=gtk_combo_box_text_new ();
gtk_action_bar_pack_start (actionbar2,rulescombo);
    gtk_widget_set_size_request((GtkWidget *)actionbar2, 950, 12);
g_signal_connect (rulescombo ,"changed", G_CALLBACK (activate_add), NULL);    
gtk_widget_show(rulescombo);
 */


/*
for(int i=0;i<grammar->rule_list.size();i++){
	
	std::string rule_str=grammar->rule_list[i]->rule_name;
	
	strcpy(rulenames[i],rule_str.c_str());
	gtk_combo_box_text_append_text((GtkComboBoxText *)rulescombo,rulenames[i]);
	
	
	}
*/
gtk_label_set_text((GtkLabel *)file_label,(grammar_filename+"    tokens: "+std::to_string(grammar->tokens_new.size())).c_str());
	//for(int i=0;i<texture_list.size();i++){
		//std::cout<<i<<texture_list[i]<<";";
		//glDeleteBuffers (1, &position_buffer1[i]);
	//}

		
	float mydata[36*8];
    for(int i=0;i<36*8;i++){
		mydata[i]=vertex_data[i]; 
	}
	
	
	for(int i=0;i<36;i++){
		for(int j=0;j<8;j++){
			if(j==0 || j==2)mydata[i*8+j]*=0.5f;
			if(j==1){
				if(mydata[i*8+j]<0)mydata[i*8+j]=0.0f;
			}
		}
	}
	for(int i=0;i<texture_list.size();i++){
		
		float *my_vertex_data=grammar->context->calc(mydata,i);
		//tex_count[i]=texcount;
		if(tex_count[i]>0){
		  //glGenVertexArrays (1, &vao1[i]);
		  glBindVertexArray (vao1[i]);

		  /* This is the buffer that holds the vertices */
		  //glGenBuffers (1, &position_buffer1[i]);
		  glBindBuffer (GL_ARRAY_BUFFER, position_buffer1[i]);
		  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*36*8*tex_count[i],my_vertex_data,GL_DYNAMIC_DRAW);
		  
		  delete my_vertex_data;
	  }

	}
	
	save_to_file(grammar_filename.c_str());

}catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(errorbuffer,&end);
	gtk_text_buffer_insert(errorbuffer,&end,error_str.c_str(),error_str.length());
}
}

   /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




void scalesliderupdate(GtkRange *range){
	
	
	scale_global=  gtk_range_get_value (range);
	
}
void scalesliderupdate3(GtkRange *range){
	
	
	angle_view=  gtk_range_get_value (range);
	
}
void scalesliderupdate4(GtkRange *range){
	
	
	elevation_view=  gtk_range_get_value (range);
	
}


void activate_moveup_alt_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}

						Token *temp=active_rule->alternate->section_tokens[1][framecounter];
						
						if(framecounter!=0){
							active_rule->alternate->section_tokens[1].erase(active_rule->alternate->section_tokens[1].begin()+framecounter);
							active_rule->alternate->section_tokens[1].insert(active_rule->alternate->section_tokens[1].begin()+framecounter-1,temp);
						activate_add(NULL);
					}

	
}
void activate_moveup_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
		int add_counter=0;

			for(int k=0;k<3;k++){
				
					add_counter++;
				
				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					if(framecounter==add_counter){
						Token *temp=active_rule->section_tokens[k][j];
						
						if(j!=0){
							active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
							active_rule->section_tokens[k].insert(active_rule->section_tokens[k].begin()+j-1,temp);
						}
						else {
							
							if(k!=0){
								active_rule->section_tokens[k-1].push_back(temp);
								active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
							}
						}
						activate_add(NULL);
						return;
						}
						add_counter++;
				}
			}
		
		
	
	
}
void activate_movedown_alt_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	
						Token *temp=active_rule->alternate->section_tokens[1][framecounter];
						if(framecounter==active_rule->alternate->section_tokens[1].size()-1){}
						else{
							active_rule->alternate->section_tokens[1].erase(active_rule->alternate->section_tokens[1].begin()+framecounter);
							active_rule->alternate->section_tokens[1].insert(active_rule->alternate->section_tokens[1].begin()+framecounter+1,temp);
						}
						
						activate_add(NULL);

}
void activate_movedown_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
		int add_counter=0;

			for(int k=0;k<3;k++){
				
					add_counter++;
				
				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==add_counter){
						Token *temp=active_rule->section_tokens[k][j];
						if(j==active_rule->section_tokens[k].size()-1){
							if(k!=2){
								active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
								active_rule->section_tokens[k+1].insert(active_rule->section_tokens[k+1].begin(),temp);
								}
							}
						else{
							active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
							active_rule->section_tokens[k].insert(active_rule->section_tokens[k].begin()+j+1,temp);
						}
						
						activate_add(NULL);
						return;
						}
						add_counter++;
				}
			}
		
		
	
	
}
void activate_add_alt_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	
						if(framecounter<=active_rule->alternate->section_tokens[1].size()){
							active_rule->alternate->section_tokens[1].insert(active_rule->alternate->section_tokens[1].begin()+framecounter,new Token(""));
						activate_add(NULL);
					}
}
void activate_add_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
		Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
		int add_counter=0;

			for(int k=0;k<3;k++){
				if(active_rule->section_tokens[k].size()==0){
					if(framecounter==add_counter){
						active_rule->section_tokens[k].push_back(new Token(""));
						activate_add(NULL);
						return;
						}
					
					
				}
				add_counter++;
				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==add_counter){
						active_rule->section_tokens[k].insert(active_rule->section_tokens[k].begin()+j+1,new Token(""));
						activate_add(NULL);
						return;
						}
					add_counter++;
				}
			}
		
		
	
	
}

void activate_remove_alt_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	
						active_rule->alternate->section_tokens[1].erase(active_rule->alternate->section_tokens[1].begin()+framecounter);
						activate_add(NULL);
					

}
void activate_remove_token(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int remove_counter=0;
			for(int k=0;k<3;k++){

					remove_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
							
					if(framecounter==remove_counter){
						active_rule->section_tokens[k].erase(active_rule->section_tokens[k].begin()+j);
						activate_add(NULL);
						return;
						}
				remove_counter++;
				}
			}
		
		
	
}
void activate_combo1_alt_changed(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}

						active_rule->alternate->section_tokens[1][framecounter]->token_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)item);
						//activate_add(NULL);

	
}
void activate_combo1_changed(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int add_counter=0;
			for(int k=0;k<3;k++){

					add_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
							
					if(framecounter==add_counter){
						active_rule->section_tokens[k][j]->token_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)item);
						//activate_add(NULL);
						return;
						}
				add_counter++;
				}
			}
		
		
	
}
void activate_combo1_alt_changed_var(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}

						active_rule->alternate->section_tokens[1][framecounter]->var_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)item);
						//activate_add(NULL);
	
}
void activate_combo1_changed_var(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int add_counter=0;
			for(int k=0;k<3;k++){

					add_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
							
					if(framecounter==add_counter){
						active_rule->section_tokens[k][j]->var_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)item);
						//activate_add(NULL);
						return;
						}
				add_counter++;
				}
			}
		
		
	
}
void activate_probcombo_changed(GtkWidget *item,int *user_data){
	int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
float prob=atof(gtk_combo_box_text_get_active_text((GtkComboBoxText *)item));

if(prob>0.0f && prob<1.0f){
	active_rule->probability=prob;
	active_rule->alternate=new Rule(active_rule->rule_name+"_alt",1);
	activate_add(NULL);
}
else active_rule->probability=1.0f;


}
void
activate_add_alt_changed (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(tokennamestr=="S" || tokennamestr=="I" || tokennamestr=="T" ){
							active_rule->alternate->section_tokens[1][framecounter]->token_name=tokennamestr;
							active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
							active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
							
							if(tokennamestr=="I")active_rule->alternate->section_tokens[1][framecounter]->addInstanceType("Cube");
							else active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
						}
						else if (tokennamestr=="R" || tokennamestr=="R*"){
							if(tokennamestr=="R*")active_rule->alternate->section_tokens[1][framecounter]->integer=true;
							active_rule->alternate->section_tokens[1][framecounter]->token_name="R";
							active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
							active_rule->alternate->section_tokens[1][framecounter]->addArgument(0.0);
						}
						else if(tokennamestr=="[" || tokennamestr=="]"){
							active_rule->alternate->section_tokens[1][framecounter]->token_name=tokennamestr;
						}
						else if(tokennamestr=="Rule"){
							active_rule->alternate->section_tokens[1][framecounter]->token_name="";
						}
						activate_add(NULL);
	
 }
 void
activate_changed1 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int change_counter=0;
			for(int k=0;k<3;k++){
					change_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==change_counter){
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[k][j]->var_names[0]!="")active_rule->section_tokens[k][j]->var_names[0]=tokennamestr;
						else if(active_rule->section_tokens[k][j]->arguments.size()>0)active_rule->section_tokens[k][j]->arguments[0]=atof(tokennamestr.c_str());
						
						
					}
				change_counter++;
				}
			}
}
 void
activate_changed2 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int change_counter=0;
			for(int k=0;k<3;k++){
					change_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==change_counter){
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[k][j]->var_names[1]!="")active_rule->section_tokens[k][j]->var_names[1]=tokennamestr;
						else if(active_rule->section_tokens[k][j]->arguments.size()>1)active_rule->section_tokens[k][j]->arguments[1]=atof(tokennamestr.c_str());
						
						
					}
				change_counter++;
				}
			}
}
 void
activate_changed3 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int change_counter=0;
			for(int k=0;k<3;k++){
					change_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==change_counter){
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[k][j]->var_names[2]!="")active_rule->section_tokens[k][j]->var_names[2]=tokennamestr;
						else if(active_rule->section_tokens[k][j]->arguments.size()>2)active_rule->section_tokens[k][j]->arguments[2]=atof(tokennamestr.c_str());
						
					}
				change_counter++;
				}
			}
}
void
activate_alt_changed1 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[1][framecounter]->var_names[0]!="")active_rule->section_tokens[1][framecounter]->var_names[0]=tokennamestr;
						else if(active_rule->section_tokens[1][framecounter]->arguments.size()>0)active_rule->section_tokens[1][framecounter]->arguments[0]=atof(tokennamestr.c_str());
						
}
void activate_alt_changed2 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[1][framecounter]->var_names[1]!="")active_rule->section_tokens[1][framecounter]->var_names[1]=tokennamestr;
						else if(active_rule->section_tokens[1][framecounter]->arguments.size()>1)active_rule->section_tokens[1][framecounter]->arguments[1]=atof(tokennamestr.c_str());
						
}
void activate_alt_changed3 (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(active_rule->section_tokens[1][framecounter]->var_names[2]!="")active_rule->section_tokens[1][framecounter]->var_names[2]=tokennamestr;
						else if(active_rule->section_tokens[1][framecounter]->arguments.size()>2)active_rule->section_tokens[1][framecounter]->arguments[2]=atof(tokennamestr.c_str());
						
}
void
activate_add_changed (GtkComboBox *widget,
               int *     user_data){
				   
			int framecounter=(*user_data);
	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	int remove_counter=0;
			for(int k=0;k<3;k++){
					remove_counter++;

				for(int j=0;j<active_rule->section_tokens[k].size();j++){
					
					if(framecounter==remove_counter){
						std::string tokennamestr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
						if(tokennamestr=="S" || tokennamestr=="I" || tokennamestr=="T" ){
							
							active_rule->section_tokens[k][j]->token_name=tokennamestr;
							float arg1=0.0f,arg2=0.0f,arg3=0.0f;
							if(tokennamestr=="S"){arg1=1.0f;arg2=1.0f;arg3=1.0f;}
							if(tokennamestr=="I"){arg1=0.0f;arg2=1.0f;arg3=0.125f;}
							
							active_rule->section_tokens[k][j]->addArgument(arg1);
							active_rule->section_tokens[k][j]->addArgument(arg2);
							active_rule->section_tokens[k][j]->addArgument(arg3);
							
							if(tokennamestr=="I")active_rule->section_tokens[k][j]->addInstanceType("Cube");
							
						}
						else if (tokennamestr=="R" || tokennamestr=="R*"){
							if(tokennamestr=="R*")active_rule->section_tokens[k][j]->integer=true;
							active_rule->section_tokens[k][j]->token_name="R";
							active_rule->section_tokens[k][j]->addArgument(0.0);
							active_rule->section_tokens[k][j]->addArgument(1.0);
						}
						else if(tokennamestr=="[" || tokennamestr=="]"){
							active_rule->section_tokens[k][j]->token_name=tokennamestr;
						}
						else if(tokennamestr=="Rule"){
							active_rule->section_tokens[k][j]->token_name="";
						}
						activate_add(NULL);
						return;
						}
				remove_counter++;		
				}
			}
		
		
		   
 }
 void
activate_vars_add (GtkComboBox *widget,
               int *     user_data){
				   int framecounter=(*user_data);
				   	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	std::string varstr=gtk_combo_box_text_get_active_text((GtkComboBoxText *)widget);
	bool vars_found=false;
	for(int i=0;i<active_rule->var_counter;i++){
		if(active_rule->var_names[i]==varstr)vars_found=true;
	}
				if(vars_found==false)active_rule->var_names[active_rule->var_counter++]=varstr;
				activate_add(NULL);
}
 void
activate_vars_remove (GtkComboBox *widget,
               int *     user_data){
				   int framecounter=(*user_data);
				   	//std::cout<<framecounter<<std::endl;
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	if(selected_rule_name=="")return;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				break;
		}
	}
	
	
	std::vector<std::string> vars_str;
	for(int i=0;i<active_rule->var_counter;i++){
			vars_str.push_back(active_rule->var_names[i]);
			active_rule->var_names[i]="";
	}
				vars_str.erase(vars_str.begin()+framecounter);
	for(int i=0;i<vars_str.size();i++){
			active_rule->var_names[i]=vars_str[i];
	}
	active_rule->var_counter--;
	activate_add(NULL);
}

void
 activate_buildrule(GtkComboBox *widget,
               int *     user_data){
				   try{
				   
			   
			   
				std::string full_text="";
			   for(int i=0;i<grammar->rule_list.size();i++)
				full_text+=grammar->rule_list[i]->print()+"\n";

			   gtk_text_buffer_set_text (mybuffer,full_text.c_str(),full_text.length());
			   init_buffer=false;
			   txt_changed (mybuffer,NULL);
			    
			    
			 //   grammar->Reread();    
    
    
		//grammar->addContext();

 
	
	

	//setup_vars=true;

	
	//generate_widgets();
	

	
		//grammar->context->genPrimitives();
		
	//	grammar->generateGeometry();
	
}catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(errorbuffer,&end);
	gtk_text_buffer_insert(errorbuffer,&end,error_str.c_str(),error_str.length());
}

}
 
 void
activate_addrule (GtkComboBox *widget,
               int *     user_data){
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				return;
		}
	} 
	
   grammar->rule_list.push_back(new Rule(selected_rule_name,1));
   gtk_combo_box_text_append_text((GtkComboBoxText *)rulescombo,selected_rule_name.c_str());
   activate_add(NULL);
}
GtkWidget *probcombo;
GtkWidget *problabel;
GtkWidget *varscombo[25];
GtkWidget *varslabel[25];
GtkWidget *vars_remove_button[25];
int vars_framecounter=0;
void activate_add(GtkWidget *item) {
	Rule *active_rule;
	std::string selected_rule_name=gtk_combo_box_text_get_active_text((GtkComboBoxText *)rulescombo);
	bool found_rule=false;
	for(int i=0;i<grammar->rule_list.size();i++){
		if(grammar->rule_list[i]->rule_name==selected_rule_name){
				active_rule=grammar->rule_list[i];
				found_rule=true;
				break;
		}
	}
	
	
	gtk_container_remove((GtkContainer *)layout,probcombo);
	gtk_container_remove((GtkContainer *)layout,problabel);
	for(int i=0;i<frame_counter;i++){
	gtk_container_remove((GtkContainer *)layout,(GtkWidget *)combo1[i]);
	gtk_container_remove((GtkContainer *)layout,(GtkWidget *)combo2[i]);
	gtk_container_remove((GtkContainer *)layout,(GtkWidget *)combo3[i]);
	gtk_container_remove((GtkContainer *)layout,(GtkWidget *)token_label[i]);
	gtk_container_remove((GtkContainer *)layout,(GtkWidget *)token_button[i]);
	gtk_container_remove((GtkContainer *)layout,(GtkWidget *)token_button_add[i]);
	gtk_container_remove((GtkContainer *)layout,(GtkWidget *)token_button_add_movedown[i]);
	gtk_container_remove((GtkContainer *)layout,(GtkWidget *)token_button_add_moveup[i]);
	}
	for(int i=0;i<vars_framecounter;i++){
		gtk_container_remove((GtkContainer *)layout,(GtkWidget *)varscombo[i]);
		gtk_container_remove((GtkContainer *)layout,(GtkWidget *)varslabel[i]);
		gtk_container_remove((GtkContainer *)layout,(GtkWidget *)vars_remove_button[i]);
	}
	if(found_rule==false)return;
    std::vector<std::string> token_names;
    token_names.push_back("A");
    token_names.push_back("D");
    token_names.push_back("T");
    token_names.push_back("S");
    token_names.push_back("R");
    token_names.push_back("R*");
    token_names.push_back("I");
    token_names.push_back("[");
    token_names.push_back("]");
    token_names.push_back("Rule");
    

	frame_counter=0;
	
	
	
	
	//std::cout<<"frames"<<std::endl;
	int token_counter=0;
	bool section_label=false;
	int *mypointer;
	int K=3;
	vars_framecounter=0;
	
	//std::cout<<active_rule->var_counter<<std::endl;
	//variables
	for(int i=0;i<active_rule->var_counter+1;i++){
		//std::cout<<i<<std::endl;
		varscombo[i]=gtk_combo_box_text_new_with_entry();
		varslabel[i]=gtk_combo_box_text_new();
		vars_remove_button[i]=gtk_button_new_with_label("X");
		gtk_combo_box_text_append_text((GtkComboBoxText *)varslabel[i],"Randomise");
		for(int j=0;j<variable_list.size();j++){
			gtk_combo_box_text_append_text((GtkComboBoxText *)varscombo[i],variable_list[j]->var_name.c_str());
			if(i!=active_rule->var_counter){
				if(active_rule->var_names[i]==variable_list[j]->var_name)gtk_combo_box_set_active((GtkComboBox *)varscombo[i],j);
			}
		}
		
		
		gtk_combo_box_set_active((GtkComboBox *)varslabel[i],0);
		gtk_widget_set_size_request(varslabel[i],60,20);
		gtk_layout_put ((GtkLayout *)layout,varslabel[i],20,30*token_counter);
		gtk_widget_set_size_request(varscombo[i],60,20);
		gtk_layout_put ((GtkLayout *)layout,varscombo[i],150,30*token_counter);
		gtk_widget_set_size_request(vars_remove_button[i],20,20);
		gtk_layout_put ((GtkLayout *)layout,vars_remove_button[i],350,30*token_counter);
		
		gtk_widget_show(varslabel[i]);
		gtk_widget_show(varscombo[i]);
		if(i!=active_rule->var_counter)gtk_widget_show(vars_remove_button[i]);
		mypointer=new int(i);
		if(i!=active_rule->var_counter)g_signal_connect (vars_remove_button[i],"clicked", G_CALLBACK (activate_vars_remove), mypointer); 
		else g_signal_connect (varscombo[i],"changed", G_CALLBACK (activate_vars_add), mypointer); 
		token_counter++;
		vars_framecounter++;
	}
	
	
	
	
	//probability
	mypointer=new int(0);
	probcombo=gtk_combo_box_text_new_with_entry();
	problabel=gtk_combo_box_text_new();
	gtk_combo_box_text_append_text((GtkComboBoxText *)problabel,"Probability");
	gtk_combo_box_text_append_text((GtkComboBoxText *)probcombo,std::to_string(active_rule->probability).c_str());
	
	gtk_combo_box_set_active((GtkComboBox *)probcombo,0);
	gtk_combo_box_set_active((GtkComboBox *)problabel,0);
	gtk_widget_set_size_request(problabel,60,20);
	gtk_layout_put ((GtkLayout *)layout,problabel,20,30*token_counter);
	gtk_widget_set_size_request(probcombo,60,20);
	gtk_layout_put ((GtkLayout *)layout,probcombo,200,30*token_counter);
	gtk_widget_show(problabel);
	gtk_widget_show(probcombo);
	g_signal_connect (probcombo,"changed", G_CALLBACK (activate_probcombo_changed), mypointer); 
	token_counter++;
	
	if(active_rule->probability<1.0f){
		K=4;
	}
	
	
	
	for(int k=0;k<K;k++){
		
		//section labels
		if(k==0){token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();gtk_combo_box_text_append_text(token_label[frame_counter],"Start Section");gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],0);}
		else if(k==1){token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();gtk_combo_box_text_append_text(token_label[frame_counter],"Repeat Section");gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],0);}
		else if(k==2){token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();gtk_combo_box_text_append_text(token_label[frame_counter],"End Section");gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],0);}
		else if(k==3){token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();gtk_combo_box_text_append_text(token_label[frame_counter],"Alt Rule");gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],0);}
		gtk_widget_set_size_request((GtkWidget *)token_label[frame_counter],60,20);
		gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)token_label[frame_counter],20,30*token_counter);
		
		if(k==1){
			combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
			if(active_rule->var_name=="")gtk_combo_box_text_append_text(combo1[frame_counter],std::to_string(active_rule->repeat).c_str());
			else gtk_combo_box_text_append_text(combo1[frame_counter],active_rule->var_name.c_str());
			gtk_widget_set_size_request((GtkWidget *)combo1[frame_counter],60,20);
			gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)combo1[frame_counter],200,30*token_counter);
			gtk_combo_box_set_active((GtkComboBox *)combo1[frame_counter],0);
		gtk_widget_show((GtkWidget *)combo1[frame_counter]);	
		}
		
		combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
		combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
		token_button[frame_counter]=gtk_button_new_with_label("");
		token_button_add[frame_counter]=gtk_button_new_with_label("+");
		token_button_add_moveup[frame_counter]=gtk_button_new_with_label("");
		token_button_add_movedown[frame_counter]=gtk_button_new_with_label("");
		gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)token_button_add[frame_counter],400,30*token_counter);
		
		if(k!=3){mypointer=new int(frame_counter);g_signal_connect (token_button_add[frame_counter] ,"clicked", G_CALLBACK (activate_add_token), mypointer); }
		else {mypointer=new int(0);g_signal_connect (token_button_add[frame_counter] ,"clicked", G_CALLBACK (activate_add_alt_token), mypointer); }
		gtk_widget_show(token_button_add[frame_counter]);
		gtk_widget_show((GtkWidget *)token_label[frame_counter]);
		frame_counter++;
		token_counter++;
		
		std::vector<Token *> section_tokens;
		if(k!=3)section_tokens=active_rule->section_tokens[k];
		else section_tokens=active_rule->alternate->section_tokens[1];
		for(int i=0;i<section_tokens.size();i++){
			  //std::cout<<section_tokens[i]->token_name.c_str()<<std::endl;
			  std::string strn=section_tokens[i]->token_name;
				if(k!=3)mypointer=new int(frame_counter);
				else mypointer=new int(i);
				token_label[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();
				
				
				if( strn=="A" ||strn=="D" || strn=="R" || strn=="S" || strn=="T" || strn=="I" || strn=="[" || strn=="]" ){
				
					if(strn=="R"){
						if(section_tokens[i]->integer==true)strn="R*";
						if(k!=3)g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_combo1_changed_var), mypointer); 
						else g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_combo1_alt_changed_var), mypointer); 
				  
					}
				
					for(int j=0;j<token_names.size();j++){
						gtk_combo_box_text_append_text(token_label[frame_counter],token_names[j].c_str());
						if(strn==token_names[j]){
								gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],j);
						}
					}
					if(strn=="[" || strn=="]"){
						combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new();
						combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
						combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
						
					}
					
				}
				else {
				combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry();
				combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					
					for(int j=0;j<token_names.size();j++){
						gtk_combo_box_text_append_text(token_label[frame_counter],token_names[j].c_str());
						if("Rule"==token_names[j]){
								gtk_combo_box_set_active((GtkComboBox *)token_label[frame_counter],j);
						}
					}
					gtk_combo_box_text_append_text(combo1[frame_counter],strn.c_str());
					for(int j=0;j<grammar->rule_list.size();j++){
						if(strn!=grammar->rule_list[j]->rule_name)gtk_combo_box_text_append_text(combo1[frame_counter],grammar->rule_list[j]->rule_name.c_str());
					}
					if(k!=3)g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_combo1_changed), mypointer); 
					else g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_combo1_alt_changed), mypointer); 
					
				}
	
				
				if(  strn=="A" || strn=="D" || strn=="S" || strn=="T"  ){
					combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				
					if(section_tokens[i]->var_names[0]!="")gtk_combo_box_text_append_text(combo1[frame_counter],(section_tokens[i]->var_names[0]).c_str());
					else if(section_tokens[i]->arguments.size()>0)gtk_combo_box_text_append_text(combo1[frame_counter],std::to_string(section_tokens[i]->arguments[0]).c_str());
					
					if(section_tokens[i]->var_names[1]!="")gtk_combo_box_text_append_text(combo2[frame_counter],(section_tokens[i]->var_names[1]).c_str());
					else if(section_tokens[i]->arguments.size()>1)gtk_combo_box_text_append_text(combo2[frame_counter],std::to_string(section_tokens[i]->arguments[1]).c_str());
					
					if(section_tokens[i]->var_names[2]!="")gtk_combo_box_text_append_text(combo3[frame_counter],(section_tokens[i]->var_names[2]).c_str());
					else if(section_tokens[i]->arguments.size()>2)gtk_combo_box_text_append_text(combo3[frame_counter],std::to_string(section_tokens[i]->arguments[2]).c_str());
				}
				else if( strn=="I" ){
					combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
					combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				
					
					gtk_combo_box_text_append_text(combo1[frame_counter],(section_tokens[i]->instance_type).c_str());
					
					if(section_tokens[i]->var_names[0]!="")gtk_combo_box_text_append_text(combo2[frame_counter],(section_tokens[i]->var_names[0]).c_str());
					else if(section_tokens[i]->arguments.size()>0)gtk_combo_box_text_append_text(combo2[frame_counter],std::to_string(section_tokens[i]->arguments[0]).c_str());
					
					if(section_tokens[i]->var_names[1]!="")gtk_combo_box_text_append_text(combo3[frame_counter],(section_tokens[i]->var_names[1]).c_str());
					else if(section_tokens[i]->arguments.size()>1)gtk_combo_box_text_append_text(combo3[frame_counter],std::to_string(section_tokens[i]->arguments[1]).c_str());
					
					
				}
				else if( strn=="R" || strn=="R*"){
					combo1[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry();
				combo2[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				combo3[frame_counter]=(GtkComboBoxText *)gtk_combo_box_text_new_with_entry ();
				
					gtk_combo_box_text_append_text(combo1[frame_counter],(section_tokens[i]->var_name).c_str());
					
					if(section_tokens[i]->var_names[0]!="")gtk_combo_box_text_append_text(combo2[frame_counter],(section_tokens[i]->var_names[0]).c_str());
					else if(section_tokens[i]->arguments.size()>0)gtk_combo_box_text_append_text(combo2[frame_counter],std::to_string(section_tokens[i]->arguments[0]).c_str());
					
					if(section_tokens[i]->var_names[1]!="")gtk_combo_box_text_append_text(combo3[frame_counter],(section_tokens[i]->var_names[1]).c_str());
					else if(section_tokens[i]->arguments.size()>1)gtk_combo_box_text_append_text(combo3[frame_counter],std::to_string(section_tokens[i]->arguments[1]).c_str());
				}
				
				gtk_widget_set_size_request((GtkWidget *)token_label[frame_counter],60,20);
				gtk_widget_set_size_request((GtkWidget *)combo1[frame_counter],60,20);
				gtk_widget_set_size_request((GtkWidget *)combo2[frame_counter],60,20);
				gtk_widget_set_size_request((GtkWidget *)combo3[frame_counter],60,20);		
				token_button[frame_counter]=gtk_button_new_with_label("X");
				gtk_widget_set_size_request((GtkWidget *)token_button[frame_counter],20,20);
				
				gtk_combo_box_set_active((GtkComboBox *)combo1[frame_counter],0);
				gtk_combo_box_set_active((GtkComboBox *)combo2[frame_counter],0);
				gtk_combo_box_set_active((GtkComboBox *)combo3[frame_counter],0);
				
				gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)token_label[frame_counter],20,30*token_counter);
				gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)combo1[frame_counter],100,30*token_counter);
				gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)combo2[frame_counter],300,30*token_counter);
				gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)combo3[frame_counter],500,30*token_counter);
				gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)token_button[frame_counter],700,30*token_counter);
				
				
				token_button_add[frame_counter]=gtk_button_new_with_label("+");
				gtk_widget_set_size_request((GtkWidget *)token_button_add[frame_counter],20,20);
				token_button_add_moveup[frame_counter]=gtk_button_new_with_label("\u25b2");
				gtk_widget_set_size_request((GtkWidget *)token_button_add_moveup[frame_counter],20,20);
				token_button_add_movedown[frame_counter]=gtk_button_new_with_label("\u25bc");
				gtk_widget_set_size_request((GtkWidget *)token_button_add_movedown[frame_counter],20,20);
				
				gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)token_button_add[frame_counter],750,30*token_counter);
				gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)token_button_add_moveup[frame_counter],800,30*token_counter);
				gtk_layout_put ((GtkLayout *)layout,(GtkWidget *)token_button_add_movedown[frame_counter],850,30*token_counter);
				
				gtk_widget_show(token_button_add[frame_counter]);
				gtk_widget_show(token_button_add_moveup[frame_counter]);
				gtk_widget_show(token_button_add_movedown[frame_counter]);
				
				gtk_widget_show((GtkWidget *)token_label[frame_counter]);
				gtk_widget_show((GtkWidget *)combo1[frame_counter]);
				gtk_widget_show((GtkWidget *)combo2[frame_counter]);
				gtk_widget_show((GtkWidget *)combo3[frame_counter]);
				gtk_widget_show((GtkWidget *)token_button[frame_counter]);
				
				if(k!=3){
					g_signal_connect (token_button_add[frame_counter] ,"clicked", G_CALLBACK (activate_add_token), mypointer); 
					g_signal_connect (token_button_add_moveup[frame_counter] ,"clicked", G_CALLBACK (activate_moveup_token), mypointer); 
					g_signal_connect (token_button_add_movedown[frame_counter] ,"clicked", G_CALLBACK (activate_movedown_token), mypointer); 
					g_signal_connect (token_label[frame_counter] ,"changed", G_CALLBACK (activate_add_changed), mypointer); 
					g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_changed1), mypointer); 
					g_signal_connect (combo2[frame_counter] ,"changed", G_CALLBACK (activate_changed2), mypointer); 
					g_signal_connect (combo3[frame_counter] ,"changed", G_CALLBACK (activate_changed3), mypointer); 
					g_signal_connect (token_button[frame_counter] ,"clicked", G_CALLBACK (activate_remove_token), mypointer); 
				}
				else {
					g_signal_connect (token_label[frame_counter] ,"changed", G_CALLBACK (activate_add_alt_changed), mypointer); 
					g_signal_connect (combo1[frame_counter] ,"changed", G_CALLBACK (activate_alt_changed1), mypointer); 
					g_signal_connect (combo2[frame_counter] ,"changed", G_CALLBACK (activate_alt_changed2), mypointer); 
					g_signal_connect (combo3[frame_counter] ,"changed", G_CALLBACK (activate_alt_changed3), mypointer); 
					
					g_signal_connect (token_button_add_moveup[frame_counter] ,"clicked", G_CALLBACK (activate_moveup_alt_token), mypointer); 
					g_signal_connect (token_button_add_movedown[frame_counter] ,"clicked", G_CALLBACK (activate_movedown_alt_token), mypointer); 
					g_signal_connect (token_button[frame_counter] ,"clicked", G_CALLBACK (activate_remove_alt_token), mypointer); 
					mypointer=new int(i+1);
					g_signal_connect (token_button_add[frame_counter] ,"clicked", G_CALLBACK (activate_add_alt_token), mypointer); 
					}
			
				frame_counter++;
				token_counter++;
				//std::cout<<frame_counter<<std::endl;
		}
		
	}
		
	guint layout_width,layout_height;
	gtk_layout_get_size ((GtkLayout *)layout,&layout_width,&layout_height);
	gtk_layout_set_size ((GtkLayout *)layout,layout_width,(token_counter+1)*30);
	//gtk_entry_set_text(entry1,std::string(active_rule->repeat).c_str())
	
	
	//GtkWidget *label2=gtk_label_new("StartSection:");
	//GtkWidget *textview1=gtk_text_view_new();
	//GtkWidget *scrollwin=gtk_scrolled_window_new (NULL,NULL);
	
	
	//GtkTextBuffer *start_section_text=gtk_text_view_get_buffer (textview1);
	//gtk_text_buffer_set_text(start_section_text,active->)
	
	
	//gtk_widget_set_size_request(entry1,60,20);
	//gtk_widget_set_size_request(scrollwin,260,180);
	
	//gtk_container_add (GTK_CONTAINER (scrollwin), textview1);
	//gtk_text_view_set_wrap_mode((GtkTextView *)textview1,GTK_WRAP_WORD);
	
	
	//gtk_layout_put ((GtkLayout *)layout,scrollwin,80,80);
	//gtk_layout_put ((GtkLayout *)layout,label2,20,80);
	
	//gtk_layout_put ((GtkLayout *)layout,entry1,80,40);
	//gtk_layout_put ((GtkLayout *)layout,label,20,50);
	
	
	
	
	
	
	//GtkWidget *myimage=gtk_image_new_from_file("129_concrete brick wall texture-seamless.png");
	//gtk_layout_put ((GtkLayout *)layout,myimage,80,130);
	//gtk_widget_show (myimage);
	
	
	
	//gtk_widget_show (scrollwin);
	//gtk_widget_show (textview1);
	//gtk_widget_show (entry1);
	//gtk_widget_show (label);
	 //gtk_widget_set_size_request (frames[frame_counter-1], 220, 175);
	//gtk_widget_show(frames[frame_counter-1]);
	
	gtk_widget_show (layout);
}
void activate_scale_slider_alpha(GtkRange *range,int *data){
	int index=(*data);
	
	texture_list_alpha[index]=gtk_range_get_value (range);
	
	
}


 void
activate_button_movedown_img (GtkComboBox *widget,
               int *     user_data){
int img_index=(*user_data);
if(img_index==images.size()-1)return;
	GtkWidget *img=images[img_index];
	images.erase(images.begin()+img_index);
	images.insert(images.begin()+img_index+1,img);
	int tex=texture_list[img_index];
	texture_list.erase(texture_list.begin()+img_index);
	texture_list.insert(texture_list.begin()+img_index+1,tex);
	int *mypointer;
	for(int i=0;i<img_counter;i++){
		
		int j=i%7;
		int k=i/7;
		gtk_layout_move ((GtkLayout *)layout2,images[i],80+k*230,130+j*135);
		
		gtk_range_set_value((GtkRange *)scale_slider_alpha[i],texture_list_alpha[i]);
		
	
	}	
	
}


 void
activate_button_moveup_img (GtkComboBox *widget,
               int *     user_data){
int img_index=(*user_data);
if(img_index==0)return;
	GtkWidget *img=images[img_index];
	images.erase(images.begin()+img_index);
	images.insert(images.begin()+img_index-1,img);
	int tex=texture_list[img_index];
	texture_list.erase(texture_list.begin()+img_index);
	texture_list.insert(texture_list.begin()+img_index-1,tex);
	int *mypointer;
	for(int i=0;i<img_counter;i++){
		
		int j=i%7;
		int k=i/7;
		gtk_layout_move ((GtkLayout *)layout2,images[i],80+k*230,130+j*135);
		
		gtk_range_set_value((GtkRange *)scale_slider_alpha[i],texture_list_alpha[i]);
		
	}
}

 void
activate_button_remove_img (GtkComboBox *widget,
               int *     user_data){
int img_index=(*user_data);
	//for(int i=img_index;i<img_counter;i++){
		
		//
	//}
	gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)button_remove_img[img_counter-1]);
	gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)button_movedown_img[img_counter-1]);
	gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)button_moveup_img[img_counter-1]);
	gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)scale_slider_alpha[img_counter-1]);
	gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)images[img_index]);
	
	//std::cout<<img_index<<std::endl;
	images.erase(images.begin()+img_index);
	texture_list.erase(texture_list.begin()+img_index);
	texture_list_alpha.erase(texture_list_alpha.begin()+img_index);
	int *mypointer;
	img_counter--;
	
	
	for(int i=0;i<img_counter;i++){
		
		int j=i%7;
		int k=i/7;
		gtk_layout_move ((GtkLayout *)layout2,images[i],80+k*230,130+j*135);
		
		gtk_range_set_value((GtkRange *)scale_slider_alpha[i],texture_list_alpha[i]);
		
	}
}





void generate_widgets(){
	try{
if(setup_textures && loggedin==false){				 
	errorout("setup textures");  
	
	
	int *mypointer;
	
	for(int i=0;i<img_counter;i++){
		//images[i]=gtk_image_new_from_file(texture_filenames[i].c_str());
		int j=i%7;
		int k=i/7;
		button_remove_img[i]=gtk_button_new_with_label("X");
		button_moveup_img[i]=gtk_button_new_with_label("\u25b2");
		button_movedown_img[i]=gtk_button_new_with_label("\u25bc");
		scale_slider_alpha[i]=gtk_scale_new_with_range (GTK_ORIENTATION_VERTICAL,0.025,1.0,0.025);
        gtk_widget_set_size_request(scale_slider_alpha[i],20,120);
		gtk_widget_set_size_request((GtkWidget *)button_remove_img[i],20,20);
		gtk_widget_set_size_request((GtkWidget *)button_moveup_img[i],20,20);
		gtk_widget_set_size_request((GtkWidget *)button_movedown_img[i],20,20);
		gtk_layout_put ((GtkLayout *)layout2,images[i],80+k*230,130+j*135);
		gtk_layout_put ((GtkLayout *)layout2,scale_slider_alpha[i],210+k*230,130+j*135);
		gtk_layout_put ((GtkLayout *)layout2,button_remove_img[i],250+k*230,130+j*135);
		gtk_layout_put ((GtkLayout *)layout2,button_moveup_img[i],250+k*230,130+j*135+30);
		gtk_layout_put ((GtkLayout *)layout2,button_movedown_img[i],250+k*230,130+j*135+60);
		gtk_widget_show (images[i]);
		gtk_widget_show (scale_slider_alpha[i]);
		gtk_widget_show (button_remove_img[i]);
		gtk_widget_show (button_movedown_img[i]);
		gtk_widget_show (button_moveup_img[i]);
		gtk_range_set_value((GtkRange *)scale_slider_alpha[i],texture_list_alpha[i]);
		mypointer=new int(i);
		g_signal_connect (button_remove_img[i] ,"clicked", G_CALLBACK (activate_button_remove_img), mypointer); 
		g_signal_connect (button_movedown_img[i] ,"clicked", G_CALLBACK (activate_button_movedown_img), mypointer); 
		g_signal_connect (button_moveup_img[i] ,"clicked", G_CALLBACK (activate_button_moveup_img), mypointer); 
		g_signal_connect (scale_slider_alpha[i] ,"value-changed", G_CALLBACK (activate_scale_slider_alpha), mypointer); 
	
	}
	setup_textures=false;  
		
}
else if(setup_textures && loggedin==true){				 
	
	setup_textures=false;  
}
if(setup_vars){				   
	std::stringstream ss;
	ss<<"setup variables"<<std::endl;  
	ss<<full_variable_list.size()<<std::endl;
	errorout(ss.str());
for(int i=0;i<full_variable_list.size();i++){
		labels[i]=gtk_label_new ((full_variable_list[i]->var_name+":="+std::to_string(full_variable_list[i]->value)).c_str());
		gtk_layout_put ((GtkLayout *)layout3,(GtkWidget *)labels[i],80,i*20);
		gtk_widget_show ((GtkWidget *)labels[i]);
	
	}
	guint layout_width,layout_height;
	gtk_layout_get_size ((GtkLayout *)layout3,&layout_width,&layout_height);
	gtk_layout_set_size ((GtkLayout *)layout3,layout_width,full_variable_list.size()*20);
	gtk_adjustment_configure (vadj,0,0,full_variable_list.size()*20,20,layout_height,layout_height);
	setup_vars=false;
}
if(setup_rules){				
	errorout("setup rules");     
for(int i=0;i<grammar->rule_list.size();i++){
	
	std::string rule_str=grammar->rule_list[i]->rule_name;
	
	strcpy(rulenames[i],rule_str.c_str());
	gtk_combo_box_text_append_text((GtkComboBoxText *)rulescombo,rulenames[i]);
	
	
	}
	setup_rules=false;
}
}catch(...){
	
	std::string error_str="Setup Error "+std::to_string(22);
	errorout(error_str);
}
}


gboolean
layout_focus (GtkWidget       *widget,
               GtkDirectionType direction,
               gpointer         user_data){
generate_widgets();
return true;
}

gboolean
rule_focus (GtkWidget       *widget,
               GtkDirectionType direction,
               gpointer         user_data){
generate_widgets();
return true;
}

gboolean
vars_focus (GtkWidget       *widget,
               GtkDirectionType direction,
               gpointer         user_data){
generate_widgets();
return true;
}


gboolean
texture_focus (GtkWidget       *widget,
               GtkDirectionType direction,
               gpointer         user_data){
generate_widgets();
return true;				   
}



void activate_stop(GtkButton *item) {
	
	play=false;
	
}

void activate_play(GtkButton *item) {
	
	play=true;
	
}


			   
void activate_saveas(GtkButton *item) {
	
 try{
    GtkFileChooserNative *native;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;
     GtkFileFilter *filter;

    native = gtk_file_chooser_native_new("Save File",
                                         (GtkWindow *)window,
                                         action,
                                         "_Save",
                                         "_Cancel");

    // Filters
    filter = gtk_file_filter_new(); // filter 1
    gtk_file_filter_set_name(filter, "Grammar file");
    gtk_file_filter_add_pattern(filter, "*.grammar");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);
    filter = gtk_file_filter_new(); // filter 2
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);

    // default file name
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), "test.grammar");

    res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        grammar_filename = std::string(gtk_file_chooser_get_filename(chooser));

        // save the file
        save_to_file(grammar_filename.c_str());

activate(NULL);
        //g_free(grammar_filename);
    }

    g_object_unref(native);
}catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	errorout(error_str);
}
}


	   
void activate_choose_image_button(GtkButton *item) {
	
 
    GtkFileChooserNative *native;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;
     GtkFileFilter *filter;

    native = gtk_file_chooser_native_new("Open Image File",
                                         (GtkWindow *)window,
                                         action,
                                         "_Open",
                                         "_Cancel");

    // Filters
    filter = gtk_file_filter_new(); // filter 1
    gtk_file_filter_set_name(filter, "Image file");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);
    filter = gtk_file_filter_new(); // filter 2
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);

    // default file name
    //gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), "test.grammar");

    res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        std::string image_filename = std::string(gtk_file_chooser_get_filename(chooser));

        
		GError *err = NULL;
			/* Create pixbuf */
			if(pix!=NULL)g_object_unref(pix);
			
			image = cv::imread(image_filename.c_str(), cv::IMREAD_COLOR);
			
			//pix = gdk_pixbuf_new_from_file(image_filename.c_str(), &err);
			if(err)
			{
				printf("Error : %s\n", err->message);
				g_error_free(err);
			pix=NULL;
			}
	
        //g_free(grammar_filename);
    }

    g_object_unref(native);

}
GtkTextTag *tag=NULL,*tag2=NULL,*boldtag=NULL;



void
txt_inserted (GtkTextBuffer *textbuffer,
               GtkTextIter   *location,
               gchar         *text,
               gint           len,
               gpointer       user_data){
				   
std::string txt(text);
if(txt==" " || txt=="\n" || len >3 ){
	init_buffer=false;
	txt_changed(mybuffer,NULL);
	
setup_vars=true;
setup_rules=true;
	
	generate_widgets();
}

}


void
txt_changed (GtkTextBuffer *textbuffer,
               gpointer       user_data){
GtkTextIter start, end;



bool keyword_initiated=false;
char keyword[100];
int start_of_keyword=0,end_of_keyword=0;
int keyword_counter=0;
if(tag==NULL)tag = gtk_text_buffer_create_tag (textbuffer, "blue_foreground",
	   		            "foreground", "blue", NULL); 
if(tag2==NULL)tag2 = gtk_text_buffer_create_tag (textbuffer, "green_foreground",
	   		            "foreground", "green", NULL); 
if(boldtag==NULL)boldtag = gtk_text_buffer_create_tag (textbuffer, "bold", "weight", 600, NULL);

if(init_buffer==false){
	gtk_text_buffer_get_start_iter (textbuffer,&start);
	gtk_text_buffer_get_iter_at_offset (textbuffer, &end, 0);

	int counter=1;
	char character;
	while((character=gtk_text_iter_get_char (&end))!=0){
		
		
		if( !( (character>='0' && character<='9') || (character>='a' && character<='z') || (character>='A' && character<='Z')) ){
			keyword_initiated=false;
			end_of_keyword=counter;
			keyword[keyword_counter]=0;
			keyword_counter=0;
			bool matched=false;
			if(matched==false){
			 if('R'==keyword[0]  || 'S'==keyword[0] || 'T'==keyword[0] || 'I'==keyword[0] || 'A'==keyword[0] || 'D'==keyword[0] ){
				 	 gtk_text_buffer_get_iter_at_offset (textbuffer, &end, end_of_keyword);
					 gtk_text_buffer_get_iter_at_offset (textbuffer, &start, start_of_keyword);
					 if(end_of_keyword-start_of_keyword<3){
					 gtk_text_buffer_apply_tag (textbuffer, boldtag, &start, &end);
					 matched=true;
					}
			 }
			}
			if(matched==false){
					for(int i=0;i<variable_list.size();i++){
				   if(variable_list[i]->var_name==std::string(keyword)){
					 gtk_text_buffer_get_iter_at_offset (textbuffer, &end, end_of_keyword);
					 gtk_text_buffer_get_iter_at_offset (textbuffer, &start, start_of_keyword);
					 gtk_text_buffer_apply_tag (textbuffer, tag2, &start, &end);
					 matched=true;
					 break;	   
				   }	
				}
			}
			if(matched==false){
				for(int i=0;i<grammar->rule_list.size();i++){
				 if(grammar->rule_list[i]->rule_name==std::string(keyword)){
					 gtk_text_buffer_get_iter_at_offset (textbuffer, &end, end_of_keyword);
					 gtk_text_buffer_get_iter_at_offset (textbuffer, &start, start_of_keyword);
					 gtk_text_buffer_apply_tag (textbuffer, tag, &start, &end);
					 matched=true;
					 break;					 
				 }
				}
			}
		}
		else {
			if(keyword_initiated==false){
				keyword_initiated=true;
				start_of_keyword=counter;
			}
			keyword[keyword_counter]=character;
			keyword_counter++;
			
		}
		
		counter++;
		gtk_text_buffer_get_iter_at_offset (textbuffer, &end, counter);
	}
	init_buffer=true;
}

}


gboolean
keypressed_textview (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data){
				   //gtk_text_view_reset_cursor_blink((GtkTextView *)view);
				   //std::cout<<"*";
				   return TRUE;
			   }

bool perspective_transform=false;
void activate_perspective_transform_button(GtkButton *button){
	if(perspective_transform==false){
	perspective_transform=true;
	mouse_index=0;

	}
}

void activateeval(GtkButton *item) {
	
squares.clear();
GtkTextIter start;
GtkTextIter end;
gtk_text_buffer_get_start_iter (buffer2,&start);
	gtk_text_buffer_get_iter_at_offset (buffer2, &end, 0);

std::string buffer2_text_str=gtk_text_buffer_get_text ((GtkTextBuffer *)buffer2,&start,&end,FALSE);


std::vector<std::string> rectangle_lines=breakup_into_lines(buffer2_text_str,"\n");
std::string input_str;
for(int i=0;i<rectangle_lines.size();i++){
	std::istringstream lin(rectangle_lines[i]);
	lin>>input_str;
	while( input_str!="]"){
		std::vector<Point> points;
		float x1,x2,y1,y2;
		lin>>input_str;
		if(input_str=="T"){
			lin>>input_str;
			if(input_str=="("){
				lin >>input_str;
				x1=atof(input_str.c_str());
				lin >>input_str;
				y1=atof(input_str.c_str());
				lin>>input_str;
				lin>>input_str;
			}
		}
		lin>>input_str;
		if(input_str== "S"){
			lin>>input_str;
			if(input_str=="("){
				lin >>input_str;
				x2=x1+atof(input_str.c_str());
				lin >>input_str;
				y2=y1+atof(input_str.c_str());
				lin>>input_str;
				lin>>input_str;
			}
		}
		lin>>input_str;
		if(input_str== "I"){
			lin>>input_str;
			if(input_str=="("){
				do{
					lin>>input_str;
				}
				while(input_str!=")");
				
			}
			
		}
		lin>>input_str;
		points.push_back(Point(x1,y1));
		points.push_back(Point(x2,y1));
		points.push_back(Point(x2,y2));
		points.push_back(Point(x1,y2));
		
		squares.push_back(points);



	}
}

}


std::string tohexstr(unsigned short uchar){
	std::ostringstream ss;
	ss << std::hex<<uchar;
	return ss.str();
}



void activate_texture_filechooser(GtkButton *item) {
	try{
GtkFileChooserNative *native;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;
     GtkFileFilter *filter;

    native = gtk_file_chooser_native_new("Open Texture File",
                                         (GtkWindow *)window,
                                         action,
                                         "_Open",
                                         "_Cancel");

    // Filters
    filter = gtk_file_filter_new(); // filter 1
    gtk_file_filter_set_name(filter, "texture file");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);
    
    filter = gtk_file_filter_new(); // filter 1
    gtk_file_filter_set_name(filter, "texture file");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);
    
    filter = gtk_file_filter_new(); // filter 2
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);

    // default file name
    //gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), "texture.png");
     std::string filename;
    res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT)
    {
   
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        filename = gtk_file_chooser_get_filename(chooser);
		
	
		
		
		//for(int i=0;i<img_counter;i++){
		 // gtk_container_remove((GtkContainer *)layout2,images[i]);
		//}
		
		
		//texture_filenames.push_back(filename);
			
			
		
			 //int picwidth,picheight,n;
			// unsigned char* texture_data = stbi_load(filename.c_str(), &picwidth, &picheight, &n,STBI_rgb_alpha);
			cv::Mat newtexture = cv::imread(filename.c_str(), cv::IMREAD_COLOR);
			//if(newtexture.cols!=newtexture.rows){ g_object_unref(native);
			//					return;}
			//if(newtexture.channels()!=3){ g_object_unref(native);
			//		return;}
			
			cv::resize(newtexture, newtexture,cv::Size(128, 128),0,0, cv::INTER_LINEAR);
			
			cv::Mat newtexture2;
			cv::cvtColor(newtexture, newtexture, cv::COLOR_BGR2RGB);
			cv::cvtColor(newtexture, newtexture2, cv::COLOR_BGR2RGB);
			
			//int texid=generateTextureFromBuf(texture_data);
			//texture_list.push_back(texid);
			/*if(){
					SSL_CTX *ctx;
					int client;
					SSL *ssl;
					char buf[20024];
					char input[BUFFER];
					int bytes;
					std::string writeout;
					
		
				ctx = InitCTX();
				LoadCertificates(ctx, "mycert.pem", "mycertkey.pem");
					client= OpenConnection(hostname.c_str(), atoi(portnum.c_str()));   
					ssl = SSL_new(ctx);      
					SSL_set_fd(ssl, client);  
					if ( SSL_connect(ssl) != FAIL ) 
					{    
					
					
						std::cout<<"Connected with encryption "<< SSL_get_cipher(ssl)<<std::endl;
						ShowCerts(ssl);
						


							
							SSL_write(ssl, username.c_str(), strlen(username.c_str()));  
							bytes = SSL_read(ssl, buf, sizeof(buf)); 
							SSL_write(ssl, password.c_str(), strlen(password.c_str())); 
						
							bytes = SSL_read(ssl, buf, sizeof(buf));
							buf[bytes]=0;
							std::string OK=buf;
							std::cout<<OK<<std::endl;
							if(OK=="DOWN"){
								writeout="TEXTUREUP";
								SSL_write(ssl, writeout.c_str(),strlen(writeout.c_str()));
								bytes = SSL_read(ssl, buf, sizeof(buf)); 
								buf[bytes]=0;
								 OK=buf;
								 
								for(int j=0;j<20;j++){
											std::stringstream	ss;		
									for(int i=j*128*128*3/20;i<(j+1)*128*128*3/20;i++){
									
										   ss  <<(int)(newtexture.data[i])<<",";
										
									}
									std::string outstr=ss.str();
									SSL_write(ssl, outstr.c_str(),strlen(outstr.c_str()));
									//std::cout<<outstr<<std::endl;
								}
								bytes = SSL_read(ssl, buf, sizeof(buf));
								buf[bytes]=0;
								 OK=buf;
								
								}
							else if(OK=="UP"){
								std::ifstream fin(grammar_filename.c_str());
								if (!fin.is_open()) {
									std::cout << "E: Could not open file " << grammar_filename << std::endl;
								}
								std::string fulltext;
								std::string line;
								while(std::getline(fin, line) ) {
								  fulltext+=line;
								}
								fin.close();
								SSL_write(ssl, fulltext.c_str(), strlen(fulltext.c_str())); 
								
							}
						
					}
					
					SSL_free(ssl);       
					close(client);       
					SSL_CTX_free(ctx);   
					
				}
				else {
				//cv::Mat newtexture2 = cv::imread(filename.c_str(), cv::IMREAD_RGB);
				//cv::resize(newtexture2, (128, 128),interpolation = cv::INTER_LINEAR)
				//cv::imwrite(("texture"+std::to_string(texture_filenames.size())+".png").c_str(),newtexture);
				
				}
		*/
		//cv::imwrite(("texture"+std::to_string(img_counter)+".png").c_str(),newtexture);
		//images[img_counter]=gtk_image_new_from_file(("texture"+std::to_string(img_counter)+".png").c_str());
		
		
	//	for(int i=0;i<img_counter;i++){
	//			gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)button_remove_img[i]);
	//			gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)images[i]);
	//		}
		
		pixsbuffer.push_back((unsigned char *)malloc(128*128*3));
		memcpy(pixsbuffer[pixsbuffer.size()-1],newtexture.data,128*128*3);
		pixbuf.push_back(gdk_pixbuf_new_from_data((const guchar*)pixsbuffer[pixsbuffer.size()-1],GDK_COLORSPACE_RGB,FALSE,8,newtexture.rows,newtexture.cols,newtexture.step,NULL,NULL));
		images.push_back(gtk_image_new_from_pixbuf(pixbuf[pixbuf.size()-1]));
		img_counter++;
		//gtk_widget_queue_draw_area(drawing_area2,0,0, draw2width,draw2height);
    
		int i=img_counter-1;
		int j=i%7;
		int k=i/7;
		button_remove_img[i]=gtk_button_new_with_label("X");
		button_moveup_img[i]=gtk_button_new_with_label("\u25b2");
		button_movedown_img[i]=gtk_button_new_with_label("\u25bc");
		gtk_widget_set_size_request((GtkWidget *)button_remove_img[i],20,20);
		gtk_widget_set_size_request((GtkWidget *)button_moveup_img[i],20,20);
		gtk_widget_set_size_request((GtkWidget *)button_movedown_img[i],20,20);
		gtk_layout_put ((GtkLayout *)layout2,images[i],80+k*230,130+j*135);
		gtk_layout_put ((GtkLayout *)layout2,button_remove_img[i],220+k*230,130+j*135);
		gtk_layout_put ((GtkLayout *)layout2,button_moveup_img[i],220+k*230,130+j*135+30);
		gtk_layout_put ((GtkLayout *)layout2,button_movedown_img[i],220+k*230,130+j*135+60);
		gtk_widget_show (images[i]);
		gtk_widget_show (button_remove_img[i]);
		gtk_widget_show (button_movedown_img[i]);
		gtk_widget_show (button_moveup_img[i]);
		
		int *mypointer=new int(i);
		g_signal_connect (button_remove_img[i] ,"clicked", G_CALLBACK (activate_button_remove_img), mypointer); 
		g_signal_connect (button_movedown_img[i] ,"clicked", G_CALLBACK (activate_button_movedown_img), mypointer); 
		g_signal_connect (button_moveup_img[i] ,"clicked", G_CALLBACK (activate_button_moveup_img), mypointer); 
		
		
			 
		float mydata[36*8];
		for(int i=0;i<36*8;i++){
			mydata[i]=vertex_data[i]; 
		}
		
		
		for(int i=0;i<36;i++){
			for(int j=0;j<8;j++){
				if(j==0 || j==2)mydata[i*8+j]*=0.5f;
				if(j==1){
					if(mydata[i*8+j]<0)mydata[i*8+j]=0.0f;
				}
			}
		}
		for(int i=0;i<texture_list.size();i++){
			
			float *my_vertex_data=grammar->context->calc(mydata,i);
			//tex_count[i]=texcount;
			if(tex_count[i]>0){
			  //glGenVertexArrays (1, &vao1[i]);
			  glBindVertexArray (vao1[i]);

			  /* This is the buffer that holds the vertices */
			  //glGenBuffers (1, &position_buffer1[i]);
			  glBindBuffer (GL_ARRAY_BUFFER, position_buffer1[i]);
			  glBufferData(GL_ARRAY_BUFFER,sizeof(float)*36*8*tex_count[i],my_vertex_data,GL_DYNAMIC_DRAW);
			  
			  delete my_vertex_data;
		  }

		}
        
    }
	 g_object_unref(native);

}   catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	errorout(error_str);
}

			  
}	
void activate_generate_image_button(GtkButton *item, gpointer userdata){
	try
	{
	int octaves=17;//atoi(gtk_combo_box_text_get_active_text((GtkComboBoxText *)gencombo1));
	float persistence=4.0f;//atof(gtk_combo_box_text_get_active_text((GtkComboBoxText *)gencombo2));
	Noise2d n2d(octaves,persistence);
	int imgbyte_counter=0;
	GdkRGBA color;
	gtk_color_chooser_get_rgba ((GtkColorChooser *)colorchooser1,&color);
	unsigned char colorRGB[3];
	std::cout<<128*color.red<<std::endl;
	colorRGB[0]=(unsigned char)(color.red*255);
	colorRGB[1]=(unsigned char)(color.green*255);
	colorRGB[2]=(unsigned char)(color.blue*255);
	double rawdata[128*128];
	cv::Mat newtexture=cv::Mat::zeros(128,128,CV_8UC3);
	double max=0.0,min=999999.0;
	int rawdata_counter=0;
	for (int y = 0; y < n2d.numY; ++y) {
		for (int x = 0; x < n2d.numX; ++x) {
			rawdata[rawdata_counter]=n2d.ValueNoise_2D(x, y);
			if(rawdata[rawdata_counter]<min)min=rawdata[rawdata_counter];
			if(rawdata[rawdata_counter]>max)max=rawdata[rawdata_counter];
			
			rawdata_counter++;
		}
	}
	imgbyte_counter=0;
	rawdata_counter=0;
	for (int y = 0; y < n2d.numY; y++) {
		for (int x = 0; x < n2d.numX; x++) {
			for(int c=0;c<3;c++){
				newtexture.data[imgbyte_counter]=(unsigned char)(double(colorRGB[c])*(rawdata[rawdata_counter]-min)/(max-min));
				imgbyte_counter++;
			}
			rawdata_counter++;
		}
	}
	
	//for(int i=0;i<images.size();i++){
				//gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)button_remove_img[i]);
				//gtk_container_remove((GtkContainer *)layout2,(GtkWidget *)images[i]);
		//	}
	
	pixsbuffer.push_back((unsigned char *)malloc(128*128*3));
		memcpy(pixsbuffer[pixsbuffer.size()-1],newtexture.data,128*128*3);
		pixbuf.push_back(gdk_pixbuf_new_from_data((const guchar*)pixsbuffer[pixsbuffer.size()-1],GDK_COLORSPACE_RGB,FALSE,8,newtexture.rows,newtexture.cols,newtexture.step,NULL,NULL));
		images.push_back(gtk_image_new_from_pixbuf(pixbuf[pixbuf.size()-1]));
		img_counter++;
		int i=img_counter-1;
		int j=i%7;
		int k=i/7;
		button_remove_img[i]=gtk_button_new_with_label("X");
		button_moveup_img[i]=gtk_button_new_with_label("\u25b2");
		button_movedown_img[i]=gtk_button_new_with_label("\u25bc");
		gtk_widget_set_size_request((GtkWidget *)button_remove_img[i],20,20);
		gtk_widget_set_size_request((GtkWidget *)button_moveup_img[i],20,20);
		gtk_widget_set_size_request((GtkWidget *)button_movedown_img[i],20,20);
		gtk_layout_put ((GtkLayout *)layout2,images[i],80+k*230,130+j*135);
		gtk_layout_put ((GtkLayout *)layout2,button_remove_img[i],220+k*230,130+j*135);
		gtk_layout_put ((GtkLayout *)layout2,button_moveup_img[i],220+k*230,130+j*135+30);
		gtk_layout_put ((GtkLayout *)layout2,button_movedown_img[i],220+k*230,130+j*135+60);
		gtk_widget_show (images[i]);
		gtk_widget_show (button_remove_img[i]);
		gtk_widget_show (button_movedown_img[i]);
		gtk_widget_show (button_moveup_img[i]);
		
		int *mypointer=new int(i);
		g_signal_connect (button_remove_img[i] ,"clicked", G_CALLBACK (activate_button_remove_img), mypointer); 
		g_signal_connect (button_movedown_img[i] ,"clicked", G_CALLBACK (activate_button_movedown_img), mypointer); 
		g_signal_connect (button_moveup_img[i] ,"clicked", G_CALLBACK (activate_button_moveup_img), mypointer); 
			//		setup_textures=true;
			//generate_widgets();
}
catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	errorout(error_str);
}
}
			   
void activateply(GtkButton *item) {
	try{
GtkFileChooserNative *native=NULL;
    GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;
     GtkFileFilter *filter;

    native = gtk_file_chooser_native_new("Save File",
                                         (GtkWindow *)window,
                                         action,
                                         "_Save",
                                         "_Cancel");

    // Filters
    filter = gtk_file_filter_new(); // filter 1
    gtk_file_filter_set_name(filter, "PLY file");
    gtk_file_filter_add_pattern(filter, "*.ply");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);
    filter = gtk_file_filter_new(); // filter 2
    gtk_file_filter_set_name(filter, "All files");
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(native), filter);

    // default file name
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(native), "test.ply");

    res = gtk_native_dialog_run(GTK_NATIVE_DIALOG(native));
    if (res == GTK_RESPONSE_ACCEPT)
    {
        std::string filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(native);
        filename = gtk_file_chooser_get_filename(chooser);
 float mydata[36*8];
    for(int i=0;i<36*8;i++){
		mydata[i]=vertex_data[i]; 
	}
	
	
	for(int i=0;i<36;i++){
		for(int j=0;j<8;j++){
			if(j==0 || j==2)mydata[i*8+j]*=0.5f;
			if(j==1){
				if(mydata[i*8+j]<0)mydata[i*8+j]=0.0f;
			}
		}
	}
	
         grammar->context->PLY(mydata,filename);
         
        //if(filename!=NULL)g_free(filename);
        //else throw 2;
    }
    else 
    throw 555;

    if(native!=NULL)g_object_unref(native);
    else throw 1;
}
catch(...){
	
	std::string error_str="Error "+std::to_string(22);
	errorout(error_str);
}
}		


gboolean mouse_press_callback(GtkWidget *da, GdkEventButton *event, gpointer userdata)
{
 int w = gtk_widget_get_allocated_width (da);
  int h = gtk_widget_get_allocated_height (da);
		//std::cout<<"mouse pressed"<<event->x<<event->y<<std::endl;
        if(mouse_pressed==false){
			if(perspective_transform){
				mouse_index++;
				perspective_points.push_back(Point2f(event->x*image.cols/w,event->y*image.rows/h));
				if(mouse_index==4){
						 int w = gtk_widget_get_allocated_width (da);
						int h = gtk_widget_get_allocated_height (da);
						std::vector<Point2f> screen_points;
						screen_points.push_back(Point2f(0,0));
						screen_points.push_back(Point2f(w,0));
						screen_points.push_back(Point2f(w,h));
						screen_points.push_back(Point2f(0,h));
						
						
						Mat M = getPerspectiveTransform(perspective_points,screen_points);
						 warpPerspective(image,image,M,Size(w,h),INTER_LINEAR,BORDER_CONSTANT,Scalar(1.0));
						perspective_transform=false;
								mouse_index=0;
				}
			}
			else {
			p1=Point(event->x,event->y);
			mouse_pressed=true;
			}
		}
		else {
			p2=Point(event->x,event->y);
			std::vector<Point> points;
			points.push_back(Point(p1.x*image.cols/w,p1.y*image.rows/h));
			points.push_back(Point(p1.x*image.cols/w,p2.y*image.rows/h));
			points.push_back(Point(p2.x*image.cols/w,p2.y*image.rows/h));
			points.push_back(Point(p2.x*image.cols/w,p1.y*image.rows/h));
			std::string grammar_snippet="Rule ->  [ T ( "+std::to_string((float)(w-p1.x*image.cols/w)/(float)h)+" "+std::to_string((float)(p2.y*image.rows/h-h)/(float)h-1.0f)+" z ) S ( "+std::to_string((float)(p2.x*image.cols/w-p1.x*image.cols/w)/(float)(h*4.0f))+" "+std::to_string((float)(p2.y*image.rows/h-p1.y*image.rows/h)/(float)(h*4.0f))+" Z ) I ( CubeX TextureIndex ) ] \n";
			
			GtkTextIter end;
			gtk_text_buffer_get_iter_at_offset (buffer2, &end, 0);
			gtk_text_buffer_insert (buffer2,&end,grammar_snippet.c_str(),grammar_snippet.size());
			
			squares.push_back(points);
			mouse_pressed=false;
		}
		gtk_widget_queue_draw(da);

    return TRUE;
}
static gboolean
handle_mouse (GtkWidget *da, GdkEventButton *event, gpointer t){
	 int w = gtk_widget_get_allocated_width (da);
  int h = gtk_widget_get_allocated_height (da);
  mousex=event->x;
  mousey=event->y;
	gtk_widget_queue_draw_area (da, 0, 0, w, h);
	return TRUE;
}


static gboolean
handle_mouse2 (GtkWidget *da, GdkEventButton *event, gpointer t){
	 int w = gtk_widget_get_allocated_width (da);
  int h = gtk_widget_get_allocated_height (da);
  //mousex=event->x;
  //mousey=event->y;
	gtk_widget_queue_draw_area (da, 0, 0, w, h);
	return TRUE;
}
		
gboolean
draw_callback2 (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  guint width, height;
  GdkRGBA color;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  draw2width = gtk_widget_get_allocated_width (widget);
  draw2height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);
  
  
  cairo_select_font_face(cr, "Courier",
      CAIRO_FONT_SLANT_NORMAL,
      CAIRO_FONT_WEIGHT_BOLD);

  cairo_set_font_size(cr, 30);
  
	for(int i=0;i<img_counter;i++){
			int j=i%10;
			int k=i/10;
			//gtk_layout_put ((GtkLayout *)layout2,(GtkWidget *)images[i],80+k*130,j*135);
			//gtk_widget_show ((GtkWidget *)images[i]);
		
    
		//gdk_cairo_set_source_pixbuf(cr, pixbufs[i], 80+220*k,j*70);
	

  

  
		
   cairo_paint(cr);
	}
	 cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
	for(int i=0;i<img_counter;i++){
					int j=i%10;
			int k=i/10;
		  std::string indexstr=std::to_string(i);
	cairo_move_to(cr, 200+220*k, j*70+35);
  cairo_show_text(cr, indexstr.c_str());
  cairo_stroke(cr);
	}
	    
  
  
   
  //cairo_fill (cr);

 return FALSE;
}	   
gboolean
draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  guint width, height;
  GdkRGBA color;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (context, cr, 0, 0, width, height);
  
  if(perspective_transform==true){
	  for(int i=0;i<perspective_points.size();i++){
			cv::circle(image,perspective_points[i],3,Scalar(0,255,0),1);
	  }
  }
  else cv::polylines(image, squares, true, Scalar(0, 255, 0), 3, LINE_AA);
  
  if(image.data!=NULL)pix = gdk_pixbuf_new_from_data((const guchar*)image.data,GDK_COLORSPACE_RGB,FALSE,8,image.cols,image.rows,image.cols * 3,NULL,NULL);
  else pix=NULL;
  
	 if(pix!=NULL)pix = gdk_pixbuf_scale_simple((GdkPixbuf *)pix,width,height,GDK_INTERP_BILINEAR); 
    if(pix!=NULL){
		gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
		cairo_paint(cr);
	}
	
	    
  
  
   
  //cairo_fill (cr);

 return FALSE;
}





void activate_skip(GtkButton *item) {
	gtk_container_remove(GTK_CONTAINER (window),(GtkWidget *) box6);
  gtk_container_add (GTK_CONTAINER (window), (GtkWidget *)paned);
  gtk_widget_show(drawing_area);
   gtk_widget_show(view2);
    gtk_widget_show((GtkWidget *)actionbar);
  gtk_widget_show(runbutton);
  gtk_widget_show(newbutton);
  gtk_widget_show((GtkWidget *)actionbar);
  gtk_widget_show((GtkWidget *)actionbar3);
  gtk_widget_show(playbutton3);
  gtk_widget_show(stopbutton3);
  gtk_widget_show(rulescombo);
  gtk_widget_show_all(GTK_WIDGET(window));
}
  
void activate_login(GtkButton *item) {



	username=gtk_entry_get_text((GtkEntry *)entry1);
	password=gtk_entry_get_text((GtkEntry *)entry2);
	  
	   std::ofstream fout("userdetails.txt");
		if (!fout.is_open()) {
			std::cerr << "E: Could not open file for saving"  << std::endl;
			
		}
		else {
		
			fout<<username<<std::endl;
			fout<<password;

			fout.close();
		}

loggedin=true;

SSL_CTX *ctx;
	int client;
	SSL *ssl;
	char buf[10024];
	char input[BUFFER];
	int bytes;
	std::string writeout;

ctx = InitCTX();
LoadCertificates(ctx, "mycert.pem", "mycertkey.pem");
	client= OpenConnection(hostname.c_str(), atoi(portnum.c_str()));   /*converting ascii port to interger */
	ssl = SSL_new(ctx);      /* create new SSL connection state */
	SSL_set_fd(ssl, client);    /* attach the socket descriptor */
	if ( SSL_connect(ssl) != FAIL )   /* perform the connection */
	{    
	
	
		std::cout<<"Connected with encryption "<< SSL_get_cipher(ssl)<<std::endl;
		ShowCerts(ssl);
		/* get any certs */


		
			
			
			SSL_write(ssl, username.c_str(), strlen(username.c_str()));   /* encrypt & send message */
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			SSL_write(ssl, password.c_str(), strlen(password.c_str()));   /* encrypt & send message */
		
			bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
			buf[bytes]=0;
			std::string OK=buf;
			if(OK=="UP") {
				std::ifstream fin(grammar_filename.c_str());
				if (!fin.is_open()) {
					std::cout << "E: Could not open file " << grammar_filename << std::endl;
				}
				std::string fulltext;
				std::string line;
				while(std::getline(fin, line) ) {
				  fulltext+=line;
				}
				fin.close();
				SSL_write(ssl, fulltext.c_str(), strlen(fulltext.c_str())); 
	
			}
			else if(OK=="DOWN"){
				writeout="GRAMMAR";
				SSL_write(ssl, writeout.c_str(), strlen(writeout.c_str()));
				bytes = SSL_read(ssl, buf, sizeof(buf)); /* get request */
				buf[bytes]=0;
				std::string fulltext=buf;
				std::ofstream fout(grammar_filename.c_str());
				fout<<fulltext;
				fout.close();
			}
		}
	
	SSL_free(ssl);        /* release connection state */
	close(client);         /* close socket */
	SSL_CTX_free(ctx);        /* release */
	
gtk_container_remove(GTK_CONTAINER (window), (GtkWidget *)box6);
  gtk_container_add (GTK_CONTAINER (window), (GtkWidget *)paned);
  gtk_widget_show(drawing_area);
   gtk_widget_show(view2);
    gtk_widget_show((GtkWidget *)actionbar);
  gtk_widget_show(runbutton);
  gtk_widget_show(newbutton);
  
  gtk_widget_show((GtkWidget *)actionbar);
  gtk_widget_show((GtkWidget *)actionbar3);
  gtk_widget_show(playbutton3);
  gtk_widget_show(stopbutton3);
  gtk_widget_show(rulescombo);
  gtk_widget_show_all(GTK_WIDGET(window));
}
std::string text1,text2;
			   
void activate_app(GtkApplication *app){
		
		
	
	uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
    rng.seed(ss);
    
    
   /* Create new top level window. */
  window = gtk_application_window_new (app);//gtk_window_new( GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(window),SCREEN_WIDTH,SCREEN_HEIGHT);
  gtk_window_set_title(GTK_WINDOW(window), appname);
  gtk_container_set_border_width(GTK_CONTAINER(window), 10);
 
 
 GtkWidget *button_login=gtk_button_new_with_label("Login");
 GtkWidget *button_skip=gtk_button_new_with_label("Skip");
 entry1=gtk_entry_new();
 entry2=gtk_entry_new();
 GtkWidget *label1=gtk_label_new("Username:");
 GtkWidget *label2=gtk_label_new("Password:");
 gtk_entry_set_visibility ((GtkEntry *)entry2,FALSE);
 GtkWidget *labeltext=gtk_label_new(NULL);
 gtk_label_set_markup((GtkLabel *)labeltext,"<span size='xx-large'>Progen3d Cloud Login</span>\nSee progen.org.au for account sign up. Currently disabled.");
 
 gtk_widget_set_size_request(button_login,50, 30);
 gtk_widget_set_size_request(button_skip,50, 30);
 gtk_widget_set_size_request(entry1,60, 30);
 gtk_widget_set_size_request(entry2,60, 30);
 gtk_widget_set_size_request(label1,60, 30);
 gtk_widget_set_size_request(label2,60, 30);
 
 
  box6 = gtk_box_new (GTK_ORIENTATION_VERTICAL, FALSE);
  g_object_set (box6, "margin", 220, NULL);
  box7 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, FALSE);
  box8 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, FALSE);
  box9 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, FALSE);
  g_object_set (box9, "margin", 120, NULL);
  gtk_box_pack_start (GTK_BOX(box7), (GtkWidget *)label1,FALSE,FALSE, 0);
  gtk_box_pack_start (GTK_BOX(box8), (GtkWidget *)label2,FALSE,FALSE, 0);
  gtk_box_pack_start (GTK_BOX(box7), (GtkWidget *)entry1,FALSE,FALSE, 0);
  gtk_box_pack_start (GTK_BOX(box8), (GtkWidget *)entry2,FALSE,FALSE, 0);
  
  gtk_box_pack_start (GTK_BOX(box6), (GtkWidget *)labeltext,FALSE,FALSE, 0);
  gtk_box_pack_start (GTK_BOX(box6), (GtkWidget *)box7,FALSE,FALSE, 0);
  gtk_box_pack_start (GTK_BOX(box6), (GtkWidget *)box8,FALSE,FALSE, 0);
  gtk_box_pack_start (GTK_BOX(box9), (GtkWidget *)button_login,FALSE,FALSE, 0);
  gtk_box_pack_start (GTK_BOX(box9), (GtkWidget *)button_skip,FALSE,FALSE, 0);
  gtk_box_pack_start (GTK_BOX(box6), (GtkWidget *)box9,FALSE,FALSE, 0);
  
  g_signal_connect (button_login ,"clicked", G_CALLBACK (activate_skip), NULL);
  g_signal_connect (button_skip ,"clicked", G_CALLBACK (activate_skip), NULL);
  
  
   std::ifstream fin("userdetails.txt");
	if (fin.is_open()) {
		
		std::getline(fin,text1);
		std::getline(fin,text2);
		
		
		gtk_entry_set_text((GtkEntry *)entry1,text1.c_str());
		gtk_entry_set_text((GtkEntry *)entry2,text2.c_str());
		fin.close();
	}
  
  gtk_container_add (GTK_CONTAINER (window), box6);
  gtk_widget_show(label1);
  gtk_widget_show(label2);
  gtk_widget_show(entry1);
  gtk_widget_show(entry2);
  gtk_widget_show(button_login);
  gtk_widget_show(button_skip);
  gtk_widget_show_all(GTK_WIDGET(window));
   
  
  
  
  
  
  
  
  
  
  
  
  
  
  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, FALSE);
  //box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, FALSE);
  paned=gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  vpaned=gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  box3=gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
  box4=gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
  box5=gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
  box9=gtk_box_new(GTK_ORIENTATION_VERTICAL, FALSE);
  //box10 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, FALSE);
  box10=gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  box11=gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  //g_object_set (box, "margin", 12, NULL);
  //gtk_box_set_spacing (GTK_BOX (box), 6);
//  g_object_set (paned, "margin", 12, NULL);
 // gtk_box_set_spacing (GTK_BOX (paned), 6);
  
 actionbar2=(GtkActionBar *)gtk_action_bar_new ();
 actionbar3=(GtkActionBar *)gtk_action_bar_new ();
 actionbar4=(GtkActionBar *)gtk_action_bar_new ();
 actionbar5=(GtkActionBar *)gtk_action_bar_new ();
 actionbar6=(GtkActionBar *)gtk_action_bar_new ();
 actionbar7=(GtkActionBar *)gtk_action_bar_new ();

  gtk_box_pack_start (GTK_BOX(box4), (GtkWidget *)actionbar2,0,0, 0);
    gtk_box_pack_start (GTK_BOX(box9), (GtkWidget *)actionbar6,0,0, 0);

  
  
  gl_area = gtk_gl_area_new ();
  GtkWidget *scrollwin=gtk_scrolled_window_new (NULL,NULL);
  view = gtk_text_view_new();
  view2= gtk_text_view_new();
  viewerror=gtk_text_view_new();
  	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 10);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 10);
  gtk_container_add (GTK_CONTAINER (scrollwin), view);
  gtk_text_view_set_wrap_mode((GtkTextView *)view,GTK_WRAP_WORD);
  gtk_text_view_set_monospace ((GtkTextView *)view,TRUE);
  gtk_text_view_set_monospace ((GtkTextView *)view2,TRUE);
  
  
  
  GtkWidget *scrollwin2 = gtk_scrolled_window_new(NULL, NULL);
  GtkWidget *scrollwin3 = gtk_scrolled_window_new(NULL, NULL);
  GtkWidget *scrollwin4 = gtk_scrolled_window_new(NULL, NULL);
  
  
  gtk_container_add (GTK_CONTAINER (scrollwin4), viewerror);
  
    
    gtk_widget_set_size_request((GtkWidget *)actionbar2, 950, 12);
notebook=gtk_notebook_new ();
//notebooktextview=gtk_notebook_new();

    file_label=gtk_label_new("");
    gtk_widget_set_size_request((GtkWidget *)actionbar3, 950, 12);
    gtk_box_pack_start(GTK_BOX(box),file_label, 0, 0, 0);
    gtk_box_pack_start(GTK_BOX(box),notebook, 0, 0, 0);
    gtk_notebook_append_page ((GtkNotebook *)notebook,
                          scrollwin,
                          gtk_label_new ("Grammar Text"));
    layout=gtk_layout_new (NULL,NULL);
layout2=gtk_layout_new (NULL,NULL);

gtk_widget_set_size_request(notebook, 950, 1250);
gtk_widget_set_size_request(file_label, 950, 12);

vadj=gtk_adjustment_new (0,0,2000,1,100,100);
                    
layout3=gtk_layout_new (NULL,NULL);
layout4=gtk_layout_new (NULL,NULL);

drawing_area2 = gtk_drawing_area_new ();
gtk_widget_set_size_request(layout2, 950, 1200);
gtk_container_add (GTK_CONTAINER (scrollwin2), layout3);
gtk_container_add (GTK_CONTAINER (scrollwin3), layout);

gtk_box_pack_start (GTK_BOX(box4),scrollwin3,1,1, 0);
gtk_box_pack_start (GTK_BOX(box9),layout2,1,1, 0);

gtk_notebook_append_page ((GtkNotebook *)notebook,
                          box4,
                          gtk_label_new ("Rule Layout"));
    
    
     drawing_area = gtk_drawing_area_new ();
     
  gtk_widget_set_size_request (drawing_area, 900, 500);
                            
 gtk_paned_add1(GTK_PANED(vpaned), drawing_area);
  gtk_paned_add2 (GTK_PANED(vpaned), view2);
   
   
   
   
   GtkWidget *choose_image_button=gtk_button_new_with_label("Open Image");
   GtkWidget *perspective_transform_button=gtk_button_new_with_label("Perspective Transform");
   
   gtk_action_bar_pack_start (actionbar4,choose_image_button);  
   gtk_action_bar_pack_start (actionbar4,perspective_transform_button);  
   
  gtk_box_pack_start(GTK_BOX(box5),(GtkWidget *)actionbar4, 0, 0, 0);
  gtk_box_pack_start(GTK_BOX(box5),vpaned, 1, 1, 1);
  gtk_box_pack_start(GTK_BOX(box5),(GtkWidget *)actionbar5, 0, 0, 0);
  
gtk_notebook_append_page ((GtkNotebook *)notebook,
                          box5,
                          gtk_label_new ("SketchImport"));
  
  
  
  g_signal_connect (G_OBJECT (drawing_area), "draw",
                    G_CALLBACK (draw_callback), NULL);
   //g_signal_connect (G_OBJECT (drawing_area2), "draw",
     //               G_CALLBACK (draw_callback2), NULL);
    
  GtkWidget *addbutton=gtk_button_new_with_label("Add");
  GtkWidget *buildbutton=gtk_button_new_with_label("Build");
  
  GtkWidget *evalbutton=gtk_button_new_with_label("Evaluate");
  
  //GtkWidget *generate_image_button=gtk_button_new_with_label("Generate");
  texture_filechooser=gtk_button_new_with_label("Open Texture");
rulescombo=gtk_combo_box_text_new_with_entry();


/*
GtkWidget *genlabel1=gtk_label_new("Octaves");
GtkWidget *genlabel2=gtk_label_new("Persistence");
GtkWidget *gencombo1=gtk_combo_box_text_new_with_entry();
GtkWidget *gencombo2=gtk_combo_box_text_new_with_entry();
gtk_combo_box_text_append((GtkComboBoxText *)gencombo1,NULL,"7");
gtk_combo_box_text_append((GtkComboBoxText *)gencombo2,NULL,"0.5");
gtk_combo_box_set_active((GtkComboBox *)gencombo1,0);
gtk_combo_box_set_active((GtkComboBox *)gencombo2,0);

gtk_widget_set_size_request((GtkWidget *)genlabel1,20,20);
gtk_widget_set_size_request((GtkWidget *)genlabel2,20,20);
gtk_widget_set_size_request((GtkWidget *)gencombo1,20,20);
gtk_widget_set_size_request((GtkWidget *)gencombo2,20,20);





gtk_layout_put ((GtkLayout *)layout2,genlabel1,80,10);
gtk_layout_put ((GtkLayout *)layout2,genlabel2,80,60);
gtk_layout_put ((GtkLayout *)layout2,gencombo1,180,10);
gtk_layout_put ((GtkLayout *)layout2,gencombo2,180,60);
colorchooser1=gtk_color_chooser_widget_new();
gtk_widget_set_size_request((GtkWidget *)colorchooser1,120,50);
gtk_layout_put ((GtkLayout *)layout2,colorchooser1,400,0);
*/

 gtk_action_bar_pack_start (actionbar2,buildbutton);  
 gtk_action_bar_pack_start (actionbar2,addbutton);    
 
 gtk_action_bar_pack_start (actionbar6,texture_filechooser);  
 //gtk_action_bar_pack_start (actionbar6,generate_image_button);  
 
   
   gtk_action_bar_pack_start (actionbar2,rulescombo);
   gtk_action_bar_pack_start (actionbar5,evalbutton);
       
                  //gtk_widget_set_size_request((GtkWidget *)actionbar2, 950,50);
                      gtk_widget_set_size_request((GtkWidget *)scrollwin3,950, 1000);
  gtk_widget_set_size_request(addbutton,10, 30);
  
gtk_widget_add_events (drawing_area,GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK);

gtk_notebook_append_page ((GtkNotebook *)notebook,
                          box9,
                          gtk_label_new ("Texture"));


gtk_notebook_append_page ((GtkNotebook *)notebook,
                          scrollwin2,
                          gtk_label_new ("Variables"));

//gtk_notebook_append_page ((GtkNotebook *)notebook,
  //                        layout4,
    //                      gtk_label_new ("Rules"));
                          
                          
 gtk_paned_add1(GTK_PANED(paned), box3);
  gtk_paned_add2 (GTK_PANED(paned), box);
  
  
  actionbar=(GtkActionBar *)gtk_action_bar_new ();
  runbutton=gtk_button_new_with_label("Run");
  newbutton=gtk_button_new_with_label("New");
  savebutton=gtk_button_new_with_label("SaveAs");
  plybutton=gtk_button_new_with_label("Export PLY");
  GtkWidget *scale_slider=gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,0.3,3.0,0.05);
   gtk_widget_set_hexpand (scale_slider, TRUE);
  gtk_action_bar_pack_start (actionbar,newbutton);
  gtk_action_bar_pack_start (actionbar,runbutton);
  gtk_action_bar_pack_start (actionbar,savebutton);
  gtk_action_bar_pack_start (actionbar,plybutton);
  gtk_action_bar_pack_start (actionbar,scale_slider);
  
  gtk_widget_set_size_request((GtkWidget *)actionbar, 650,50);
    gtk_widget_set_size_request((GtkWidget *)gl_area,650, 1100);
  gtk_widget_set_size_request(runbutton,10, 30);
  gtk_widget_set_size_request(newbutton,10, 30);
  gtk_widget_set_size_request(scale_slider,400, 20);
  gtk_widget_set_size_request(texture_filechooser,10, 30);
  
  
  
  scale_slider4=gtk_scale_new_with_range (GTK_ORIENTATION_VERTICAL,0.0,90.0,2.0);
  gtk_widget_set_size_request(scale_slider4,20, 870);
  gtk_box_pack_start (GTK_BOX(box3), (GtkWidget *)actionbar,1,1, 0);
  gtk_box_pack_start (GTK_BOX(box3), (GtkWidget *)box10,1,1, 0);
  gtk_paned_add1(GTK_PANED(box10),(GtkWidget *) actionbar7);
  gtk_paned_add2(GTK_PANED(box10),(GtkWidget *)box11);
  gtk_paned_add1 (GTK_PANED(box11), gl_area);
  gtk_paned_add2 (GTK_PANED(box11),scrollwin4);
  gtk_action_bar_pack_start (actionbar7,scale_slider4);
  gtk_paned_set_position(GTK_PANED(box10),40);
  gtk_paned_set_position(GTK_PANED(box11),1100);
   
  



  
   playbutton3=gtk_button_new_with_label("Play");
  stopbutton3=gtk_button_new_with_label("Stop");
  
  scale_slider3=gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL,0.0,360.0,2.0);
  
   gtk_widget_set_hexpand (scale_slider3, TRUE);
  gtk_action_bar_pack_start (actionbar3,playbutton3);
  gtk_action_bar_pack_start (actionbar3,stopbutton3);
  
  gtk_action_bar_pack_start (actionbar3,scale_slider3);
gtk_widget_set_size_request((GtkWidget *)actionbar3, 650,50);
    
  gtk_widget_set_size_request(playbutton3,10, 30);
    gtk_box_pack_start (GTK_BOX(box3), (GtkWidget *)actionbar3,1,1, 0);
  
  
  
  
  
   gtk_text_view_set_editable((GtkTextView *)view,TRUE);
    //gtk_text_view_set_overwrite((GtkTextView *)view,TRUE);

 
   
  buffer2 = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view2));
  errorbuffer= gtk_text_view_get_buffer (GTK_TEXT_VIEW (viewerror));
    
    mybuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    //g_signal_connect(drawing_area2,"motion_notify_event",G_CALLBACK(handle_mouse2),   NULL);
   // g_signal_connect(generate_image_button,"clicked",G_CALLBACK(activate_generate_image_button),   NULL);
    g_signal_connect(addbutton,"clicked",G_CALLBACK(activate_addrule),   NULL);
    g_signal_connect(buildbutton,"clicked",G_CALLBACK(activate_buildrule),   NULL);
   // g_signal_connect(drawing_area,"motion_notify_event",G_CALLBACK(handle_mouse),   NULL);
   // g_signal_connect(drawing_area,"button_press_event",G_CALLBACK(mouse_press_callback),   NULL);
    
    g_signal_connect (mybuffer ,"changed", G_CALLBACK (txt_changed), NULL);  
    g_signal_connect (mybuffer ,"insert-text", G_CALLBACK (txt_inserted), NULL);  
    g_signal_connect (layout4 ,"focus", G_CALLBACK (rule_focus), NULL);  
    g_signal_connect (box9 ,"focus", G_CALLBACK (texture_focus), NULL);  
    g_signal_connect (layout3 ,"focus", G_CALLBACK (vars_focus), NULL); 
    g_signal_connect (box4 ,"focus", G_CALLBACK (layout_focus), NULL);   
    
    g_signal_connect (perspective_transform_button,"clicked",G_CALLBACK (activate_perspective_transform_button), NULL);    
    g_signal_connect (rulescombo ,"changed", G_CALLBACK (activate_add), NULL);    
    g_signal_connect (runbutton ,"clicked", G_CALLBACK (activate), NULL);
    g_signal_connect (newbutton ,"clicked", G_CALLBACK (new_activate), NULL);
    g_signal_connect (savebutton ,"clicked", G_CALLBACK (activate_saveas), NULL); 
    g_signal_connect (texture_filechooser,"clicked", G_CALLBACK (activate_texture_filechooser), NULL); 
    
    g_signal_connect (choose_image_button ,"clicked", G_CALLBACK (activate_choose_image_button), NULL); 
    g_signal_connect (plybutton ,"clicked", G_CALLBACK (activateply), NULL); 
    g_signal_connect (evalbutton ,"clicked", G_CALLBACK (activateeval), NULL); 
    
    g_signal_connect (playbutton3 ,"clicked", G_CALLBACK (activate_play), NULL);
    g_signal_connect (stopbutton3 ,"clicked", G_CALLBACK (activate_stop), NULL); 
    
    
    g_signal_connect (scale_slider ,"value-changed", G_CALLBACK (scalesliderupdate), NULL); 
    g_signal_connect (scale_slider3 ,"value-changed", G_CALLBACK (scalesliderupdate3), NULL); 
    g_signal_connect (scale_slider4 ,"value-changed", G_CALLBACK (scalesliderupdate4), NULL); 
  
  /* We need to initialize and free GL resources, so we use
  * the realize and unrealize signals on the widget
  */
  gtk_gl_area_set_has_depth_buffer(GTK_GL_AREA (gl_area), TRUE);
  g_signal_connect (gl_area, "realize", G_CALLBACK (realize), NULL);
  g_signal_connect (gl_area, "unrealize", G_CALLBACK (unrealize), NULL);
  g_signal_connect (gl_area, "resize", G_CALLBACK (resize_glarea), NULL);

  /* The main "draw" call for GtkGLArea */
  g_signal_connect (gl_area, "render", G_CALLBACK (render), NULL);
  /* Quit form main if got delete event */
  g_signal_connect(G_OBJECT(window), "delete-event",
                 G_CALLBACK(gtk_main_quit), NULL);
                 
                 
  g_signal_connect(view,"leave-notify-event",G_CALLBACK(keypressed_textview), NULL);
                 
  //gtk_widget_show_all(GTK_WIDGET(window));
 
   
   
    gtk_window_present (GTK_WINDOW (window));
   
   
}
   
int main( int argc, char* argv[] ){

GtkApplication *app;

for(int i=0;i<100;i++)rulenames[i]=new char(40);//due to GTK combobox error

	
	
	
	

	

	SSL_library_init();   /*load encryption and hash algo's in ssl*/
	
	
	
	
	
	 app= gtk_application_new ("snap.progen3d.progen3d", G_APPLICATION_FLAGS_NONE);
	
	
       g_signal_connect (app ,"activate", G_CALLBACK (activate_app), NULL);
   
  
  
     int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);





  return status;
  
	
}

































