#include "game/ChatSystem.h"
#include "raymath.h"
#include <algorithm>
#include <iostream>

ChatSystem::ChatSystem() {
  m_usernames = {"Rajat_99",       "ToxicBoy",  "AruFan",   "GamerX",
                 "SlayyPoint_Alt", "SigmaMale", "JayFanBoy"};
  m_hypeTexts = {"W game", "he cooking", "perfect dodge!", "WUSHANG W",
                 "bhai kya mara hai"};
  m_toxicTexts = {"L quality", "bro fell off", "delete kerde", "L kancept",
                  "cringe"};
  m_boredTexts = {"mid", "content khatam", "refund", "bhai so jau kya",
                  "boring"};
}

void ChatSystem::Initialize() {
  m_emojiFont = LoadFont("assets/symbola.fnt");
  if (m_emojiFont.texture.id != 0) {
    SetTextureFilter(m_emojiFont.texture, TEXTURE_FILTER_BILINEAR);
  }
}

void ChatSystem::Update(float deltaTime) {
  m_displayViewership =
      Lerp(m_displayViewership, (float)m_targetViewership, 5.0f * deltaTime);

  if (m_predictionCooldown > 0.0f)
    m_predictionCooldown -= deltaTime;

  if (m_currentMood != ChatMood::NEUTRAL && m_currentMood != ChatMood::BORED) {
    m_inactivityTimer -= deltaTime;

    if (m_currentMood == ChatMood::TOXIC && m_inactivityTimer <= 4.0f) {
      m_currentMood = ChatMood::NEUTRAL;
      ChatMessage msg = {"SYSTEM", "Chat is calming down...", LIGHTGRAY};
      m_activeMessages.push_back(msg);
      if (m_activeMessages.size() > m_maxMessages)
        m_activeMessages.pop_front();
    } else if (m_inactivityTimer <= 0.0f) {
      m_currentMood = ChatMood::BORED;
      ChatMessage msg = {"AruFan", "Play aggressive coward", GRAY};
      m_activeMessages.push_back(msg);
      if (m_activeMessages.size() > m_maxMessages)
        m_activeMessages.pop_front();
    }
  }

  m_chatTimer -= deltaTime;
  if (m_chatTimer <= 0.0f) {
    AddRandomMessage();
    if (m_currentMood == ChatMood::HYPE)
      m_chatInterval = 0.5f;
    else if (m_currentMood == ChatMood::BORED)
      m_chatInterval = 2.5f;
    else
      m_chatInterval = 1.5f;
    m_chatTimer = m_chatInterval;
  }

  m_organicDriftTimer -= deltaTime;
  if (m_organicDriftTimer <= 0.0f) {
    int driftAmount = 0;
    if (m_currentMood == ChatMood::HYPE) {
      driftAmount = GetRandomValue(5, 15);
      m_organicDriftTimer = (float)GetRandomValue(8, 15) / 10.0f;
    } else if (m_currentMood == ChatMood::TOXIC ||
               m_currentMood == ChatMood::BORED) {
      driftAmount = GetRandomValue(-15, -5);
      m_organicDriftTimer = (float)GetRandomValue(8, 15) / 10.0f;
    } else {
      driftAmount = GetRandomValue(-2, 2);
      m_organicDriftTimer = (float)GetRandomValue(30, 50) / 10.0f;
    }
    m_targetViewership += driftAmount;
    if (m_targetViewership < 0)
      m_targetViewership = 0;
  }

  if (m_spamTimerL > 0.0f) {
    m_spamTimerL -= deltaTime;
    if (m_spamTimerL <= 0.0f)
      m_spamCountL = 0;
  }
  if (m_spamTimerCringe > 0.0f) {
    m_spamTimerCringe -= deltaTime;
    if (m_spamTimerCringe <= 0.0f)
      m_spamCountCringe = 0;
  }
  if (m_spamTimerCancel > 0.0f) {
    m_spamTimerCancel -= deltaTime;
    if (m_spamTimerCancel <= 0.0f)
      m_spamCountCancel = 0;
  }
}

