// anim.cpp version 5.0 -- Template code for drawing an articulated figure. CS 174A.
#include "../CS174a template/Utilities.h"
#include "../CS174a template/Shapes.h"

std::stack<mat4> mvstack;

int g_width = 800, g_height = 600;
float zoom = 1;
int animate = 0, recording = 0, basis_to_display = -1;
double TIME = 0;

const unsigned X = 0, Y = 1, Z = 2;

vec4 eye(0, 0, 15, 1), ref(0, 0, 0, 1), up(0, 1, 0, 0);

mat4	orientation, model_view;
ShapeData cubeData, sphereData, coneData, cylData, pyramidData;
GLuint	texture_heli, texture_bee, texture_wing; // My textures
GLint   uModelView, uProjection, uView,
		uAmbient, uDiffuse, uSpecular, uLightPos, uShininess,
		uTex, uEnableTex;

void init()
{
#ifdef EMSCRIPTEN
    GLuint program = LoadShaders( "vshader.glsl", "fshader.glsl" );
    TgaImage heliImage ("heli.tga");
	TgaImage beeImage ("bee.tga");
	TgaImage wingImage ("wing.tga");

#else
	GLuint program = LoadShaders( "../my code/vshader.glsl", "../my code/fshader.glsl" );
    TgaImage heliImage ("../my code/heli.tga");
	TgaImage beeImage("../my code/bee.tga");
	TgaImage wingImage("../my code/wing.tga");
#endif
    glUseProgram(program);

	generateCube(program, &cubeData); // Generate vertex arrays for geometric shapes
    generateSphere(program, &sphereData);
    generateCone(program, &coneData);
    generateCylinder(program, &cylData);
	generatePyramid(program, &pyramidData);

    uModelView  = glGetUniformLocation( program, "ModelView"  );
    uProjection = glGetUniformLocation( program, "Projection" );
    uView		= glGetUniformLocation( program, "View"       );
    uAmbient	= glGetUniformLocation( program, "AmbientProduct"  );
    uDiffuse	= glGetUniformLocation( program, "DiffuseProduct"  );
    uSpecular	= glGetUniformLocation( program, "SpecularProduct" );
    uLightPos	= glGetUniformLocation( program, "LightPosition"   );
    uShininess	= glGetUniformLocation( program, "Shininess"       );
    uTex		= glGetUniformLocation( program, "Tex"             );
    uEnableTex	= glGetUniformLocation( program, "EnableTex"       );

    glUniform4f( uAmbient,    0.2,  0.2,  0.2, 1 );
    glUniform4f( uDiffuse,    0.6,  0.6,  0.6, 1 );
    glUniform4f( uSpecular,   0.2,  0.2,  0.2, 1 );
    glUniform4f( uLightPos,  15.0, 15.0, 30.0, 0 );
    glUniform1f( uShininess, 100);

    glEnable(GL_DEPTH_TEST);

	// Camo texture for helicopters
    glGenTextures( 1, &texture_heli );
    glBindTexture( GL_TEXTURE_2D, texture_heli );
    glTexImage2D(GL_TEXTURE_2D, 0, 4, heliImage.width, heliImage.height, 0,
                 (heliImage.byteCount == 3) ? GL_BGR : GL_BGRA,
                 GL_UNSIGNED_BYTE, heliImage.data );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

	// Bee texture
	glGenTextures(1, &texture_bee);
	glBindTexture(GL_TEXTURE_2D, texture_bee);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, beeImage.width, beeImage.height, 0,
		(beeImage.byteCount == 3) ? GL_BGR : GL_BGRA,
		GL_UNSIGNED_BYTE, beeImage.data);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Texture for bee wings
	glGenTextures(1, &texture_wing);
	glBindTexture(GL_TEXTURE_2D, texture_wing);
	glTexImage2D(GL_TEXTURE_2D, 0, 4, wingImage.width, wingImage.height, 0,
		(wingImage.byteCount == 3) ? GL_BGR : GL_BGRA,
		GL_UNSIGNED_BYTE, wingImage.data);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glUniform1i( uTex, 0);	// Set texture sampler variable to texture unit 0
	glEnable(GL_DEPTH_TEST);
}

struct color{ color( float r, float g, float b) : r(r), g(g), b(b) {} float r, g, b;};
std::stack<color> colors;
void set_color(float r, float g, float b)
{
	colors.push(color(r, g, b));

	float ambient  = 0.2, diffuse  = 0.6, specular = 0.2;
    glUniform4f(uAmbient,  ambient*r,  ambient*g,  ambient*b,  1 );
    glUniform4f(uDiffuse,  diffuse*r,  diffuse*g,  diffuse*b,  1 );
    glUniform4f(uSpecular, specular*r, specular*g, specular*b, 1 );
}

int mouseButton = -1, prevZoomCoord = 0 ;
vec2 anchor;
void myPassiveMotionCallBack(int x, int y) { anchor = vec2(2. * x / g_width - 1, -2. * y / g_height + 1); }

void myMouseCallBack(int button, int state, int x, int y)
{
    mouseButton = button;
   
    if( button == GLUT_LEFT_BUTTON && state == GLUT_UP )
        mouseButton = -1 ;
    if( button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN )
        prevZoomCoord = -2. * y / g_height + 1;

    glutPostRedisplay() ;
}

void myMotionCallBack(int x, int y)
{
	vec2 arcball_coords( 2. * x / g_width - 1, -2. * y / g_height + 1 );
	 
    if( mouseButton == GLUT_LEFT_BUTTON )
    {
	   orientation = RotateX( -10 * (arcball_coords.y - anchor.y) ) * orientation;
	   orientation = RotateY(  10 * (arcball_coords.x - anchor.x) ) * orientation;
    }
	
	if( mouseButton == GLUT_RIGHT_BUTTON )
		zoom *= 1 + .1 * (arcball_coords.y - anchor.y);
    glutPostRedisplay() ;
}

void idleCallBack(void)
{
    if( !animate ) return;
	double prev_time = TIME;
    TIME = TM.GetElapsedTime() ;
	if( prev_time == 0 ) TM.Reset();
    glutPostRedisplay() ;
}

void drawCylinder()
{
    glUniformMatrix4fv( uModelView, 1, GL_FALSE, transpose(model_view) );
    glBindVertexArray( cylData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cylData.numVertices );
}

