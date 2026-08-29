#include "game/enemy.h"
#include "engine/DrawableComponent.h"
#include "engine/PhysicsComponent.h"
#include "raymath.h"

Enemy::Enemy(Engine *engine, BossType type, Vector3 spawnPos) {
  m_engine = engine;
  m_type = type;
  this->position = spawnPos;
  this->isDestructible = true;

  m_engine->GetAssetLoader().loadPackedFile("level001");

  // aru data
  auto aruData = m_engine->GetAssetLoader().getAssetData("aru/aru.png");
  auto aruWalk1Data =
      m_engine->GetAssetLoader().getAssetData("aru/aru_walk1.png");
  auto aruWalk2Data =
      m_engine->GetAssetLoader().getAssetData("aru/aru_walk2.png");
  auto aruAttackData =
      m_engine->GetAssetLoader().getAssetData("aru/aru_attack1.png");

  // baua data
  auto bauaGroundData =
      m_engine->GetAssetLoader().getAssetData("baua/baua_ground.png");
  auto bauaJumpData =
      m_engine->GetAssetLoader().getAssetData("baua/baua_jump.png");

  // jay data
  auto jayIdleData =
      m_engine->GetAssetLoader().getAssetData("jay/jay_idle.png");
  auto jayAttack1Data =
      m_engine->GetAssetLoader().getAssetData("jay/jay_attack1.png");
  auto jayAttack2Data =
      m_engine->GetAssetLoader().getAssetData("jay/jay_attack2.png");

  // rishu data
  auto rishuIdleData =
      m_engine->GetAssetLoader().getAssetData("rishu/vau_idle.png");
  auto hhwData = m_engine->GetAssetLoader().getAssetData("rishu/hhw.png");
  auto arpitData =
      m_engine->GetAssetLoader().getAssetData("rishu/arpit_bala.png");
  auto rishuJeepData =
      m_engine->GetAssetLoader().getAssetData("rishu/jeep.png");
  auto rishuLandmineData =
      m_engine->GetAssetLoader().getAssetData("rishu/landMine.png");

  // SUNRAYBEE data
  auto mmyIdleData =
      m_engine->GetAssetLoader().getAssetData("sunraybee/sunraybee_idle.png");
  auto mmyAttack1Data = m_engine->GetAssetLoader().getAssetData(
      "sunraybee/sunraybee_attack1.png");
  auto mmyAttack2Data = m_engine->GetAssetLoader().getAssetData(
      "sunraybee/sunraybee_attack2.png");

  Color bossColor = WHITE;
  Vector3 bossScale = {2.0f, 2.0f, 2.0f};
  float mass = 20.0f;

  switch (type) {
  case BossType::ITZARUGOD:
    this->name = "BOSS_ITZARUGOD";
    this->health = 5;
    m_speed = 15.0f;
    this->m_spriteTexture =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(aruData, ".png");
    this->m_spriteWalk1Tex =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(aruWalk1Data,
                                                            ".png");
    this->m_spriteWalk2Tex =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(aruWalk2Data,
                                                            ".png");
    this->m_spriteAttackTex =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(aruAttackData,
                                                            ".png");
    this->m_isSprite = true;
    this->m_spriteSize = {4.0f, 4.0f};
    break;

  case BossType::JAYIXH:
    this->name = "BOSS_JAYIXH";
    this->health = 8;
    m_speed = 12.0f;
    this->m_isSprite = true;
    this->m_spriteSize = {4.0f, 4.0f};
    this->m_spriteTexture = m_engine->GetAssetLoader().LoadTextureFromAssetData(
        jayIdleData, ".png");
    this->m_spriteAttackTex =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(jayAttack1Data,
                                                            ".png");
    this->m_spriteAttack2Tex =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(jayAttack2Data,
                                                            ".png");
    this->m_isSprite = true;
    this->m_spriteSize = {4.0f, 4.0f};

    break;

  case BossType::SUNRAYBEE:
    this->name = "BOSS_SUNRAYBEE";
    this->health = 10;
    m_speed = 14.0f;
    this->m_spriteTexture = m_engine->GetAssetLoader().LoadTextureFromAssetData(
        mmyIdleData, ".png");
    this->m_spriteAttackTex =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(mmyAttack1Data,
                                                            ".png");
    this->m_spriteAttack2Tex =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(mmyAttack2Data,
                                                            ".png");
    this->m_isSprite = true;
    this->m_spriteSize = {4.0f, 4.0f};

    break;

  case BossType::BOA:
    this->name = "BOSS_BAUA";
    this->health = 3;
    m_speed = 28.0f;
    this->m_spriteTexture = m_engine->GetAssetLoader().LoadTextureFromAssetData(
        bauaGroundData, ".png");
    this->m_spriteWalk1Tex =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(bauaGroundData,
                                                            ".png");
    this->m_spriteWalk2Tex =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(bauaJumpData,
                                                            ".png");

    mass = 5.0f;
    this->m_isSprite = true;
    this->m_spriteSize = {4.0f, 4.0f};

    break;

  case BossType::ARPIT_BALA:
    this->name = "BOSS_ARPIT_BALA";
    this->health = 25;
    m_speed = 10.0f;
    bossColor = BLUE;
    this->m_spriteTexture =
        m_engine->GetAssetLoader().LoadTextureFromAssetData(arpitData, ".png");
    this->m_isSprite = true;
    this->m_spriteSize = {4.0f, 4.0f};

    break;

  case BossType::DANK_RISHU:
    this->name = "BOSS_DANK_RISHU";
    this->health = 50;
    m_speed = 12.0f;
    bossColor = PURPLE;
    bossScale = {5.0f, 5.0f, 5.0f};
    mass = 100.0f;
    this->m_isSprite = true;
    this->m_spriteSize = {4.0f, 4.0f};

    break;
  }

  if (!this->m_isSprite) {
    Model enemyModel =
        LoadModelFromMesh(GenMeshCube(bossScale.x, bossScale.y, bossScale.z));
    enemyModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = bossColor;
    enemyModel.materials[0].shader = m_engine->GetRenderer().GetShadowShader();
    this->AddComponent<DrawableComponent>(enemyModel);
  }

  btCollisionShape *shape = new btBoxShape(
      btVector3(bossScale.x / 2.0f, bossScale.y / 2.0f, bossScale.z / 2.0f));
  this->AddComponent<PhysicsComponent>(&m_engine->GetPhysicsSystem(), shape,
                                       PhysicsShapeType::BOX, mass,
                                       this->position, QuaternionIdentity());
}

void Enemy::UpdateAI(Vector3 playerPos, float deltaTime) {
  auto physComp = this->GetComponent<PhysicsComponent>();
  if (physComp && physComp->GetRigidBody()) {

    Vector3 dir = Vector3Subtract(playerPos, this->position);
    dir.y = 0;
    dir = Vector3Normalize(dir);

    physComp->GetRigidBody()->activate(true);
    physComp->GetRigidBody()->applyCentralForce(
        btVector3(dir.x * m_speed, 0, dir.z * m_speed));
  }
}
