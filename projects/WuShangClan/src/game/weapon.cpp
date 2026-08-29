#include "game/weapon.h"
#include "engine/Engine.h"
#include "engine/GameObject.h"
#include "engine/PhysicsComponent.h"
#include "engine/asset_loader.hpp"
#include "iostream"
#include "raylib.h"
#include "raymath.h"
#include "vector"

Weapon::Weapon(Engine *engine) : m_engine(engine) {
  m_fireRateTimer = 0.0f;
  m_fireRate = 0.15f;
  hudSize = 0.5f;
  currentWeapon = 0;
  spamMode = 0; // 0 = L/RATIO, 1 = CRINGE, 2 = CANCELLED
}

Weapon::~Weapon() {}

void Weapon::Initialize() {
  auto &assetLoader = m_engine->GetAssetLoader();

  assetLoader.loadPackedFile("hud.pack");

  auto spamData = assetLoader.getAssetData("handHUD_spam.png");
  auto mijoData = assetLoader.getAssetData("handHUD_mijo.png");
  auto bkgData = assetLoader.getAssetData("BKG.png");

  spamTex = assetLoader.LoadTextureFromAssetData(spamData, ".png");
  mijoTex = assetLoader.LoadTextureFromAssetData(mijoData, ".png");
  bkgTex = assetLoader.LoadTextureFromAssetData(bkgData, ".png");

  SetTextureFilter(spamTex, TEXTURE_FILTER_POINT);
  SetTextureFilter(mijoTex, TEXTURE_FILTER_POINT);
  SetTextureFilter(bkgTex, TEXTURE_FILTER_POINT);
}

void Weapon::Update(float deltaTime) {
  float wheel = GetMouseWheelMove();
  if (wheel != 0.0f) {
    currentWeapon = (currentWeapon == 0) ? 1 : 0;
  }

  if (currentWeapon == 0) {
    if (IsKeyPressed(KEY_ONE)) {
      spamMode = 0;
      std::cout << "MODE: L/RATIO (SMG)\n";
    }
    if (IsKeyPressed(KEY_TWO)) {
      spamMode = 1;
      std::cout << "MODE: CRINGE (Sniper/Stun)\n";
    }
    if (IsKeyPressed(KEY_THREE)) {
      spamMode = 2;
      std::cout << "MODE: CANCELLED (Shotgun)\n";
    }
  }

  m_fireRateTimer -= deltaTime;

  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && m_fireRateTimer <= 0.0f) {
    Shoot();
  }

  for (auto it = m_activeShockwaves.begin(); it != m_activeShockwaves.end();) {
    it->currentRadius += it->speed * deltaTime;
    it->alpha -= deltaTime * 1.5f;
    if (it->alpha <= 0.0f) {
      it = m_activeShockwaves.erase(it);
    } else {
      ++it;
    }
  }
}