void drawCone()
{
    glUniformMatrix4fv( uModelView, 1, GL_FALSE, transpose(model_view) );
    glBindVertexArray( coneData.vao );
    glDrawArrays( GL_TRIANGLES, 0, coneData.numVertices );
}

void drawCube()
{
    glUniformMatrix4fv( uModelView, 1, GL_FALSE, transpose(model_view) );
    glBindVertexArray( cubeData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cubeData.numVertices );
}

void drawSphere()
{
    glUniformMatrix4fv( uModelView, 1, GL_FALSE, transpose(model_view) );
    glBindVertexArray( sphereData.vao );
    glDrawArrays( GL_TRIANGLES, 0, sphereData.numVertices );
}

// Draw plain custom pyramid
void drawPyramid()
{
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(pyramidData.vao);
	glDrawArrays(GL_TRIANGLES, 0, pyramidData.numVertices);
}

// Draw cube with camo heli texture
void drawHeliCube()
{
	glBindTexture(GL_TEXTURE_2D, texture_heli);
	glUniform1i(uEnableTex, 1);
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(cubeData.vao);
	glDrawArrays(GL_TRIANGLES, 0, cubeData.numVertices);
	glUniform1i(uEnableTex, 0);
}

// Draw sphere with camo heli texture
void drawHeliSphere()
{
	glBindTexture(GL_TEXTURE_2D, texture_heli);
	glUniform1i(uEnableTex, 1);
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(sphereData.vao);
	glDrawArrays(GL_TRIANGLES, 0, sphereData.numVertices);
	glUniform1i(uEnableTex, 0);
}

// Draw pyramid for the bee wings with wing texture
void drawBeePyramid()
{
	glBindTexture(GL_TEXTURE_2D, texture_wing);
	glUniform1i(uEnableTex, 1);
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(pyramidData.vao);
	glDrawArrays(GL_TRIANGLES, 0, pyramidData.numVertices);
	glUniform1i(uEnableTex, 0);
}

// Draw sphere with bee stripes
void drawBeeSphere()
{
	glBindTexture(GL_TEXTURE_2D, texture_bee);
	glUniform1i(uEnableTex, 1);
	glUniformMatrix4fv(uModelView, 1, GL_FALSE, transpose(model_view));
	glBindVertexArray(sphereData.vao);
	glDrawArrays(GL_TRIANGLES, 0, sphereData.numVertices);
	glUniform1i(uEnableTex, 0);
}

int basis_id = 0;
void drawOneAxis()
{
	mat4 origin = model_view;
	model_view *= Translate	( 0, 0, 4 );
	model_view *= Scale(.25) * Scale( 1, 1, -1 );
	drawCone();
	model_view = origin;
	model_view *= Translate	( 1,  1, .5 );
	model_view *= Scale		( .1, .1, 1 );
	drawCube();
	model_view = origin;
	model_view *= Translate	( 1, 0, .5 );
	model_view *= Scale		( .1, .1, 1 );
	drawCube();
	model_view = origin;
	model_view *= Translate	( 0,  1, .5 );
	model_view *= Scale		( .1, .1, 1 );
	drawCube();
	model_view = origin;
	model_view *= Translate	( 0,  0, 2 );
	model_view *= Scale(.1) * Scale(   1, 1, 20);
    drawCylinder();	
	model_view = origin;
}

void drawAxes(int selected)
{
	if( basis_to_display != selected ) 
		return;
	mat4 given_basis = model_view;
	model_view *= Scale		(.25);
	drawSphere();
	model_view = given_basis;
	set_color( 0, 0, 1 );
	drawOneAxis();
	model_view *= RotateX	(-90);
	model_view *= Scale		(1, -1, 1);
	set_color( 1, 1, 1);
	drawOneAxis();
	model_view = given_basis;
	model_view *= RotateY	(90);
	model_view *= Scale		(-1, 1, 1);
	set_color( 1, 0, 0 );
	drawOneAxis();
	model_view = given_basis;
	
	colors.pop();
	colors.pop();
	colors.pop();
	set_color( colors.top().r, colors.top().g, colors.top().b );
}

// Custom objects start here
float missile_dim = 0.5;
void draw_missile(bool fired)
{
	float missile_len = 0.9;

	mvstack.push(model_view);

	mvstack.push(model_view);
	model_view *= Scale(missile_len, missile_dim, missile_dim);
	drawSphere();
	model_view = mvstack.top(); mvstack.pop();

	// Flame
	if (fired)
	{
		model_view *= Translate(missile_len, 0.0, 0.0);
		model_view *= RotateY(90);
		mvstack.push(model_view);
		model_view *= Scale(0.5, 0.5, 1.5);
		if ( (int)( (TIME-(int)TIME)*10 ) % 2 == 0)
			set_color(0.8, 0.0, 0.0);
		else
			set_color(0.8, 0.6, 0.1);
		drawCone();
		model_view = mvstack.top(); mvstack.pop();
		colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
	}

	model_view = mvstack.top(); mvstack.pop();
}

