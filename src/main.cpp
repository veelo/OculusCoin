/**************************************************************************\
 * Copyright (c) Bastiaan Veelo (Bastiaan a_t Veelo d_o_t net)
 * All rights reserved. Contact me if the below is too restrictive for you.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include <QApplication>
#include <QGLWidget>
#include <QTimer>
#include <QDebug>
#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOffscreenRenderer.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoFrustumCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
//#include <OVR_CAPI_GL.h>
#include <../Src/OVR_CAPI_GL.h>


class CoinRiftWidget : public QGLWidget
{
    ovrHmd hmd;
    ovrHmdDesc hmdDesc;
    ovrEyeDesc eyes[2];
    ovrEyeRenderDesc eyeRenderDesc[2];
    ovrTexture eyeTexture[2];

    SoOffscreenRenderer *renderer;
    SoSeparator *rootScene[2];
    SoFrustumCamera *camera[2];
    SoNode *scene;
public:
    explicit CoinRiftWidget();
    ~CoinRiftWidget();
    void setSceneGraph(SoNode *sceneGraph);
protected:
    void initializeGL();
    void paintGL();
};


CoinRiftWidget::CoinRiftWidget() : QGLWidget()
{
    for (int eye = 0; eye < 2; eye++)
        reinterpret_cast<ovrGLTextureData*>(&eyeTexture[eye])->TexId = 0;

    // OVR will do the swapping.
    setAutoBufferSwap(false);

    hmd = ovrHmd_Create(0);
    if (!hmd) {
        qDebug() << "Could not find Rift device.";
        exit(2);
    }
    ovrHmd_GetDesc(hmd, &hmdDesc);
    if (!ovrHmd_StartSensor(hmd, ovrHmdCap_Orientation |    // Capabilities we support.
                                 ovrHmdCap_YawCorrection |
                                 ovrHmdCap_Position |
                                 ovrHmdCap_LowPersistence,
                            ovrHmdCap_Orientation)) {       // Capabilities we require.
        qDebug() << "Could not start Rift motion sensor.";
        exit(3);
    }

    resize(hmdDesc.Resolution.w, hmdDesc.Resolution.h);

    //ConfigureStereosettings.
    ovrSizei recommenedTex0Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Left,
                                                           hmdDesc.DefaultEyeFov[0], 1.0f);
    ovrSizei recommenedTex1Size = ovrHmd_GetFovTextureSize(hmd, ovrEye_Right,
                                                           hmdDesc.DefaultEyeFov[1], 1.0f);

    renderer = new SoOffscreenRenderer(SbViewportRegion(std::max(recommenedTex0Size.w, recommenedTex0Size.w),
                                                        std::max(recommenedTex1Size.h, recommenedTex1Size.h)));
    renderer->setComponents(SoOffscreenRenderer::RGB_TRANSPARENCY);
    renderer->setBackgroundColor(SbColor(.0f, .0f, .8f));

    scene = new SoSeparator(0);   // Placeholder.
    for (int eye = 0; eye < 2; eye++) {
        rootScene[eye] = new SoSeparator(3);
        rootScene[eye]->ref();
        camera[eye] = new SoFrustumCamera();
        camera[eye]->position.setValue(0.0f, 0.0f, 5.0f);
        camera[eye]->focalDistance.setValue(5.0f);
        camera[eye]->viewportMapping.setValue(SoCamera::LEAVE_ALONE);
        rootScene[eye]->addChild(camera[eye]);
        rootScene[eye]->addChild(new SoDirectionalLight());  // TODO Connect direction to camera.
        rootScene[eye]->addChild(scene);
    }

    // Populate ovrEyeDesc[2].
    eyes[0].Eye                     = ovrEye_Left;
    eyes[1].Eye                     = ovrEye_Right;
    eyes[0].Fov                     = hmdDesc.DefaultEyeFov[0];
    eyes[1].Fov                     = hmdDesc.DefaultEyeFov[1];
    eyes[0].TextureSize.w           = renderer->getViewportRegion().getViewportSizePixels().getValue()[0];
    eyes[0].TextureSize.h           = renderer->getViewportRegion().getViewportSizePixels().getValue()[1];
    eyes[1].TextureSize             = eyes[0].TextureSize;
    eyes[0].RenderViewport.Pos.x    = 0;
    eyes[0].RenderViewport.Pos.y    = 0;
    eyes[0].RenderViewport.Size     = eyes[0].TextureSize;
    eyes[1].RenderViewport.Pos      = eyes[0].RenderViewport.Pos;
    eyes[1].RenderViewport.Size     = eyes[1].TextureSize;

    const int backBufferMultisample = 0;    // TODO This is a guess?
    ovrGLConfig cfg;
    cfg.OGL.Header.API          = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.RTSize       = hmdDesc.Resolution;
    cfg.OGL.Header.Multisample  = backBufferMultisample;
    cfg.OGL.Window              = reinterpret_cast<HWND>(winId());
    makeCurrent();
    cfg.OGL.WglContext          = wglGetCurrentContext();   // http://stackoverflow.com/questions/17532033/qglwidget-get-gl-contextes-for-windows
    cfg.OGL.GdiDc               = wglGetCurrentDC();        // TODO QT5.
    qDebug() << "Window:" << cfg.OGL.Window;
    qDebug() << "Context:" << cfg.OGL.WglContext;
    qDebug() << "DC:" << cfg.OGL.GdiDc;

    bool VSyncEnabled(false);       // TODO This is a guess.
    if (!ovrHmd_ConfigureRendering(hmd, &cfg.Config, (VSyncEnabled ? 0 : ovrHmdCap_NoVSync),
                                   hmdDesc.DistortionCaps, eyes, eyeRenderDesc)) {
        qDebug() << "Could not configure OVR rendering.";
        exit(3);
    }

    for (int eye = 0; eye < 2; eye++) {
        camera[eye]->aspectRatio.setValue((eyeRenderDesc[eye].Desc.Fov.LeftTan + eyeRenderDesc[eye].Desc.Fov.RightTan) /
                (eyeRenderDesc[eye].Desc.Fov.UpTan + eyeRenderDesc[eye].Desc.Fov.DownTan));
        camera[eye]->nearDistance.setValue(1.0f);
        camera[eye]->farDistance.setValue(100.0f);
        camera[eye]->left.setValue(-eyeRenderDesc[eye].Desc.Fov.LeftTan);
        camera[eye]->right.setValue(eyeRenderDesc[eye].Desc.Fov.RightTan);
        camera[eye]->top.setValue(eyeRenderDesc[eye].Desc.Fov.UpTan);
        camera[eye]->bottom.setValue(-eyeRenderDesc[eye].Desc.Fov.DownTan);
    }
    qDebug() << "Left UpTan:" << eyeRenderDesc[0].Desc.Fov.UpTan << "Right UpTan:" << eyeRenderDesc[1].Desc.Fov.UpTan;
    qDebug() << "Left DownTan:" << eyeRenderDesc[0].Desc.Fov.DownTan << "Right DownTan:" << eyeRenderDesc[1].Desc.Fov.DownTan;
    qDebug() << "Left LeftTan:" << eyeRenderDesc[0].Desc.Fov.LeftTan << "Right LeftTan:" << eyeRenderDesc[1].Desc.Fov.LeftTan;
    qDebug() << "Left RightTan:" << eyeRenderDesc[0].Desc.Fov.RightTan << "Right RightTan:" << eyeRenderDesc[1].Desc.Fov.RightTan;
}


CoinRiftWidget::~CoinRiftWidget()
{
    delete renderer;
    for (int eye = 0; eye < 2; eye++) {
        rootScene[eye]->unref();
        ovrGLTextureData *texData = reinterpret_cast<ovrGLTextureData*>(&eyeTexture[eye]);
        if (texData->TexId)
            glDeleteTextures(1, &texData->TexId);
    }
    scene = 0;
    ovrHmd_StopSensor(hmd);
    ovrHmd_Destroy(hmd);
}


void CoinRiftWidget::setSceneGraph(SoNode *sceneGraph)
{
    rootScene[0]->replaceChild(scene, sceneGraph);
    rootScene[1]->replaceChild(scene, sceneGraph);
    scene = sceneGraph;
}


void CoinRiftWidget::initializeGL()
{
    // Create rendering target textures.
    glEnable(GL_TEXTURE_2D);
    for (int eye = 0; eye < 2; eye++) {
        ovrGLTextureData *texData = reinterpret_cast<ovrGLTextureData*>(&eyeTexture[eye]);
        texData->Header.API            = ovrRenderAPI_OpenGL;
        texData->Header.TextureSize    = eyes[eye].TextureSize;
        texData->Header.RenderViewport = eyes[eye].RenderViewport;
        glGenTextures(1, &texData->TexId);
        glBindTexture(GL_TEXTURE_2D, texData->TexId);
        Q_ASSERT(!glGetError());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, eyes[eye].TextureSize.w, eyes[eye].TextureSize.h, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        Q_ASSERT(!glGetError());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glDisable(GL_TEXTURE_2D);
}


void CoinRiftWidget::paintGL()
{
    const int ms(1000 / 60);
    QTimer::singleShot(ms, this, SLOT(updateGL()));
    makeCurrent();

    glEnable(GL_TEXTURE_2D);

    /*ovrFrameTiming hmdFrameTiming =*/ ovrHmd_BeginFrame(hmd, 0);
    for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++) {
        ovrEyeType eye              = hmdDesc.EyeRenderOrder[eyeIndex];
        ovrGLTextureData *texData   = reinterpret_cast<ovrGLTextureData*>(&eyeTexture[eye]);
        ovrPosef eyePose            = ovrHmd_BeginEyeRender(hmd, eye);

        camera[eye]->orientation.setValue(eyePose.Orientation.x,
                                          eyePose.Orientation.y,
                                          eyePose.Orientation.z,
                                          eyePose.Orientation.w);

        SbVec3f originalPosition(camera[eye]->position.getValue());
        camera[eye]->position.setValue(originalPosition - SbVec3f(eyeRenderDesc[eye].ViewAdjust.x,
                                                                  eyeRenderDesc[eye].ViewAdjust.y,
                                                                  eyeRenderDesc[eye].ViewAdjust.z));
        renderer->render(rootScene[eye]);
        camera[eye]->position.setValue(originalPosition);

//        qDebug() << "Binding texture" << texData->TexId;
        glBindTexture(GL_TEXTURE_2D, texData->TexId);
        Q_ASSERT(!glGetError());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     eyes[eye].TextureSize.w,
                     eyes[eye].TextureSize.h,
                     0, GL_BGRA, GL_UNSIGNED_BYTE, renderer->getBuffer());
        Q_ASSERT(!glGetError());

        ovrHmd_EndEyeRender(hmd, eye, eyePose, &eyeTexture[eye]);
    }

    ovrHmd_EndFrame(hmd);

