// modefied the skybox code so that it uses the asset bundle insted of loading
// texture directly from png files

#ifndef SKYBOX_H
#define SKYBOX_H

#include "raylib.h"
#include "raymath.h" // Required for: MatrixPerspective(), MatrixLookAt()
#include "rlgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else // PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION 100
#endif

typedef struct {
  bool useHDR;
  char skyboxFileName[256];
  Shader shaderbox;
  Model modelbox;
  Mesh meshbox;
} Skyboxlib;

extern Skyboxlib skybox;

Skyboxlib SkyboxLoad(const char *ImgSkyboxPath, const char *TextureHDRPath,
                     const char *SkyboxVSPath, const char *SkyboxFSPath,
                     const char *CubemapVSPath, const char *CubemapFSPath);

Skyboxlib SkyboxLoadFromMemory(const unsigned char *fileData, int dataSize,
                               const char *fileExtension,
                               const char *SkyboxVSPath,
                               const char *SkyboxFSPath,
                               const char *CubemapVSPath,
                               const char *CubemapFSPath);

void SkyboxUpdate(Skyboxlib *skybox);
void DrawSkyboxModel(Skyboxlib *skybox);
void DrawSkyboxTexture(Skyboxlib *skybox);
void UnloadSkybox(Skyboxlib *skybox);

Skyboxlib SkyboxLoadFromMemoryAll(const unsigned char *fileData, int dataSize,
                               const char *fileExtension,
                               const char *skyboxVSCode, const char *skyboxFSCode,
                               const char *cubemapVSCode, const char *cubemapFSCode);


#ifdef __cplusplus
}
#endif

#endif // SKYBOX_H
