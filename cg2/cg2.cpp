#include "glut.h"
#include <cmath>
#include <numbers>

struct Point
{
    int x, y;
};

// Encapsulates application state to eliminate unsafe mutable global variables.
class RasterGrid
{
private:
    static constexpr int gridExtent = 30;
    static constexpr float pointRadius = 0.35f;
    static constexpr int countVertices = 12;
    static constexpr Point polygonVertices[countVertices] =
        {{15, 29}, {22, 27}, {27, 22}, {29, 15}, {27, 8}, {22, 3}, {15, 1}, {8, 3}, {3, 8}, {1, 15}, {3, 22}, {8, 27}};

    int activeDisplayMode = 1;
    bool isCircleFilled = false;

    // Renders a basic circular point at the specified grid intersection.
    void drawDisc(int xCoordinate, int yCoordinate, float red, float green, float blue) const
    {
        if (xCoordinate < 0 || xCoordinate > gridExtent || yCoordinate < 0 || yCoordinate > gridExtent)
            return;
        glColor3f(red, green, blue);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(static_cast<float>(xCoordinate), static_cast<float>(yCoordinate));
        for (int vertexIndex = 0; vertexIndex <= 16; ++vertexIndex)
        {
            const float angle = 2.0f * std::numbers::pi_v<float> * vertexIndex / 16.0f;
            glVertex2f(xCoordinate + std::cos(angle) * pointRadius, yCoordinate + std::sin(angle) * pointRadius);
        }
        glEnd();
    }

    void drawShallowThickPixel(int xCoordinate, int yCoordinate) const
    {
        drawDisc(xCoordinate, yCoordinate, 0.0f, 0.0f, 0.0f);
        drawDisc(xCoordinate, yCoordinate + 1, 0.0f, 0.0f, 0.0f);
        drawDisc(xCoordinate, yCoordinate - 1, 0.0f, 0.0f, 0.0f);
    }

    // Evaluates Bresenham's algorithm for lines with a slope magnitude less than or equal to 1.
    void drawShallowLine(int startX, int startY, int endX, int endY) const
    {
        int deltaX = std::abs(endX - startX), deltaY = std::abs(endY - startY);
        int stepX = (startX < endX) ? 1 : -1, stepY = (startY < endY) ? 1 : -1;
        int decisionVariable = 2 * deltaY - deltaX;
        int deltaEast = 2 * deltaY, deltaNorthEast = 2 * (deltaY - deltaX);
        int currentX = startX, currentY = startY;
        while (currentX != endX)
        {
            drawShallowThickPixel(currentX, currentY);
            currentX += stepX;
            if (decisionVariable < 0)
                decisionVariable += deltaEast;
            else
            {
                currentY += stepY;
                decisionVariable += deltaNorthEast;
            }
        }
        drawShallowThickPixel(currentX, currentY);
    }

    void drawSteepThickPixel(int xCoordinate, int yCoordinate) const
    {
        drawDisc(xCoordinate, yCoordinate, 0.0f, 0.0f, 0.0f);
        drawDisc(xCoordinate + 1, yCoordinate, 0.0f, 0.0f, 0.0f);
        drawDisc(xCoordinate - 1, yCoordinate, 0.0f, 0.0f, 0.0f);
    }

    // Evaluates Bresenham's algorithm for lines with a slope magnitude greater than 1.
    void drawSteepLine(int startX, int startY, int endX, int endY) const
    {
        int deltaX = std::abs(endX - startX), deltaY = std::abs(endY - startY);
        int stepX = (startX < endX) ? 1 : -1, stepY = (startY < endY) ? 1 : -1;
        int decisionVariable = 2 * deltaX - deltaY;
        int deltaNorth = 2 * deltaX, deltaNorthEast = 2 * (deltaX - deltaY);
        int currentX = startX, currentY = startY;
        while (currentY != endY)
        {
            drawSteepThickPixel(currentX, currentY);
            currentY += stepY;
            if (decisionVariable < 0)
                decisionVariable += deltaNorth;
            else
            {
                currentX += stepX;
                decisionVariable += deltaNorthEast;
            }
        }
        drawSteepThickPixel(currentX, currentY);
    }

    // Routes the coordinate rendering to the appropriate steep or shallow Bresenham calculation.
    void drawBresenhamLine(int startX, int startY, int endX, int endY) const
    {
        glColor3f(0.0f, 0.0f, 1.0f);
        glBegin(GL_LINES);
        glVertex2i(startX, startY);
        glVertex2i(endX, endY);
        glEnd();
        if (std::abs(endY - startY) <= std::abs(endX - startX))
            drawShallowLine(startX, startY, endX, endY);
        else
            drawSteepLine(startX, startY, endX, endY);
    }

    // Handles rendering of the inner fill pixels if the circle fill mode is toggled.
    void fillCircleScanline(int startX, int endX, int yCoordinate) const
    {
        if (!isCircleFilled)
            return;
        for (int xCoordinate = startX; xCoordinate <= endX; ++xCoordinate)
            drawDisc(xCoordinate, yCoordinate, 0.0f, 0.0f, 0.0f);
    }

