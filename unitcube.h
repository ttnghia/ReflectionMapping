//------------------------------------------------------------------------------------------
//
//
// Created on: 1/21/2015
//     Author: Nghia Truong
//
//------------------------------------------------------------------------------------------
#include <QOpenGLWidget>
#include <QList>
#include <QVector3D>
#include <QVector2D>

#ifndef UNITCUBE_H
#define UNITCUBE_H


class UnitCube
{
public:
    UnitCube();
    ~UnitCube();

    int getNumVertices();
    int getNumIndices();
    int getVertexOffset();
    int getTexCoordOffset();
    int getIndexOffset();


    GLfloat* getVertices();
    GLfloat* getVertexColors();
    GLfloat* getNormals();
    GLfloat* getNegativeNormals();
    GLfloat* getTexureCoordinates(float _scale);
    GLushort* getIndices();


private:
    void clearData();

    QList<QVector3D> vertexList;
    QList<QVector3D> colorList;
    QList<QVector2D> texCoordList;
    QList<QVector3D> normalsList;
    GLfloat* vertices;
    GLfloat* colors;
    GLfloat* texCoord;
    GLfloat* normals;
    GLfloat* negNormals;
    static GLushort indices[];
};

#endif // UNITCUBE_H
