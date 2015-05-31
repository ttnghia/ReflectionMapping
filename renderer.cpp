//------------------------------------------------------------------------------------------
// renderer.cpp
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#include "renderer.h"

//------------------------------------------------------------------------------------------
Renderer::Renderer(QWidget* _parent):
    QOpenGLWidget(_parent),
    enabledZAxisRotation(false),
    enabledObjectTransformation(false),
    enabledDynamicEnvMapping(false),
    enabledBackgroundRendering(true),
    useGlobalEnvTexture(true),
    enabledTextureAnisotropicFiltering(true),
    iboPlane(QOpenGLBuffer::IndexBuffer),
    iboCube(QOpenGLBuffer::IndexBuffer),
    iboSphere(QOpenGLBuffer::IndexBuffer),
    specialKeyPressed(Renderer::NO_KEY),
    mouseButtonPressed(Renderer::NO_BUTTON),
    translation(0.0f, 0.0f, 0.0f),
    translationLag(0.0f, 0.0f, 0.0f),
    rotation(0.0f, 0.0f, 0.0f),
    rotationLag(0.0f, 0.0f, 0.0f),
    zooming(0.0f),
    planeObject(NULL),
    cubeObject(NULL),
    sphereObject(NULL),
    sphereNumStacks(30),
    sphereNumSlices(30),
    shadingMode(PHONG_SHADING),
    cameraPosition(DEFAULT_CAMERA_POSITION),
    cameraFocus(DEFAULT_CAMERA_FOCUS),
    cameraUpDirection(0.0f, 1.0f, 0.0f),
    floorTexture(CHECKERBOARD)
{
    retinaScale = devicePixelRatio();
    setFocusPolicy(Qt::StrongFocus);
}

//------------------------------------------------------------------------------------------
Renderer::~Renderer()
{

}

//------------------------------------------------------------------------------------------
void Renderer::checkOpenGLVersion()
{
    QString verStr = QString((const char*)glGetString(GL_VERSION));
    int major = verStr.left(verStr.indexOf(".")).toInt();
    int minor = verStr.mid(verStr.indexOf(".") + 1, 1).toInt();

    if(!(major >= 4 && minor >= 0))
    {
        QMessageBox::critical(this, "Error",
                              QString("Your OpenGL version is %1.%2, which does not satisfy this program requirement (OpenGL >= 4.0)").arg(
                                  major).arg(minor));
        close();
    }

//    qDebug() << major << minor;
//    qDebug() << verStr;
//    TRUE_OR_DIE(major >= 4 && minor >= 0, "OpenGL version must >= 4.0");
}
//------------------------------------------------------------------------------------------
bool Renderer::initProgram(ShadingProgram _shadingMode)
{
    QOpenGLShaderProgram* program;
    GLint location;

    /////////////////////////////////////////////////////////////////
    glslPrograms[_shadingMode] = new QOpenGLShaderProgram;
    program = glslPrograms[_shadingMode];
    bool success;

    success = program->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                               vertexShaderSourceMap.value(_shadingMode));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                               fragmentShaderSourceMap.value(_shadingMode));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->link();
    TRUE_OR_DIE(success, "Cannot link GLSL program.");

    location = program->attributeLocation("v_coord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex coordinate.");
    attrVertex[_shadingMode] = location;

    location = program->attributeLocation("v_normal");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex normal.");
    attrNormal[_shadingMode] = location;

    location = program->attributeLocation("v_texcoord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute texture coordinate.");
    attrTexCoord[_shadingMode] = location;


    location = glGetUniformBlockIndex(program->programId(), "Matrices");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMatrices[_shadingMode] = location;


    location = glGetUniformBlockIndex(program->programId(), "Light");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniLight[_shadingMode] = location;


    location = glGetUniformBlockIndex(program->programId(), "Material");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMaterial[_shadingMode] = location;

    location = program->uniformLocation("cameraPosition");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform cameraPosition.");
    uniCameraPosition[_shadingMode] = location;

    location = program->uniformLocation("envTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform envTex.");
    uniEnvTexture[_shadingMode] = location;


    location = program->uniformLocation("objTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform objTex.");
    uniObjTexture[_shadingMode] = location;


    location = program->uniformLocation("hasObjTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform hasObjTex.");
    uniHasObjTexture[_shadingMode] = location;

    return true;
}

//------------------------------------------------------------------------------------------
bool Renderer::initBackgroundShadingProgram()
{
    GLint location;
    glslPrograms[BACKGROUND_SHADING] = new QOpenGLShaderProgram;
    QOpenGLShaderProgram* program = glslPrograms[BACKGROUND_SHADING];
    bool success;

    success = program->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                               vertexShaderSourceMap.value(BACKGROUND_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                               fragmentShaderSourceMap.value(BACKGROUND_SHADING));
    TRUE_OR_DIE(success, "Cannot compile shader from file.");

    success = program->link();
    TRUE_OR_DIE(success, "Cannot link GLSL program.");

    location = program->attributeLocation("v_coord");
    TRUE_OR_DIE(location >= 0, "Cannot bind attribute vertex coordinate.");
    attrVertex[BACKGROUND_SHADING] = location;

    location = glGetUniformBlockIndex(program->programId(), "Matrices");
    TRUE_OR_DIE(location >= 0, "Cannot bind block uniform.");
    uniMatrices[BACKGROUND_SHADING] = location;

    location = program->uniformLocation("cameraPosition");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform cameraPosition.");
    uniCameraPosition[BACKGROUND_SHADING] = location;

    location = program->uniformLocation("envTex");
    TRUE_OR_DIE(location >= 0, "Cannot bind uniform envTex.");
    uniEnvTexture[BACKGROUND_SHADING] = location;

    return true;
}

//------------------------------------------------------------------------------------------
bool Renderer::initShaderPrograms()
{
    vertexShaderSourceMap.insert(PHONG_SHADING, ":/shaders/phong-shading.vs.glsl");
    vertexShaderSourceMap.insert(BACKGROUND_SHADING, ":/shaders/background.vs.glsl");

    fragmentShaderSourceMap.insert(PHONG_SHADING, ":/shaders/phong-shading.fs.glsl");
    fragmentShaderSourceMap.insert(BACKGROUND_SHADING, ":/shaders/background.fs.glsl");


    return (initBackgroundShadingProgram() &&
            initProgram(PHONG_SHADING));
}

