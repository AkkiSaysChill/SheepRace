#pragma once
#include "raylib.h"
#include <deque>
#include <string>
#include <vector>

enum class ChatMood { NEUTRAL, HYPE, TOXIC, BORED };

struct ChatMessage {
  std::string sender;
  std::string text;
  Color color;
};

class ChatSystem {
public:
  ChatSystem();
  ~ChatSystem() = default;

  void Initialize(); // Emojis load karne ke liye
  void Update(float deltaTime);
  void DrawHUD(); // Engine pointer hata diya

  void OnComboSuccess();
  void OnPerfectInterrupt();
  void OnCancelledUsed();
  void OnPlayerTakeDamage();

  int GetViewership() const { return m_targetViewership; }

private:
  int m_targetViewership = 10000;
  float m_displayViewership = 10000.0f;
  ChatMood m_currentMood = ChatMood::NEUTRAL;

  float m_chatTimer = 0.0f;
  float m_chatInterval = 1.5f;
  float m_organicDriftTimer = 3.0f;
  float m_inactivityTimer = 0.0f;
  bool m_hasActionStarted = false;

  int m_spamCountL = 0;
  float m_spamTimerL = 0.0f;
  int m_spamCountCringe = 0;
  float m_spamTimerCringe = 0.0f;
  int m_spamCountCancel = 0;
  float m_spamTimerCancel = 0.0f;

  int m_historyL = 0;
  int m_historyCringe = 0;
  int m_historyCancel = 0;
  float m_predictionCooldown = 0.0f;

  // YEH THA MISSING VARIABLE
  Font m_emojiFont = {0};

  std::deque<ChatMessage> m_activeMessages;
  int m_maxMessages = 6;

  std::vector<std::string> m_hypeTexts;
  std::vector<std::string> m_toxicTexts;
  std::vector<std::string> m_boredTexts;
  std::vector<std::string> m_usernames;

  void AddRandomMessage();
};