void Weapon::Shoot() {
  auto &camera = m_engine->GetCamera();
  auto &physicsSystem = m_engine->GetPhysicsSystem();
  auto &gameObjectManager = m_engine->GetGameObjectManager();

  // Extract Player Body & Velocity
  btRigidBody *playerBody = nullptr;
  btVector3 playerVelocity(0, 0, 0);

  for (const auto &obj : gameObjectManager.GetGameObjects()) {
    if (obj->name == "PLAYER") {
      auto phys = obj->GetComponent<PhysicsComponent>();
      if (phys && phys->GetRigidBody()) {
        playerBody = phys->GetRigidBody();
        playerVelocity = playerBody->getLinearVelocity();
      }
      break;
    }
  }

  if (currentWeapon == 0) {
    auto SpawnText = [&](std::string text, Vector3 aimDir, float speed) {
      Vector3 forward =
          Vector3Normalize(Vector3Subtract(camera.target, camera.position));
      Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
      Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));

      Vector3 spawnPos = camera.position;
      spawnPos = Vector3Add(spawnPos, Vector3Scale(forward, 1.5f));
      spawnPos = Vector3Add(spawnPos, Vector3Scale(right, 0.5f));
      spawnPos = Vector3Subtract(spawnPos, Vector3Scale(up, 0.2f));

      auto projectile = std::make_shared<GameObject>();
      projectile->name = "PROJECTILE|" + text;
      projectile->position = spawnPos;
      projectile->timeToLive = 2.0f; // Expiry

      // Small physical hitbox
      btCollisionShape *shape = new btBoxShape(btVector3(0.1f, 0.1f, 0.1f));
      auto physComp = projectile->AddComponent<PhysicsComponent>(
          &physicsSystem, shape, PhysicsShapeType::BOX, 0.1f, spawnPos,
          QuaternionIdentity());

      // Bullet inherit momentum from player
      btVector3 btVelocity((aimDir.x * speed) + playerVelocity.x(),
                           (aimDir.y * speed) + playerVelocity.y(),
                           (aimDir.z * speed) + playerVelocity.z());

      if (physComp && physComp->GetRigidBody()) {
        physComp->GetRigidBody()->activate(true);
        physComp->GetRigidBody()->setLinearVelocity(btVelocity);

        // NO GRAVITY, NO BOUNCE
        physComp->GetRigidBody()->setGravity(btVector3(0, 0, 0));
        physComp->GetRigidBody()->setRestitution(0.0f);
        physComp->GetRigidBody()->setFriction(1.0f);

        physComp->GetRigidBody()->setCcdMotionThreshold(0.05f);
        physComp->GetRigidBody()->setCcdSweptSphereRadius(0.04f);

        // BULLET WILL NEVER HIT THE PLAYER!
        if (playerBody) {
          physComp->GetRigidBody()->setIgnoreCollisionCheck(playerBody, true);
        }
      }

      gameObjectManager.AddGameObject(projectile);
    };

    Vector3 baseAim =
        Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    if (spamMode == 0) {
      m_fireRateTimer = 0.1f;
      SpawnText((GetRandomValue(0, 1) == 0) ? "L" : "RATIO", baseAim, 60.0f);
    } else if (spamMode == 1) {
      m_fireRateTimer = 0.8f;
      SpawnText("CRINGE", baseAim, 120.0f);
    } else if (spamMode == 2) {
      m_fireRateTimer = 1.2f;
      for (int i = 0; i < 5; i++) {
        Vector3 spread = baseAim;
        spread.x += (GetRandomValue(-150, 150) / 1000.0f);
        spread.y += (GetRandomValue(-150, 150) / 1000.0f);
        spread.z += (GetRandomValue(-150, 150) / 1000.0f);
        spread = Vector3Normalize(spread);
        SpawnText("CANCELLED", spread, 45.0f);
      }
    }
  } else if (currentWeapon == 1) {
    m_fireRateTimer = 0.5f;
    float shockwaveRadius = 50.0f;
    float knockbackForce = 450.0f;

    for (const auto &obj : gameObjectManager.GetGameObjects()) {
      auto physComp = obj->GetComponent<PhysicsComponent>();
      if (!physComp || !physComp->GetRigidBody() ||
          physComp->GetRigidBody()->getInvMass() == 0.0f ||
          obj->name == "PLAYER")
        continue;

      float distance = Vector3Distance(camera.position, obj->position);
      if (distance <= shockwaveRadius) {
        Vector3 pushDir =
            Vector3Normalize(Vector3Subtract(obj->position, camera.position));
        pushDir.y += 0.5f;
        pushDir = Vector3Normalize(pushDir);

        float forceMultiplier = 1.0f - (distance / shockwaveRadius);
        if (forceMultiplier < 0.0f)
          forceMultiplier = 0.0f;

        float finalForce = knockbackForce * forceMultiplier;
        btVector3 btImpulse(pushDir.x * finalForce, pushDir.y * finalForce,
                            pushDir.z * finalForce);

        physComp->GetRigidBody()->activate(true);
        physComp->GetRigidBody()->applyCentralImpulse(btImpulse);
      }
    }

    ShockwaveVFX wave;
    wave.center = camera.position;
    wave.center.y -= 1.0f;
    wave.currentRadius = 1.0f;
    wave.maxRadius = shockwaveRadius;
    wave.speed = 30.0f;
    wave.alpha = 1.0f;
    m_activeShockwaves.push_back(wave);
  }
}

