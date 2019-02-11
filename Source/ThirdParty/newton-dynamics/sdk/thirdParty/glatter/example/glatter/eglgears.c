/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * This is a port of the infamous "glxgears" demo to straight EGL
 * Based on the port by Dane Rushton, 10 July 2005
 *
 * Further modifications by Ioannis Makris, 10 April 2017
 * 
 * No command line options.
 */

#define EGL_EGLEXT_PROTOTYPES

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glatter/glatter.h>


#define BENCHMARK

#ifdef BENCHMARK

/* return current time (in seconds) */
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN 1
    #include <windows.h>
 
    static double
    current_time(void)
    {
        DWORD tv;
 
        tv = GetTickCount();
        return (tv/1000.0);
    }
 
#else
    #include <sys/time.h>
    #include <unistd.h>
 
    /* return current time (in seconds) */
    static double
    current_time(void)
    {
        struct timeval tv;
    #ifdef __VMS
        (void) gettimeofday(&tv, NULL );
    #else
        struct timezone tz;
        (void) gettimeofday(&tv, &tz);
    #endif
        return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
    }
#endif

#endif /*BENCHMARK*/


#ifndef M_PI
#define M_PI 3.14159265
#endif


int has_quit = 0;


static GLint autoexit = 0;
static GLfloat viewDist = 40.0;

typedef struct {
    GLfloat pos[3];
    GLfloat norm[3];
}
vertex_t;

typedef struct {
    vertex_t *vertices;
    GLshort *indices;
    GLfloat color[4];
    int nvertices, nindices;
    GLuint ibo;
}
gear_t;

static gear_t *red_gear;
static gear_t *green_gear;
static gear_t *blue_gear;


/**
 
  Draw a gear wheel.  You'll probably want to call this function when
  building a display list since we do a lot of trig here.
 
  Input:  inner_radius - radius of hole at center
          outer_radius - radius at center of teeth
          width - width of gear
          teeth - number of teeth
          tooth_depth - depth of tooth
 
 **/
