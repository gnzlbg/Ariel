// Kai: FLIP Fluid Simulator
// Written by Yining Karl Li
//
// File: flip.cpp
// Implements the FLIP sim

#include "flip.hpp"
#include "../math/kernels.inl"
#include <omp.h>

using namespace fluidCore;

flipsim::flipsim(const vec3& maxres, const float& density){
	dimensions = maxres;	
	pgrid = new particlegrid(maxres);
	max_density = 0.0f;
	this->density = density;


	float h = density/dimensions.x;
	FOR_EACH_CELL(10, 10, 10){
		particle* p = new particle;
		p->p = (vec3(i,j,k) + vec3(0.5f))*h;
		p->type = FLUID;
		p->mass = 1.0f;
		particles.push_back(p);
	}
	pgrid->sort(particles);
	max_density = 1.0f;
	computeDensity();
	max_density = 0.0f;

	for( int n=0; n<particles.size(); n++ ) {
		particle *p = particles[n];
		max_density = fmax(max_density,p->density);
		delete p;
	}

	cout << max_density << endl;
}

flipsim::~flipsim(){
	delete pgrid;
	int particlecount = particles.size();
	for(int i=0; i<particlecount; i++){
		delete particles[i];
	}
	particles.clear();
	clearMacgrid(mgrid);
}

void flipsim::computeDensity(){

	int particlecount = particles.size();
	#pragma omp parallel for
	for(int i=0; i<particlecount; i++){
		//Find neighbours
		if(particles[i]->type==SOLID){
			particles[i]->density = 1.0f;
		}else{
			vec3 position = particles[i]->p;

			position.x = (int)fmax(0,fmin((int)dimensions.x-1,(int)dimensions.x*position.x));
			position.y = (int)fmax(0,fmin((int)dimensions.y-1,(int)dimensions.y*position.y));
			position.z = (int)fmax(0,fmin((int)dimensions.z-1,(int)dimensions.z*position.z));
			vector<particle *> neighbors;
			neighbors = pgrid->getCellNeighbors(position, vec3(1));
			float weightsum = 0.0f;
			int neighborscount = neighbors.size();
			for(int m=0; m<neighborscount; m++){
				if(neighbors[m]->type!=SOLID){
					float sqd = mathCore::sqrlength(neighbors[m]->p, particles[i]->p);
					//TODO: figure out a better density smooth approx than density/dimensions.x
					float weight = neighbors[m]->mass * mathCore::smooth(sqd, 4.0f*density/dimensions.x);
					weightsum = weightsum + weight;
				}
			}
			particles[i]->density = weightsum/max_density;
		}
	}
}