float heli_body_dim = 1.0;
float missile_pos = heli_body_dim+missile_dim;
void draw_heli(float rot_x, float rot_y, float rot_z, bool show_missile)
{
	float
		body_len = 2.0,
		blade_len = 75.0,
		spin = 750.0,
		tail_len = 3.0,
		tail_dim = 0.25;

	mvstack.push(model_view);

	model_view *= RotateX(rot_x);
	model_view *= RotateY(rot_y);
	model_view *= RotateZ(rot_z);

	// Body
	mvstack.push(model_view);
	model_view *= Scale(body_len, heli_body_dim, heli_body_dim);
	drawHeliSphere();
	model_view = mvstack.top(); mvstack.pop();

	// missile
	if (show_missile)
	{
		mvstack.push(model_view);
		model_view *= Translate(0.0, -missile_pos, 0.0);
		draw_missile(false);
		model_view = mvstack.top(); mvstack.pop();
	}

	// Landing thingies
	mvstack.push(model_view);
	set_color(0.1, 0.1, 0.1);
	model_view *= Translate(0.0, -1.0, -0.75);
	mvstack.push(model_view);
	model_view *= Scale(2*body_len, 0.5, tail_dim);
	drawPyramid();
	model_view = mvstack.top(); mvstack.pop();
	model_view *= Translate(0.0, 0.0, 1.5);
	mvstack.push(model_view);
	model_view *= Scale(2*body_len, 0.5, tail_dim);
	drawPyramid();
	model_view = mvstack.top(); mvstack.pop();
	colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
	model_view = mvstack.top(); mvstack.pop();

	// Propeller blades
	mvstack.push(model_view);
	model_view *= Translate(0.0, 1.1, 0.0);
	model_view *= RotateY(spin*TIME);
	model_view *= Scale(0.1, 0.1, 0.1);
	drawHeliSphere();
	mvstack.push(model_view);
	model_view *= Scale(blade_len, 1.0, 1.0);
	set_color(0.1, 0.1, 0.1);
	drawCube();
	model_view = mvstack.top(); mvstack.pop();
	model_view *= Scale(1.0, 1.0, blade_len);
	drawCube();
	colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
	model_view = mvstack.top(); mvstack.pop();

	// Tail
	model_view *= Translate(body_len+(tail_len/2.0), 0.0, 0.0);
	mvstack.push(model_view);
	model_view *= Scale(tail_len, tail_dim, tail_dim);
	drawHeliCube();
	model_view = mvstack.top(); mvstack.pop();
	model_view *= Translate(tail_len/2.0, 0.0, 0.0);
	mvstack.push(model_view);
	model_view *= RotateZ(90);
	model_view *= Scale(tail_dim, 2*tail_dim, tail_len/2.0);
	set_color(0.1, 0.1, 0.1);
	drawPyramid();
	model_view = mvstack.top(); mvstack.pop();
	mvstack.push(model_view);
	model_view *= Translate(-tail_dim/2.0, tail_dim/2.0, 0.0);
	model_view *= Scale(tail_dim, tail_len/3.0, tail_dim);
	drawPyramid();
	colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
	model_view = mvstack.top(); mvstack.pop();

	model_view = mvstack.top(); mvstack.pop();
}

void draw_wings(float bee_dim, bool hit)
{
	float
		thick = 1 / 5.0,
		wid = 1.0,
		len = 3.0,
		amp = 25.0,
		omega = 600.0;

	// Right
	mvstack.push(model_view);
	model_view *= Translate(0.0f, bee_dim/2.0f, bee_dim/2.0f);
	if(!hit) // Flap wings
		model_view *= RotateX(amp*sin(omega*DegreesToRadians*TIME));
	mvstack.push(model_view);
	model_view *= Translate(bee_dim/2.0f, thick/2.0f, 0.0f);
	model_view *= RotateX(90);
	model_view *= Scale(wid, len, thick);
	drawBeePyramid();
	model_view = mvstack.top(); mvstack.pop();
	mvstack.push(model_view);
	model_view *= Translate(-bee_dim/2.0f, thick/2.0f, 0.0f);
	model_view *= RotateX(90);
	model_view *= Scale(wid, len, thick);
	drawBeePyramid();
	model_view = mvstack.top(); mvstack.pop();
	model_view = mvstack.top(); mvstack.pop();

	// Left
	mvstack.push(model_view);
	model_view *= Translate(0.0f, bee_dim/2.0f, -bee_dim/2.0f);
	if(!hit)
		model_view *= RotateX(amp*(-sin(omega*DegreesToRadians*TIME)));
	mvstack.push(model_view);
	model_view *= Translate(bee_dim/2.0f, thick/2.0f, 0.0f);
	model_view *= RotateX(-90);
	model_view *= Scale(wid, len, thick);
	drawBeePyramid();
	model_view = mvstack.top(); mvstack.pop();
	mvstack.push(model_view);
	model_view *= Translate(-bee_dim/2.0f, thick/2.0f, 0.0f);
	model_view *= RotateX(-90);
	model_view *= Scale(wid, len, thick);
	drawBeePyramid();
	model_view = mvstack.top(); mvstack.pop();
	model_view = mvstack.top(); mvstack.pop();
}

void draw_leg(float bee_dim, float x, float z, bool left, bool hit)
{
	float
		dim = 1/5.0,
		len = 0.75,
		amp = 15.0,
		omega = 400.0,
		leg_shift = dim/2.0f,
		flap = sin(omega*DegreesToRadians*TIME);

	if (left)
	{
		leg_shift *= -1.0f;
		flap *= -1.0f;
	}

	set_color(0.1f, 0.1f, 0.1f);

	// Upper leg
	mvstack.push(model_view);
	model_view *= Translate(x, -bee_dim/2.0f, z);
	if(!hit) // Move legs
		model_view *= RotateX(amp*flap);
	mvstack.push(model_view);
	model_view *= Translate(0.0f, -len/2.0f, leg_shift);
	model_view *= Scale(dim, len, dim);
	drawCube();
	model_view = mvstack.top(); mvstack.pop();

	// Lower leg
	model_view *= Translate(0.0f, -len, 0.0f);
	if(!hit)
		model_view *= RotateX(amp*flap);
	model_view *= Translate(0.0f, -len/2.0f, leg_shift);
	model_view *= Scale(dim, len, dim);
	drawCube();
	model_view = mvstack.top(); mvstack.pop();

	colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
}

void draw_bee(bool hit, float rot_x, float rot_y, float rot_z)
{
	float
		dim = 1.0,
		body_len = 1.75,
		butt_len = 1.5,
		head_dim = dim / 1.5f,
		circle_omega = 100.0,
		vert_amp = 7.0,
		vert_omega = 300.0;

	model_view *= RotateX(rot_x);
	model_view *= RotateY(rot_y);
	model_view *= RotateZ(rot_z);

	// Make bee bigger for this project
	model_view *= Scale(2.0, 2.0, 2.0);

	// Body
	mvstack.push(model_view);
	model_view *= Scale(body_len, dim, dim);
	if (hit) // Darken color/texture to "burnt"
		set_color(0.15, 0.15, 0.15);
	drawBeeSphere();
	if (hit)
		colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
	model_view = mvstack.top(); mvstack.pop();

	// Head
	mvstack.push(model_view);
	set_color(0.1, 0.1, 0.1);
	model_view *= Translate(head_dim+(body_len/2.0f), 0.0f, 0.0f);
	model_view *= Scale(head_dim, head_dim, head_dim);
	drawSphere();
	colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
	model_view = mvstack.top(); mvstack.pop();

	// Booty
	mvstack.push(model_view);
	model_view *= Translate((-body_len/2.0f)-butt_len, 0.0f, 0.0f);
	model_view *= Scale(butt_len, dim, dim);
	if (hit)
		set_color(0.15, 0.15, 0.15);
	drawBeeSphere();
	if (hit)
	 colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
	model_view = mvstack.top(); mvstack.pop();

	draw_wings(dim, hit);

	draw_leg(dim, dim / 2.0f, dim / 2.0f, false, hit);
	draw_leg(dim, 0.0f, dim / 2.0f, false, hit);
	draw_leg(dim, -dim / 2.0f, dim / 2.0f, false, hit);
	draw_leg(dim, dim / 2.0f, -dim / 2.0f, true, hit);
	draw_leg(dim, 0.0f, -dim / 2.0f, true, hit);
	draw_leg(dim, -dim / 2.0f, -dim / 2.0f, true, hit);
}