static gear_t* gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth, GLfloat color[])
{
    GLint i, j;
    GLfloat r0, r1, r2;
    GLfloat ta, da;
    GLfloat u1, v1, u2, v2, len;
    GLfloat cos_ta, cos_ta_1da, cos_ta_2da, cos_ta_3da, cos_ta_4da;
    GLfloat sin_ta, sin_ta_1da, sin_ta_2da, sin_ta_3da, sin_ta_4da;
    GLshort ix0, ix1, ix2, ix3, ix4, ix5;
    vertex_t *vt, *nm;
    GLshort *ix;
 
    gear_t *gear = calloc(1, sizeof(gear_t));
    gear->nvertices = teeth * 40;
    gear->nindices = teeth * 66 * 3;
    gear->vertices = calloc(gear->nvertices, sizeof(vertex_t));
    gear->indices = calloc(gear->nindices, sizeof(GLshort));
    memcpy(&gear->color[0], &color[0], sizeof(GLfloat) * 4);
 
    r0 = inner_radius;
    r1 = outer_radius - tooth_depth / 2.0;
    r2 = outer_radius + tooth_depth / 2.0;
    da = 2.0 * M_PI / teeth / 4.0;
 
    vt = gear->vertices;
    nm = gear->vertices;
    ix = gear->indices;
 
    #define VERTEX(x,y,z) ((vt->pos[0] = x),(vt->pos[1] = y),(vt->pos[2] = z), \
                        (vt++ - gear->vertices))
    #define NORMAL(x,y,z) ((nm->norm[0] = x),(nm->norm[1] = y),(nm->norm[2] = z), \
                        (nm++ - gear->vertices))
    #define INDEX(a,b,c) ((*ix++ = a),(*ix++ = b),(*ix++ = c))
 
    for (i = 0; i < teeth; i++) {
        ta = i * 2.0 * M_PI / teeth;
 
        cos_ta = cos(ta);
        cos_ta_1da = cos(ta + da);
        cos_ta_2da = cos(ta + 2 * da);
        cos_ta_3da = cos(ta + 3 * da);
        cos_ta_4da = cos(ta + 4 * da);
        sin_ta = sin(ta);
        sin_ta_1da = sin(ta + da);
        sin_ta_2da = sin(ta + 2 * da);
        sin_ta_3da = sin(ta + 3 * da);
        sin_ta_4da = sin(ta + 4 * da);
 
        u1 = r2 * cos_ta_1da - r1 * cos_ta;
        v1 = r2 * sin_ta_1da - r1 * sin_ta;
        len = sqrt(u1 * u1 + v1 * v1);
        u1 /= len;
        v1 /= len;
        u2 = r1 * cos_ta_3da - r2 * cos_ta_2da;
        v2 = r1 * sin_ta_3da - r2 * sin_ta_2da;
 
        /* front face */
        ix0 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          width * 0.5);
        ix1 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          width * 0.5);
        ix2 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          width * 0.5);
        ix3 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      width * 0.5);
        ix4 = VERTEX(r0 * cos_ta_4da,      r0 * sin_ta_4da,      width * 0.5);
        ix5 = VERTEX(r1 * cos_ta_4da,      r1 * sin_ta_4da,      width * 0.5);
        for (j = 0; j < 6; j++) {
            NORMAL(  0.0,                  0.0,                  1.0);
        }
        INDEX(ix0, ix1, ix2);
        INDEX(ix1, ix3, ix2);
        INDEX(ix2, ix3, ix4);
        INDEX(ix3, ix5, ix4);
 
        /* front sides of teeth */
        ix0 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          width * 0.5);
        ix1 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      width * 0.5);
        ix2 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      width * 0.5);
        ix3 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      width * 0.5);
        for (j = 0; j < 4; j++) {
            NORMAL(  0.0,                  0.0,                  1.0);
        }
        INDEX(ix0, ix1, ix2);
        INDEX(ix1, ix3, ix2);
 
        /* back face */
        ix0 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          -width * 0.5);
        ix1 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          -width * 0.5);
        ix2 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      -width * 0.5);
        ix3 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          -width * 0.5);
        ix4 = VERTEX(r1 * cos_ta_4da,      r1 * sin_ta_4da,      -width * 0.5);
        ix5 = VERTEX(r0 * cos_ta_4da,      r0 * sin_ta_4da,      -width * 0.5);
        for (j = 0; j < 6; j++) {
            NORMAL(  0.0,                  0.0,                  -1.0);
        }
        INDEX(ix0, ix1, ix2);
        INDEX(ix1, ix3, ix2);
        INDEX(ix2, ix3, ix4);
        INDEX(ix3, ix5, ix4);
 
        /* back sides of teeth */
        ix0 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      -width * 0.5);
        ix1 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      -width * 0.5);
        ix2 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          -width * 0.5);
        ix3 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      -width * 0.5);
        for (j = 0; j < 4; j++) {
            NORMAL(  0.0,                  0.0,                  -1.0);
        }
        INDEX(ix0, ix1, ix2);
        INDEX(ix1, ix3, ix2);
 
        /* draw outward faces of teeth */
        ix0 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          width * 0.5);
        ix1 = VERTEX(r1 * cos_ta,          r1 * sin_ta,          -width * 0.5);
        ix2 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      width * 0.5);
        ix3 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      -width * 0.5);
        for (j = 0; j < 4; j++) {
            NORMAL(  v1,                   -u1,                  0.0);
        }
        INDEX(ix0, ix1, ix2);
        INDEX(ix1, ix3, ix2);
        ix0 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      width * 0.5);
        ix1 = VERTEX(r2 * cos_ta_1da,      r2 * sin_ta_1da,      -width * 0.5);
        ix2 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      width * 0.5);
        ix3 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      -width * 0.5);
        for (j = 0; j < 4; j++) {
            NORMAL(  cos_ta,               sin_ta,               0.0);
        }
        INDEX(ix0, ix1, ix2);
        INDEX(ix1, ix3, ix2);
        ix0 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      width * 0.5);
        ix1 = VERTEX(r2 * cos_ta_2da,      r2 * sin_ta_2da,      -width * 0.5);
        ix2 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      width * 0.5);
        ix3 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      -width * 0.5);
        for (j = 0; j < 4; j++) {
            NORMAL(  v2,                   -u2,                  0.0);
        }
        INDEX(ix0, ix1, ix2);
        INDEX(ix1, ix3, ix2);
        ix0 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      width * 0.5);
        ix1 = VERTEX(r1 * cos_ta_3da,      r1 * sin_ta_3da,      -width * 0.5);
        ix2 = VERTEX(r1 * cos_ta_4da,      r1 * sin_ta_4da,      width * 0.5);
        ix3 = VERTEX(r1 * cos_ta_4da,      r1 * sin_ta_4da,      -width * 0.5);
        for (j = 0; j < 4; j++) {
            NORMAL(  cos_ta,               sin_ta,               0.0);
        }
        INDEX(ix0, ix1, ix2);
        INDEX(ix1, ix3, ix2);
 
        /* draw inside radius cylinder */
        ix0 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          -width * 0.5);
        ix1 = VERTEX(r0 * cos_ta,          r0 * sin_ta,          width * 0.5);
        ix2 = VERTEX(r0 * cos_ta_4da,      r0 * sin_ta_4da,      -width * 0.5);
        ix3 = VERTEX(r0 * cos_ta_4da,      r0 * sin_ta_4da,      width * 0.5);
        NORMAL(      -cos_ta,              -sin_ta,              0.0);
        NORMAL(      -cos_ta,              -sin_ta,              0.0);
        NORMAL(      -cos_ta_4da,          -sin_ta_4da,          0.0);
        NORMAL(      -cos_ta_4da,          -sin_ta_4da,          0.0);
        INDEX(       ix0,                  ix1,                  ix2);
        INDEX(       ix1,                  ix3,                  ix2);
    }
    return gear;
}

 
 
