/*
    ShadowMapping sem Shaders
    Copyright (C) 2009 Vitor Fernando Pamplona

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <GL/glut.h>
#include <cstdio>
#include <cmath>

class Scene {
private:
	// Desenha o chão no plano Y=0.
	// O tamanho do plano é 200x200 com o seu centro em 0,0,0
	void drawFloor() {
		glColor3f(0.808, 0.015, 0.024);

		glBegin(GL_TRIANGLE_STRIP);
			glNormal3f(0,1,0);
			glVertex3f(-100, 0, -100);
			glVertex3f(-100, 0,  100);
			glVertex3f( 100, 0, -100);
			glVertex3f( 100, 0,  100);
		glEnd();
	}

	// Desenha um torus para projetar a sombra
	// Posição do torus está no centro a 25 pontos acima do plano
	void drawTorus() {
		glPushMatrix();

		// Alterando a posição e rotação do torus
		glTranslatef(0, 25, 0);
		glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

		// Desenhando o torys
		glColor3f(0.308, 0.615, 0.824);
		glutSolidTorus(5, 10, 60, 60);

		glPopMatrix();
	}
public:
	void draw() {
		drawTorus();
		drawFloor();
	}

	Scene() {}
	virtual ~Scene() {}
};

class Camera {
public:
	Camera() {}
	virtual ~Camera() {}

	// Prepara a câmera.
	void setup(int window_width, int window_height) {
		glViewport(0,0,window_width,window_height);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		// fovy, aspect, near, far.
		gluPerspective(60, 1, 85, 400);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		gluLookAt(0, 200, 200,   // posição
				  0, 0, 0,       // olhando para...
				  0, 1, 0);      // up vector.

		//Limpa o buffer de cor e o Depth Buffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
};

class Light {
	float lightAngleInRad;
	GLenum lightNumber;

public:
	GLfloat lightPos[4];

	Light(GLenum _lightNumber) {
		lightAngleInRad = 0;
		lightNumber = _lightNumber;
	}
	virtual ~Light() {}

	void updatePosition() {
		lightAngleInRad += 3.1416/180.0f;
		lightPos[0] = sin(lightAngleInRad) * 100;
		lightPos[1] = 100;
		lightPos[2] = cos(lightAngleInRad) * 100;
		lightPos[3] = 1;

		glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	}

	void enable() {
	    glEnable(lightNumber);

	    GLfloat lightColorAmb[4] = {0.4f, 0.4f, 0.4f, 1.0f};
	    GLfloat lightColorSpc[4] = {0.2f, 0.2f, 0.2f, 1.0f};
	    GLfloat lightColorDif[4] = {0.4f, 0.4f, 0.4f, 1.0f};
	    glLightfv(GL_LIGHT0, GL_AMBIENT, lightColorAmb);
	    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColorDif);
	    glLightfv(GL_LIGHT0, GL_SPECULAR, lightColorSpc);
	    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
	}

	void disable() {
		glDisable(lightNumber);
	}

	void draw() {
		glPushMatrix();

		// Alterando a posição e rotação do torus
		glTranslatef(lightPos[0], lightPos[1], lightPos[2]);

			// Desenhando o torys
		glColor3f(1, 1, 1);
		glutSolidSphere(2, 30, 30);

		glPopMatrix();
	}
};

class ShadowMapping {
	// Considera apenas uma fonte de luz.
	GLuint shadowMapTexture;

	// Este tamanho não pode ser maior que a resolução da window.
	// Para isto, deve-se usar FBO com Off-screen rendering.
	int shadowMapSize;

	GLfloat textureTrasnformS[4];
	GLfloat textureTrasnformT[4];
	GLfloat textureTrasnformR[4];
	GLfloat textureTrasnformQ[4];
private:
	void createDepthTexture() {
		//Create the shadow map texture
		glGenTextures(1, &shadowMapTexture);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

		//Enable shadow comparison
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

		//Shadow comparison should be true (ie not in shadow) if r<=texture
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

		//Shadow comparison should generate an INTENSITY result
		glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
	}

	void loadTextureTransform() {
		GLfloat lightProjectionMatrix[16];
		GLfloat lightViewMatrix[16];

		// Busca as matrizes de view e projection da luz
		glGetFloatv(GL_PROJECTION_MATRIX, lightProjectionMatrix);
		glGetFloatv(GL_MODELVIEW_MATRIX, lightViewMatrix);

		// Salva o estado da matrix mode.
		glPushAttrib(GL_TRANSFORM_BIT);
		glMatrixMode(GL_TEXTURE);
		glPushMatrix();

		//Calculate texture matrix for projection
		//This matrix takes us from eye space to the light's clip space
		//It is postmultiplied by the inverse of the current view matrix when specifying texgen
		GLfloat biasMatrix[16]= {0.5f, 0.0f, 0.0f, 0.0f,
								 0.0f, 0.5f, 0.0f, 0.0f,
								 0.0f, 0.0f, 0.5f, 0.0f,
								 0.5f, 0.5f, 0.5f, 1.0f};	//bias from [-1, 1] to [0, 1]

		GLfloat textureMatrix[16];

		// Aplica as 3 matrizes em uma só, levando um fragmento em 3D para o espaço
		// canônico da câmera.
		glLoadMatrixf(biasMatrix);
		glMultMatrixf(lightProjectionMatrix);
		glMultMatrixf(lightViewMatrix);
		glGetFloatv(GL_TEXTURE_MATRIX, textureMatrix);

		// Separa as colunas em arrays diferentes por causa da opengl
		for (int i=0; i<4; i++) {
			textureTrasnformS[i] = textureMatrix[i*4];
			textureTrasnformT[i] = textureMatrix[i*4+1];
			textureTrasnformR[i] = textureMatrix[i*4+2];
			textureTrasnformQ[i] = textureMatrix[i*4+3];
		}

		glPopMatrix();
		glPopAttrib();
	}

public:
	void enableDepthCapture() {
		// Protege o código anterior a esta função
		glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_VIEWPORT_BIT | GL_COLOR_BUFFER_BIT);

		// Se a textura ainda não tiver sido criada, crie
		if (!shadowMapTexture) createDepthTexture();

		// Seta a viewport com o mesmo tamanho da textura.
		// O tamanho da viewport não pode ser maior que o tamanho da tela.
		// SE for, deve-se usar offline rendering e FBOs.
		glViewport(0, 0, shadowMapSize, shadowMapSize);

		// Calcula a transformação do espaço de câmera para o espaço da luz
		// e armazena a transformação para ser utilizada no teste de sombra do rendering
		loadTextureTransform();

		// Habilita Offset para evitar flickering.
		// Desloca o mapa de altura 1.9 vezes + 4.00 para trás.
		glPolygonOffset(1.9, 4.00);
		glEnable(GL_POLYGON_OFFSET_FILL);

		// Flat shading for speed
		glShadeModel(GL_FLAT);
		// Disable Lighting for performance.
		glDisable(GL_LIGHTING);
		// Não escreve no buffer de cor, apenas no depth
		glColorMask(0, 0, 0, 0);
	}

	void disableDepthCapture() {
		// Copia o Depth buffer para a textura.
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);

		// SubTexture não realoca a textura toda, como faz o texture
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, shadowMapSize, shadowMapSize);

		// Limpa o Buffer de profundidade
		glClear(GL_DEPTH_BUFFER_BIT);

		// Retorna as configurações anteriores ao depthCapture
		glPopAttrib();
	}

	void enableShadowTest() {
		// Protege o código anterior a esta função
		glPushAttrib(GL_TEXTURE_BIT |  GL_ENABLE_BIT);

		// Habilita a geração automática de coordenadas de textura do ponto de vista da câmera
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

		// Aplica a transformação destas coordenadas para o espaço da luz
		glTexGenfv(GL_S, GL_EYE_PLANE, textureTrasnformS);
		glTexGenfv(GL_T, GL_EYE_PLANE, textureTrasnformT);
		glTexGenfv(GL_R, GL_EYE_PLANE, textureTrasnformR);
		glTexGenfv(GL_Q, GL_EYE_PLANE, textureTrasnformQ);

		// Ativa
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_R);
		glEnable(GL_TEXTURE_GEN_Q);

		//Bind & enable shadow map texture
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture);
	}

	void disableShadowTest() {
		// Retorna as configurações anteriores do programa
		glPopAttrib();
	}

	ShadowMapping(int resolution = 512) {
		shadowMapTexture = NULL;
		shadowMapSize = resolution;
	}
	virtual ~ShadowMapping() {
		shadowMapTexture = NULL;
		glDeleteTextures(1, &shadowMapTexture);
	}
};

int SCREEN_SIZE = 600;
Scene scene;
Light light(GL_LIGHT0);
Camera camera;
ShadowMapping shadowRenderer;

// Prepara a câmera real.
void setupCameraInLightPosition() {
	//Use viewport the same size as the shadow map
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// fovy, aspect, near, far.
	gluPerspective(60, 1, 85, 400);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(light.lightPos[0], light.lightPos[1], light.lightPos[2],   // posição
			  0, 0, 0, // olhando para...
			  0, 1, 0); // up vector.
}

// Callback da glut.
void display() {
	// Limpa os Buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Posiciona uma camera na posição da luz
	setupCameraInLightPosition();

	shadowRenderer.enableDepthCapture();
	scene.draw();
	shadowRenderer.disableDepthCapture();

	camera.setup(SCREEN_SIZE, SCREEN_SIZE);

    shadowRenderer.enableShadowTest();
    light.enable();
    scene.draw();
	light.disable();
    shadowRenderer.disableShadowTest();

	light.draw();

	glutSwapBuffers();
	glutPostRedisplay();  // Pede para a glut chamar esta função novamente
}

// Cria o buffer para renderizar a primeira passada.
void setupGL() {

	// Configuração dos Algoritimos de Visibilidade
    glClearDepth(1.0f);       // Máximo valor para a Limpeza do DepthBuffer
    glEnable(GL_DEPTH_TEST);  // Habilita o teste usando o DepthBuffer
    glEnable(GL_CULL_FACE);   // Habilita o corte das faces
    glCullFace(GL_BACK);      // Configura o corte para as faces cuja normal não aponta na direção da camera

    glShadeModel(GL_SMOOTH);

    // Illumination
    glEnable(GL_LIGHTING);
    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
    glEnable(GL_COLOR_MATERIAL);
}

void updateLight(int value) {
	light.updatePosition();
	glutTimerFunc(10,updateLight,0);
}

void createWindow(int argc, char *argv[]) {
	// Inicialização Glut
	glutInit(&argc, argv);
	// Configurando a janela para aceitar o Double Buffer, Z-Buffer e Alpha.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	// Tamanho da Janela
	glutInitWindowSize(SCREEN_SIZE, SCREEN_SIZE);
	// Cria efetivamente a janela e o contexto OpenGL.
	glutCreateWindow("Shadow Mapping Without Shaders");
	// Configura a Callback que desenhará na tela.
	glutDisplayFunc(display);
	// Configura a atualização da luz
	glutTimerFunc(10,updateLight,0);
}

int main(int argc, char *argv[]) {
	// Cria uma Janela OpenGL.
	createWindow(argc, argv);
	// Configura a máquina de estados da OpenGL
	setupGL();

	// Loop da Glut.
	glutMainLoop();
	return 0;
}
