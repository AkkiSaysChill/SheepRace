#include "engine/TextManager.h"
#include <iostream>

TextManager::TextManager() {}
TextManager::~TextManager() {}

void TextManager::Initialize() {
  // Tere Raylib example wala exact font load kar rahe hain
  m_emojiFont = LoadFont("assets/symbola.fnt");

  if (m_emojiFont.texture.id != 0) {
    SetTextureFilter(m_emojiFont.texture, TEXTURE_FILTER_BILINEAR);
    m_isReady = true;
    std::cout
        << "[ENGINE] TextManager: Emoji Font (Symbola) Loaded Successfully!\n";
  } else {
    std::cout << "[ENGINE] ERROR: Could not load Emoji Font. Make sure "
                 "symbola.fnt and symbola.png are in assets/\n";
  }
}

void TextManager::Shutdown() {
  if (m_isReady) {
    UnloadFont(m_emojiFont);
  }
}

void TextManager::DrawEmojiText(const std::string &text, Vector2 position,
                                float fontSize, Color color) {
  if (m_isReady) {
    // Raylib ka DrawTextEx automatically Unicode aur Emojis handle kar leta hai
    // agar us font ke andar woh emojis available hon!
    DrawTextEx(m_emojiFont, text.c_str(), position, fontSize, 1.0f, color);
  } else {
    // Fallback agar font file na mile
    DrawText(text.c_str(), (int)position.x, (int)position.y, (int)fontSize,
             color);
  }
}

void TextManager::DrawEmojiTextWithShadow(const std::string &text,
                                          Vector2 position, float fontSize,
                                          Color color) {
  // 1. Pehle shadow (Black) draw karo thode offset ke sath
  DrawEmojiText(text, {position.x + 2.0f, position.y + 2.0f}, fontSize, BLACK);

  // 2. Phir asli colored text uske upar
  DrawEmojiText(text, position, fontSize, color);
}