void ChatSystem::AddRandomMessage() {
  ChatMessage msg;
  msg.sender = m_usernames[GetRandomValue(0, m_usernames.size() - 1)];

  if (m_hasActionStarted && m_inactivityTimer < 5.0f &&
      m_inactivityTimer > 0.0f && m_predictionCooldown <= 0.0f) {
    bool predicted = false;

    if (m_historyCancel >= 4 && m_historyCancel > m_historyL &&
        m_historyCancel > m_historyCringe) {
      std::vector<std::string> pred = {"he's gonna shotgun again watch",
                                       "waiting for the cancel",
                                       "shotgun crutch incoming"};
      msg.text = pred[GetRandomValue(0, pred.size() - 1)];
      msg.color = YELLOW;
      predicted = true;
    } else if (m_historyL >= 4 && m_historyL > m_historyCancel &&
               m_historyL > m_historyCringe) {
      std::vector<std::string> pred = {"L incoming in 3... 2...",
                                       "he only knows L ratio",
                                       "get ready for the SMG spam"};
      msg.text = pred[GetRandomValue(0, pred.size() - 1)];
      msg.color = ORANGE;
      predicted = true;
    } else if (m_historyCringe >= 4 && m_historyCringe > m_historyCancel &&
               m_historyCringe > m_historyL) {
      std::vector<std::string> pred = {"waiting for the CRINGE shot",
                                       "bro is camping for that interrupt",
                                       "do it, use cringe"};
      msg.text = pred[GetRandomValue(0, pred.size() - 1)];
      msg.color = PINK;
      predicted = true;
    }

    if (predicted) {
      m_predictionCooldown = 8.0f;
      m_activeMessages.push_back(msg);
      if (m_activeMessages.size() > m_maxMessages)
        m_activeMessages.pop_front();
      return;
    }
  }

  if (m_currentMood == ChatMood::NEUTRAL) {
    if (!m_hasActionStarted) {
      std::vector<std::string> startTexts = {
          "hello", "first", "aur bhai", "kaise ho", "stream start?", "???"};
      msg.text = startTexts[GetRandomValue(0, startTexts.size() - 1)];
    } else {
      std::vector<std::string> calmTexts = {
          "kya hua?", "stream lag?", "sogaya kya", "awaz nahi aari", "..."};
      msg.text = calmTexts[GetRandomValue(0, calmTexts.size() - 1)];
    }
    msg.color = LIGHTGRAY;
  } else if (m_currentMood == ChatMood::HYPE) {
    msg.text = m_hypeTexts[GetRandomValue(0, m_hypeTexts.size() - 1)];
    msg.color = GREEN;
  } else if (m_currentMood == ChatMood::TOXIC) {
    msg.text = m_toxicTexts[GetRandomValue(0, m_toxicTexts.size() - 1)];
    msg.color = RED;
  } else if (m_currentMood == ChatMood::BORED) {
    msg.text = m_boredTexts[GetRandomValue(0, m_boredTexts.size() - 1)];
    msg.color = GRAY;
  }

  m_activeMessages.push_back(msg);
  if (m_activeMessages.size() > m_maxMessages)
    m_activeMessages.pop_front();
}

// --- COMBAT TRIGGERS ---
void ChatSystem::OnComboSuccess() {
  m_hasActionStarted = true;
  m_inactivityTimer = 8.0f;
  m_spamCountCringe = 0;
  m_spamCountCancel = 0;

  m_historyL += 1;
  m_historyCringe = std::max(0, m_historyCringe - 1);
  m_historyCancel = std::max(0, m_historyCancel - 1);

  m_spamCountL++;
  m_spamTimerL = 3.0f;

  if (m_spamCountL == 20) {
    m_currentMood = ChatMood::TOXIC;
    ChatMessage msg = {"GamerX", "Bro only knows 2 buttons", RED};
    m_activeMessages.push_back(msg);
    if (m_activeMessages.size() > m_maxMessages)
      m_activeMessages.pop_front();
  } else if (m_spamCountL > 20) {
    m_currentMood = ChatMood::TOXIC;
    m_targetViewership -= 20;
    if (m_spamCountL % 5 == 0) {
      ChatMessage msg = {"ToxicBoy", "L spam is boring tbh", RED};
      m_activeMessages.push_back(msg);
      if (m_activeMessages.size() > m_maxMessages)
        m_activeMessages.pop_front();
    }
  } else {
    if (m_currentMood == ChatMood::TOXIC) {
      m_targetViewership += 100;
      if (m_spamCountL == 1) {
        ChatMessage msg = {"GamerX", "ok wait he is cooking now", GREEN};
        m_activeMessages.push_back(msg);
        if (m_activeMessages.size() > m_maxMessages)
          m_activeMessages.pop_front();
      }
    } else {
      m_targetViewership += 15;
    }
    m_currentMood = ChatMood::HYPE;
  }
}

