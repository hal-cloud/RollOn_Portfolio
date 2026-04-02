#include "gameParticle.h"

int GameParticle::RandomLife()
{
    return (rand() % 120) + 60;
}

Vector3 GameParticle::RandomVelocity()
{
    return Vector3(
        (rand() % 50 - 25) / 200.0f,
        (rand() % 50)      / 200.0f,
        (rand() % 50 - 25) / 200.0f
    );
}