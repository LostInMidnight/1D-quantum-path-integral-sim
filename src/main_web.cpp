#include <GLES2/gl2.h>
#include <cmath>
#include <complex>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

int LATTICE_SIZE = 100;
int TIME_STEPS = 50;
int NUM_PATHS = 500;
double HBAR = 1.0;
double MASS = 1.0;
double DT = 0.1;
double DX = 0.1;

const char *vertexShaderSource = R"(
attribute vec2 a_position;
attribute vec4 a_color;
varying vec4 v_color;

void main() {
    gl_Position = vec4(a_position, 0.0, 1.0);
    gl_PointSize = 8.0;
    v_color = a_color;
}
)";

const char *fragmentShaderSource = R"(
precision mediump float;
varying vec4 v_color;

void main() {
    gl_FragColor = v_color;
}
)";

struct Path {
    std::vector<double> positions;
    std::complex<double> amplitude;
    double action;
};

class WebGLRenderer {
private:
    GLuint shaderProgram;
    GLuint vertexBuffer;
    GLuint colorBuffer;
    GLint positionAttrib;
    GLint colorAttrib;

    std::vector<float> vertices;
    std::vector<float> colors;

public:
    bool init() {
        GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
        GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

        if (!vertexShader || !fragmentShader) {
            return false;
        }

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);

        GLint success;
        glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
            std::cout << "Shader program linking failed: " << infoLog << std::endl;
            return false;
        }

        positionAttrib = glGetAttribLocation(shaderProgram, "a_position");
        colorAttrib = glGetAttribLocation(shaderProgram, "a_color");

        glGenBuffers(1, &vertexBuffer);
        glGenBuffers(1, &colorBuffer);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return true;
    }

    void beginRender() {
        vertices.clear();
        colors.clear();
        glUseProgram(shaderProgram);
    }

    void addLine(float x1, float y1, float x2, float y2, float r, float g,
                 float b, float a) {
        vertices.push_back(x1);
        vertices.push_back(y1);
        vertices.push_back(x2);
        vertices.push_back(y2);

        colors.push_back(r);
        colors.push_back(g);
        colors.push_back(b);
        colors.push_back(a);
        colors.push_back(r);
        colors.push_back(g);
        colors.push_back(b);
        colors.push_back(a);
                 }

                 void addPoint(float x, float y, float r, float g, float b, float a) {
                     vertices.push_back(x);
                     vertices.push_back(y);
                     colors.push_back(r);
                     colors.push_back(g);
                     colors.push_back(b);
                     colors.push_back(a);
                 }

                 void renderLines() {
                     if (vertices.empty())
                         return;

                     glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
                     glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                                  vertices.data(), GL_DYNAMIC_DRAW);
                     glEnableVertexAttribArray(positionAttrib);
                     glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

                     glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
                     glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(),
                                  GL_DYNAMIC_DRAW);
                     glEnableVertexAttribArray(colorAttrib);
                     glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);

                     glDrawArrays(GL_LINES, 0, vertices.size() / 2);

                     vertices.clear();
                     colors.clear();
                 }

                 void renderPoints() {
                     if (vertices.empty())
                         return;

                     glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
                     glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                                  vertices.data(), GL_DYNAMIC_DRAW);
                     glEnableVertexAttribArray(positionAttrib);
                     glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, 0);

                     glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
                     glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(),
                                  GL_DYNAMIC_DRAW);
                     glEnableVertexAttribArray(colorAttrib);
                     glVertexAttribPointer(colorAttrib, 4, GL_FLOAT, GL_FALSE, 0, 0);

                     glDrawArrays(GL_POINTS, 0, vertices.size() / 2);

                     vertices.clear();
                     colors.clear();
                 }

                 void endRender() {
                     glDisableVertexAttribArray(positionAttrib);
                     glDisableVertexAttribArray(colorAttrib);
                 }

private:
    GLuint compileShader(GLenum type, const char *source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cout << "Shader compilation failed: " << infoLog << std::endl;
            return 0;
        }

        return shader;
    }
};

class PathIntegralSimulation {
private:
    std::vector<Path> paths;
    std::mt19937 rng;
    std::normal_distribution<double> gaussian;
    int currentFrame;
    double totalTime;
    int canvasWidth, canvasHeight;
    WebGLRenderer renderer;

    double V(double x) { return 0.5 * x * x; }

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

    void worldToScreen(double wx, double wy, float &sx, float &sy) {
        sx = (float)((wx + 5.0) / 10.0 * 2.0 - 1.0);
        sy = (float)((wy + 3.0) / 6.0 * 2.0 - 1.0);
    }

public:
    PathIntegralSimulation()
    : rng(42), gaussian(0.0, 1.0), currentFrame(0), totalTime(0.0),
    canvasWidth(800), canvasHeight(600) {
        generatePaths();
    }

