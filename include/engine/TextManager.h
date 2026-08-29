#pragma once
#include "raylib.h"
#include <string>

class TextManager {
public:
  TextManager();
  ~TextManager();

  void Initialize();
  void Shutdown();

  // Apna custom function jo Raylib ka DrawTextEx use karega
  void DrawEmojiText(const std::string &text, Vector2 position, float fontSize,
                     Color color);

  // AAA Shadow effect wala function
  void DrawEmojiTextWithShadow(const std::string &text, Vector2 position,
                               float fontSize, Color color);

private:
  Font m_emojiFont;
  bool m_isReady = false;
};
