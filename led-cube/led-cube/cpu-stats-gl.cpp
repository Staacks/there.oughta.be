#include "led-matrix.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <math.h>

#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT 1234
#define ANIMSTEP 0.5

const int w = 192;
const int h = 64;

using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

using namespace rgb_matrix;

const int cores = 12;
float temperature = 30.f;
float cpu[cores] = {0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.};
float ttemperature = 30.f;
float tcpu[cores] = {0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.,0.};

float t = 0.f;
float updateTime = -10.0f;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

static const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 8,

    EGL_SAMPLE_BUFFERS, 1,
    EGL_SAMPLES, 4,

    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};

// Width and height of the desired framebuffer
static const EGLint pbufferAttribs[] = {
    EGL_WIDTH,
    w,
    EGL_HEIGHT,
    h,
    EGL_NONE,
};

static const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                        EGL_NONE};


static const GLfloat vertices[] = {
    -1.0f,
    -1.0f,
    0.0f,
    -1.0f,
    1.0f,
    0.0f,

    -0.33333333333f,
    -1.0f,
    0.0f,
    -0.33333333333f,
    1.0f,
    0.0f,

    -0.33333333333f,
    -1.0f,
    0.0f,
    -0.33333333333f,
    1.0f,
    0.0f,

    0.33333333333f,
    -1.0f,
    0.0f,
    0.33333333333f,
    1.0f,
    0.0f,

    0.33333333333f,
    -1.0f,
    0.0f,
    0.33333333333f,
    1.0f,
    0.0f,

    1.0f,
    -1.0f,
    0.0f,
    1.0f,
    1.0f,
    0.0f,
};

static const GLfloat vcoords[] = {
    0.0f,
    0.0f,
    -0.866f,
    0.5f,

    0.0,
    -1.0f,
    -0.866,
    -0.5f,

    0.0f,
    0.0f,
    0.0f,
    -1.0f,

    0.866f,
    0.5f,
    0.866f,
    -0.5f,

    0.0f,
    0.0f,
    0.866f,
    0.5f,

    -0.866f,
    0.5f,
    0.0f,
    1.0f,
};

#define STRINGIFY(x) #x
static const char *vertexShaderCode = STRINGIFY(
    attribute vec3 pos;
    attribute vec2 coord;
    varying vec2 fragCoord;
    void main() {
        fragCoord = coord;
        gl_Position = vec4(pos, 1.0);
    }
);

static const char *fragmentShaderCode = STRINGIFY(
    uniform float temperature;
    uniform float cpu[12];
    vec3 mcolor = vec3(0.1, 0.6, 0.4);
    vec3 mcolorwarm = vec3(0.5, 0.5, 0.1);
    vec3 mcolorhot = vec3(0.6, 0.2, 0.1);
    vec3 ccolor = vec3(0.7, 1.0, 0.9);
    vec3 ccolorwarm = vec3(1.0, 1.0, 0.6);
    vec3 ccolorhot = vec3(1.0, 1.0, 1.0);
    uniform float age;
    uniform float time;
    varying vec2 fragCoord;

    float phi;
    float cpuf = 0.0;

    float circle(vec2 uv, float w0, float width) {
        float f = length(uv) + (sin(normalize(uv).y * 5.0 + time * 2.0) - sin(normalize(uv).x * 5.0 + time * 2.0)) / 100.0;
        float w = width + width*cpuf*0.1;
        return smoothstep(w0-w, w0, f) - smoothstep(w0, w0+w, f);
    }

    void main() {
        vec2 coords = fragCoord.xy*0.5;
        float phi = (atan(coords.y, coords.x)+3.1415926538)/3.1415926538*6.0;

        cpuf += clamp(1.0-abs(phi-0.0), 0.0, 1.0)*cpu[0];
        cpuf += clamp(1.0-abs(phi-1.0), 0.0, 1.0)*cpu[1];
        cpuf += clamp(1.0-abs(phi-2.0), 0.0, 1.0)*cpu[2];
        cpuf += clamp(1.0-abs(phi-3.0), 0.0, 1.0)*cpu[3];
        cpuf += clamp(1.0-abs(phi-4.0), 0.0, 1.0)*cpu[4];
        cpuf += clamp(1.0-abs(phi-5.0), 0.0, 1.0)*cpu[5];
        cpuf += clamp(1.0-abs(phi-6.0), 0.0, 1.0)*cpu[6];
        cpuf += clamp(1.0-abs(phi-7.0), 0.0, 1.0)*cpu[7];
        cpuf += clamp(1.0-abs(phi-8.0), 0.0, 1.0)*cpu[8];
        cpuf += clamp(1.0-abs(phi-9.0), 0.0, 1.0)*cpu[9];
        cpuf += clamp(1.0-abs(phi-10.0), 0.0, 1.0)*cpu[10];
        cpuf += clamp(1.0-abs(phi-11.0), 0.0, 1.0)*cpu[11];
        cpuf += clamp(1.0-abs(phi-12.0), 0.0, 1.0)*cpu[0];

        vec2 p = fragCoord.xy * 0.5 * 10.0 - vec2(19.0);
        vec2 i = p;
        float c = 1.0;
        float inten = 0.05;
        
        for (int n = 0; n < 8; n++) {
        	float t = time * (0.7 - (0.2 / float(n+1)));
            i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
            c += 1.0 / length(vec2(p.x / (2.0 * sin(i.x + t) / inten), p.y / (cos(i.y + t) / inten)));
        }
        
        c /= 8.0;
        c = 1.5 - sqrt(pow(c, 2.0));
        mcolor.g = clamp(coords.x, 0.0, 1.0);
        mcolor = smoothstep(60.0, 40.0, temperature)*mcolor + smoothstep(40.0, 60.0, temperature)*smoothstep(80.0, 60.0, temperature)*mcolorwarm + smoothstep(60.0, 80.0, temperature)*mcolorhot;
         
        ccolor = smoothstep(50.0, 0.0, cpuf)*ccolor + smoothstep(0.0, 50.0, cpuf)*smoothstep(100.0, 50.0, cpuf)*ccolorwarm + smoothstep(50.0, 100.0, cpuf)*ccolorhot;
        ccolor *= circle(coords, 0.25, 0.01);

        vec3 outcolor = mcolor * c * c * c * c + ccolor;
    	vec3 grayXfer = vec3(0.3, 0.59, 0.11);
    	vec3 gray = vec3(dot(grayXfer, outcolor));

        gl_FragColor = vec4(mix(outcolor, gray, smoothstep(5.0, 10.0, age)), 1.0);
    }
);

