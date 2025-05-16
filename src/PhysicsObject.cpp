#include "Angel.h"
#include "PhysicsObject.h"

float bottomY = -0.76f;

PhysicsObject::PhysicsObject(vec3 initPos, vec3 initVel, vec3 initAcc, float initMass)
{
    PhysicsObject::position = initPos;
    PhysicsObject::velocity = initVel;
    PhysicsObject::acceleration = initAcc;
    PhysicsObject::mass = initMass;

}


void PhysicsObject::update(double deltaTime)
{
    if (position.y <= bottomY && velocity.y < 0.0f)
    {
        vec3 surface(0.0, 1.0, 0.0);
        bounce(surface);
    }

    vec3 gravity(0.0, -2.5, 0.0);
    vec3 gravitionalForce = gravity * mass;
    vec3 resistence(1.0f, 0.1f, 0.1f);

    applyForce(gravitionalForce);
    applyResistence(resistence);

    position = position + 0.5 * acceleration * deltaTime * deltaTime + velocity * deltaTime;
    velocity = velocity + acceleration * deltaTime;
    acceleration = vec3(0.0, 0.0, 0.0);
}

void PhysicsObject::applyForce(vec3 force)
{
    acceleration += force / mass;
}

void PhysicsObject::bounce(vec3 surface_normal)
{
    float restitution = 0.9f;
    vec3 normalized_surface_normal = normalize(surface_normal);
    velocity = velocity - (1.0f + restitution) * dot(velocity, normalized_surface_normal) * normalized_surface_normal;
    if (position.y <= bottomY)
    {
        position.y = bottomY;  // not to sink
    }
}
void PhysicsObject::applyResistence(vec3 resistenceCoefficient)
{
    float velocityThresholdX = 0.03f; // Adjust as needed for x-velocity
 
    // Apply threshold for x-velocity
    if (abs(velocity.x) > velocityThresholdX)
    {
        vec3 resistance_force_x = -vec3(resistenceCoefficient.x * abs(velocity.x) * velocity.x, 0.0f, 0.0f);
        applyForce(resistance_force_x);
    }
    else {
        velocity.x = 0.0f;
    }

    // Apply quadratic resistance for y and z velocities
    vec3 resistance_force_yz = -vec3(0.0f,
        resistenceCoefficient.y * abs(velocity.y) * velocity.y,
        resistenceCoefficient.z * abs(velocity.z) * velocity.z);
    applyForce(resistance_force_yz);
}
