/*

Copyright (c) 2010 Outerra

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <iostream>
#include <iomanip>
#include <assert.h>
#include <sys/stat.h>

#include "glew.h"
#include "wglew.h"

#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#pragma warning(disable:4201)
#include <mmsystem.h>

//#define DBL_MAX (999.0)

#ifdef _DEBUG
#define glcheck() checkoglforerror()
#else
#define glcheck()
#endif

void checkoglforerror()
{
    GLuint err = glGetError();

    switch(err) {
    case GL_INVALID_ENUM:
        assert(false); break;
    case GL_INVALID_VALUE:
        assert(false);break;
    case GL_INVALID_OPERATION:
        assert(false); break;
    case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
        assert(false); break;
    case GL_NO_ERROR:
        break;
    case GL_OUT_OF_MEMORY:
        assert(false);
        break;
    default:
        assert(err == GL_NO_ERROR);
    }
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

struct vtx {
    unsigned short pos[4];
};

int _wnd_width=800;
int _wnd_height=600;

int _nprimitives=0;

GLuint _fb=0;
GLuint _textree=0;
GLuint _target=0;

struct pixel_format {
    GLenum _internal_format;
    GLenum _format;
    GLenum _type;
    int _size;
    const char *_desc;
};

pixel_format _fmts[]={
    { GL_RGBA32F,GL_RGBA,GL_FLOAT,16,"GL_RGBA32F" },
    { GL_RG32F,GL_RG,GL_FLOAT,8,"GL_RG32F" },
    { GL_R32F,GL_RED,GL_FLOAT,4,"GL_R32F" },

    { GL_RGBA8,GL_BGRA,GL_UNSIGNED_BYTE,4,"GL_RGBA8" },
    { GL_RG8,GL_RG,GL_UNSIGNED_BYTE,2,"GL_RG8" },
    { GL_R8,GL_RED,GL_UNSIGNED_BYTE,1,"GL_R8" },

    { GL_RGBA16F,GL_RGBA,GL_HALF_FLOAT,8,"GL_RGBA16F" },
    { GL_RG16F,GL_RG,GL_HALF_FLOAT,4,"GL_RG16F" },
    { GL_R16F,GL_RED,GL_HALF_FLOAT,2,"GL_R16F" },
};

HWND _hwnd=0;
HGLRC _hrc=0;
HDC _hdc=0;

GLuint _vs_shad=0;
GLuint _gs_shad=0;
GLuint _fs_shad=0;
GLuint _prg=0;

GLuint _vtx_vbo;
GLuint _unibuf;

GLuint cgp_p1=0, cgp_p2=0, cgp_m1=0, cgp_m2=0;

//DWORD __stdcall _threadproc(void*);

////////////////////////////////////////////////////////////////////////////////

LRESULT WINAPI MsgProc( HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam )
{
    switch( msg ) {
    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

    case WM_PAINT:
        ValidateRect( _hwnd,0 );
        break;
    }

    return DefWindowProc( hwnd,msg,wParam,lParam );
}

////////////////////////////////////////////////////////////////////////////////

const char WND_CLASS_NAME[]="amd_test_class_wnd";

void create_window()
{
    WNDCLASSEX wc={ 
        sizeof(WNDCLASSEX),
        CS_CLASSDC,
        MsgProc,
        0L,
        0L,
        GetModuleHandle(0),
        0,0,0,0,
        WND_CLASS_NAME,
        0 
    };

    if( RegisterClassEx( &wc )==0 )
        throw std::exception( "RegisterClassEx failed!" );

    _hwnd=CreateWindow(
        WND_CLASS_NAME,
        "amd_test",
        WS_OVERLAPPED|WS_CAPTION,
        0,0,
        _wnd_width,_wnd_height,
        GetDesktopWindow(),
        0,
        wc.hInstance,
        0);
    if( _hwnd==0 )
        throw std::exception( "CreateWindow failed!" );

    ShowWindow( _hwnd,SW_SHOWDEFAULT );
    UpdateWindow( _hwnd );
}

////////////////////////////////////////////////////////////////////////////////
GLuint load_texture(const char* fname, int w, int h)
{
    GLuint handle;
    glGenTextures(1,&handle);


    glBindTexture(GL_TEXTURE_2D, handle);


    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.f);


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);



    FILE* f = fopen(fname, "rb");
    void* pdata = ::malloc(w*h*4);
    fread(pdata, 4, w*h, f);
    fclose(f);

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pdata);
    glcheck();

    ::free(pdata);

    glBindTexture(GL_TEXTURE_2D,0);


    return handle;
}

////////////////////////////////////////////////////////////////////////////////
GLuint create_texture(int w, int h)
{
    GLuint handle;
    glGenTextures(1,&handle);
    glBindTexture(GL_TEXTURE_2D, handle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, w, h, 0, GL_RED_INTEGER, GL_INT, 0);

    glBindTexture(GL_TEXTURE_2D,0);
    return handle;
}

////////////////////////////////////////////////////////////////////////////////
GLuint create_and_compile_shaders(const GLchar *src,int len,GLuint sh)
{
    GLuint shad = glCreateShader(sh);

    glShaderSource(shad, 1, &src, &len);
    glCompileShader(shad);

    int cs;
    glGetShaderiv(shad, GL_COMPILE_STATUS, &cs);


    if(!cs) {
        const char* glver = (const char*)glGetString(GL_VERSION);

        GLchar buf[512];
        GLsizei len=0;
        glGetShaderInfoLog(shad, sizeof(buf)-1, &len, buf);


        buf[len]=0;
        printf("error compiling the shader: \n%s", buf);

        glDeleteShader(shad);

        shad = 0;
    }
    return shad;
}


////////////////////////////////////////////////////////////////////////////////

const char* read_data(const char* fname, unsigned int* size) {
    FILE* f;
    struct _stat st;
    if(0 != ::_stat(fname, &st)) return 0;

    *size = (unsigned int)st.st_size;

    char* fsrc = (char*)::malloc(st.st_size);
    f = fopen(fname, "rb");
    fread(fsrc, 1, st.st_size, f);
    fclose(f);

    return fsrc;
}


bool load_shaders()
{
    unsigned int fsize, gsize, vsize;

    const char* fsrc = read_data("fs.glsl", &fsize);
    //const char* gsrc = read_data("gs.glsl", &gsize);
    const char* vsrc = read_data("vs.glsl", &vsize);

    if(!fsrc || !vsrc)// || !gsrc)
        return false;


    _vs_shad = create_and_compile_shaders(vsrc,vsize,GL_VERTEX_SHADER);
    //_gs_shad = create_and_compile_shaders(gsrc,gsize,GL_GEOMETRY_SHADER);
    _fs_shad = create_and_compile_shaders(fsrc,fsize,GL_FRAGMENT_SHADER);

    if(!_vs_shad || /*!_gs_shad ||*/ !_fs_shad)
        return false;

    _prg = glCreateProgram();


    glAttachShader(_prg, _vs_shad); 

    //glAttachShader(_prg, _gs_shad); 
    //glcheck();
    glAttachShader(_prg, _fs_shad);


    glLinkProgram(_prg);


    GLchar buf[4096];
    GLsizei len=0;
    GLint res;

    glGetProgramiv(_prg, GL_LINK_STATUS, &res);

    if(!res) {
        glGetProgramInfoLog(_prg, sizeof(buf)-1, &len, buf);
        buf[len]=0;

        printf("error linking the program: \n%s", buf);

        int size=0;
        glGetShaderiv(_vs_shad, GL_SHADER_SOURCE_LENGTH, &size);

        //glGetShaderSource(_vs_shad, size, &size, tmpsrc.reserve(size));
        //glcheck();

        glDeleteProgram(_prg);


        return false;
    }

    cgp_p1 = glGetUniformLocation(_prg, "p1");
    cgp_p2 = glGetUniformLocation(_prg, "p2");
    cgp_m1 = glGetUniformLocation(_prg, "m1");
    cgp_m2 = glGetUniformLocation(_prg, "m2");

    return true;
}