static const char *eglGetErrorStr() {
    switch (eglGetError()) {
    case EGL_SUCCESS:
        return "The last function succeeded without error.";
    case EGL_NOT_INITIALIZED:
        return "EGL is not initialized, or could not be initialized, for the "
               "specified EGL display connection.";
    case EGL_BAD_ACCESS:
        return "EGL cannot access a requested resource (for example a context "
               "is bound in another thread).";
    case EGL_BAD_ALLOC:
        return "EGL failed to allocate resources for the requested operation.";
    case EGL_BAD_ATTRIBUTE:
        return "An unrecognized attribute or attribute value was passed in the "
               "attribute list.";
    case EGL_BAD_CONTEXT:
        return "An EGLContext argument does not name a valid EGL rendering "
               "context.";
    case EGL_BAD_CONFIG:
        return "An EGLConfig argument does not name a valid EGL frame buffer "
               "configuration.";
    case EGL_BAD_CURRENT_SURFACE:
        return "The current surface of the calling thread is a window, pixel "
               "buffer or pixmap that is no longer valid.";
    case EGL_BAD_DISPLAY:
        return "An EGLDisplay argument does not name a valid EGL display "
               "connection.";
    case EGL_BAD_SURFACE:
        return "An EGLSurface argument does not name a valid surface (window, "
               "pixel buffer or pixmap) configured for GL rendering.";
    case EGL_BAD_MATCH:
        return "Arguments are inconsistent (for example, a valid context "
               "requires buffers not supplied by a valid surface).";
    case EGL_BAD_PARAMETER:
        return "One or more argument values are invalid.";
    case EGL_BAD_NATIVE_PIXMAP:
        return "A NativePixmapType argument does not refer to a valid native "
               "pixmap.";
    case EGL_BAD_NATIVE_WINDOW:
        return "A NativeWindowType argument does not refer to a valid native "
               "window.";
    case EGL_CONTEXT_LOST:
        return "A power management event has occurred. The application must "
               "destroy all contexts and reinitialise OpenGL ES state and "
               "objects to continue rendering.";
    default:
        break;
    }
    return "Unknown error!";
}

#define BUFLEN 512
void receiveUDP() {
    struct sockaddr_in si_me, si_other;
    int s, i, slen=sizeof(si_other);
    char buf[BUFLEN+1];

    memset((char *) &si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    while (!interrupt_received) {
        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
            usleep(500000);
            printf("Error creating socket.\n");
            continue;
        }

        if (bind(s, (struct sockaddr *) &si_me, sizeof(si_me))==-1) {
            usleep(500000);
            printf("Error binding socket.\n");
            continue;
        }

        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            usleep(500000);
            printf("Error setting socket timeout.\n");
            continue;
        }        

        while (!interrupt_received) {
            int nIn = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, (socklen_t *) &slen);
            if (nIn <= 0) {
                usleep(0);
                continue;
            }

            updateTime = t;

            buf[nIn] = 0;
            int i = 0;
            int j = 0;
            while (i < nIn) {
                float entry = atof(buf+i);
                switch (j) {
                    case 0: ttemperature = entry;
                            break;
                    default:
                            int k = j-1;
                            if (k >= cores)
                                break;
                            tcpu[k] = entry;
                            break;
                }
                j++;
                while (buf[i] != ',' && i < nIn)
                    i++;
                i++;
            }
        }

        close(s);        
    }
}