    bool init() { return renderer.init(); }

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

        if (std::abs(sum) > 1e-10) {
            for (auto &path : paths) {
                path.amplitude /= sum;
            }
        }
    }

    void update() {
        currentFrame++;
        totalTime += 0.016;

        if (currentFrame % 180 == 0) {
            generatePaths();
        }
    }

    void setCanvasSize(int width, int height) {
        canvasWidth = width;
        canvasHeight = height;
        glViewport(0, 0, width, height);
    }

    void render() {
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        renderer.beginRender();

        float sx1, sy1, sx2, sy2;

        worldToScreen(-5, 0, sx1, sy1);
        worldToScreen(5, 0, sx2, sy2);
        renderer.addLine(sx1, sy1, sx2, sy2, 0.3f, 0.3f, 0.3f, 1.0f);

        worldToScreen(0, -3, sx1, sy1);
        worldToScreen(0, 3, sx2, sy2);
        renderer.addLine(sx1, sy1, sx2, sy2, 0.3f, 0.3f, 0.3f, 1.0f);

        for (int i = -40; i <= 40; i += 10) {
            worldToScreen(i * 0.1, -3, sx1, sy1);
            worldToScreen(i * 0.1, 3, sx2, sy2);
            renderer.addLine(sx1, sy1, sx2, sy2, 0.1f, 0.1f, 0.1f, 0.5f);
        }
        for (int i = -20; i <= 20; i += 10) {
            worldToScreen(-5, i * 0.1, sx1, sy1);
            worldToScreen(5, i * 0.1, sx2, sy2);
            renderer.addLine(sx1, sy1, sx2, sy2, 0.1f, 0.1f, 0.1f, 0.5f);
        }

        renderer.renderLines();

        for (int i = -39; i <= 39; i++) {
            double x1 = i * 0.1;
            double x2 = (i + 1) * 0.1;
            double potential1 = V(x1);
            double potential2 = V(x2);

            double wx1 = x1, wy1 = -2.8 + potential1 * 0.15;
            double wx2 = x2, wy2 = -2.8 + potential2 * 0.15;

            float sx1, sy1, sx2, sy2;
            worldToScreen(wx1, wy1, sx1, sy1);
            worldToScreen(wx2, wy2, sx2, sy2);
            renderer.addLine(sx1, sy1, sx2, sy2, 0.3f, 0.3f, 1.0f, 0.8f);
        }

        renderer.renderLines();

        for (size_t i = 0; i < paths.size(); i++) {
            const Path &path = paths[i];

            double magnitude = std::abs(path.amplitude);
            double phase = std::arg(path.amplitude);

            float r = (float)(0.5 + 0.5 * cos(phase));
            float g = (float)(0.5 + 0.5 * cos(phase + 2 * M_PI / 3));
            float b = (float)(0.5 + 0.5 * cos(phase + 4 * M_PI / 3));

            float alpha = (float)(magnitude * 15);
            if (alpha > 1.0f)
                alpha = 1.0f;
            if (alpha < 0.1f)
                alpha = 0.1f;

            r *= alpha;
            g *= alpha;
            b *= alpha;
            alpha *= 0.8f;

            for (int t = 0; t < (int)path.positions.size() - 1; t++) {
                double wx1 = path.positions[t];
                double wy1 = -2.5 + 5.0 * t / (path.positions.size() - 1);
                double wx2 = path.positions[t + 1];
                double wy2 = -2.5 + 5.0 * (t + 1) / (path.positions.size() - 1);

                float sx1, sy1, sx2, sy2;
                worldToScreen(wx1, wy1, sx1, sy1);
                worldToScreen(wx2, wy2, sx2, sy2);
                renderer.addLine(sx1, sy1, sx2, sy2, r, g, b, alpha);
            }
        }

        renderer.renderLines();

        float startX, startY, endX, endY;
        worldToScreen(-2.0, -2.5, startX, startY);
        worldToScreen(2.0, 2.5, endX, endY);

        renderer.addPoint(startX, startY, 1.0f, 0.2f, 0.2f, 1.0f);
        renderer.addPoint(endX, endY, 1.0f, 0.2f, 0.2f, 1.0f);

        renderer.renderPoints();

        renderer.endRender();
    }

    void keyPressed(int key) {
        switch (key) {
            case 82:
            case 114:
                generatePaths();
                break;
        }
    }

    void mouseClick(double x, double y) {
        generatePaths();
    }

    void setLatticeSize(int size) {
        LATTICE_SIZE = size;
        generatePaths();
    }

    void setTimeSteps(int steps) {
        if (steps > 0) {
            TIME_STEPS = steps;
            generatePaths();
        }
    }

    void setNumPaths(int paths) {
        if (paths > 0 && paths <= 2000) {
            NUM_PATHS = paths;
            generatePaths();
        }
    }

    void setHbar(double hbar) {
        if (hbar > 0) {
            HBAR = hbar;
            generatePaths();
        }
    }

    void setMass(double mass) {
        if (mass > 0) {
            MASS = mass;
            generatePaths();
        }
    }

    void setDt(double dt) {
        if (dt > 0) {
            DT = dt;
            generatePaths();
        }
    }

    void setDx(double dx) {
        if (dx > 0) {
            DX = dx;
            generatePaths();
        }
    }
};