void draw_stinger(bool rotate)
{
	mvstack.push(model_view);
	set_color(1.0, 1.0, 1.0);
	if(rotate)
		model_view *= RotateX(180*TIME);
	model_view *= RotateZ(-90);
	model_view *= Scale(1.0, 2.0, 1.0);
	drawPyramid();
	colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
	model_view = mvstack.top(); mvstack.pop();
}

void draw_enlarging_ball(bool offset, float t)
{
	float init_size = 0.3;
	float enlarge_rate = 0.8*t;

	if (offset)
	{
		if ((int)((TIME-(int)TIME)*10) % 2 == 0)
			set_color(0.8, 0.0, 0.0);
		else
			set_color(0.8, 0.6, 0.1);
	}
	else
	{
		if ((int)((TIME-(int)TIME)*10) % 2 == 1)
			set_color(0.8, 0.0, 0.0);
		else
			set_color(0.8, 0.6, 0.1);
	}

	mvstack.push(model_view);
	model_view *= Scale(init_size+enlarge_rate, init_size+enlarge_rate, init_size+enlarge_rate);
	drawSphere();
	colors.pop(); set_color(colors.top().r, colors.top().g, colors.top().b);
	model_view = mvstack.top(); mvstack.pop();
}

void draw_explosion(float t)
{
	float d = 0.2+0.5*t;

	mvstack.push(model_view);
	model_view *= RotateX(123*TIME);
	model_view *= RotateY(212*TIME);
	model_view *= RotateZ(321*TIME);

	// +x
	mvstack.push(model_view);
	model_view *= Translate(d, 0.0, 0.0);
	draw_enlarging_ball(true, t);
	// +x+y+z
	mvstack.push(model_view);
	model_view *= Translate(0.0, d, d);
	draw_enlarging_ball(true, t);
	model_view = mvstack.top(); mvstack.pop();
	// +x+y-z
	mvstack.push(model_view);
	model_view *= Translate(0.0, d, -d);
	draw_enlarging_ball(false, t);
	model_view = mvstack.top(); mvstack.pop();
	// +x-y+z
	mvstack.push(model_view);
	model_view *= Translate(0.0, -d, d);
	draw_enlarging_ball(false, t);
	model_view = mvstack.top(); mvstack.pop();
	// +x-y-z
	mvstack.push(model_view);
	model_view *= Translate(0.0, -d, -d);
	draw_enlarging_ball(false, t);
	model_view = mvstack.top(); mvstack.pop();
	model_view = mvstack.top(); mvstack.pop();

	// -x
	mvstack.push(model_view);
	model_view *= Translate(-d, 0.0, 0.0);
	draw_enlarging_ball(true, t);
	// -x+y+z
	mvstack.push(model_view);
	model_view *= Translate(0.0, d, d);
	draw_enlarging_ball(true, t);
	model_view = mvstack.top(); mvstack.pop();
	// -x+y-z
	mvstack.push(model_view);
	model_view *= Translate(0.0, d, -d);
	draw_enlarging_ball(false, t);
	model_view = mvstack.top(); mvstack.pop();
	// -x-y+z
	mvstack.push(model_view);
	model_view *= Translate(0.0, -d, d);
	draw_enlarging_ball(false, t);
	model_view = mvstack.top(); mvstack.pop();
	// -x-y-z
	mvstack.push(model_view);
	model_view *= Translate(0.0, -d, -d);
	draw_enlarging_ball(false, t);
	model_view = mvstack.top(); mvstack.pop();
	model_view = mvstack.top(); mvstack.pop();

	// +y
	mvstack.push(model_view);
	model_view *= Translate(0.0, d, 0.0);
	draw_enlarging_ball(false, t);
	model_view = mvstack.top(); mvstack.pop();
	// -y
	mvstack.push(model_view);
	model_view *= Translate(0.0, -d, 0.0);
	draw_enlarging_ball(true, t);
	model_view = mvstack.top(); mvstack.pop();

	// +z
	mvstack.push(model_view);
	model_view *= Translate(0.0, 0.0, d);
	draw_enlarging_ball(false, t);
	model_view = mvstack.top(); mvstack.pop();
	// -z
	mvstack.push(model_view);
	model_view *= Translate(0.0, 0.0, -d);
	draw_enlarging_ball(true, t);
	model_view = mvstack.top(); mvstack.pop();

	model_view = mvstack.top(); mvstack.pop();
}

