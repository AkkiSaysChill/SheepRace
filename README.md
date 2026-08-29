~~# Sheep Racing (cant think of a better name)~~
# SHEEPER BUILDS (think of a better name)

- made in c++ using raylib imgui 

- uses raylib for graphics and imgui for GUI 


### build the game 

- if you are on linux or macOS, you can just run the build.sh script in the root directory of the repository, it will create a build directory and build the game for you.
- just run the build.sh script in the root directory of the repository, it will create a build directory and build the game for you.

```
./build.sh
```

- if you are on windows, you can run the build.bat script in the root directory of the repository, it will create a build directory and build the game for you.
[you need to have cmake and visual studio installed](https://learn.microsoft.com/en-us/cpp/build/projects-and-build-systems-cpp?view=msvc-170)

```
.\build.bat
```


- Here is a guide on how to add a physics component and collider to future objects, following the project's
  current structure:


   1. Create your object as a `GameObject`:
   2. Choose a Collision Shape:
      Here is a complete example of creating a dynamic sphere object:


  `cpp
      #include "GameObject.h"
      #include "PhysicsSystem.h"
      #include "PhysicsComponent.h"
      #include <btBulletDynamicsCommon.h> // For collision shapes

      // (Assuming you have a 'physicsSystem' variable available)

      // 1. Create the GameObject
      GameObject mySphereObject;

      // 2. Define the collision shape
      float sphereRadius = 1.5f;
      btCollisionShape* sphereShape = new btSphereShape(sphereRadius);

      // 3. Define the object's properties
      float mass = 5.0f; // A mass > 0 makes it a dynamic object that can move and fall
      Vector3 initialPosition = {10.0f, 20.0f, 5.0f};
      Quaternion initialRotation = {0.0f, 0.0f, 0.0f, 1.0f};

      // 4. Add the PhysicsComponent to the GameObject
      mySphereObject.AddComponent<PhysicsComponent>(
          &physicsSystem,
          sphereShape,
          mass,
          initialPosition,
          initialRotation
      );
      `

  To create a static, immovable object (like a wall or the floor):

  The process is the same, but you set the mass to 0.0f.

  `cpp
  // For a static box (e.g., a wall)
  btCollisionShape* wallShape = new btBoxShape(btVector3(10.0f, 2.0f, 0.5f));
  float wallMass = 0.0f; // Mass of 0 makes it static
  // ... then call AddComponent with wallMass
  `


- now there should be a sheepRace executable in the build directory


