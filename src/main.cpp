#include <GL/glut.h>
#include <cmath>
#include <complex>
#include <iostream>
#include <random>
#include <vector>

const int LATTICE_SIZE = 100;
const int TIME_STEPS = 50;
const int NUM_PATHS = 1000;
const double HBAR = 1.0;
const double MASS = 1.0;
const double DT = 0.1;
const double DX = 0.1;

struct Path {
    std::vector<double> positions;
    std::complex<double> amplitude;
    double action;
};

class PathIntegralSimulation {
private:
    std::vector<Path> paths;
    std::mt19937 rng;
    std::normal_distribution<double> gaussian;
    int currentFrame;
    double totalTime;

    double V(double x) {
        return 0.5 * x * x;
    }

    double calculateAction(const std::vector<double> &positions) {
        double action = 0.0;
        for (int t = 1; t < positions.size(); t++) {
            double dx = positions[t] - positions[t - 1];
            double kinetic = 0.5 * MASS * dx * dx / (DT * DT);
            double potential = V(positions[t]);
            action += (kinetic - potential) * DT;
        }
        return action;
    }

    std::vector<double> generateRandomPath(double x0, double xf) {
        std::vector<double> path(TIME_STEPS + 1);
        path[0] = x0;
        path[TIME_STEPS] = xf;

        for (int t = 1; t < TIME_STEPS; t++) {
            double alpha = (double)t / TIME_STEPS;
            path[t] = (1 - alpha) * x0 + alpha * xf;
            path[t] += gaussian(rng) * 0.5;
        }

        return path;
    }

public:
    PathIntegralSimulation()
    : rng(42), gaussian(0.0, 1.0), currentFrame(0), totalTime(0.0) {
        generatePaths();
    }

    void generatePaths() {
        paths.clear();
        double x0 = -2.0;
        double xf = 2.0;

        for (int i = 0; i < NUM_PATHS; i++) {
            Path path;
            path.positions = generateRandomPath(x0, xf);
            path.action = calculateAction(path.positions);

            std::complex<double> phase(0, -path.action / HBAR);
            path.amplitude = std::exp(phase);

            paths.push_back(path);
        }

        std::complex<double> sum(0, 0);
        for (const auto &path : paths) {
            sum += path.amplitude;
        }

        for (auto &path : paths) {
            path.amplitude /= sum;
        }
    }

    void update() {
        currentFrame++;
        totalTime += 0.016;

        if (currentFrame % 120 == 0) {
            generatePaths();
        }
    }

    void render() {
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-5, 5, -3, 3, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_LINES);
        glVertex2f(-5, 0);
        glVertex2f(5, 0);
        glVertex2f(0, -3);
        glVertex2f(0, 3);
        glEnd();

        glColor3f(0.1f, 0.1f, 0.1f);
        glBegin(GL_LINES);
        for (int i = -50; i <= 50; i++) {
            if (i % 10 == 0)
                continue;
            glVertex2f(i * 0.1f, -3);
            glVertex2f(i * 0.1f, 3);
        }
        for (int i = -30; i <= 30; i++) {
            if (i % 10 == 0)
                continue;
            glVertex2f(-5, i * 0.1f);
            glVertex2f(5, i * 0.1f);
        }
        glEnd();

        for (size_t i = 0; i < paths.size(); i++) {
            const Path &path = paths[i];

            double magnitude = std::abs(path.amplitude);
            double phase = std::arg(path.amplitude);

            float r = (float)(0.5 + 0.5 * cos(phase));
            float g = (float)(0.5 + 0.5 * cos(phase + 2 * M_PI / 3));
            float b = (float)(0.5 + 0.5 * cos(phase + 4 * M_PI / 3));

            float alpha = (float)(magnitude * 10);
            if (alpha > 1.0f)
                alpha = 1.0f;

            glColor4f(r * alpha, g * alpha, b * alpha, alpha);

            glBegin(GL_LINE_STRIP);
            for (int t = 0; t < path.positions.size(); t++) {
                float x = (float)path.positions[t];
                float y = (float)(-2.5 + 5.0 * t / (path.positions.size() - 1));
                glVertex2f(x, y);
            }
            glEnd();

            glPointSize(2.0f);
            glBegin(GL_POINTS);
            for (int t = 0; t < path.positions.size(); t++) {
                float x = (float)path.positions[t];
                float y = (float)(-2.5 + 5.0 * t / (path.positions.size() - 1));
                glVertex2f(x, y);
            }
            glEnd();
        }

        glPointSize(8.0f);
        glColor3f(1.0f, 0.0f, 0.0f);
        glBegin(GL_POINTS);
        glVertex2f(-2.0f, -2.5f);
        glVertex2f(2.0f, 2.5f);
        glEnd();

        glColor3f(0.5f, 0.5f, 1.0f);
        glBegin(GL_LINE_STRIP);
        for (int i = -50; i <= 50; i++) {
            double x = i * 0.1;
            double potential = V(x);
            float px = (float)x;
            float py = (float)(-2.8 + potential * 0.1);
            glVertex2f(px, py);
        }
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(-4.8f, 2.7f);
        std::string info =
        "1D Quantum Path Integral - Paths: " + std::to_string(NUM_PATHS);
        for (char c : info) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        }

        glRasterPos2f(-4.8f, 2.5f);
        std::string timeInfo = "Time Steps: " + std::to_string(TIME_STEPS);
        for (char c : timeInfo) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
        }

        glRasterPos2f(-4.8f, -2.9f);
        std::string legend =
        "Red: Start/End | Blue: Harmonic Potential | Colors: Path Amplitudes";
        for (char c : legend) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, c);
        }

        glutSwapBuffers();
    }

    void keyPressed(unsigned char key, int x, int y) {
        switch (key) {
            case 'r':
            case 'R':
                generatePaths();
                break;
            case 27:
                exit(0);
                break;
        }
    }
};

PathIntegralSimulation *sim = nullptr;

void display() {
    if (sim)
        sim->render();
}

void update(int value) {
    if (sim)
        sim->update();
    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    if (sim)
        sim->keyPressed(key, x, y);
}

void reshape(int w, int h) { glViewport(0, 0, w, h); }

int main(int argc, char **argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1200, 800);
    glutCreateWindow("1D Quantum Mechanical Path Integral Simulation");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    sim = new PathIntegralSimulation();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, update, 0);

    std::cout << "1D Quantum Path Integral Simulation" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  R - Regenerate paths" << std::endl;
    std::cout << "  ESC - Exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Simulation shows quantum paths between red start/end points."
    << std::endl;
    std::cout << "Colors represent quantum amplitudes (phase and magnitude)."
    << std::endl;
    std::cout << "Blue curve shows harmonic oscillator potential." << std::endl;

    glutMainLoop();

    delete sim;
    return 0;
}
