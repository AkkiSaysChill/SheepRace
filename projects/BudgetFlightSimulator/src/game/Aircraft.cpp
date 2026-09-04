#include "game/Aircraft.h"
#include <cmath>

Aircraft::Aircraft() { Reset({0.0f, 150.0f, 0.0f}); }

void Aircraft::SetStats(const AircraftStats &newStats) { 
    stats = newStats; 
    missileAmmo = stats.maxMissiles;
    health = 100.0f;
}

void Aircraft::Reset(Vector3 spawnPos) {
  position = spawnPos;
  orientation = QuaternionIdentity();

  velocity = {0.0f, 0.0f, -55.0f};
  angularVelocity = {0.0f, 0.0f, 0.0f};

  isStalling = false;
  isOnGround = false;
  currentGForce = 1.0f;
  health = 100.0f;
  missileAmmo = stats.maxMissiles;
  minigunCooldown = 0.0f;
  missileCooldown = 0.0f;
}

void Aircraft::UpdateWeaponTimers(float dt) {
    if (minigunCooldown > 0.0f) minigunCooldown -= dt;
    if (missileCooldown > 0.0f) missileCooldown -= dt;
}

bool Aircraft::FireMinigun(std::vector<BulletTracer>& outBullets) {
    if (!stats.hasWeapons || minigunCooldown > 0.0f) return false;

    minigunCooldown = stats.minigunFireRate;
    Vector3 fwd = GetForward();
    Vector3 rgt = GetRight();

    for (float side : { -1.8f, 1.8f }) {
        BulletTracer b;
        b.position = Vector3Add(position, Vector3Scale(rgt, side));
        b.velocity = Vector3Add(Vector3Scale(fwd, stats.bulletSpeed), velocity);
        b.life = 1.6f;
        b.isEnemy = false;
        b.active = true;
        outBullets.push_back(b);
    }
    return true;
}

bool Aircraft::FireMissile(std::vector<HomingMissile>& outMissiles, const Vector3& lockedTargetPos) {
    if (!stats.hasWeapons || missileCooldown > 0.0f || missileAmmo <= 0) return false;

    missileCooldown = stats.missileCooldownTime;
    missileAmmo--;

    Vector3 fwd = GetForward();
    Vector3 down = Vector3Negate(GetUp());

    HomingMissile m;
    m.position = Vector3Add(position, Vector3Scale(down, 1.8f));
    m.velocity = Vector3Add(Vector3Scale(fwd, 120.0f), velocity);
    m.targetPos = lockedTargetPos;
    m.isTrackingPlayer = false;
    m.speed = 340.0f;
    m.turnRate = 3.2f;
    m.life = 6.0f;
    m.blastRadius = 38.0f;
    m.damage = 100.0f;
    m.active = true;

    outMissiles.push_back(m);
    return true;
}

Vector3 Aircraft::GetForward() const {
  return Vector3RotateByQuaternion({0.0f, 0.0f, -1.0f}, orientation);
}

Vector3 Aircraft::GetUp() const {
  return Vector3RotateByQuaternion({0.0f, 1.0f, 0.0f}, orientation);
}

Vector3 Aircraft::GetRight() const {
  return Vector3RotateByQuaternion({1.0f, 0.0f, 0.0f}, orientation);
}

float Aircraft::GetAirspeedKmh() const {
  return Vector3Length(velocity) * 3.6f;
}

float Aircraft::GetAirspeedKnots() const {
  return Vector3Length(velocity) * 1.94384f;
}

float Aircraft::GetAltitudeMeters() const { return position.y; }

float Aircraft::GetVerticalSpeed() const { return velocity.y; }

float Aircraft::GetPitchAngle() const {
  Vector3 fwd = GetForward();
  return asinf(Clamp(fwd.y, -1.0f, 1.0f)) * RAD2DEG;
}

float Aircraft::GetRollAngle() const {
  Vector3 rgt = GetRight();
  Vector3 up = GetUp();
  return atan2f(rgt.y, up.y) * RAD2DEG;
}

float Aircraft::GetHeading() const {
  Vector3 fwd = GetForward();
  float heading = atan2f(fwd.x, -fwd.z) * RAD2DEG;
  if (heading < 0.0f)
    heading += 360.0f;
  return heading;
}