PathIntegralSimulation *sim = nullptr;

#ifdef __EMSCRIPTEN__
void emscripten_main_loop() {
    if (sim) {
        sim->update();
        sim->render();
    }
}

EM_BOOL keydown_callback(int eventType, const EmscriptenKeyboardEvent *keyEvent,
                         void *userData) {
    if (sim) {
        sim->keyPressed(keyEvent->keyCode);
    }
    return EM_TRUE;
                         }

                         EM_BOOL click_callback(int eventType, const EmscriptenMouseEvent *mouseEvent,
                                                void *userData) {
                             if (sim) {
                                 sim->mouseClick(mouseEvent->clientX, mouseEvent->clientY);
                             }
                             return EM_TRUE;
                                                }

                                                EM_BOOL resize_callback(int eventType, const EmscriptenUiEvent *uiEvent,
                                                                        void *userData) {
                                                    int width, height;
                                                    emscripten_get_canvas_element_size("#canvas", &width, &height);
                                                    if (sim) {
                                                        sim->setCanvasSize(width, height);
                                                    }
                                                    return EM_TRUE;
                                                                        }
                                                                        #endif

                                                                        extern "C" {
                                                                            void setLatticeSize(int size) {
                                                                                if (sim)
                                                                                    sim->setLatticeSize(size);
                                                                            }

                                                                            void setTimeSteps(int steps) {
                                                                                if (sim)
                                                                                    sim->setTimeSteps(steps);
                                                                            }

                                                                            void setNumPaths(int paths) {
                                                                                if (sim)
                                                                                    sim->setNumPaths(paths);
                                                                            }

                                                                            void setHbar(double hbar) {
                                                                                if (sim)
                                                                                    sim->setHbar(hbar);
                                                                            }

                                                                            void setMass(double mass) {
                                                                                if (sim)
                                                                                    sim->setMass(mass);
                                                                            }

                                                                            void setDt(double dt) {
                                                                                if (sim)
                                                                                    sim->setDt(dt);
                                                                            }

                                                                            void setDx(double dx) {
                                                                                if (sim)
                                                                                    sim->setDx(dx);
                                                                            }

                                                                            void regeneratePaths() {
                                                                                if (sim)
                                                                                    sim->generatePaths();
                                                                            }
                                                                        }

                                                                        int main(int argc, char **argv) {
                                                                            std::cout << "1D Quantum Path Integral Simulation - WebGL Version"
                                                                            << std::endl;

                                                                            #ifdef __EMSCRIPTEN__
                                                                            EmscriptenWebGLContextAttributes attrs;
                                                                            emscripten_webgl_init_context_attributes(&attrs);
                                                                            attrs.alpha = false;
                                                                            attrs.depth = false;
                                                                            attrs.stencil = false;
                                                                            attrs.antialias = true;
                                                                            attrs.majorVersion = 2;
                                                                            attrs.minorVersion = 0;

                                                                            EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context =
                                                                            emscripten_webgl_create_context("#canvas", &attrs);
                                                                            emscripten_webgl_make_context_current(context);

                                                                            int width, height;
                                                                            emscripten_get_canvas_element_size("#canvas", &width, &height);

                                                                            emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1,
                                                                                                            keydown_callback);
                                                                            emscripten_set_click_callback("#canvas", nullptr, 1, click_callback);
                                                                            emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1,
                                                                                                           resize_callback);

                                                                            glViewport(0, 0, (int)width, (int)height);
                                                                            #endif

                                                                            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                                                                            glEnable(GL_BLEND);
                                                                            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                                                                            sim = new PathIntegralSimulation();

                                                                            if (!sim->init()) {
                                                                                std::cout << "Failed to initialize simulation!" << std::endl;
                                                                                return -1;
                                                                            }

                                                                            #ifdef __EMSCRIPTEN__
                                                                            sim->setCanvasSize(width, height);

                                                                            emscripten_set_main_loop(emscripten_main_loop, 60, 1);
                                                                            #else
                                                                            std::cout << "This version is designed for web deployment with Emscripten."
                                                                            << std::endl;
                                                                            std::cout << "For desktop use, please use the original GLUT version."
                                                                            << std::endl;
                                                                            #endif

                                                                            return 0;
                                                                        }