void ChatSystem::OnPerfectInterrupt() {
  m_hasActionStarted = true;
  m_inactivityTimer = 12.0f;
  m_spamCountL = 0;
  m_spamCountCancel = 0;

  m_historyCringe += 2;
  m_historyL = std::max(0, m_historyL - 1);
  m_historyCancel = std::max(0, m_historyCancel - 1);

  m_spamCountCringe++;
  m_spamTimerCringe = 5.0f;

  if (m_spamCountCringe == 3) {
    m_currentMood = ChatMood::TOXIC;
    ChatMessage msg = {"SigmaMale", "Stop spamming cringe wtf", RED};
    m_activeMessages.push_back(msg);
    if (m_activeMessages.size() > m_maxMessages)
      m_activeMessages.pop_front();
  } else if (m_spamCountCringe > 3) {
    m_currentMood = ChatMood::TOXIC;
    m_targetViewership -= 150;
    ChatMessage msg = {"AruFan", "WE GET IT BRO", RED};
    m_activeMessages.push_back(msg);
    if (m_activeMessages.size() > m_maxMessages)
      m_activeMessages.pop_front();
  } else {
    if (m_currentMood == ChatMood::TOXIC) {
      m_targetViewership += 600;
      ChatMessage msg = {"SYSTEM", "CROWD SILENCED! PERFECT INTERRUPT!", GOLD};
      m_activeMessages.push_back(msg);
      if (m_activeMessages.size() > m_maxMessages)
        m_activeMessages.pop_front();
    } else {
      m_targetViewership += 300;
      ChatMessage msg = {"SYSTEM", "PERFECT CRINGE INTERRUPT!", GOLD};
      m_activeMessages.push_back(msg);
      if (m_activeMessages.size() > m_maxMessages)
        m_activeMessages.pop_front();
    }
    m_currentMood = ChatMood::HYPE;
  }
}

void ChatSystem::OnCancelledUsed() {
  m_hasActionStarted = true;
  m_inactivityTimer = 10.0f;
  m_spamCountL = 0;
  m_spamCountCringe = 0;

  m_historyCancel += 2;
  m_historyL = std::max(0, m_historyL - 1);
  m_historyCringe = std::max(0, m_historyCringe - 1);

  m_spamCountCancel++;
  m_spamTimerCancel = 5.0f;

  m_targetViewership -= 500;
  m_currentMood = ChatMood::TOXIC;

  if (m_spamCountCancel == 3) {
    ChatMessage msg = {"SigmaMale", "Bro gonna cancel again", RED};
    m_activeMessages.push_back(msg);
    if (m_activeMessages.size() > m_maxMessages)
      m_activeMessages.pop_front();
  } else if (m_spamCountCancel > 3) {
    m_targetViewership -= 300;
    std::vector<std::string> spamRoasts = {"Told ya", "L spammer",
                                           "cancel abuser", "boring"};
    ChatMessage msg = {
        "ToxicBoy", spamRoasts[GetRandomValue(0, spamRoasts.size() - 1)], RED};
    m_activeMessages.push_back(msg);
    if (m_activeMessages.size() > m_maxMessages)
      m_activeMessages.pop_front();
  }
}

