//------------------------------------------------------------------------------------------
// renderer.h
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#ifndef GLRENDERER_H
#define GLRENDERER_H

#include <QtGui>
#include <QtWidgets>
#include <QOpenGLFunctions_4_0_Core>

#include "unitcube.h"
#include "unitsphere.h"
#include "unitplane.h"

//------------------------------------------------------------------------------------------
#define PRINT_ERROR(_errStr) \
{ \
    qDebug()<< "Error occured at line:" << __LINE__ << ", file:" << __FILE__; \
    qDebug()<< "Error message:" << _errStr; \
}

#define PRINT_AND_DIE(_errStr) \
{ \
    qDebug()<< "Error occured at line:" << __LINE__ << ", file:" << __FILE__; \
    qDebug()<< "Error message:" << _errStr; \
    exit(EXIT_FAILURE); \
}

#define TRUE_OR_DIE(_condition, _errStr) \
{ \
    if(!(_condition)) \
    { \
        qDebug()<< "Fatal error occured at line:" << __LINE__ << ", file:" << __FILE__; \
        qDebug()<< "Error message:" << _errStr; \
        exit(EXIT_FAILURE); \
    } \
}

#define SIZE_OF_MAT4 (4 * 4 *sizeof(GLfloat))
#define SIZE_OF_VEC4 (4 * sizeof(GLfloat))
//------------------------------------------------------------------------------------------
#define MOVING_INERTIA 0.9f
#define CUBE_MAP_SIZE 512
#define DEFAULT_CAMERA_POSITION QVector3D(-4.0f,  5.0f, 15.0f)
#define DEFAULT_CAMERA_FOCUS QVector3D(-4.0f,  2.0f, 0.0f)
#define DEFAULT_LIGHT_POSITION QVector3D(0.0f, 100.0f, 100.0f)
#define DEFAULT_CUBE_POSITION QVector3D(0.0f, 1.001f, 0.0f)
#define DEFAULT_SPHERE_POSITION QVector3D(0.0f, 4.0f, 0.0f)
#define DEFAULT_REFLECTIVE_SPHERE_POSITION QVector3D(-8.0f, 1.001f, 0.0f)

struct Light
{
    Light():
        position(10.0f, 10.0f, 10.0f, 1.0f),
        color(1.0f, 1.0f, 1.0f, 1.0f),
        intensity(1.0f) {}

    int getStructSize()
    {
        return (2 * 4 + 1) * sizeof(GLfloat);
    }

    QVector4D position;
    QVector4D color;
    GLfloat intensity;
};

struct Material
{
    Material():
        diffuseColor(-10.0f, 1.0f, 0.0f, 1.0f),
        specularColor(1.0f, 1.0f, 1.0f, 1.0f),
        reflection(0.0f),
        shininess(10.0f) {}

    int getStructSize()
    {
        return (2 * 4 + 2) * sizeof(GLfloat);
    }

    void setDiffuse(QVector4D _diffuse)
    {
        diffuseColor = _diffuse;
    }

    void setSpecular(QVector4D _specular)
    {
        specularColor = _specular;
    }

    void setReflection(float _reflection)
    {
        reflection = _reflection;
    }

    QVector4D diffuseColor;
    QVector4D specularColor;
    GLfloat reflection;
    GLfloat shininess;
};

enum FloorTexture
{
    CHECKERBOARD = 0,
    NUM_FLOOR_TEXTURES
};

enum EnvironmentTexture
{
    SKY = 0,
    NUM_ENVIRONMENT_TEXTURES
};

enum ShadingProgram
{
    PHONG_SHADING = 0,
    BACKGROUND_SHADING,
    NUM_SHADING_MODE
};


enum UBOBinding
{
    BINDING_MATRICES = 0,
    BINDING_LIGHT,
    BINDING_BACKGROUND_MATERIAL,
    BINDING_FLOOR_MATERIAL,
    BINDING_CUBE_MATERIAL,
    BINDING_SEMIREFLECTIVE_SPHERE_MATERIAL,
    BINDING_REFLECTIVE_SPHERE_MATERIAL,
    NUM_BINDING_POINTS
};