//    glDisable(GL_TEXTURE_2D);
    doneCurrent();

}


static void cleanup()
{
//    Quarter::clean();
    SoDB::finish();
    ovr_Shutdown();
}


int main(int argc, char *argv[])
{
    SoDB::init();

    QApplication app(argc, argv);
    qAddPostRoutine(cleanup);

    // Moved here because of https://developer.oculusvr.com/forums/viewtopic.php?f=17&t=7915&p=108503#p108503
    // Init libovr.
    if (!ovr_Initialize()) {
        qDebug() << "Could not initialize Oculus SDK.";
        exit(1);
    }

    CoinRiftWidget window;
    window.show();

    static const char * inlineSceneGraph[] = {
        "#Inventor V2.1 ascii\n",
        "\n",
        "Separator {\n",
        "  Rotation { rotation 1 0 0  0.3 }\n",
        "  Cone { }\n",
        "  BaseColor { rgb 1 0 0 }\n",
        "  Scale { scaleFactor .7 .7 .7 }\n",
        "  Cube { }\n",
        "\n",
        "  DrawStyle { style LINES }\n",
        "  ShapeHints { vertexOrdering COUNTERCLOCKWISE }\n",
        "  Coordinate3 {\n",
        "    point [\n",
        "       -2 -2 1.1,  -2 -1 1.1,  -2  1 1.1,  -2  2 1.1,\n",
        "       -1 -2 1.1,  -1 -1 1.1,  -1  1 1.1,  -1  2 1.1\n",
        "        1 -2 1.1,   1 -1 1.1,   1  1 1.1,   1  2 1.1\n",
        "        2 -2 1.1,   2 -1 1.1,   2  1 1.1,   2  2 1.1\n",
        "      ]\n",
        "  }\n",
        "\n",
        "  Complexity { value 0.7 }\n",
        "  NurbsSurface {\n",
        "     numUControlPoints 4\n",
        "     numVControlPoints 4\n",
        "     uKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]\n",
        "     vKnotVector [ 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 ]\n",
        "  }\n",
        "}\n",
        NULL
    };

    SoInput in;
    in.setStringArray(inlineSceneGraph);

    window.setSceneGraph(SoDB::readAll(&in));

    return app.exec();
}
