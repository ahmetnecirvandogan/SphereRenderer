
#ifndef PHYSICS_OBJECT_H
#define PHYSICS_OBJECT_H

#include "Angel.h"

class PhysicsObject {
public:
    vec3 position;
    vec3 velocity;
    vec3 acceleration;
    float mass;
   
    

    PhysicsObject(vec3 initPos = vec3(0.0, 0.0, 0.0), vec3 initVel = vec3(0.0, 0.0, 0.0), vec3 initAcc = vec3(0.0, 0.0, 0.0), float initMass = 1.0);

    void update(double deltaTime);

    void applyForce(vec3 force);

    void bounce(vec3 surface_normal);

    void applyResistence(vec3 resistence);


};

#endif // PHYSICS_OBJECT_H