enum ReflectiveObjects
{
    SEMI_REFLECTIVE_SPHERE = 0,
    TOTAL_REFLECTIVE_SPHERE,
    NUM_REFLECTIVE_OBJECTS,
    INVALID_OBJECT
};

//------------------------------------------------------------------------------------------
class Renderer : public QOpenGLWidget, QOpenGLFunctions_4_0_Core// QOpenGLFunctions
{
public:
    enum SpecialKey
    {
        NO_KEY,
        SHIFT_KEY,
        CTRL_KEY
    };

    enum MouseButton
    {
        NO_BUTTON,
        LEFT_BUTTON,
        RIGHT_BUTTON
    };

    Renderer(QWidget* parent = 0);
    ~Renderer();

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
    void keyPressEvent(QKeyEvent* _event);
    void keyReleaseEvent(QKeyEvent* _event);
    void wheelEvent(QWheelEvent* _event);

    void changeShadingMode(ShadingProgram _shadingMode);
    void changeSphereResolution(int _numStacks, int _numSlices);
    void changeFloorTexture(FloorTexture _texture);
    void changeEnvironmentTexture(EnvironmentTexture _texture);
    void changeFloorTextureFilteringMode(QOpenGLTexture::Filter _textureFiltering);
    void changeSphereReflectionPercentage(int _reflectionPercentage);
    void changeCubeColor(float _r, float _g, float _b);

public slots:
    void enableDepthTest(bool _status);
    void enableZAxisRotation(bool _status);
    void enableObjectTransformation(bool _status);
    void enableDynamicEnvironmentMapping(bool _state);
    void enableBackgroundRendering(bool _state);
    void enableTextureAnisotropicFiltering(bool _state);
    void resetCameraPosition();
    void changePlaneSize(int _planeSize);
    void resetObjectPositions();

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void mousePressEvent(QMouseEvent* _event);
    void mouseMoveEvent(QMouseEvent* _event);
    void mouseReleaseEvent(QMouseEvent* _event);

private:
    void checkOpenGLVersion();
    bool initShaderPrograms();
    bool initProgram(ShadingProgram _shadingMode);
    bool initBackgroundShadingProgram();
    void initRenderingData();
    void initSharedBlockUniform();
    void initTexture();
    void initDynamicCubeMapBufferObject();
    void initSceneMemory();
    void initPlaneMemory();
    void initCubeMemory();
    void initSphereMemory();
    void initVertexArrayObjects();
    void initPlaneVAO(ShadingProgram _shadingMode);
    void initCubeVAO(ShadingProgram _shadingMode);
    void initSphereVAO(ShadingProgram _shadingMode);
    void initSceneMatrices();

    void updateCamera();
    void translateCamera();
    void rotateCamera();
    void zoomCamera();

    void translateObjects();
    void rotateObjects();

    void createDynamicCubeMapTexture(ReflectiveObjects _object);
    void createObjectCubeMapTextures();

    void renderScene(ReflectiveObjects _hiddenObj = INVALID_OBJECT);
    void renderBackground();
    void renderFloor();
    void renderCube();
    void renderSemireflectiveSphere();
    void renderReflectiveSphere();

    QOpenGLTexture* floorTextures[NUM_FLOOR_TEXTURES];
    QOpenGLTexture* sphereTexture;
    QOpenGLTexture* decalTexture;
    QOpenGLTexture* cubeMapEnvTexture[NUM_ENVIRONMENT_TEXTURES];
    QOpenGLTexture* currentEnvTexture;
    UnitPlane* planeObject;
    UnitCube* cubeObject;
    UnitSphere* sphereObject;
    int sphereNumStacks;
    int sphereNumSlices;