void ChatSystem::OnPlayerTakeDamage() {
  m_hasActionStarted = true;
  m_targetViewership -= 100;
  m_currentMood = ChatMood::TOXIC;
  m_inactivityTimer = 6.0f;

  std::vector<std::string> hitRoasts = {"LMAO", "noob", "bro is blind",
                                        "skill issue", "bot walk"};
  ChatMessage msg = {"ToxicBoy",
                     hitRoasts[GetRandomValue(0, hitRoasts.size() - 1)], RED};
  m_activeMessages.push_back(msg);
  if (m_activeMessages.size() > m_maxMessages)
    m_activeMessages.pop_front();
}

void ChatSystem::DrawHUD() {
  int topBoxWidth = 350;
  int topBoxHeight = 80;
  int topBoxX = GetScreenWidth() / 2 - (topBoxWidth / 2);
  int topBoxY = 10;
  // DrawRectangle(topBoxX, topBoxY, topBoxWidth, topBoxHeight, Fade(BLACK,
  // 0.7f));

  int chatBoxWidth = 350;
  int chatBoxHeight = (m_maxMessages * 30) + 20;
  int chatBoxX = 10;
  int chatBoxY = GetScreenHeight() - chatBoxHeight - 30;
  DrawRectangle(chatBoxX, chatBoxY, chatBoxWidth, chatBoxHeight,
                Fade(BLACK, 0.6f));

  std::string viewStr =
      "LIVE WATCHING: " + std::to_string((int)m_displayViewership);
  Color viewColor = (m_targetViewership > 5000)
                        ? GREEN
                        : ((m_targetViewership > 2000) ? YELLOW : RED);

  DrawText(viewStr.c_str(), topBoxX + 20 + 2, topBoxY + 10 + 2, 30, BLACK);
  DrawText(viewStr.c_str(), topBoxX + 20, topBoxY + 10, 30, viewColor);

  std::string moodText = "MOOD: NEUTRAL";
  std::string emojiHex = "\xF0\x9F\x98\x90"; // 😐
  Color emojiColor = WHITE;

  if (m_currentMood == ChatMood::HYPE) {
    moodText = "MOOD: HYPE";
    emojiHex = "\xF0\x9F\x94\xA5"; // 🔥
    emojiColor = GREEN;
  } else if (m_currentMood == ChatMood::TOXIC) {
    moodText = "MOOD: TOXIC";
    emojiHex = "\xF0\x9F\x92\x80"; // 💀
    emojiColor = RED;
  } else if (m_currentMood == ChatMood::BORED) {
    moodText = "MOOD: BORED";
    emojiHex = "\xF0\x9F\x98\xB4"; // 😴
    emojiColor = GRAY;
  }

  // 1. English text normal font se
  DrawText(moodText.c_str(), topBoxX + 80 + 2, topBoxY + 50 + 2, 20, BLACK);
  DrawText(moodText.c_str(), topBoxX + 80, topBoxY + 50, 20, WHITE);

  // 2. Emoji custom font se (normal text ke aage)
  int textWidth = MeasureText(moodText.c_str(), 20);
  Vector2 emojiPos = {(float)(topBoxX + 80 + textWidth + 10),
                      (float)(topBoxY + 48)};

  if (m_emojiFont.texture.id != 0) {
    DrawTextEx(m_emojiFont, emojiHex.c_str(),
               {emojiPos.x + 2.0f, emojiPos.y + 2.0f}, 24.0f, 1.0f, BLACK);
    DrawTextEx(m_emojiFont, emojiHex.c_str(), emojiPos, 24.0f, 1.0f,
               emojiColor);
  }

  int startY = chatBoxY + 10;
  for (size_t i = 0; i < m_activeMessages.size(); i++) {
    std::string fullLine =
        m_activeMessages[i].sender + ": " + m_activeMessages[i].text;
    DrawText(fullLine.c_str(), chatBoxX + 12, startY + (i * 30) + 2, 20, BLACK);
    DrawText(fullLine.c_str(), chatBoxX + 10, startY + (i * 30), 20,
             m_activeMessages[i].color);
  }
}