////////////////////////////////////////////////////////////////////////////////
void WINAPI gl_debug_msg_proc_arb(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,GLvoid *userParam)
{
    if(severity == GL_DEBUG_SEVERITY_HIGH_ARB)
        throw std::exception(message);
}

////////////////////////////////////////////////////////////////////////////////
bool init_gl()
{
    _hdc=GetWindowDC( _hwnd );
    if( _hdc==0 )
        throw std::exception("GetWindowDC failed!");

    int pf;
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
        1,								// version number
        PFD_DRAW_TO_WINDOW |			// support window
        PFD_SUPPORT_OPENGL |			// support OpenGL
        PFD_DOUBLEBUFFER |				// double buffered
        PFD_GENERIC_ACCELERATED |
        PFD_SWAP_COPY,					// dont copy just exchange
        PFD_TYPE_RGBA,					// RGBA type
        24,								// 24-bit color depth
        0, 0, 0, 0, 0, 0,				// color bits ignored
        0,								// no alpha buffer
        0,								// shift bit ignored
        0,								// no accumulation buffer
        0, 0, 0, 0,						// accum bits ignored
        0,								// 32-bit z-buffer
        0,								// 8-bit stencil buffer
        0,								// no auxiliary buffer
        PFD_MAIN_PLANE,					// main layer
        0,								// reserved
        0, 0, 0							// layer masks ignored
    };


    if((pf=ChoosePixelFormat(_hdc, &pfd)) == 0) 
        throw std::exception("ChoosePixelFormat failed!");

    if(SetPixelFormat(_hdc, pf, &pfd) == 0) 
        throw std::exception("SetPixelFormat failed!");

    if((_hrc=wglCreateContext(_hdc)) == 0) 
        throw std::exception("wglCreateContext failed!");

    if(wglMakeCurrent(_hdc, _hrc) == 0)
        throw std::exception("wglMakeCurrent failed!");

    glewExperimental = GL_TRUE;

    const GLenum err = glewInit();
    if( GLEW_OK != err )
        throw std::exception("glewInit failed!");

    if( wglewIsSupported("WGL_ARB_create_context")!=1 )
        throw std::exception("WGL_ARB_create_context is not supported! (is it OpenGL 3.2 capable card?)");

    wglMakeCurrent(0,0);
    wglDeleteContext((HGLRC)_hrc);


    int attribs[]={
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
        0
    };

    if(0 == (_hrc = wglCreateContextAttribsARB(_hdc, 0, attribs)))
        throw std::exception("wglCreateContext failed!");


    if( wglMakeCurrent(_hdc, _hrc)==0 )
        throw std::exception("wglMakeCurrent failed!");


    /*PFNGLDEBUGMESSAGECONTROLARBPROC*/ glDebugMessageControlARB=(PFNGLDEBUGMESSAGECONTROLARBPROC)wglGetProcAddress("glDebugMessageControlARB");
    /*PFNGLDEBUGMESSAGECALLBACKARBPROC*/ glDebugMessageCallbackARB=(PFNGLDEBUGMESSAGECALLBACKARBPROC)wglGetProcAddress("glDebugMessageCallbackARB");
    /*PFNGLDEBUGMESSAGEINSERTARBPROC*/ glDebugMessageInsertARB=(PFNGLDEBUGMESSAGEINSERTARBPROC)wglGetProcAddress("glDebugMessageInsertARB");

    if( glDebugMessageControlARB && glDebugMessageCallbackARB && glDebugMessageInsertARB ) {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        glcheck();
        glDebugMessageCallbackARB(&gl_debug_msg_proc_arb,0);
        glcheck();
        glDebugMessageControlARB(GL_DONT_CARE,GL_DONT_CARE,GL_DONT_CARE,0,0,GL_TRUE);
        glcheck();
    }


    // INIT RESOURCES

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    glClearColor(0.8f, 0.8f, 0.8f, 0.f);        // background color
    glClearStencil(0);                          // clear stencil buffer
    glClearDepth(1.0f);                         // 0 is near, 1 is far
    glDepthFunc(GL_LEQUAL);


    glGenFramebuffers(1, &_fb);


    _target = create_texture(2,2);

    //_textree = load_texture("AustrianPine.raw", 32, 128);

    if(!load_shaders())
        return false;

    {
        float quad[] = {1.f, 0.f, 1.f, 0.f,
            1.f, 1.f, 1.f, 1.f,
            0.f, 0.f, 0.f, 0.f,
            0.f, 1.f, 0.f, 1.f};

        glGenBuffers(1,&_vtx_vbo);
        glBindBuffer(GL_ARRAY_BUFFER,_vtx_vbo);
        glBufferDataARB(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER,0);
    }

    return true;
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