    QOpenGLTexture* objEnvTexture[NUM_REFLECTIVE_OBJECTS];
    QOpenGLTexture* objEnvTextureBuffer1[NUM_REFLECTIVE_OBJECTS];
    QOpenGLTexture* objEnvTextureBuffer2[NUM_REFLECTIVE_OBJECTS];
    QOpenGLFramebufferObject* FBOCubeMap;

    QMap<ShadingProgram, QString> vertexShaderSourceMap;
    QMap<ShadingProgram, QString> fragmentShaderSourceMap;
    QOpenGLShaderProgram* glslPrograms[NUM_SHADING_MODE];
    QOpenGLShaderProgram* currentProgram;
    GLuint UBOBindingIndex[NUM_BINDING_POINTS];
    GLuint UBOMatrices;
    GLuint UBOLight;
    GLuint UBOPlaneMaterial;
    GLuint UBOCubeMaterial;
    GLuint UBOSemireflectiveSphereMaterial;
    GLuint UBOReflectiveSphereMaterial;
    GLint attrVertex[NUM_SHADING_MODE];
    GLint attrNormal[NUM_SHADING_MODE];
    GLint attrTexCoord[NUM_SHADING_MODE];

    GLint uniMatrices[NUM_SHADING_MODE];
    GLint uniCameraPosition[NUM_SHADING_MODE];
    GLint uniLight[NUM_SHADING_MODE];
    GLint uniMaterial[NUM_SHADING_MODE];
    GLint uniObjTexture[NUM_SHADING_MODE];
    GLint uniEnvTexture[NUM_SHADING_MODE];
    GLint uniHasObjTexture[NUM_SHADING_MODE];

    QOpenGLVertexArrayObject vaoPlane[NUM_SHADING_MODE];
    QOpenGLVertexArrayObject vaoCube[NUM_SHADING_MODE];
    QOpenGLVertexArrayObject vaoSphere[NUM_SHADING_MODE];
    QOpenGLBuffer vboPlane;
    QOpenGLBuffer vboCube;
    QOpenGLBuffer vboSphere;
    QOpenGLBuffer iboPlane;
    QOpenGLBuffer iboSphere;
    QOpenGLBuffer iboCube;

    Material planeMaterial;
    Material cubeMaterial;
    Material semiReflectiveSphereMaterial;
    Material reflectiveSphereMaterial;
    Light light;


    QMatrix4x4 viewMatrix;
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 viewProjectionMatrix;
    QMatrix4x4 backgroundCubeModelMatrix;
    QMatrix4x4 planeModelMatrix;
    QMatrix4x4 planeNormalMatrix;
    QMatrix4x4 cubeModelMatrix;
    QMatrix4x4 cubeNormalMatrix;
    QMatrix4x4 semiReflectiveSphereModelMatrix;
    QMatrix4x4 semiReflectiveSphereNormalMatrix;
    QMatrix4x4 reflectiveSphereModelMatrix;
    QMatrix4x4 reflectiveSphereNormalMatrix;
    QMap<ReflectiveObjects, QVector3D> reflectiveObject2LocationMap;
    QMap<ReflectiveObjects, QMatrix4x4> reflectiveObject2ModelMatrixMap;

    qreal retinaScale;
    float zooming;
    QVector3D cameraPosition;
    QVector3D cameraFocus;
    QVector3D cameraUpDirection;

    QVector2D lastMousePos;
    QVector3D translation;
    QVector3D translationLag;
    QVector3D rotation;
    QVector3D rotationLag;
    QVector3D scalingLag;
    SpecialKey specialKeyPressed;
    MouseButton mouseButtonPressed;

    ShadingProgram shadingMode;
    FloorTexture floorTexture;
    bool enabledZAxisRotation;
    bool enabledObjectTransformation;
    bool enabledDynamicEnvMapping;
    bool enabledBackgroundRendering;
    bool enabledTextureAnisotropicFiltering;
    bool useGlobalEnvTexture;
};

#endif // GLRENDERER_H