void Aircraft::Update(const FlightInput &input, float dt) {
  dt = Clamp(dt, 0.0f, 0.033f);

  Vector3 forward = GetForward();
  Vector3 up = GetUp();
  Vector3 right = GetRight();

  float currentSpeed = Vector3Length(velocity);
  float currentPitchDeg = GetPitchAngle();

  bool isInverted = (up.y < 0.0f) && (!isOnGround);
  float invertedFactor = isInverted ? fabsf(up.y) : 0.0f;

  // 1. Engine & Drag Integration
  float engineTargetSpeed = input.throttle * stats.maxThrustSpeed;
  float throttleBlend =
      (input.throttle > 0.05f) ? stats.throttleResponse : 1.2f;
  if (input.brake)
    throttleBlend = stats.brakeDecel;

  float gravitySlope = -forward.y * 18.0f;

  if (input.throttle > 0.05f) {
    currentSpeed = Lerp(currentSpeed, engineTargetSpeed, throttleBlend * dt) +
                   (gravitySlope * dt);
  } else {
    float dragDecel = stats.baseDragFactor * (currentSpeed / 30.0f);
    if (forward.y > 0.0f) {
      dragDecel += forward.y * stats.climbDragMultiplier;
    }
    currentSpeed += (gravitySlope - dragDecel) * dt;
  }

  if (input.brake) {
    currentSpeed = Lerp(currentSpeed, 0.0f, throttleBlend * dt);
  }

  currentSpeed = Clamp(currentSpeed, 0.0f, stats.maxThrustSpeed * 1.5f);
  isStalling = (currentSpeed < stats.stallSpeedThreshold) && (!isOnGround);

  float controlAuthority = Clamp(currentSpeed / 25.0f, 0.35f, 1.0f);
  if (isStalling) {
    controlAuthority = 0.70f;
  }

  // 2. Control Torques
  float targetPitchRate = -input.pitch * stats.pitchRate * controlAuthority;
  float targetRollRate = input.roll * stats.rollRate * controlAuthority;
  float targetYawRate = input.yaw * stats.yawRate * controlAuthority;

  float bankAmount = fabsf(right.y);
  if (bankAmount > 0.12f && !isStalling) {
    float slipTorque = right.y * stats.bankNoseSlip * controlAuthority;
    targetYawRate += slipTorque;
  }

  if (isInverted) {
    float horizonAllowance = Clamp(forward.y + 0.3f, 0.0f, 1.0f);
    float gentleNoseDrop = 0.32f * invertedFactor * horizonAllowance;
    targetPitchRate += gentleNoseDrop;
  }

  float pitchBlendSpeed = stats.pitchInertia;
  if (isStalling && !isOnGround) {
    if (targetPitchRate > 0.0f && !isInverted) {
      if (currentPitchDeg >= 0.0f) {
        targetPitchRate = 0.0f;
      } else if (currentPitchDeg > -6.0f) {
        float upwardAllowance = (-currentPitchDeg) / 6.0f;
        targetPitchRate *= Clamp(upwardAllowance, 0.0f, 1.0f);
      }
    }

    if (currentPitchDeg > 0.0f && !isInverted) {
      float gentleDrop = -Clamp(currentPitchDeg * 0.025f, 0.15f, 0.65f);
      targetPitchRate = fminf(targetPitchRate, gentleDrop);
      pitchBlendSpeed = stats.pitchInertia * 1.25f;
    }
  }

  angularVelocity.x =
      Lerp(angularVelocity.x, targetPitchRate, pitchBlendSpeed * dt);
  angularVelocity.y = Lerp(angularVelocity.y, targetYawRate, 3.2f * dt);
  angularVelocity.z = Lerp(angularVelocity.z, targetRollRate, 3.8f * dt);

  Quaternion deltaPitch =
      QuaternionFromAxisAngle({1.0f, 0.0f, 0.0f}, angularVelocity.x * dt);
  Quaternion deltaYaw =
      QuaternionFromAxisAngle({0.0f, 1.0f, 0.0f}, angularVelocity.y * dt);
  Quaternion deltaRoll =
      QuaternionFromAxisAngle({0.0f, 0.0f, -1.0f}, angularVelocity.z * dt);

  Quaternion localDelta =
      QuaternionMultiply(deltaPitch, QuaternionMultiply(deltaYaw, deltaRoll));
  orientation =
      QuaternionNormalize(QuaternionMultiply(orientation, localDelta));

  forward = GetForward();
  up = GetUp();
  right = GetRight();

  // 3. Velocity Integration
  if (!isStalling) {
    velocity = Vector3Scale(forward, currentSpeed);

    if (!isOnGround) {
      if (input.throttle < 0.05f) {
        if (velocity.y > 0.0f) {
          velocity.y = Lerp(velocity.y, -8.0f * forward.y, 4.0f * dt);
        } else {
          float glideSink = stats.glideSinkRate * (1.0f - fabsf(forward.y));
          velocity.y -= glideSink;
        }
      }

      if (bankAmount > 0.15f) {
        float throttleFactor = Clamp(1.0f - input.throttle, 0.10f, 1.0f);
        float bankSink =
            ((bankAmount - 0.15f) * stats.bankSinkStrength) * throttleFactor;
        float rudderTrim = fabsf(input.yaw) * 1.5f;
        bankSink = fmaxf(0.0f, bankSink - rudderTrim);
        velocity.y -= bankSink;
      }

      if (isInverted) {
        float gentleInvertedSink = (4.5f * invertedFactor) *
                                   (1.0f - Clamp(forward.y * 3.0f, 0.0f, 1.0f));
        velocity.y -= gentleInvertedSink * dt;
      }
    }
  } else {
    velocity.x = forward.x * currentSpeed;
    velocity.z = forward.z * currentSpeed;

    float targetStallSink = -8.5f + fminf(0.0f, forward.y * 16.0f);
    velocity.y = Lerp(velocity.y, targetStallSink, 2.0f * dt);

    if (velocity.y > -1.0f) {
      velocity.y = -1.0f;
    }
  }

  position = Vector3Add(position, Vector3Scale(velocity, dt));
  currentGForce = 1.0f + (angularVelocity.x * (currentSpeed / 30.0f));

  // 4. Ground Collision
  const float groundLevel = 2.0f;
  if (position.y <= groundLevel) {
    position.y = groundLevel;
    isOnGround = true;

    if (velocity.y < 0.0f)
      velocity.y = 0.0f;

    float friction = input.brake ? 0.70f : 0.985f;
    velocity.x *= friction;
    velocity.z *= friction;

    angularVelocity = {0.0f, 0.0f, 0.0f};
  } else {
    isOnGround = false;
  }
}