void release_gl()
{
    wglMakeCurrent(0,0);
    wglDeleteContext(_hrc);
    ReleaseDC(_hwnd,_hdc);
}

int xabs(int x) {
    return x<0 ? -x : x;
}

int random_perm( int v )
{
    int p = abs(v) % 16807;
    //int p = 2047483673*v;
    //p = p - 16807*(p/16807);

    return p;
}



void process_frame()
{
    glDisable(GL_CULL_FACE);

    glBindFramebuffer(GL_FRAMEBUFFER_EXT, _fb);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        _target,
        0);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_STENCIL_ATTACHMENT,GL_TEXTURE_2D,0,0);

    static const GLenum db[]={
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4,
        GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6,
        GL_COLOR_ATTACHMENT7
    };
    glDrawBuffers(1,db);

    glViewport(0, 0, 2, 2);

    glUseProgram(_prg);

    float v1[] = {1, 2, 3};
    float v2[] = {4.5f, 3.3f, -2.0f};
    glUniform3fv(cgp_p1, 1, v1);
    glUniform3fv(cgp_p2, 1, v2);

    float m1[] = {2, 5, 9};
    float m2[] = {0, -6, 2};
    glUniform3fv(cgp_m1, 1, m1);
    glUniform3fv(cgp_m2, 1, m2);


    // BUG ATTR0 has to be bound to make it work
    glBindBuffer(GL_ARRAY_BUFFER,_vtx_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,4,GL_FLOAT,false,4*sizeof(float),0);

    glDrawArrays(GL_TRIANGLE_STRIP,0,3);

    //int vcpu = random_perm(v);


    glBindTexture(GL_TEXTURE_2D, _target);
    int val[4];

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_INT, val);
    glBindTexture(GL_TEXTURE_2D, 0);

    printf("matrix test: %s", val[0]!=0 ? "succeeded" : "failed");

    //printf("CPU: %d\nGPU: %d static %d\nDIV: %d static %d\n", vcpu, val[1], val[0], val[3], val[2]);

    glBindBuffer(GL_ARRAY_BUFFER,0);
    glEnable(GL_CULL_FACE);

    SwapBuffers(_hdc);


}


//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

int main(int, char**)
{
    create_window();

    if(!init_gl())
        return -1;

    // process window messages
    MSG msg;
    /*
    for(;;)
    {
    if( PeekMessage( &msg,0,0U,0U,PM_REMOVE )!=0 ) {
    if( msg.message!=WM_QUIT ) {
    TranslateMessage( &msg );
    DispatchMessage( &msg );
    } else
    break;
    }
    process_frame();
    }*/

    process_frame();


    release_gl();

    return 0;
}
