#pragma once

#include <glad/glad.h>
#include <stdlib.h>
#include <ctime>
#include <cmath>


typedef struct tag_PARTICLE
{
	GLfloat xpos;//(xpos,ypos,zpos)��particle��position
	GLfloat ypos;
	GLfloat zpos;
	GLfloat xspeed;//(xspeed,yspeed,zspeed)��particle��speed 
	GLfloat yspeed;
	GLfloat zspeed;
	GLfloat r;//(r,g,b)��particle��color
	GLfloat g;
	GLfloat b;
	GLfloat life;// particle���ةR 
	GLfloat fade;// particle���I��t��
	GLfloat size;// particle���j�p  
	GLbyte bFire;
	GLbyte nExpl;//����particle�ĪG  
	GLbyte bAddParts;// particle�O�_�t������
	GLfloat AddSpeed;//���ڲɤl���[�t��  
	GLfloat AddCount;//���ڲɤl���W�[�q  
	tag_PARTICLE* pNext;//�U�@particle 
	tag_PARTICLE* pPrev;//�W�@particle   
} Particle;


void InitParticle(Particle& ep);
void AddParticle(Particle ex);
void DeleteParticle(Particle** p);

void Explosion1(Particle* par);
void Explosion2(Particle* par);
void Explosion3(Particle* par);
void Explosion4(Particle* par);
void Explosion5(Particle* par);
void Explosion6(Particle* par);
void Explosion7(Particle* par);

void ProcessParticles();
void DrawParticles();


class Particle2
{
public:
};