vec4 eye_orig = eye;
vec4 eye_stop1, eye_stop2;
int calls = 0;
double prev_time = 0.0;
void display(void)
{
	basis_id = 0;
    glClearColor( 0.1, 0.2, 0.4, 1 );
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	set_color( 0.6, 0.6, 0.6 );
	
	model_view = LookAt( eye, ref, up );

	model_view *= orientation;
    model_view *= Scale(zoom);												drawAxes(basis_id++);

	// Frame rate output
	if (animate == 1)
	{
		calls++;
		if (TIME - prev_time >= 1.0)
		{
			std::cout << calls << " frames/sec" << std::endl;
			prev_time = TIME;
			calls = 0;
		}
	}

	float up_down1 = 5.0+0.5*sin(10*TIME);
	float up_down2 = 0.5*sin(5*TIME)-5.0;
	float side_wobble1 = 2*sin(25*TIME);
	float side_wobble2 = 4*sin(25*TIME);

	// Rotate around single heli to facing front of heli
	if (TIME < 3.0)
	{
		eye = RotateY(90*TIME)*eye_orig;
		eye_stop1 = eye;
	}
	// Zoom out
	else if (TIME > 3.0 && TIME < 4.0)
	{
		eye = Translate(-15*(TIME-3.0), 0.0, 0.0)*eye_stop1;
		eye_stop2 = eye;
	}
	// Pause for 1 second then rotate to facing back of heli
	else if (TIME > 5.0 && TIME < 8.0)
	{
		eye = RotateY(60*(TIME-5.0))*eye_stop2;
		eye_stop1 = eye;
	}
	else;

	// Initial heli flight
	if (TIME < 8.0)
	{
		draw_heli(side_wobble1, 0.0, 15.0, true);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(10.0, up_down1, 10.0);
		draw_heli(side_wobble2, 0.0, 15.0, true);
		model_view *= Translate(10.0, up_down2, 10.0);
		draw_heli(side_wobble1, 0.0, 15.0, true);
		model_view *= Translate(10.0, up_down1, 10.0);
		draw_heli(side_wobble2, 0.0, 15.0, true);
		model_view *= Translate(10.0, up_down2, 10.0);
		draw_heli(side_wobble1, 0.0, 15.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(10.0, up_down1, -10.0);
		draw_heli(side_wobble2, 0.0, 15.0, true);
		model_view *= Translate(10.0, up_down2, -10.0);
		draw_heli(side_wobble1, 0.0, 15.0, true);
		model_view *= Translate(10.0, up_down1, -10.0);
		draw_heli(side_wobble2, 0.0, 15.0, true);
		model_view *= Translate(10.0, up_down2, -10.0);
		draw_heli(side_wobble1, 0.0, 15.0, true);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Bee comes into view and heli get into position
	else if (TIME > 8.0 && TIME < 12.0)
	{
		float t = TIME-8.0;
		float x_heli = 2.6*t;
		float y_heli = x_heli/2.0;

		float untilt;
		if (TIME < 10.0)
			untilt = 25 * (TIME - 8.0);
		else
			untilt = 50-(35*(TIME - 10.0)/2.0);

		float a = 0.5-(0.5*t/4.0);
		up_down1 = (5.0+a*sin(10*t));
		float b = 0.5-(0.5*t/4.0);
		up_down2 = (b*sin(5*t)-5.0);

		eye = Translate(12*t, 0.0, 0.0)*eye_stop1;
		eye_stop2 = eye;

		draw_heli(side_wobble1, 0.0, 15.0-untilt, true);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(10.0-x_heli, up_down1-y_heli, 10.0);
		draw_heli(side_wobble2, 0.0, 15.0-untilt, true);
		model_view *= Translate(10.0-x_heli, up_down2+y_heli, 10.0);
		draw_heli(side_wobble1, 0.0, 15.0-untilt, true);
		model_view *= Translate(10.0-x_heli, up_down1-y_heli, 10.0);
		draw_heli(side_wobble2, 0.0, 15.0-untilt, true);
		model_view *= Translate(10.0-x_heli, up_down2+y_heli, 10.0);
		draw_heli(side_wobble1, 0.0, 15.0-untilt, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(10.0-x_heli, up_down1-y_heli, -10.0);
		draw_heli(side_wobble2, 0.0, 15.0-untilt, true);
		model_view *= Translate(10.0-x_heli, up_down2+y_heli, -10.0);
		draw_heli(side_wobble1, 0.0, 15.0-untilt, true);
		model_view *= Translate(10.0-x_heli, up_down1-y_heli, -10.0);
		draw_heli(side_wobble2, 0.0, 15.0-untilt, true);
		model_view *= Translate(10.0-x_heli, up_down2+y_heli, -10.0);
		draw_heli(side_wobble1, 0.0, 15.0-untilt, true);
		model_view = mvstack.top(); mvstack.pop();

		// Bee
		mvstack.push(model_view);
		model_view *= Translate((450/4.0)*t-500.0, 0.0, -10.0);
		draw_bee(false, 0.0, (90/4.0)*t-90.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
		mvstack.push(model_view);
		model_view *= Translate((450/4.0)*t-500.0, 0.0, 10.0);
		draw_bee(false, 0.0, 90.0-(90/4.0)*t, 0.0);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Pause for half second in position
	else if (TIME > 12.0 && TIME < 12.5)
	{
		draw_heli(side_wobble1, 0.0, 0.0, true);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, -10.0);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Change camera angle, right bee begins attack
	else if (TIME > 12.5 && TIME < 13.5)
	{
		eye = Translate(-35.0, 25.0, 20.0)*eye_stop2;
		eye = RotateY(-10)*eye;
		eye_stop1 = eye;

		draw_heli(side_wobble1, 0.0, 0.0, true);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -10.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 10.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, -10.0);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		draw_bee(false, 0.0, 0.0, 90*(TIME-12.5));
		model_view = mvstack.top(); mvstack.pop();
	}
	// Left bee begins attack, right bee in position to fire
	else if (TIME > 13.5 && TIME < 14.5)
	{
		draw_heli(side_wobble1, 0.0, 0.0, true);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -10.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 10.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, -10.0);
		draw_bee(false, 0.0, 0.0, 90*(TIME-13.5));
		model_view = mvstack.top(); mvstack.pop();
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		draw_bee(false, 0.0, 0.0, 90+90*(TIME-13.5));
		model_view = mvstack.top(); mvstack.pop();
	}
	// Right bee fires and left bee in position to fire
	else if (TIME > 14.5 && TIME < 15.5)
	{
		draw_heli(side_wobble1, 0.0, 0.0, true);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -10.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 10.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, -10.0);
		draw_bee(false, 0.0, 0.0, 90+90*(TIME-14.5));
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		mvstack.push(model_view);
		draw_bee(false, 0.0, 0.0, 180+90*(TIME-14.5));
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(7.5+20*(TIME-14.5), 0.0, 0.0);
		draw_stinger(true);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Right bee returns to position and left bee fires
	// One of the right helis starts flying away
	else if (TIME > 15.5 && TIME < 16.5)
	{
		float t = TIME-15.5;

		draw_heli(side_wobble1, 0.0, 0.0, true);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -10.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);

		// Wussy heli
		mvstack.push(model_view);
		model_view *= Translate(0.0, 5*t, 0.0);
		draw_heli(0.0, 10.0+55*t, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Farthest right
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, -10.0);
		mvstack.push(model_view);
		draw_bee(false, 0.0, 0.0, 180+90*t);
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(7.5+20*t, 0.0, 0.0);
		draw_stinger(true);
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		mvstack.push(model_view);
		draw_bee(false, 0.0, 0.0, 270+90*t);
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(27.5+20*t, 0.0, 0.0);
		draw_stinger(true);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Left bee completes flip and stinger reaches hit heli's original position
	// Heli gets hit and begins descent
	else if (TIME > 16.5 && TIME < 17.5)
	{
		float t = TIME-16.5;
		float wussy_speed = 15*t;

		draw_heli(side_wobble1, 0.0, 0.0, true);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);

		// Downed heli
		mvstack.push(model_view);
		model_view *= Translate(20*t, -10*t, 0.0);
		model_view *= RotateZ(-45*t);
		mvstack.push(model_view);
		model_view *= Translate(-2.25, 0.0, 0.0);
		draw_stinger(false);
		model_view = mvstack.top(); mvstack.pop();
		draw_heli(0.0, 0.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Left
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -10.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);

		// Dodging heli
		mvstack.push(model_view);
		model_view *= Translate(0.0, 5.0*t, 0.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);

		// Wussy heli
		mvstack.push(model_view);
		model_view *= Translate(wussy_speed, 5+5*t, 1.5*wussy_speed);
		draw_heli(0.0, 65+55*t, 15*t, true);
		model_view = mvstack.top(); mvstack.pop();

		// Farthest right
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, -10.0);
		mvstack.push(model_view);
		draw_bee(false, 0.0, 0.0, 270+90*t);
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(27.5+20*t, 0.0, 0.0);
		draw_stinger(true);
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Left stinger disappears
	// Shot down heli is gone
	// Left flank begins repositioning
	// Center heli fires
	else if (TIME > 17.5 && TIME < 18.5)
	{
		float t = TIME-17.5;
		float tilt = -45*cos(0.5*PI*t);
		float rotate = 5*t;
		float wussy_speed = 15*t;

		// Center heli
		mvstack.push(model_view);
		model_view *= RotateY(5.0);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		
		// Downed heli
		mvstack.push(model_view);
		model_view *= Translate(20+20*t, -10-20*t, 0.0);
		model_view *= RotateZ(-45-45*t);
		mvstack.push(model_view);
		model_view *= Translate(-2.25, 0.0, 0.0);
		draw_stinger(false);
		model_view = mvstack.top(); mvstack.pop();
		draw_heli(0.0, 0.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Left
		model_view *= Translate(0.0, 0.0, 10.0-10.0*t);
		draw_heli(tilt, -5.0+rotate, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(tilt, -10.0+rotate, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(tilt, -15.0+rotate, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		
		// Dodging heli
		mvstack.push(model_view);
		model_view *= Translate(0.0, 5.0-5.0*t, 0.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		
		// Wussy heli
		mvstack.push(model_view);
		model_view *= Translate(15+wussy_speed, 10+5*t, 22.5+1.5*wussy_speed);
		draw_heli(0.0, 120.0, 15.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Farthest right
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, -10.0);
		mvstack.push(model_view);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(47.5+20*t, 0.0, 0.0);
		draw_stinger(true);
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Left flank repositions
	// Center heli's missile on the way
	// Farthest left heli fires
	// Wussy heli is gone
	// Right bee rotates a bit to dodge
	else if (TIME > 18.5 && TIME < 20.5)
	{
		float t = TIME-18.5;

		// Center heli
		mvstack.push(model_view);
		model_view *= RotateY(5.0);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-20-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, 10.0);

		// Farthest left
		mvstack.push(model_view);
		model_view *= RotateY(-10.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, true);
		model_view *= Translate(0.0, 0.0, -10.0);

		// Wussy heli
		float wussy_speed = 15*t;
		mvstack.push(model_view);
		model_view *= Translate(30+wussy_speed, 15+5*t, 45+1.5*wussy_speed);
		draw_heli(0.0, 120.0, 15.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Farthest right
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 15.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, -10.0);
		mvstack.push(model_view);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(67.5+20*t, 0.0, 0.0);
		draw_stinger(true);
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		float dodge;
		if (TIME < 19.5)
			dodge = 0.0;
		else
			dodge = 45*sin(PI*(TIME-19.5));
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		draw_bee(false, dodge, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Right bee dodges center's missile
	// Farthest left's missile passes by
	// Left fires
	// Right flank fires
	else if (TIME > 20.5 && TIME < 22.5)
	{
		float t = TIME-20.5;

		// Center heli
		mvstack.push(model_view);
		model_view *= RotateY(5.0);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-60-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		mvstack.push(model_view);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(0.0, 0.0, 10.0);
		mvstack.push(model_view);
		model_view *= RotateY(-5.0);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(0.0, 0.0, 10.0);
		
		// Farthest left
		mvstack.push(model_view);
		model_view *= RotateY(-10.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-40-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Close-right heli
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		mvstack.push(model_view);
		if (TIME > 22.0)
		{
			draw_heli(side_wobble2, 0.0, 0.0, false);
			mvstack.push(model_view);
			model_view *= Translate(-20*(TIME-22.0), -missile_pos, 0.0);
			draw_missile(true);
			model_view = mvstack.top(); mvstack.pop();
		}
		else
			draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Right-center heli
		model_view *= Translate(0.0, 0.0, -10.0);
		mvstack.push(model_view);
		model_view *= RotateY(5.0);
		if (TIME > 21.75)
		{
			draw_heli(side_wobble2, 0.0, 0.0, false);
			mvstack.push(model_view);
			model_view *= Translate(-20*(TIME-21.75), -missile_pos, 0.0);
			draw_missile(true);
			model_view = mvstack.top(); mvstack.pop();
		}
		else
			draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();

		// Far-right heli
		model_view *= Translate(0.0, 0.0, -20.0);
		mvstack.push(model_view);
		model_view *= RotateY(15.0);
		if (TIME > 21.5)
		{
			draw_heli(side_wobble2, 0.0, 0.0, false);
			mvstack.push(model_view);
			model_view *= Translate(-20*(TIME-21.5), -missile_pos, 0.0);
			draw_missile(true);
			model_view = mvstack.top(); mvstack.pop();
		}
		else
			draw_heli(side_wobble2, 0.0, 0.0, true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, -10.0);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Center's missile gone after this
	// Farthest left's missile almost gone
	// Right bee gets hit
	// Left bee begins roll
	else if (TIME > 22.5 && TIME < 24.5)
	{
		float t = TIME-22.5;

		// Center heli
		mvstack.push(model_view);
		model_view *= RotateY(5.0);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-100-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		mvstack.push(model_view);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		if (TIME < 22.75)
		{
			mvstack.push(model_view);
			model_view *= Translate(-40-20*t, -missile_pos, 0.0);
			draw_missile(true);
			model_view = mvstack.top(); mvstack.pop();
		}
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(0.0, 0.0, 10.0);
		mvstack.push(model_view);
		model_view *= RotateY(-5.0);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-40-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(0.0, 0.0, 10.0);
		
		// Farthest left
		mvstack.push(model_view);
		model_view *= RotateY(-10.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-80-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Close-right heli
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		mvstack.push(model_view);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-10-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Right-center heli
		model_view *= Translate(0.0, 0.0, -10.0);
		mvstack.push(model_view);
		model_view *= RotateY(5.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-15-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Far-right heli
		model_view *= Translate(0.0, 0.0, -20.0);
		mvstack.push(model_view);
		model_view *= RotateY(15.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-20-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		if (TIME < 23.5)
		{
			model_view *= Translate(-50.0, 0.0, -10.0);
			draw_bee(false, 0.0, 0.0, 0.0);
		}
		else
		{
			model_view *= Translate(-50.0, 0.0, -10.0+10*(TIME-23.5));
			draw_bee(false, 360*sin(0.5*PI*(TIME-23.5)), 0.0, 0.0);
		}
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		if(TIME < 22.75)
			draw_bee(false, 0.0, 0.0, 0.0);
		else
		{
			draw_explosion(TIME-22.75);
			mvstack.push(model_view);
			model_view *= Translate(-20*(TIME-22.75), -10*(TIME-22.75), 0.0);
			draw_bee(true, 0.0, 0.0, 45+180*(TIME-22.75));
			model_view = mvstack.top(); mvstack.pop();
		}
		model_view = mvstack.top(); mvstack.pop();
	}
	// Farthest left's missile gone
	// Right bee death procedure begins
	// Left bee rolled away
	else if (TIME > 24.5 && TIME < 26.5)
	{
		float t = TIME-24.5;

		// Center heli
		mvstack.push(model_view);
		draw_heli(side_wobble1, 5.0, 0.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, 10.0);
		mvstack.push(model_view);
		model_view *= RotateY(-5.0);
		draw_heli(side_wobble1, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-80-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view *= Translate(0.0, 0.0, 10.0);

		// Farthest left
		mvstack.push(model_view);
		model_view *= RotateY(-10.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-120-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Close-right heli
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		mvstack.push(model_view);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-50-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Right-center heli
		model_view *= Translate(0.0, 0.0, -10.0);
		mvstack.push(model_view);
		model_view *= RotateY(5.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-55-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Far-right heli
		model_view *= Translate(0.0, 0.0, -20.0);
		mvstack.push(model_view);
		model_view *= RotateY(15.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		mvstack.push(model_view);
		model_view *= Translate(-60-20*t, -missile_pos, 0.0);
		draw_missile(true);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 0.0);
		draw_bee(false, 0.0, 0.0, 0.0);
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		mvstack.push(model_view);
		model_view *= Translate(-35-20*t, -17.5-15*t, 0.0);
		draw_bee(true, 0.0, 0.0, 180*t);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
	}
	// Right bee continues falling
	// Right flank's missiles collide
	// Left bee begins charge, center and close-left heli begin charge a second later
	else if (TIME > 26.5 && TIME < 28.5)
	{
		float t = TIME-26.5;

		float tilt, charge;
		if (TIME < 27.5)
		{
			tilt = 0.0;
			charge = 0.0;
		}
		else
		{
			tilt = 20*(TIME-27.5);
			charge = 10*(TIME-27.5);
		}

		// missile explosion
		mvstack.push(model_view);
		model_view *= Translate(-90.0, 0.0, -10.0);
		draw_explosion(t);
		model_view = mvstack.top(); mvstack.pop();

		// Center heli
		mvstack.push(model_view);
		model_view *= RotateY(5.0-2.5*t);
		model_view *= Translate(-charge, 0.0, 0.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view = mvstack.top(); mvstack.pop();

		// For the left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);

		// Charging left
		mvstack.push(model_view);
		model_view *= RotateY(-10*t);
		model_view *= Translate(-charge, 0.0, 0.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view = mvstack.top(); mvstack.pop();

		// Left
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble1, -5.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -10.0, 0.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, -20.0);
		draw_heli(side_wobble2, 15.0, 0.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(10*t-50.0, 0.0, 0.0);
		draw_bee(false, 0.0, 0.0, -10*t);
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		mvstack.push(model_view);
		model_view *= Translate(-75-20*t, -47.5-20*t, 0.0);
		draw_bee(true, 0.0, 0.0, 180*t);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
	}
	// Right bee disappears after this
	// Center and close-left heli meet the charge
	else if (TIME > 28.5 && TIME < 29.2)
	{
		float t = TIME-28.5;
		float charge = -10-10*t;

		eye = Translate(-60*t, 10*t, 30*t)*eye_stop1;
		eye_stop2 = eye;

		// Center heli
		mvstack.push(model_view);
		model_view *= Translate(charge, 0.0, 0.0);
		draw_heli(0.0, 0.0, 20.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// For the left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);

		// Charging left
		mvstack.push(model_view);
		model_view *= RotateY(-20);
		model_view *= Translate(charge, 0.0, 0.0);
		draw_heli(0.0, 0.0, 20.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Left
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble1, -5.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -10.0, 0.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, -20.0);
		draw_heli(side_wobble2, 15.0, 0.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(10*t-30.0, 0.0, 0.0);
		draw_bee(false, 0.0, 0.0, -20);
		model_view = mvstack.top(); mvstack.pop();

		// Right bee
		mvstack.push(model_view);
		model_view *= Translate(-50.0, 0.0, 10.0);
		mvstack.push(model_view);
		model_view *= Translate(-115-20*t, -87.5-25*t, 0.0);
		draw_bee(true, 0.0, 0.0, 180*t);
		model_view = mvstack.top(); mvstack.pop();
		model_view = mvstack.top(); mvstack.pop();
	}
	// Bee and helis collide
	else if (TIME > 29.2 && TIME < 33.0)
	{
		float t = TIME-29.2;

		// Collision explosion
		mvstack.push(model_view);
		model_view *= Translate(-17.0, 0.0, 0.0);
		draw_explosion(t);
		model_view = mvstack.top(); mvstack.pop();

		// Center heli
		mvstack.push(model_view);
		model_view *= Translate(-17+20*t, -10*t*t, 0.0);
		draw_heli(0.0, 0.0, -180*t, false);
		model_view = mvstack.top(); mvstack.pop();

		// For the left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0);

		// Charging left
		mvstack.push(model_view);
		model_view *= RotateY(-20);
		model_view *= Translate(-17+20*t, -10*t*t, 0.0);
		draw_heli(0.0, 0.0, -180*t, false);
		model_view = mvstack.top(); mvstack.pop();

		// Left
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble1, -5.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -10.0, 0.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, -20.0);
		draw_heli(side_wobble2, 15.0, 0.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(-23-20*t, -10*t*t, 0.0);
		draw_bee(true, 0.0, 0.0, 180*t);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Other heli get into formation
	else if (TIME > 33.0 && TIME < 34.0)
	{
		float t = TIME-33.0;

		// Center heli
		mvstack.push(model_view);
		model_view *= Translate(59+20*t, -144-10*t*t, 0.0);
		draw_heli(0.0, 0.0, -180*t, false);
		model_view = mvstack.top(); mvstack.pop();

		// For the left
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, 10.0-10.0*t);

		// Charging left
		mvstack.push(model_view);
		model_view *= RotateY(-20);
		model_view *= Translate(59+20*t, -144-10*t*t, 0.0);
		draw_heli(0.0, 0.0, -180*t, false);
		model_view = mvstack.top(); mvstack.pop();

		// Left
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble1, -5.0+5.0*t, 0.0, false);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(side_wobble2, -10.0+10.0*t, 0.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(0.0, 0.0, -10.0+10.0*t);
		draw_heli(side_wobble2, 0.0, 0.0, false);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(side_wobble2, 5.0-5.0*t, 0.0, false);
		model_view *= Translate(0.0, 0.0, -20.0+10.0*t);
		draw_heli(side_wobble2, 15.0-15.0*t, 0.0, false);
		model_view = mvstack.top(); mvstack.pop();

		// Left bee
		mvstack.push(model_view);
		model_view *= Translate(-99-20*t, -144-10*t*t, 0.0);
		draw_bee(true, 0.0, 0.0, 180*t);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Helis begin flying off
	else if (TIME > 34.0 && TIME < 35.0)
	{
		float tilt = 15*(TIME-34.0);
		float v = 20*(TIME-34.0);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(-v, 0.0, 10.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(-v, 0.0, 0.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view = mvstack.top(); mvstack.pop();
	}
	// Helis fly off screen
	else if (TIME > 35.0 && TIME < 37.0)
	{
		float tilt = 15.0;
		float v = 20+20*(TIME-35.0);

		// Left
		mvstack.push(model_view);
		model_view *= Translate(-v, 0.0, 10.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view *= Translate(0.0, 0.0, 10.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view = mvstack.top(); mvstack.pop();

		// Right
		mvstack.push(model_view);
		model_view *= Translate(-v, 0.0, 0.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view *= Translate(0.0, 0.0, -10.0);
		draw_heli(0.0, 0.0, tilt, false);
		model_view = mvstack.top(); mvstack.pop();
	}
	else;
   
    glutSwapBuffers();
}

void myReshape(int w, int h)	// Handles window sizing and resizing.
{    
    mat4 projection = Perspective( 50, (float)w/h, 1, 1000 );
    glUniformMatrix4fv( uProjection, 1, GL_FALSE, transpose(projection) );
	glViewport(0, 0, g_width = w, g_height = h);	
}		

void instructions() {	 std::cout <<	"Press:"									<< '\n' <<
										"  r to restore the original view."			<< '\n' <<
										"  0 to restore the original state."		<< '\n' <<
										"  a to toggle the animation."				<< '\n' <<
										"  b to show the next basis's axes."		<< '\n' <<
										"  B to show the previous basis's axes."	<< '\n' <<
										"  q to quit."								<< '\n';	}

void myKey(unsigned char key, int x, int y)
{
    switch (key) {
        case 'q':   case 27:				// 27 = esc key
            exit(0); 
		case 'b':
			std::cout << "Basis: " << ++basis_to_display << '\n';
			break;
		case 'B':
			std::cout << "Basis: " << --basis_to_display << '\n';
			break;
        case 'a':							// toggle animation           		
            if(animate) std::cout << "Elapsed time " << TIME << '\n';
            animate = 1 - animate ; 
            break ;
		case '0':							// Add code to reset your object here.
			TIME = 0;	TM.Reset() ;											
        case 'r':
			orientation = mat4();			
            break ;
    }
    glutPostRedisplay() ;
}

int main() 
{
	char title[] = "Title";
	int argcount = 1;	 char* title_ptr = title;
	glutInit(&argcount,		 &title_ptr);
	glutInitWindowPosition (230, 70);
	glutInitWindowSize     (g_width, g_height);
	glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow(title);
	#if !defined(__APPLE__) && !defined(EMSCRIPTEN)
		glewExperimental = GL_TRUE;
		glewInit();
	#endif
    std::cout << "GL version " << glGetString(GL_VERSION) << '\n';
	instructions();
	init();
	
	glutDisplayFunc(display);
    glutIdleFunc(idleCallBack) ;
    glutReshapeFunc (myReshape);
    glutKeyboardFunc( myKey );
    glutMouseFunc(myMouseCallBack) ;
    glutMotionFunc(myMotionCallBack) ;
    glutPassiveMotionFunc(myPassiveMotionCallBack) ;

	glutMainLoop();
}