int main(int argc, char *argv[]) {
    EGLDisplay display;
    int major, minor;
    int desiredWidth, desiredHeight;
    GLuint program, vert, frag, vbo, vbocoord;
    GLint posLoc, coordLoc, temperatureLoc, cpuLoc, timeLoc, ageLoc, result;

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        fprintf(stderr, "Failed to get EGL display! Error: %s\n",
                eglGetErrorStr());
        return EXIT_FAILURE;
    }

    if (eglInitialize(display, &major, &minor) == EGL_FALSE) {
        fprintf(stderr, "Failed to get EGL version! Error: %s\n",
                eglGetErrorStr());
        eglTerminate(display);
        return EXIT_FAILURE;
    }

    printf("Initialized EGL version: %d.%d\n", major, minor);

    EGLint numConfigs;
    EGLConfig config;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs)) {
        fprintf(stderr, "Failed to get EGL config! Error: %s\n",
                eglGetErrorStr());
        eglTerminate(display);
        return EXIT_FAILURE;
    }

    EGLSurface surface =
        eglCreatePbufferSurface(display, config, pbufferAttribs);
    if (surface == EGL_NO_SURFACE) {
        fprintf(stderr, "Failed to create EGL surface! Error: %s\n",
                eglGetErrorStr());
        eglTerminate(display);
        return EXIT_FAILURE;
    }

    eglBindAPI(EGL_OPENGL_API);

    EGLContext context =
        eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "Failed to create EGL context! Error: %s\n",
                eglGetErrorStr());
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return EXIT_FAILURE;
    }

    eglMakeCurrent(display, surface, surface, context);

    desiredWidth = pbufferAttribs[1];
    desiredHeight = pbufferAttribs[3];

    glViewport(0, 0, desiredWidth, desiredHeight);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    printf("GL Viewport size: %dx%d\n", viewport[2], viewport[3]);

    if (desiredWidth != viewport[2] || desiredHeight != viewport[3]) {
        fprintf(stderr, "Error! The glViewport/glGetIntegerv are not working! "
                        "EGL might be faulty!\n");
    }

    // Clear whole screen (front buffer)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Shader program
    program = glCreateProgram();
    glUseProgram(program);
    vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertexShaderCode, NULL);
    glCompileShader(vert);
    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragmentShaderCode, NULL);
    glCompileShader(frag);
    glAttachShader(program, frag);
    glAttachShader(program, vert);
    glLinkProgram(program);
    glUseProgram(program);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 36 * sizeof(float), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &vbocoord);
    glBindBuffer(GL_ARRAY_BUFFER, vbocoord);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), vcoords, GL_STATIC_DRAW);


    // Get vertex attribute and uniform locations
    posLoc = glGetAttribLocation(program, "pos");
    coordLoc = glGetAttribLocation(program, "coord");
    temperatureLoc = glGetUniformLocation(program, "temperature");
    cpuLoc = glGetUniformLocation(program, "cpu");
    timeLoc = glGetUniformLocation(program, "time");
    ageLoc = glGetUniformLocation(program, "age");

    // Set our vertex data
    glEnableVertexAttribArray(posLoc);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                          (void *)0);

    glEnableVertexAttribArray(coordLoc);
    glBindBuffer(GL_ARRAY_BUFFER, vbocoord);
    glVertexAttribPointer(coordLoc, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                          (void *)0);

    //LED Matrix settings
    RGBMatrix::Options defaults;
    rgb_matrix::RuntimeOptions runtime;
    defaults.hardware_mapping = "adafruit-hat-pwm";
    defaults.led_rgb_sequence = "RGB";
    defaults.pwm_bits = 11;
    defaults.pwm_lsb_nanoseconds = 50;
    defaults.panel_type = "FM6126A";
    defaults.rows = 64;
    defaults.cols = 192;
    defaults.chain_length = 1;
    defaults.parallel = 1;

    runtime.drop_privileges = 0;
    runtime.gpio_slowdown = 0;
    RGBMatrix *matrix = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &defaults, &runtime);
    if (matrix == NULL)
        return 1;
    FrameCanvas *canvas = matrix->CreateFrameCanvas();

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    unsigned char *buffer = (unsigned char *)malloc(w * h * 3);

    std::thread networking(receiveUDP);

    while (!interrupt_received) {

        t += 0.01f;
        glUniform1f(timeLoc, t);
        glUniform1f(ageLoc, float(t - updateTime));

        if (ttemperature > temperature)
            temperature += 0.2*ANIMSTEP;
        if (ttemperature < temperature)
            temperature -= 0.2*ANIMSTEP;
        for (int i = 0; i < cores; i++) {
            if (tcpu[i] > cpu[i])
                cpu[i] += ANIMSTEP;
            if (tcpu[i] < cpu[i])
                cpu[i] -= ANIMSTEP;
        }

        glUniform1f(temperatureLoc, temperature);
        glUniform1fv(cpuLoc, cores, cpu);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 12);
        glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buffer);

        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                int index = 3*(x+y*w);
                canvas->SetPixel(x, y, buffer[index], buffer[index+1], buffer[index+2]);
            }
        }

        canvas = matrix->SwapOnVSync(canvas);
    }

    networking.join();

    free(buffer);
    canvas->Clear();

    // Cleanup
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    return EXIT_SUCCESS;
}