void draw_gear(gear_t* gear)
{ 
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, gear->color);
    glVertexPointer(3, GL_FLOAT, sizeof(vertex_t), gear->vertices[0].pos);
    glNormalPointer(GL_FLOAT, sizeof(vertex_t), gear->vertices[0].norm);
    glDrawElements(GL_TRIANGLES, gear->nindices/3, GL_UNSIGNED_SHORT,
                    gear->indices);
}
 
static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static gear_t *gear1, *gear2, *gear3;
static GLfloat angle = 0.0;

static void draw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 
    glPushMatrix();
 
    glTranslatef(0.0, 0.0, -viewDist);

    glRotatef(view_rotx, 1.0, 0.0, 0.0);
    glRotatef(view_roty, 0.0, 1.0, 0.0);
    glRotatef(view_rotz, 0.0, 0.0, 1.0);
 
    glPushMatrix();
    glTranslatef(-3.0, -2.0, 0.0);
    glRotatef(angle, 0.0, 0.0, 1.0);
    draw_gear(gear1);
    glPopMatrix();
 
    glPushMatrix();
    glTranslatef(3.1, -2.0, 0.0);
    glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
    draw_gear(gear2);
    glPopMatrix();
 
    glPushMatrix();
    glTranslatef(-3.1, 4.2, 0.0);
    glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
    draw_gear(gear3);
    glPopMatrix();
 
    glPopMatrix();
    glFinish();
}