void Weapon::DrawHUD() {
  auto &camera = m_engine->GetCamera();
  auto &gameObjectManager = m_engine->GetGameObjectManager();
  Vector3 camForward =
      Vector3Normalize(Vector3Subtract(camera.target, camera.position));

  for (auto &wave : m_activeShockwaves) {
    int numTexts = 12;
    for (int i = 0; i < numTexts; i++) {
      float angle =
          ((float)i / numTexts) * PI * 2.0f + (wave.currentRadius * 0.5f);
      Vector3 textPos;
      textPos.x = wave.center.x + cosf(angle) * wave.currentRadius;
      textPos.y = wave.center.y;
      textPos.z = wave.center.z + sinf(angle) * wave.currentRadius;

      Vector3 toText = Vector3Subtract(textPos, camera.position);
      if (Vector3DotProduct(toText, camForward) > 0.0f) {
        Vector2 screenPos = GetWorldToScreen(textPos, camera);
        Color waveColor = ORANGE;
        waveColor.a = (unsigned char)(wave.alpha * 255);
        int fontSize = 20 + (int)(wave.currentRadius * 1.5f);
        int textWidth = MeasureText("MIJO LAGAO", fontSize);
        DrawText("MIJO LAGAO", screenPos.x - textWidth / 2, screenPos.y,
                 fontSize, waveColor);
      }
    }
  }

  for (const auto &obj : gameObjectManager.GetGameObjects()) {
    if (obj->name.find("PROJECTILE|") != std::string::npos) {
      Vector3 pos3D = obj->position;
      std::string textToDraw = obj->name.substr(11);

      Vector3 dirToObject = Vector3Subtract(pos3D, camera.position);
      if (Vector3DotProduct(dirToObject, camForward) > 0.0f) {
        Vector2 screenPos = GetWorldToScreen(pos3D, camera);
        float distance = Vector3Distance(camera.position, pos3D);
        if (distance < 1.0f)
          distance = 1.0f;

        int fontSize = (int)(150.0f / distance);
        if (fontSize > 100)
          fontSize = 100;
        if (fontSize < 1)
          continue;

        Color textColor = WHITE;
        if (textToDraw == "CRINGE")
          textColor = RED;
        else if (textToDraw == "CANCELLED")
          textColor = YELLOW;

        int textWidth = MeasureText(textToDraw.c_str(), fontSize);
        DrawText(textToDraw.c_str(), (int)screenPos.x - (textWidth / 2),
                 (int)screenPos.y, fontSize, textColor);
      }
    }
  }

  if (currentWeapon == 0) {
    DrawTextureEx(spamTex,
                  {GetScreenWidth() + 40.0f - (spamTex.width * hudSize),
                   GetScreenHeight() - (spamTex.height * hudSize)},
                  0.0f, hudSize, WHITE);
    std::string modeStr = "MODE: L/RATIO";
    if (spamMode == 1)
      modeStr = "MODE: CRINGE";
    if (spamMode == 2)
      modeStr = "MODE: CANCELLED";
    DrawText(modeStr.c_str(), GetScreenWidth() - 250, 50, 20, GREEN);
  } else if (currentWeapon == 1) {
    float recoilY = 0.0f;
    if (m_fireRateTimer > 0.0f)
      recoilY = (m_fireRateTimer / m_fireRate) * 150.0f;
    DrawTextureEx(mijoTex,
                  {GetScreenWidth() + 40.0f - (mijoTex.width * hudSize),
                   GetScreenHeight() - (mijoTex.height * hudSize) + recoilY},
                  0.0f, hudSize, WHITE);
  }
}