//------------------------------------------------------------------------------------------
void Renderer::initRenderingData()
{
    initTexture();
    initSceneMemory();
    initVertexArrayObjects();
}

//------------------------------------------------------------------------------------------
void Renderer::initSharedBlockUniform()
{
    /////////////////////////////////////////////////////////////////
    // setup the light and material
    cameraPosition = DEFAULT_CAMERA_POSITION;

    light.position = DEFAULT_LIGHT_POSITION;
    light.intensity = 1.0f;

    planeMaterial.shininess = 50.0f;
    planeMaterial.setSpecular(QVector4D(0.5f, 0.5f, 0.5f, 1.0f));

    cubeMaterial.shininess = 50.0f;
    cubeMaterial.setDiffuse(QVector4D(0.0f, 1.0f, 0.2f, 1.0f));

    semiReflectiveSphereMaterial.shininess = 100.0f;
    semiReflectiveSphereMaterial.setDiffuse(QVector4D(0.8f, 0.8f, 0.0f, 1.0f));
    semiReflectiveSphereMaterial.setSpecular(QVector4D(0.5f, 0.5f, 0.5f, 1.0f));
    semiReflectiveSphereMaterial.setReflection(0.5f);

    reflectiveSphereMaterial.setDiffuse(QVector4D(1.0f, 1.0f, 1.0f, 1.0f));
    reflectiveSphereMaterial.setReflection(1.0f);


    /////////////////////////////////////////////////////////////////
    // setup binding points for block uniform
    for(int i = 0; i < NUM_BINDING_POINTS; ++i)
    {
        UBOBindingIndex[i] = i + 1;
    }

    /////////////////////////////////////////////////////////////////
    // setup data for block uniform
    glGenBuffers(1, &UBOMatrices);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferData(GL_UNIFORM_BUFFER, 3 * SIZE_OF_MAT4, NULL,
                 GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOLight);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOLight);
    glBufferData(GL_UNIFORM_BUFFER, light.getStructSize(),
                 NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, light.getStructSize(),
                    &light);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOPlaneMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOPlaneMaterial);
    glBufferData(GL_UNIFORM_BUFFER, planeMaterial.getStructSize(),
                 NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, planeMaterial.getStructSize(),
                    &planeMaterial);

    glGenBuffers(1, &UBOCubeMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOCubeMaterial);
    glBufferData(GL_UNIFORM_BUFFER, cubeMaterial.getStructSize(),
                 NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, cubeMaterial.getStructSize(),
                    &cubeMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOSemireflectiveSphereMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOSemireflectiveSphereMaterial);
    glBufferData(GL_UNIFORM_BUFFER, semiReflectiveSphereMaterial.getStructSize(),
                 NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, semiReflectiveSphereMaterial.getStructSize(),
                    &semiReflectiveSphereMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenBuffers(1, &UBOReflectiveSphereMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, UBOReflectiveSphereMaterial);
    glBufferData(GL_UNIFORM_BUFFER, reflectiveSphereMaterial.getStructSize(),
                 NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, reflectiveSphereMaterial.getStructSize(),
                    &reflectiveSphereMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

//------------------------------------------------------------------------------------------
void Renderer::initTexture()
{
    if(QOpenGLContext::currentContext()->hasExtension("GL_EXT_texture_filter_anisotropic"))
    {
        qDebug() << "GL_EXT_texture_filter_anisotropic: enabled";
        glEnable(GL_EXT_texture_filter_anisotropic);
    }
    else
    {
        qDebug() << "GL_EXT_texture_filter_anisotropic: disabled";
        glDisable(GL_EXT_texture_filter_anisotropic);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // sphere texture
    sphereTexture = new QOpenGLTexture(QImage(":/textures/earth.jpg").mirrored());
    sphereTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    sphereTexture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);

    ////////////////////////////////////////////////////////////////////////////////
    // decal texture
    decalTexture = new QOpenGLTexture(
        QImage(":/textures/minion.png").mirrored().convertToFormat(QImage::Format_RGBA8888));
    decalTexture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    decalTexture->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
    decalTexture->setWrapMode(QOpenGLTexture::DirectionS,
                              QOpenGLTexture::ClampToEdge);
    decalTexture->setWrapMode(QOpenGLTexture::DirectionT,
                              QOpenGLTexture::ClampToEdge);


    ////////////////////////////////////////////////////////////////////////////////
    // floor texture
    QMap<FloorTexture, QString> floorTexture2StrMap;
    floorTexture2StrMap[CHECKERBOARD] = "checkerboard.jpg";

    TRUE_OR_DIE(floorTexture2StrMap.size() == NUM_FLOOR_TEXTURES,
                "Ohh, you forget to initialize some floor texture...");

    for(int i = 0; i < NUM_FLOOR_TEXTURES; ++i)
    {
        FloorTexture tex = static_cast<FloorTexture>(i);

        QString texFile = QString(":/textures/%1").arg(floorTexture2StrMap[tex]);
        TRUE_OR_DIE(QFile::exists(texFile), "Cannot load texture from file.");
        floorTextures[tex] = new QOpenGLTexture(QImage(texFile).mirrored());
        floorTextures[tex]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        floorTextures[tex]->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
        floorTextures[tex]->setWrapMode(QOpenGLTexture::Repeat);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // environment texture
    QMap<EnvironmentTexture, QString> envTexture2StrMap;
    envTexture2StrMap[SKY] = "sky";

    TRUE_OR_DIE(envTexture2StrMap.size() == NUM_ENVIRONMENT_TEXTURES,
                "Ohh, you forget to initialize some environment texture...");

    for(int i = 0; i < NUM_ENVIRONMENT_TEXTURES; ++i)
    {
        EnvironmentTexture tex = static_cast<EnvironmentTexture>(i);

        QString posXFile = QString(":/textures/%1/posx.jpg").arg(envTexture2StrMap[tex]);
        TRUE_OR_DIE(QFile::exists(posXFile), "Cannot load texture from file.");

        QString negXFile = QString(":/textures/%1/negx.jpg").arg(envTexture2StrMap[tex]);
        TRUE_OR_DIE(QFile::exists(negXFile), "Cannot load texture from file.");

        QString posYFile = QString(":/textures/%1/posy.jpg").arg(envTexture2StrMap[tex]);
        TRUE_OR_DIE(QFile::exists(posYFile), "Cannot load texture from file.");

        QString negYFile = QString(":/textures/%1/negy.jpg").arg(envTexture2StrMap[tex]);
        TRUE_OR_DIE(QFile::exists(negYFile), "Cannot load texture from file.");

        QString posZFile = QString(":/textures/%1/posz.jpg").arg(envTexture2StrMap[tex]);
        TRUE_OR_DIE(QFile::exists(posZFile), "Cannot load texture from file.");

        QString negZFile = QString(":/textures/%1/negz.jpg").arg(envTexture2StrMap[tex]);
        TRUE_OR_DIE(QFile::exists(negZFile), "Cannot load texture from file.");

        QImage posXTex = QImage(posXFile).convertToFormat(QImage::Format_RGBA8888);
        QImage negXTex = QImage(negXFile).convertToFormat(QImage::Format_RGBA8888);
        QImage posYTex = QImage(posYFile).convertToFormat(QImage::Format_RGBA8888);
        QImage negYTex = QImage(negYFile).convertToFormat(QImage::Format_RGBA8888);
        QImage posZTex = QImage(posZFile).convertToFormat(QImage::Format_RGBA8888);
        QImage negZTex = QImage(negZFile).convertToFormat(QImage::Format_RGBA8888);


        cubeMapEnvTexture[i] = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
        cubeMapEnvTexture[i]->create();
        cubeMapEnvTexture[i]->setSize(posXTex.width(), posXTex.height());
        cubeMapEnvTexture[i]->setFormat(QOpenGLTexture::RGBA8_UNorm);
        cubeMapEnvTexture[i]->allocateStorage();

        cubeMapEnvTexture[i]->setData(0, 0, QOpenGLTexture::CubeMapPositiveX,
                                      QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, posXTex.constBits());
        cubeMapEnvTexture[i]->setData(0, 0, QOpenGLTexture::CubeMapNegativeX,
                                      QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, negXTex.constBits());
        cubeMapEnvTexture[i]->setData(0, 0, QOpenGLTexture::CubeMapPositiveY,
                                      QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, posYTex.constBits());
        cubeMapEnvTexture[i]->setData(0, 0, QOpenGLTexture::CubeMapNegativeY,
                                      QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, negYTex.constBits());
        cubeMapEnvTexture[i]->setData(0, 0, QOpenGLTexture::CubeMapPositiveZ,
                                      QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, posZTex.constBits());
        cubeMapEnvTexture[i]->setData(0, 0, QOpenGLTexture::CubeMapNegativeZ,
                                      QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, negZTex.constBits());

        cubeMapEnvTexture[i]->setWrapMode(QOpenGLTexture::DirectionS,
                                          QOpenGLTexture::ClampToEdge);
        cubeMapEnvTexture[i]->setWrapMode(QOpenGLTexture::DirectionT,
                                          QOpenGLTexture::ClampToEdge);
        cubeMapEnvTexture[i]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        cubeMapEnvTexture[i]->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
    }

    if(QOpenGLContext::currentContext()->hasExtension("GL_ARB_seamless_cube_map"))
    {
        qDebug() << "GL_ARB_seamless_cube_map: enabled";
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }
    else
    {
        qDebug() << "GL_ARB_seamless_cube_map: disabled";
        glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }

}

//------------------------------------------------------------------------------------------
void Renderer::initDynamicCubeMapBufferObject()
{
    useGlobalEnvTexture = true;

    GLuint depthBuffer;
    glGenRenderbuffers(1, &depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, CUBE_MAP_SIZE,
                          CUBE_MAP_SIZE);

    FBOCubeMap = new QOpenGLFramebufferObject(CUBE_MAP_SIZE, CUBE_MAP_SIZE);
    FBOCubeMap->setAttachment(QOpenGLFramebufferObject::Depth);
    FBOCubeMap->bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              depthBuffer);
    FBOCubeMap->release();

    for(int i = 0; i < NUM_REFLECTIVE_OBJECTS; ++i)
    {
        objEnvTexture[i] = currentEnvTexture;

        objEnvTextureBuffer1[i] = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
        objEnvTextureBuffer1[i]->create();
        objEnvTextureBuffer1[i]->setSize(CUBE_MAP_SIZE, CUBE_MAP_SIZE);
        objEnvTextureBuffer1[i]->setFormat(QOpenGLTexture::RGBA8_UNorm);
        objEnvTextureBuffer1[i]->allocateStorage();

        objEnvTextureBuffer1[i]->setWrapMode(QOpenGLTexture::DirectionS,
                                             QOpenGLTexture::ClampToEdge);
        objEnvTextureBuffer1[i]->setWrapMode(QOpenGLTexture::DirectionT,
                                             QOpenGLTexture::ClampToEdge);
        objEnvTextureBuffer1[i]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        objEnvTextureBuffer1[i]->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);


        objEnvTextureBuffer2[i] = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
        objEnvTextureBuffer2[i]->create();
        objEnvTextureBuffer2[i]->setSize(CUBE_MAP_SIZE, CUBE_MAP_SIZE);
        objEnvTextureBuffer2[i]->setFormat(QOpenGLTexture::RGBA8_UNorm);
        objEnvTextureBuffer2[i]->allocateStorage();

        objEnvTextureBuffer2[i]->setWrapMode(QOpenGLTexture::DirectionS,
                                             QOpenGLTexture::ClampToEdge);
        objEnvTextureBuffer2[i]->setWrapMode(QOpenGLTexture::DirectionT,
                                             QOpenGLTexture::ClampToEdge);
        objEnvTextureBuffer2[i]->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        objEnvTextureBuffer2[i]->setMagnificationFilter(QOpenGLTexture::LinearMipMapLinear);
    }
}
//------------------------------------------------------------------------------------------
void Renderer::initSceneMemory()
{
    initPlaneMemory();
    initCubeMemory();
    initSphereMemory();
}

//------------------------------------------------------------------------------------------
void Renderer::initPlaneMemory()
{
    if(!planeObject)
    {
        planeObject = new UnitPlane;
    }

    if(vboPlane.isCreated())
    {
        vboPlane.destroy();
    }

    if(iboPlane.isCreated())
    {
        iboPlane.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for plane object
    vboPlane.create();
    vboPlane.bind();
    vboPlane.allocate(2 * planeObject->getVertexOffset() + planeObject->getTexCoordOffset());
    vboPlane.write(0, planeObject->getVertices(), planeObject->getVertexOffset());
    vboPlane.write(planeObject->getVertexOffset(), planeObject->getNormals(),
                   planeObject->getVertexOffset());
    vboPlane.write(2 * planeObject->getVertexOffset(),
                   planeObject->getTexureCoordinates(1.0f),
                   planeObject->getTexCoordOffset());
    vboPlane.release();
    // indices
    iboPlane.create();
    iboPlane.bind();
    iboPlane.allocate(planeObject->getIndices(), planeObject->getIndexOffset());
    iboPlane.release();

}

//------------------------------------------------------------------------------------------
void Renderer::initCubeMemory()
{
    if(!cubeObject)
    {
        cubeObject = new UnitCube;
    }

    if(vboCube.isCreated())
    {
        vboCube.destroy();
    }

    if(iboCube.isCreated())
    {
        iboCube.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for cube
    vboCube.create();
    vboCube.bind();
    vboCube.allocate(2 * cubeObject->getVertexOffset() + cubeObject->getTexCoordOffset());
    vboCube.write(0, cubeObject->getVertices(), cubeObject->getVertexOffset());
    vboCube.write(cubeObject->getVertexOffset(), cubeObject->getNormals(),
                  cubeObject->getVertexOffset());
    vboCube.write(2 * cubeObject->getVertexOffset(), cubeObject->getTexureCoordinates(1.0f),
                  cubeObject->getTexCoordOffset());
    vboCube.release();
    // indices
    iboCube.create();
    iboCube.bind();
    iboCube.allocate(cubeObject->getIndices(), cubeObject->getIndexOffset());
    iboCube.release();
}

//------------------------------------------------------------------------------------------
void Renderer::initSphereMemory()
{
    if(!sphereObject)
    {
        sphereObject = new UnitSphere;
        sphereObject->generateSphere(sphereNumStacks, sphereNumSlices);
    }

    if(vboSphere.isCreated())
    {
        vboSphere.destroy();
    }

    if(iboSphere.isCreated())
    {
        iboSphere.destroy();
    }

    ////////////////////////////////////////////////////////////////////////////////
    // init memory for sphere
    vboSphere.create();
    vboSphere.bind();
    vboSphere.allocate(2 * sphereObject->getVertexOffset() +
                       sphereObject->getTexCoordOffset());
    vboSphere.write(0, sphereObject->getVertices(), sphereObject->getVertexOffset());
    vboSphere.write(sphereObject->getVertexOffset(), sphereObject->getNormals(),
                    sphereObject->getVertexOffset());
    vboSphere.write(2 * sphereObject->getVertexOffset(), sphereObject->getTexureCoordinates(),
                    sphereObject->getTexCoordOffset());
    vboSphere.release();
    // indices
    iboSphere.create();
    iboSphere.bind();
    iboSphere.allocate(sphereObject->getIndices(), sphereObject->getIndexOffset());
    iboSphere.release();

}

//------------------------------------------------------------------------------------------
// record the buffer state by vertex array object
//------------------------------------------------------------------------------------------
void Renderer::initVertexArrayObjects()
{
    initPlaneVAO(PHONG_SHADING);

    initCubeVAO(PHONG_SHADING);

    initSphereVAO(PHONG_SHADING);

}

//------------------------------------------------------------------------------------------
void Renderer::initPlaneVAO(ShadingProgram _shadingMode)
{
    if(vaoPlane[_shadingMode].isCreated())
    {
        vaoPlane[_shadingMode].destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[_shadingMode];

    vaoPlane[_shadingMode].create();
    vaoPlane[_shadingMode].bind();

    vboPlane.bind();
    program->enableAttributeArray(attrVertex[_shadingMode]);
    program->setAttributeBuffer(attrVertex[_shadingMode], GL_FLOAT, 0, 3);

    program->enableAttributeArray(attrNormal[_shadingMode]);
    program->setAttributeBuffer(attrNormal[_shadingMode], GL_FLOAT,
                                planeObject->getVertexOffset(), 3);

    program->enableAttributeArray(attrTexCoord[_shadingMode]);
    program->setAttributeBuffer(attrTexCoord[_shadingMode], GL_FLOAT,
                                2 * planeObject->getVertexOffset(), 2);

    iboPlane.bind();

    // release vao before vbo and ibo
    vaoPlane[_shadingMode].release();
    vboPlane.release();
    iboPlane.release();

}

//------------------------------------------------------------------------------------------
void Renderer::initCubeVAO(ShadingProgram _shadingMode)
{
    if(vaoCube[_shadingMode].isCreated())
    {
        vaoCube[_shadingMode].destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[_shadingMode];

    vaoCube[_shadingMode].create();
    vaoCube[_shadingMode].bind();

    vboCube.bind();
    program->enableAttributeArray(attrVertex[_shadingMode]);
    program->setAttributeBuffer(attrVertex[_shadingMode], GL_FLOAT, 0, 3);

    program->enableAttributeArray(attrNormal[_shadingMode]);
    program->setAttributeBuffer(attrNormal[_shadingMode], GL_FLOAT,
                                cubeObject->getVertexOffset(), 3);

    program->enableAttributeArray(attrTexCoord[_shadingMode]);
    program->setAttributeBuffer(attrTexCoord[_shadingMode], GL_FLOAT,
                                2 * cubeObject->getVertexOffset(), 2);

    iboCube.bind();

    // release vao before vbo and ibo
    vaoCube[_shadingMode].release();
    vboCube.release();
    iboCube.release();

}

//------------------------------------------------------------------------------------------
void Renderer::initSphereVAO(ShadingProgram _shadingMode)
{
    if(vaoSphere[_shadingMode].isCreated())
    {
        vaoSphere[_shadingMode].destroy();
    }

    QOpenGLShaderProgram* program = glslPrograms[_shadingMode];

    vaoSphere[_shadingMode].create();
    vaoSphere[_shadingMode].bind();

    vboSphere.bind();
    program->enableAttributeArray(attrVertex[_shadingMode]);
    program->setAttributeBuffer(attrVertex[_shadingMode], GL_FLOAT, 0, 3);

    program->enableAttributeArray(attrNormal[_shadingMode]);
    program->setAttributeBuffer(attrNormal[_shadingMode], GL_FLOAT,
                                sphereObject->getVertexOffset(), 3);

    program->enableAttributeArray(attrTexCoord[_shadingMode]);
    program->setAttributeBuffer(attrTexCoord[_shadingMode], GL_FLOAT,
                                2 * sphereObject->getVertexOffset(), 2);

    iboSphere.bind();

    // release vao before vbo and ibo
    vaoSphere[_shadingMode].release();
    vboSphere.release();
    iboSphere.release();

}

//------------------------------------------------------------------------------------------
void Renderer::initSceneMatrices()
{
    /////////////////////////////////////////////////////////////////
    // background
    backgroundCubeModelMatrix.setToIdentity();
    backgroundCubeModelMatrix.scale(1000.0f);

    /////////////////////////////////////////////////////////////////
    // floor
    changePlaneSize(30);
    planeNormalMatrix = QMatrix4x4(planeModelMatrix.normalMatrix());

    /////////////////////////////////////////////////////////////////
    // center cube
    cubeModelMatrix.setToIdentity();
    cubeModelMatrix.scale(1.5);
    cubeModelMatrix.translate(DEFAULT_CUBE_POSITION);
    cubeNormalMatrix = QMatrix4x4(cubeModelMatrix.normalMatrix());

    /////////////////////////////////////////////////////////////////
    // sphere
    semiReflectiveSphereModelMatrix.setToIdentity();
    semiReflectiveSphereModelMatrix.translate(DEFAULT_SPHERE_POSITION);
    semiReflectiveSphereNormalMatrix = QMatrix4x4(
                                           semiReflectiveSphereModelMatrix.normalMatrix());
    reflectiveObject2LocationMap[SEMI_REFLECTIVE_SPHERE] = DEFAULT_SPHERE_POSITION;
    reflectiveObject2ModelMatrixMap[SEMI_REFLECTIVE_SPHERE] = semiReflectiveSphereModelMatrix;

    /////////////////////////////////////////////////////////////////
    // reflective sphere
    reflectiveSphereModelMatrix.setToIdentity();
    reflectiveSphereModelMatrix.translate(DEFAULT_REFLECTIVE_SPHERE_POSITION);
    reflectiveSphereNormalMatrix = QMatrix4x4(reflectiveSphereModelMatrix.normalMatrix());
    reflectiveObject2LocationMap[TOTAL_REFLECTIVE_SPHERE] =
        DEFAULT_REFLECTIVE_SPHERE_POSITION;
    reflectiveObject2ModelMatrixMap[TOTAL_REFLECTIVE_SPHERE] = reflectiveSphereModelMatrix;
}

//------------------------------------------------------------------------------------------
void Renderer::changeSphereResolution(int _numStacks, int _numSlices)
{
    sphereNumStacks = _numStacks;
    sphereNumSlices = _numSlices;

    makeCurrent();
    sphereObject->generateSphere(sphereNumStacks, sphereNumSlices);
    initSphereMemory();

    initSphereVAO(PHONG_SHADING);
    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::changePlaneSize(int _planeSize)
{
    planeModelMatrix.setToIdentity();
    planeModelMatrix.scale((float)_planeSize * 2.0f);

    vboPlane.bind();
    vboPlane.write(2 * planeObject->getVertexOffset(),
                   planeObject->getTexureCoordinates((float)_planeSize),
                   planeObject->getTexCoordOffset());
    vboPlane.release();
}

//------------------------------------------------------------------------------------------
void Renderer::resetObjectPositions()
{
    initSceneMatrices();
}

//------------------------------------------------------------------------------------------
void Renderer::changeFloorTexture(FloorTexture _texture)
{
    floorTexture = _texture;
}

//------------------------------------------------------------------------------------------
void Renderer::changeEnvironmentTexture(EnvironmentTexture _texture)
{
//    envTexture = _texture;
    currentEnvTexture = cubeMapEnvTexture[_texture];
}

//------------------------------------------------------------------------------------------
void Renderer::changeFloorTextureFilteringMode(QOpenGLTexture::Filter
                                               _textureFiltering)
{
    for(int i = 0; i < NUM_FLOOR_TEXTURES; ++i)
    {
        floorTextures[i]->setMinMagFilters(_textureFiltering, _textureFiltering);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::changeSphereReflectionPercentage(int _reflectionPercentage)
{
    if(!isValid())
    {
        return;
    }

    semiReflectiveSphereMaterial.setReflection((float)_reflectionPercentage / 100.0f);
    makeCurrent();
    glBindBuffer(GL_UNIFORM_BUFFER, UBOSemireflectiveSphereMaterial);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, semiReflectiveSphereMaterial.getStructSize(),
                    &semiReflectiveSphereMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::changeCubeColor(float _r, float _g, float _b)
{
    if(!isValid())
    {
        return;
    }

    cubeMaterial.setDiffuse(QVector4D(_r, _g, _b, 1.0f));
    makeCurrent();
    glBindBuffer(GL_UNIFORM_BUFFER, UBOCubeMaterial);
    glBufferData(GL_UNIFORM_BUFFER, cubeMaterial.getStructSize(),
                 NULL, GL_STREAM_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, cubeMaterial.getStructSize(),
                    &cubeMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::updateCamera()
{
    zoomCamera();

    /////////////////////////////////////////////////////////////////
    // flush camera data to uniform buffer
    viewMatrix.setToIdentity();
    viewMatrix.lookAt(cameraPosition, cameraFocus, cameraUpDirection);

    viewProjectionMatrix = projectionMatrix * viewMatrix;

    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 2 * SIZE_OF_MAT4, SIZE_OF_MAT4,
                    viewProjectionMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

//------------------------------------------------------------------------------------------
QSize Renderer::sizeHint() const
{
    return QSize(1600, 1200);
}

//------------------------------------------------------------------------------------------
QSize Renderer::minimumSizeHint() const
{
    return QSize(50, 50);
}

//------------------------------------------------------------------------------------------
void Renderer::initializeGL()
{
    initializeOpenGLFunctions();

    checkOpenGLVersion();


    if(!initShaderPrograms())
    {
        PRINT_ERROR("Cannot initialize shaders. Exit...");
        exit(EXIT_FAILURE);

    }

    initRenderingData();
    initSharedBlockUniform();
    initSceneMatrices();
    initDynamicCubeMapBufferObject();

    glEnable(GL_DEPTH_TEST);

    changeShadingMode(PHONG_SHADING);
    changeEnvironmentTexture(SKY);
}

//------------------------------------------------------------------------------------------
void Renderer::resizeGL(int w, int h)
{
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(45, (float)w / (float)h, 0.1f, 10000.0f);
}

//------------------------------------------------------------------------------------------
void Renderer::paintGL()
{
    createObjectCubeMapTextures();

    if(enabledObjectTransformation)
    {
        translateObjects();
        rotateObjects();
    }
    else
    {
        translateCamera();
        rotateCamera();

    }

    updateCamera();

    // render scene
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    renderScene();
}

//-----------------------------------------------------------------------------------------
void Renderer::mousePressEvent(QMouseEvent* _event)
{
    lastMousePos = QVector2D(_event->localPos());

    if(_event->button() == Qt::RightButton)
    {
        mouseButtonPressed = RIGHT_BUTTON;
    }
    else
    {
        mouseButtonPressed = LEFT_BUTTON;
    }
}

//-----------------------------------------------------------------------------------------
void Renderer::mouseMoveEvent(QMouseEvent* _event)
{
    QVector2D mouseMoved = QVector2D(_event->localPos()) - lastMousePos;

    switch(specialKeyPressed)
    {
    case Renderer::NO_KEY:
    {

        if(mouseButtonPressed == RIGHT_BUTTON)
        {
            translation.setX(translation.x() + mouseMoved.x() / 50.0f);
            translation.setY(translation.y() - mouseMoved.y() / 50.0f);
        }
        else
        {
            rotation.setX(rotation.x() - mouseMoved.x() / 5.0f);
            rotation.setY(rotation.y() - mouseMoved.y() / 5.0f);
        }

    }
    break;

    case Renderer::SHIFT_KEY:
    {
        if(mouseButtonPressed == RIGHT_BUTTON)
        {
            QVector2D dir = mouseMoved.normalized();
            zooming += mouseMoved.length() * dir.x() / 500.0f;
        }
        else
        {
            rotation.setX(rotation.x() + mouseMoved.x() / 5.0f);
            rotation.setZ(rotation.z() + mouseMoved.y() / 5.0f);
        }
    }
    break;

    case Renderer::CTRL_KEY:
        break;
    }

    lastMousePos = QVector2D(_event->localPos());
    update();
}

//------------------------------------------------------------------------------------------
void Renderer::mouseReleaseEvent(QMouseEvent* _event)
{
    mouseButtonPressed = NO_BUTTON;
}

//------------------------------------------------------------------------------------------
void Renderer::wheelEvent(QWheelEvent* _event)
{
    if(!_event->angleDelta().isNull())
    {
        zooming +=  (_event->angleDelta().x() + _event->angleDelta().y()) / 500.0f;
    }


    update();
}

//------------------------------------------------------------------------------------------
void Renderer::changeShadingMode(ShadingProgram _shadingMode)
{
    shadingMode = _shadingMode;
    currentProgram = glslPrograms[shadingMode];

    update();
}

//------------------------------------------------------------------------------------------
void Renderer::resetCameraPosition()
{
    cameraPosition = DEFAULT_CAMERA_POSITION;
    cameraFocus = DEFAULT_CAMERA_FOCUS;
    cameraUpDirection = QVector3D(0.0f, 1.0f, 0.0f);
}

//------------------------------------------------------------------------------------------
void Renderer::enableDepthTest(bool _status)
{
    makeCurrent();

    if(_status)
    {
        glEnable(GL_DEPTH_TEST);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }

    doneCurrent();
}

//------------------------------------------------------------------------------------------
void Renderer::enableZAxisRotation(bool _status)
{
    enabledZAxisRotation = _status;

    if(!enabledZAxisRotation)
    {
        cameraUpDirection = QVector3D(0.0f, 1.0f, 0.0f);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::enableObjectTransformation(bool _status)
{
    enabledObjectTransformation = _status;
}

//------------------------------------------------------------------------------------------
void Renderer::enableDynamicEnvironmentMapping(bool _state)
{
    enabledDynamicEnvMapping = _state;

    if(!enabledDynamicEnvMapping)
    {
        useGlobalEnvTexture = true;
    }
}

//------------------------------------------------------------------------------------------
void Renderer::enableBackgroundRendering(bool _state)
{
    enabledBackgroundRendering = _state;
}

//------------------------------------------------------------------------------------------
void Renderer::enableTextureAnisotropicFiltering(bool _state)
{
    enabledTextureAnisotropicFiltering = _state;
}

//------------------------------------------------------------------------------------------
void Renderer::keyPressEvent(QKeyEvent* _event)
{
    switch(_event->key())
    {
    case Qt::Key_Shift:
        specialKeyPressed = Renderer::SHIFT_KEY;
        break;

    case Qt::Key_Plus:
        zooming -= 0.1f;
        break;

    case Qt::Key_Minus:
        zooming += 0.1f;
        break;

    default:
        QOpenGLWidget::keyPressEvent(_event);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::keyReleaseEvent(QKeyEvent* _event)
{
    specialKeyPressed = Renderer::NO_KEY;
}

//------------------------------------------------------------------------------------------
void Renderer::translateCamera()
{
    translation *= MOVING_INERTIA;

    if(translation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D eyeVector = cameraFocus - cameraPosition;
    float scale = sqrt(eyeVector.length()) * 0.01f;

    QVector3D u(0.0f, 1.0f, 0.0f);
    QVector3D v = QVector3D::crossProduct(eyeVector, u);
    u = QVector3D::crossProduct(v, eyeVector);
    u.normalize();
    v.normalize();

    cameraPosition -= scale * (translation.x() * v + translation.y() * u);
    cameraFocus -= scale * (translation.x() * v + translation.y() * u);

}

//------------------------------------------------------------------------------------------
void Renderer::rotateCamera()
{
    rotation *= MOVING_INERTIA;

    if(rotation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D nEyeVector = cameraPosition - cameraFocus ;

    float scale = sqrt(nEyeVector.length()) * 0.02f;
    QQuaternion qRotation = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0),
                                                          rotation.y() * scale) *
                            QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), rotation.x() * scale) *
                            QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), rotation.z() * scale);
    nEyeVector = qRotation.rotatedVector(nEyeVector);

    cameraPosition = cameraFocus + nEyeVector;

    if(enabledZAxisRotation)
    {
        cameraUpDirection = qRotation.rotatedVector(cameraUpDirection);
    }
}

//------------------------------------------------------------------------------------------
void Renderer::zoomCamera()
{
    zooming *= MOVING_INERTIA;

    if(fabs(zooming) < 1e-4)
    {
        return;
    }

    QVector3D nEyeVector = cameraPosition - cameraFocus ;
    float len = nEyeVector.length();
    nEyeVector.normalize();

    len += sqrt(len) * zooming * 0.3f;

    if(len < 0.5f)
    {
        len = 0.5f;
    }

    cameraPosition = len * nEyeVector + cameraFocus;

}

//------------------------------------------------------------------------------------------
void Renderer::translateObjects()
{
    translation *= MOVING_INERTIA;

    if(translation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D eyeVector = cameraFocus - cameraPosition;
    float scale = sqrt(eyeVector.length()) * 0.05f;

    QVector3D u(0.0f, 1.0f, 0.0f);
    QVector3D v = QVector3D::crossProduct(eyeVector, u);
    u = QVector3D::crossProduct(v, eyeVector);
    u.normalize();
    v.normalize();

    QVector3D objectTrans = scale * (translation.x() * v + translation.y() * u);
    QMatrix4x4 translationMatrix;
    translationMatrix.setToIdentity();
    translationMatrix.translate(objectTrans);

    cubeModelMatrix = translationMatrix * cubeModelMatrix;
    semiReflectiveSphereModelMatrix = translationMatrix * semiReflectiveSphereModelMatrix;
    reflectiveObject2LocationMap[SEMI_REFLECTIVE_SPHERE] = semiReflectiveSphereModelMatrix *
                                                           QVector3D(
                                                               0.0f, 0.0f, 0.0f);
}

//------------------------------------------------------------------------------------------
void Renderer::rotateObjects()
{
    rotation *= MOVING_INERTIA;

    if(rotation.lengthSquared() < 1e-4)
    {
        return;
    }

    QVector3D currentPos(0.0f, 0.0f, 0.0f);
    currentPos = cubeModelMatrix * currentPos;

    float scale = -0.2f;
    QQuaternion qRotation = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f),
                                                          rotation.x() * scale) *
                            QQuaternion::fromAxisAndAngle(QVector3D(1.0f, 0.0f, 0.0f), rotation.y() * scale);
    //*
    //                      QQuaternion::fromAxisAndAngle(QVector3D(0, 0, 1), rotation.z()*scale);

    QMatrix4x4 rotationMatrix;
    rotationMatrix.setToIdentity();
    rotationMatrix.rotate(qRotation);

    QMatrix4x4 invTranslationMatrix, translationMatrix;
    invTranslationMatrix.setToIdentity();
    invTranslationMatrix.translate(-1.0f * currentPos);
    translationMatrix.setToIdentity();
    translationMatrix.translate(currentPos);

    cubeModelMatrix = translationMatrix * rotationMatrix * invTranslationMatrix *
                      cubeModelMatrix;
    semiReflectiveSphereModelMatrix = translationMatrix * rotationMatrix *
                                      invTranslationMatrix *
                                      semiReflectiveSphereModelMatrix;

    cubeNormalMatrix = QMatrix4x4(cubeModelMatrix.normalMatrix());
    semiReflectiveSphereNormalMatrix = QMatrix4x4(
                                           semiReflectiveSphereModelMatrix.normalMatrix());
    reflectiveObject2LocationMap[SEMI_REFLECTIVE_SPHERE] = semiReflectiveSphereModelMatrix *
                                                           QVector3D(
                                                               0.0f, 0.0f, 0.0f);
}


//------------------------------------------------------------------------------------------
void Renderer::createDynamicCubeMapTexture(ReflectiveObjects _object)
{
    QMatrix4x4  faceViewMatrix;
    QMatrix4x4  faceProjectionMatrix;
    QMatrix4x4  faceViewProjectionMatrix;

    /////////////////////////////////////////////////////////////////
    // render to buffer
    static QVector3D upDirs[6] =
    {
        QVector3D(0.0f, -1.0f, 0.0f), // posX
        QVector3D(0.0f, -1.0f, 0.0f), // negX
        QVector3D(0.0f, 0.0f, 1.0f), // posY
        QVector3D(0.0f, 0.0f, -1.0f), // negY
        QVector3D(0.0f, -1.0f, 0.0f), // posZ
        QVector3D(0.0f, -1.0f, 0.0f), // negZ
    };

    static QVector3D viewDirs[6] =
    {
        QVector3D(1.0f, 0.0f, 0.0f), // posX
        QVector3D(-1.0f, 0.0f, 0.0f), // negX
        QVector3D(0.0f, 1.0f, 0.0f), // posY
        QVector3D(0.0f, -1.0f, 0.0f), // negY
        QVector3D(0.0f, 0.0f, 1.0f), // posZ
        QVector3D(0.0f, 0.0f, -1.0f), // negZ
    };

    QVector3D localCamera = reflectiveObject2LocationMap[_object];

    faceProjectionMatrix.setToIdentity();
    faceProjectionMatrix.perspective(90, 1.0f, 0.1f, 10000.0f);

    FBOCubeMap->bind();
    glViewport(0, 0, CUBE_MAP_SIZE, CUBE_MAP_SIZE);

    for(int face = 0; face < 6; ++face)
    {
        faceViewMatrix.setToIdentity();
        faceViewMatrix.lookAt(localCamera, localCamera + viewDirs[face], upDirs[face]);
        faceViewProjectionMatrix = faceProjectionMatrix * faceViewMatrix;

        glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
        glBufferSubData(GL_UNIFORM_BUFFER, 2 * SIZE_OF_MAT4, SIZE_OF_MAT4,
                        faceViewProjectionMatrix.constData());
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                               objEnvTextureBuffer2[_object]->textureId(), 0);
        renderScene(_object);
    }

    FBOCubeMap->release();
    makeCurrent();


    /////////////////////////////////////////////////////////////////
    // swap texture
    qSwap(objEnvTextureBuffer1[_object], objEnvTextureBuffer2[_object]);
    useGlobalEnvTexture = false;
}

//------------------------------------------------------------------------------------------
void Renderer::createObjectCubeMapTextures()
{
    for(int i = 0; i < NUM_REFLECTIVE_OBJECTS; ++i)
    {
        if(enabledDynamicEnvMapping)
        {
            if(useGlobalEnvTexture)
            {
                objEnvTexture[i] = currentEnvTexture;
            }
            else
            {
                objEnvTexture[i] = objEnvTextureBuffer1[i];
            }

            createDynamicCubeMapTexture(static_cast<ReflectiveObjects>(i));
        }
        else
        {
            objEnvTexture[i] = currentEnvTexture;
        }
    }
}

//------------------------------------------------------------------------------------------
void Renderer::renderScene(ReflectiveObjects _hiddenObj)
{
    glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // background
    if(enabledBackgroundRendering)
    {
        renderBackground();
    }

    // set the data for rendering
    currentProgram->bind();
    currentProgram->setUniformValue(uniCameraPosition[shadingMode], cameraPosition);
    currentProgram->setUniformValue(uniObjTexture[shadingMode], 0);
    currentProgram->setUniformValue(uniEnvTexture[shadingMode], 1);

    glUniformBlockBinding(currentProgram->programId(), uniMatrices[shadingMode],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    glUniformBlockBinding(currentProgram->programId(), uniLight[shadingMode],
                          UBOBindingIndex[BINDING_LIGHT]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_LIGHT],
                     UBOLight);

    renderFloor();

    renderCube();

    if(_hiddenObj != SEMI_REFLECTIVE_SPHERE)
    {
        renderSemireflectiveSphere();
    }

    if(_hiddenObj != TOTAL_REFLECTIVE_SPHERE)
    {
        renderReflectiveSphere();
    }

    currentProgram->release();
}
//------------------------------------------------------------------------------------------
void Renderer::renderBackground()
{
    QOpenGLShaderProgram* program = glslPrograms[BACKGROUND_SHADING];
    program->bind();

    /////////////////////////////////////////////////////////////////
    // flush the model matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    backgroundCubeModelMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    program->setUniformValue(uniCameraPosition[BACKGROUND_SHADING], cameraPosition);
    program->setUniformValue(uniEnvTexture[BACKGROUND_SHADING], 1);

    glUniformBlockBinding(program->programId(), uniMatrices[BACKGROUND_SHADING],
                          UBOBindingIndex[BINDING_MATRICES]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_MATRICES],
                     UBOMatrices);

    /////////////////////////////////////////////////////////////////
    // render the background
    vaoCube[shadingMode].bind();
    currentEnvTexture->bind(1);
    glDrawElements(GL_TRIANGLES, cubeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    currentEnvTexture->release();
    vaoCube[shadingMode].release();

    program->release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderFloor()
{
    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    planeModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    planeNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentProgram->setUniformValue(uniHasObjTexture[shadingMode], GL_TRUE);

    glUniformBlockBinding(currentProgram->programId(), uniMaterial[shadingMode],
                          UBOBindingIndex[BINDING_FLOOR_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_FLOOR_MATERIAL],
                     UBOPlaneMaterial);

    /////////////////////////////////////////////////////////////////
    // render the floor
    vaoPlane[shadingMode].bind();
    floorTextures[floorTexture]->bind(0);

    if(enabledTextureAnisotropicFiltering)
    {
        GLfloat fLargest;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);
    }
    else
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
    }

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    floorTextures[floorTexture]->release();
    vaoPlane[shadingMode].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderCube()
{
    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    cubeModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    cubeNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentProgram->setUniformValue(uniHasObjTexture[shadingMode], GL_TRUE);

    glUniformBlockBinding(currentProgram->programId(), uniMaterial[shadingMode],
                          UBOBindingIndex[BINDING_CUBE_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_CUBE_MATERIAL],
                     UBOCubeMaterial);

    /////////////////////////////////////////////////////////////////
    // render the cube
    vaoCube[shadingMode].bind();
    decalTexture->bind(0);
    glDrawElements(GL_TRIANGLES, cubeObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    decalTexture->release();
    vaoCube[shadingMode].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderSemireflectiveSphere()
{
    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    semiReflectiveSphereModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    semiReflectiveSphereNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentProgram->setUniformValue(uniHasObjTexture[shadingMode], GL_TRUE);

    glUniformBlockBinding(currentProgram->programId(), uniMaterial[shadingMode],
                          UBOBindingIndex[BINDING_SEMIREFLECTIVE_SPHERE_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER,
                     UBOBindingIndex[BINDING_SEMIREFLECTIVE_SPHERE_MATERIAL],
                     UBOSemireflectiveSphereMaterial);

    /////////////////////////////////////////////////////////////////
    // render the sphere
    vaoSphere[shadingMode].bind();
    sphereTexture->bind(0);
    objEnvTexture[SEMI_REFLECTIVE_SPHERE]->bind(1);
    glDrawElements(GL_TRIANGLES, sphereObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    sphereTexture->release();
    objEnvTexture[SEMI_REFLECTIVE_SPHERE]->release();
    vaoSphere[shadingMode].release();
}

//------------------------------------------------------------------------------------------
void Renderer::renderReflectiveSphere()
{
    /////////////////////////////////////////////////////////////////
    // flush the model and normal matrices
    glBindBuffer(GL_UNIFORM_BUFFER, UBOMatrices);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, SIZE_OF_MAT4,
                    reflectiveSphereModelMatrix.constData());
    glBufferSubData(GL_UNIFORM_BUFFER, SIZE_OF_MAT4, SIZE_OF_MAT4,
                    reflectiveSphereNormalMatrix.constData());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    /////////////////////////////////////////////////////////////////
    // set the uniform
    currentProgram->setUniformValue(uniHasObjTexture[shadingMode], GL_FALSE);

    glUniformBlockBinding(currentProgram->programId(), uniMaterial[shadingMode],
                          UBOBindingIndex[BINDING_REFLECTIVE_SPHERE_MATERIAL]);
    glBindBufferBase(GL_UNIFORM_BUFFER, UBOBindingIndex[BINDING_REFLECTIVE_SPHERE_MATERIAL],
                     UBOReflectiveSphereMaterial);

    /////////////////////////////////////////////////////////////////
    // render the sphere
    vaoSphere[shadingMode].bind();
    objEnvTexture[TOTAL_REFLECTIVE_SPHERE]->bind(1);
    glDrawElements(GL_TRIANGLES, sphereObject->getNumIndices(), GL_UNSIGNED_SHORT, 0);
    objEnvTexture[TOTAL_REFLECTIVE_SPHERE]->release();
    vaoSphere[shadingMode].release();
}
