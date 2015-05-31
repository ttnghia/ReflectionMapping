//------------------------------------------------------------------------------------------
// mainwindow.cpp
//
// Created on: 1/17/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------

#include "mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QWidget(parent)
{
    setWindowTitle("Cube Mapping + Reflection Mapping");

    setupGUI();
    changeCubeColor();


    // Update continuously
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), renderer, SLOT(update()));
    timer->start(10);
}

//------------------------------------------------------------------------------------------
QSize MainWindow::sizeHint() const
{
    return QSize(1600, 1200);
}

//------------------------------------------------------------------------------------------
QSize MainWindow::minimumSizeHint() const
{
    return QSize(50, 50);
}

//------------------------------------------------------------------------------------------
void MainWindow::keyPressEvent(QKeyEvent* e)
{
    switch(e->key())
    {
    case Qt::Key_Escape:
        close();
        break;

    case Qt::Key_R:
        renderer->resetCameraPosition();
        break;

    case Qt::Key_B:
        chkBackgroundRendering->toggle();
        break;

    case Qt::Key_D:
        chkDynamicEnvMapping->toggle();
        break;

    case Qt::Key_A:
        chkTextureAnisotropicFiltering->toggle();
        break;

    default:
        renderer->keyPressEvent(e);
    }
}


//------------------------------------------------------------------------------------------
void MainWindow::setupGUI()
{
    renderer = new Renderer(this);

    ////////////////////////////////////////////////////////////////////////////////
    // texture filtering mode
    cbTextureFiltering = new QComboBox;
    QString str;

    str = QString("NEAREST");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::Nearest;

    str = QString("LINEAR");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::Linear;


    str = QString("NEAREST_MIPMAP_NEAREST");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::NearestMipMapNearest;


    str = QString("NEAREST_MIPMAP_LINEAR");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::NearestMipMapLinear;


    str = QString("LINEAR_MIPMAP_NEAREST");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::LinearMipMapNearest;

    str = QString("LINEAR_MIPMAP_LINEAR");
    cbTextureFiltering->addItem(str);
    str2TextureFilteringMap[str] = QOpenGLTexture::LinearMipMapLinear;



    QVBoxLayout* textureFilteringLayout = new QVBoxLayout;
    textureFilteringLayout->addWidget(cbTextureFiltering);
    QGroupBox* textureFilteringGroup = new QGroupBox("Floor Texture MinMag Filtering Mode");
    textureFilteringGroup->setLayout(textureFilteringLayout);

    cbTextureFiltering->setCurrentIndex(5);
    connect(cbTextureFiltering, SIGNAL(currentIndexChanged(int)), this,
            SLOT(changeTextureFilteringMode()));

    chkTextureAnisotropicFiltering = new QCheckBox("Texture Anisotropic Filtering");
    chkTextureAnisotropicFiltering->setChecked(true);
    connect(chkTextureAnisotropicFiltering, &QCheckBox::toggled, renderer, &Renderer::enableTextureAnisotropicFiltering);

    ////////////////////////////////////////////////////////////////////////////////
    // cube color picker
    sldCubeRColor = new QSlider(Qt::Horizontal);
    sldCubeRColor->setMinimum(0);
    sldCubeRColor->setMaximum(255);
    sldCubeRColor->setValue(0);

    sldCubeGColor = new QSlider(Qt::Horizontal);
    sldCubeGColor->setMinimum(0);
    sldCubeGColor->setMaximum(255);
    sldCubeGColor->setValue(255);

    sldCubeBColor = new QSlider(Qt::Horizontal);
    sldCubeBColor->setMinimum(0);
    sldCubeBColor->setMaximum(255);
    sldCubeBColor->setValue(50);

    spCubeRColor = new QSpinBox;
    spCubeRColor->setMinimum(0);
    spCubeRColor->setMaximum(255);
    spCubeRColor->setValue(0);

    spCubeGColor = new QSpinBox;
    spCubeGColor->setMinimum(0);
    spCubeGColor->setMaximum(255);
    spCubeGColor->setValue(255);

    spCubeBColor = new QSpinBox;
    spCubeBColor->setMinimum(0);
    spCubeBColor->setMaximum(255);
    spCubeBColor->setValue(50);



    connect(sldCubeRColor, &QSlider::valueChanged, spCubeRColor,
            &QSpinBox::setValue);
    connect(sldCubeGColor, &QSlider::valueChanged, spCubeGColor,
            &QSpinBox::setValue);
    connect(sldCubeBColor, &QSlider::valueChanged, spCubeBColor,
            &QSpinBox::setValue);


    connect(spCubeRColor, SIGNAL(valueChanged(int)), sldCubeRColor,
            SLOT(setValue(int)));
    connect(spCubeGColor, SIGNAL(valueChanged(int)), sldCubeGColor,
            SLOT(setValue(int)));
    connect(spCubeBColor, SIGNAL(valueChanged(int)), sldCubeBColor,
            SLOT(setValue(int)));

    connect(sldCubeRColor, &QSlider::valueChanged, this, &MainWindow::changeCubeColor);
    connect(sldCubeGColor, &QSlider::valueChanged, this, &MainWindow::changeCubeColor);
    connect(sldCubeBColor, &QSlider::valueChanged, this, &MainWindow::changeCubeColor);

    wgCubeColor = new QWidget;
    wgCubeColor->setAutoFillBackground(true);
    wgCubeColor->setFixedWidth(20);


    QGridLayout* cubeColorLayout = new QGridLayout;
    cubeColorLayout->addWidget(new QLabel("R:"), 0, 0);
    cubeColorLayout->addWidget(spCubeRColor, 0, 1);
    cubeColorLayout->addWidget(sldCubeRColor, 0, 2);
    cubeColorLayout->addWidget(new QLabel("G:"), 1, 0);
    cubeColorLayout->addWidget(spCubeGColor, 1, 1);
    cubeColorLayout->addWidget(sldCubeGColor, 1, 2);
    cubeColorLayout->addWidget(new QLabel("B:"), 2, 0);
    cubeColorLayout->addWidget(spCubeBColor, 2, 1);
    cubeColorLayout->addWidget(sldCubeBColor, 2, 2);
    cubeColorLayout->addWidget(wgCubeColor, 0, 3, 3, 1);

    QGroupBox* cubeColorGroup = new QGroupBox("Cube Color");
    cubeColorGroup->setLayout(cubeColorLayout);

    ////////////////////////////////////////////////////////////////////////////////
    // sphere reflection level
    sldSphereReflection = new QSlider(Qt::Horizontal);
    sldSphereReflection->setMinimum(0);
    sldSphereReflection->setMaximum(100);
    sldSphereReflection->setValue(50);
    connect(sldSphereReflection, &QSlider::valueChanged, renderer,
            &Renderer::changeSphereReflectionPercentage);

    QVBoxLayout* sphereReflectionLayout = new QVBoxLayout;
    sphereReflectionLayout->addWidget(sldSphereReflection);
    QGroupBox* sphereReflectionGroup = new QGroupBox("Sphere Reflection Percentage");
    sphereReflectionGroup->setLayout(sphereReflectionLayout);


    ////////////////////////////////////////////////////////////////////////////////
    // plane size
    sldPlaneSize = new QSlider(Qt::Horizontal);
    sldPlaneSize->setMinimum(1);
    sldPlaneSize->setMaximum(200);
    sldPlaneSize->setValue(10);

    connect(sldPlaneSize, &QSlider::valueChanged, renderer,
            &Renderer::changePlaneSize);

    QVBoxLayout* planeSizeLayout = new QVBoxLayout;
    planeSizeLayout->addWidget(sldPlaneSize);
    QGroupBox* planeSizeGroup = new QGroupBox("Plane Size");
    planeSizeGroup->setLayout(planeSizeLayout);



    ////////////////////////////////////////////////////////////////////////////////
    // others
    chkDynamicEnvMapping = new QCheckBox("Dynamic Environment Mapping");
    chkDynamicEnvMapping->setChecked(false);
    connect(chkDynamicEnvMapping, &QCheckBox::toggled, renderer,
            &Renderer::enableDynamicEnvironmentMapping);


    chkBackgroundRendering = new QCheckBox("Render Background");
    chkBackgroundRendering->setChecked(true);
    connect(chkBackgroundRendering, &QCheckBox::toggled, renderer,
            &Renderer::enableBackgroundRendering);

    chkEnableDepthTest = new QCheckBox("Enable Depth Test");
    chkEnableDepthTest->setChecked(true);
    connect(chkEnableDepthTest, &QCheckBox::toggled, renderer,
            &Renderer::enableDepthTest);

    chkEnableZAxisRotation = new QCheckBox("Enable Z Axis Rotation");
    chkEnableZAxisRotation->setChecked(false);
    connect(chkEnableZAxisRotation, &QCheckBox::toggled, renderer,
            &Renderer::enableZAxisRotation);

    QPushButton* btnResetObjects = new QPushButton("Reset Object Positions");
    connect(btnResetObjects, SIGNAL(clicked()), this,
            SLOT(resetObjectPositions()));

    QPushButton* btnResetCamera = new QPushButton("Reset Camera");
    connect(btnResetCamera, &QPushButton::clicked, renderer,
            &Renderer::resetCameraPosition);


    ////////////////////////////////////////////////////////////////////////////////
    // transform cube & sphere
    chkMoveCubeWithSphere = new QCheckBox( QString::fromUtf8("Transform Cube && Sphere"));
    chkMoveCubeWithSphere->setChecked(false);
    connect(chkMoveCubeWithSphere, &QCheckBox::toggled, renderer,
            &Renderer::enableObjectTransformation);


    ////////////////////////////////////////////////////////////////////////////////
    // Add slider group to parameter group
    QVBoxLayout* parameterLayout = new QVBoxLayout;
    parameterLayout->addWidget(textureFilteringGroup);
    parameterLayout->addWidget(chkTextureAnisotropicFiltering);
    parameterLayout->addWidget(cubeColorGroup);
    parameterLayout->addWidget(sphereReflectionGroup);
    parameterLayout->addWidget(planeSizeGroup);
    parameterLayout->addWidget(chkDynamicEnvMapping);
    parameterLayout->addWidget(chkBackgroundRendering);
    parameterLayout->addWidget(chkEnableDepthTest);
    parameterLayout->addWidget(chkEnableZAxisRotation);
    parameterLayout->addWidget(chkMoveCubeWithSphere);

    parameterLayout->addWidget(btnResetObjects);
    parameterLayout->addWidget(btnResetCamera);



    parameterLayout->addStretch();

    QGroupBox* parameterGroup = new QGroupBox;
    parameterGroup->setMinimumWidth(350);
    parameterGroup->setMaximumWidth(350);
    parameterGroup->setLayout(parameterLayout);



    QHBoxLayout* hLayout = new QHBoxLayout;
    hLayout->addWidget(renderer);
    hLayout->addWidget(parameterGroup);

    setLayout(hLayout);

}


//------------------------------------------------------------------------------------------
void MainWindow::changeTextureFilteringMode()
{
    QOpenGLTexture::Filter filterMode =
        str2TextureFilteringMap[cbTextureFiltering->currentText()];
    renderer->changeFloorTextureFilteringMode(filterMode);
}

//------------------------------------------------------------------------------------------
void MainWindow::resetObjectPositions()
{
    sldPlaneSize->setValue(10);
    renderer->resetObjectPositions();
}

//------------------------------------------------------------------------------------------
void MainWindow::changeCubeColor()
{
    int r = sldCubeRColor->value();
    int g = sldCubeGColor->value();
    int b = sldCubeBColor->value();

    QPalette palette = wgCubeColor->palette();
    palette.setColor(QPalette::Window, QColor(r, g, b));
    wgCubeColor->setPalette(palette);
    renderer->changeCubeColor((float) r / 255.0f, (float) g / 255.0f, (float) b / 255.0f);
}