/* new window size or exposure */
static void reshape(int width, int height)
{
    GLfloat h = (GLfloat) height / (GLfloat) width;
 
    glViewport(0, 0, (GLint) width, (GLint) height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustumf(-1.0, 1.0, -h, h, 5.0, 200.0);
    glMatrixMode(GL_MODELVIEW);
}



static void init(int argc, char *argv[])
{
    static GLfloat pos[4] = {5.0, 5.0, 10.0, 0.0};
    static GLfloat red[4] = {0.8, 0.1, 0.0, 1.0};
    static GLfloat green[4] = {0.0, 0.8, 0.2, 1.0};
    static GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};
    GLint i;
 
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
 
    glShadeModel(GL_SMOOTH);
 
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
 
    for ( i=1; i<argc; i++ ) {
        if (strcmp(argv[i], "-info")==0) {
            printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
            printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
            printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
            printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
        }
        else if ( strcmp(argv[i], "-exit")==0) {
            autoexit = 30;
            printf("Auto Exit after %i seconds.\n", autoexit );
        }
    }
 
    /* make the gears */
    gear1 = gear(1.0, 4.0, 1.0, 20, 0.7, red);
    gear2 = gear(0.5, 2.0, 2.0, 10, 0.7, green);
    gear3 = gear(1.3, 2.0, 0.5, 10, 0.7, blue);
}


#ifdef _WIN32

#include <windows.h>

LRESULT CALLBACK wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lwparam)
{
    switch (message) {
        case WM_SYSCOMMAND:
            switch (wparam)
                case SC_SCREENSAVE:
                case SC_MONITORPOWER:
                    return 0;
            break;
        case WM_CLOSE:
            has_quit = 1;
            PostQuitMessage(0);
            return 1;
    }
    return DefWindowProc(window, message, wparam, lwparam);
}


typedef HWND NativeWindowType;
HWND createNativeWindow(int w, int h)
{
    WNDCLASS wd;
    wd.style = CS_HREDRAW | CS_VREDRAW;
    wd.lpfnWndProc = wndproc;
    wd.cbClsExtra = 0;
    wd.cbWndExtra = 0;
    wd.hInstance = GetModuleHandle(NULL);
    wd.hIcon = 0;
    wd.hCursor = 0;
    wd.lpszMenuName = 0;
    wd.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wd.lpszClassName = "eglgears";

    ATOM rclass = RegisterClass(&wd);
    if (!rclass)
        return 0;

    RECT rect;
    SetRect(&rect, 0, 0, w, h);
    AdjustWindowRectEx(&rect, WS_CAPTION | WS_SYSMENU, 0, 0);

    return CreateWindow(
        "eglgears", "eglgears", WS_VISIBLE | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, GetModuleHandle(NULL), NULL);
}


void handle_system_messages()
{
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            has_quit = 1;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

#else

#error There is no implementation for this system. Your contribution would be very much appreciated!

#endif



static void run_gears(EGLDisplay dpy, EGLSurface surf)
{
    double ref_time = current_time();
    double ct = ref_time;
    int frames = 0;

    while (!has_quit && ct-ref_time < 5.) {
        handle_system_messages();

        double tt = current_time();
        double dt = tt - ct;
        ct = tt;

        /* advance rotation for next frame */
        angle += 70.0 * dt;  /* 70 degrees per second */
        if (angle > 3600.0)
            angle -= 3600.0;

        draw();
        eglSwapBuffers(dpy, surf);
        frames++;
    }

    GLfloat seconds = ct - ref_time;
    GLfloat fps = frames / seconds;
    printf("%d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds, fps);
}



int main(int argc, char *argv[])
{
    unsigned int width = 300, height = 300;

    EGLDisplay display;
    EGLConfig config;
    EGLContext context;
    EGLSurface surface;
    NativeWindowType native_window;
    EGLint num_config;
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, NULL, NULL);
    const EGLint attribute_list[] =
    {
        EGL_SURFACE_TYPE,        EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,    EGL_OPENGL_ES_BIT,
        EGL_DEPTH_SIZE,         8,
        EGL_NONE
    };
    eglChooseConfig(display, attribute_list, &config, 1, &num_config);
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, NULL);
    native_window = createNativeWindow(width, height);
    surface = eglCreateWindowSurface(display, config, native_window, NULL);
    eglMakeCurrent(display, surface, surface, context);

    init(argc, argv);
    reshape(width, height);

    run_gears(display, surface);

    eglDestroySurface(display, surface);
    eglDestroyContext(display, context);
    eglTerminate(display);

    return 0;
}