    // Plots the 8 symmetrical sectors for the midpoint circle algorithm.
    void plotCircleSectors(int centerX, int centerY, int offsetX, int offsetY) const
    {
        drawDisc(centerX + offsetX, centerY + offsetY, 0.0f, 0.0f, 0.0f);
        drawDisc(centerX + offsetX, centerY - offsetY, 0.0f, 0.0f, 0.0f);
        fillCircleScanline(centerX - offsetX, centerX + offsetX, centerY + offsetY);
        fillCircleScanline(centerX - offsetX, centerX + offsetX, centerY - offsetY);

        drawDisc(centerX - offsetX, centerY + offsetY, 0.0f, 0.0f, 0.0f);
        drawDisc(centerX - offsetX, centerY - offsetY, 0.0f, 0.0f, 0.0f);
        drawDisc(centerX + offsetY, centerY + offsetX, 0.0f, 0.0f, 0.0f);
        drawDisc(centerX + offsetY, centerY - offsetX, 0.0f, 0.0f, 0.0f);

        fillCircleScanline(centerX - offsetY, centerX + offsetY, centerY + offsetX);
        fillCircleScanline(centerX - offsetY, centerX + offsetY, centerY - offsetX);
        drawDisc(centerX - offsetY, centerY + offsetX, 0.0f, 0.0f, 0.0f);
        drawDisc(centerX - offsetY, centerY - offsetX, 0.0f, 0.0f, 0.0f);
    }

    // ! Main display of a thick rasterized.
    void drawThickCircle() const
    {
        for (size_t index = 0; index < countVertices; ++index)
        {
            drawBresenhamLine(
                polygonVertices[index].x, polygonVertices[index].y,
                polygonVertices[(index + 1) % countVertices].x,
                polygonVertices[(index + 1) % countVertices].y);
        }
    }

    // ! Main display of a rasterized circle using integer-only midpoint calculations.
    void drawMidpointCircle(int centerX, int centerY, int radius) const
    {
        glColor3f(0.0f, 0.0f, 1.0f);
        glBegin(GL_LINE_LOOP);
        for (int vertexIndex = 0; vertexIndex < 100; ++vertexIndex)
        {
            const float angle = 2.0f * std::numbers::pi_v<float> * vertexIndex / 100.0f;
            glVertex2f(centerX + radius * std::cos(angle), centerY + radius * std::sin(angle));
        }
        glEnd();

        int currentX = 0, currentY = radius, decisionVariable = 1 - radius;
        int deltaEast = 3, deltaSouthEast = -2 * radius + 5;

        while (currentX <= currentY)
        {
            plotCircleSectors(centerX, centerY, currentX, currentY);
            if (decisionVariable < 0)
            {
                decisionVariable += deltaEast;
                deltaEast += 2;
                deltaSouthEast += 2;
            }
            else
            {
                decisionVariable += deltaSouthEast;
                deltaEast += 2;
                deltaSouthEast += 4;
                currentY--;
            }
            currentX++;
        }
    }

public:
    // Singleton access to wire C++ methods to C-style GLUT callbacks safely.
    static RasterGrid &getInstance()
    {
        static RasterGrid instance;
        return instance;
    }

    void toggleDisplayMode(int mode)
    {
        activeDisplayMode = mode;
    }

    void toggleFill()
    {
        isCircleFilled = !isCircleFilled;
    }

    // Core rendering loop bridging the data state to OpenGL instructions.
    void render() const
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        glColor3f(0.85f, 0.85f, 0.85f);
        glBegin(GL_LINES);
        for (int index = 0; index <= gridExtent; ++index)
        {
            glVertex2f(static_cast<float>(index), 0.0f);
            glVertex2f(static_cast<float>(index), static_cast<float>(gridExtent));
            glVertex2f(0.0f, static_cast<float>(index));
            glVertex2f(static_cast<float>(gridExtent), static_cast<float>(index));
        }
        glEnd();

        if (activeDisplayMode == 1)
            drawThickCircle();
        if (activeDisplayMode == 2)
            drawMidpointCircle(15, 15, 13);
        glutSwapBuffers();
    }

    // Adjusts the projection to maintain square aspect ratios while preventing division by zero.
    void resize(int width, int height) const
    {
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        if (height == 0)
            height = 1;
        float aspect = static_cast<float>(width) / static_cast<float>(height),
              center = gridExtent / 2.0f,
              halfExtent = (gridExtent + 2.0f) / 2.0f;

        if (width >= height)
            glOrtho(center - halfExtent * aspect, center + halfExtent * aspect, -1.0f, gridExtent + 1.0f, -1.0f, 1.0f);
        else
            glOrtho(-1.0f, gridExtent + 1.0f, center - halfExtent / aspect, center + halfExtent / aspect, -1.0f, 1.0f);
        glMatrixMode(GL_MODELVIEW);
    }
};

void displayCallback()
{
    RasterGrid::getInstance().render();
}

void reshapeCallback(int width, int height)
{
    RasterGrid::getInstance().resize(width, height);
}

void keyboardCallback(unsigned char key, int x, int y)
{
    if (key == '1')
        RasterGrid::getInstance().toggleDisplayMode(1);
    if (key == '2')
        RasterGrid::getInstance().toggleDisplayMode(2);
    if (key == '3')
        RasterGrid::getInstance().toggleFill();
    if (key == 27)
        exit(0);
    glutPostRedisplay();
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(700, 700);
    glutCreateWindow("RasterGrid");
    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardCallback);
    glutMainLoop();
    return 0;
}
