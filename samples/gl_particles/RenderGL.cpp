#include "RenderGL.h"

#define FOV (3.14159f/4.0f)

//-----------------------------------------------------------------------------
// constructor
//-----------------------------------------------------------------------------
RenderGL::RenderGL()
{
    m_points = 0;
}

//-----------------------------------------------------------------------------
// texture loader
//-----------------------------------------------------------------------------
void* LoadBMPFileAlphaOnly(char* pFilename, int& out_width, int& out_height)
{
    FILE* fh = fopen(pFilename, "rb");
    if( fh==NULL ) {
        MessageBox(0, "Cannot open bitmap, please ensure that working directory is set correctly.\n", "Build Error", MB_OK);
        exit(-1);
    }

    BITMAPFILEHEADER fileHeader;
    fread(&fileHeader, 1, sizeof(BITMAPFILEHEADER), fh);

    BITMAPINFOHEADER infoHeader;
    fread(&infoHeader, 1, sizeof(BITMAPINFOHEADER), fh);

    out_width = infoHeader.biWidth;
    out_height = infoHeader.biHeight;
    int size = infoHeader.biWidth * infoHeader.biHeight * infoHeader.biBitCount / 4;

    BYTE* bytes = new BYTE[size];
    fread(bytes, 1, size, fh);

    fclose(fh);

    BYTE* alpha = new BYTE[infoHeader.biWidth * infoHeader.biHeight];
    for (int i = 0; i < infoHeader.biWidth * infoHeader.biHeight; i++)
    {
        alpha[i] = bytes[3 + (i*4)];
    }
    delete [] bytes;

    return alpha;
}

//-----------------------------------------------------------------------------
// create program, buffers, textures
//-----------------------------------------------------------------------------
void RenderGL::Init(int in_numPoints, GLuint* out_pPoints)
{
    // Initialize GLEW
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return;
    }

    std::string buildLog = "Empty Log";
    m_program = GLContext::LoadProgram("ParticleVS.gl", "ParticleFS.gl", &buildLog);
    if (buildLog.size())
    {
        MessageBox(0, buildLog.c_str(), "Build Error", MB_OK);
    }

    m_projectionMatrixID = glGetUniformLocation(m_program, "projectionMatrix");
    m_invViewID = glGetUniformLocation(m_program, "inverseMatrix");
    m_pointsID  = glGetUniformLocation(m_program, "particlePosition");
    m_textureID = glGetUniformLocation(m_program, "particleTexture");

    m_posID = glGetAttribLocation(m_program, "in_pos");

    Vector3 eye(0,0,500);
    Vector3 at(0,0,0);
    Vector3 up(0,1,0);
    m_viewMatrix.LookAt(eye, at, up);

    float width = 1024;
    float height = 1024;
    m_projectionMatrix.PerspectiveFovRH(FOV, float(width)/height, 0.1f, 10000.0f);
    m_worldMatrix.SetIdentity();

    CreateBuffers(in_numPoints, out_pPoints);
}

//-----------------------------------------------------------------------------
// destructor
//-----------------------------------------------------------------------------
RenderGL::~RenderGL()
{
    assert(0 == m_program);
    assert(0 == m_points_tb);
    assert(0 == m_points);
    assert(0 == m_vertexArray);
    assert(0 == m_indexBuffer);
    assert(0 == m_vertexBuffer);
}

//-----------------------------------------------------------------------------
// recommend calling Shutdown before destroying the GL context and window
//-----------------------------------------------------------------------------
void RenderGL::Shutdown()
{
    // make sure GL is totally done
    glFinish();

    glDeleteProgram(m_program);
    m_program = 0;

    glDeleteTextures(1, &m_points_tb);
    m_points_tb = 0;

    glDeleteBuffers (1, &m_points);
    m_points = 0;

    glDeleteVertexArrays(1, &m_vertexArray);
    m_vertexArray = 0;

    glDeleteBuffers(1, &m_indexBuffer);
    m_indexBuffer = 0;

    glDeleteBuffers(1, &m_vertexBuffer);
    m_vertexBuffer = 0;
}

//-----------------------------------------------------------------------------
// indices for quad
//-----------------------------------------------------------------------------
const GLuint RenderGL::m_indices[] =
{
    0, 2, 1,
    2, 3, 1,
};
int RenderGL::GetIndexSize() {return sizeof(RenderGL::m_indices);}

//-----------------------------------------------------------------------------
// vertices for quad
//-----------------------------------------------------------------------------
const float RenderGL::m_vertices[][3] = 
{
    {-1,  1, 0},
    {-1, -1, 0},

    { 1,  1, 0},
    { 1, -1, 0},
};
int RenderGL::GetVertexSize() {return sizeof(RenderGL::m_vertices);}

struct ParticleVertex
{
    Vector4 m_position;
};

//-----------------------------------------------------------------------------
// create DX buffers to be rendered
//-----------------------------------------------------------------------------
void RenderGL::CreateBuffers(int in_numPoints, GLuint* out_pPoints)
{
    m_numPoints = in_numPoints;

    // load texture
    int width;
    int height;
    void* pTextureData = LoadBMPFileAlphaOnly("particle.bmp", width, height);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // FIXME: add mipmaps
    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pTextureData);

    delete [] pTextureData;

    // create vertex buffer.  Not using instancing.
    int numVertices = in_numPoints * 4;
    int bufferSize = sizeof(Vector3) * numVertices;
    Vector3* vertices = new Vector3[numVertices];
    for (int i = 0; i < in_numPoints * 4; i++)
    {
        vertices[i] = Vector3(m_vertices[i & 3]);
        vertices[i].z = float(i >> 2); // particle index
    }
    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, vertices, GL_STATIC_DRAW);
    delete [] vertices;

    // create index buffer.  Not using instancing.
    int numIndices = in_numPoints * 6;
    bufferSize = sizeof(GLuint) * numVertices;
    GLuint* indices = new GLuint[numIndices];
    GLuint* p = indices;
    for (int i = 0; i < in_numPoints; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            GLuint base = i * 4;
            *p = m_indices[j] + base;
            p++;
        }
    }
	glGenBuffers(1, &m_indexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize, indices, GL_STATIC_DRAW);
    delete [] indices;

    glGenVertexArrays(1, &m_vertexArray);

    // shared buffers
    int arraySize = sizeof(ParticleVertex) * in_numPoints;


    // share buffers with CL
    // create gl_texture_buffers from the shared buffers to sample from vertex shader
    glGenBuffers(1, &m_points);
    glGenTextures(1, &m_points_tb);

    glBindBuffer(GL_ARRAY_BUFFER, m_points);
    glBufferData(GL_ARRAY_BUFFER, arraySize, 0, GL_DYNAMIC_DRAW);
    glBindTexture(GL_TEXTURE_BUFFER, m_points_tb);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_points);

    *out_pPoints = m_points;
}

//-----------------------------------------------------------------------------
// per-frame render loop
//-----------------------------------------------------------------------------
void RenderGL::Render(const Vector2s& in_rotation, const Vector2s& in_translation,
    const Vector2s& in_objRotation)
{
    if (in_translation != Vector2s(0,0))
    {
        m_viewMatrix.TranslateLocal(Vector3(in_translation.x, 0, -in_translation.y));
    }
    if (in_rotation != Vector2s(0,0))
    {
        m_viewMatrix.RotateLocal(-in_rotation.x, -in_rotation.y);
    }
    if (in_objRotation != Vector2s(0,0))
    {
        m_viewMatrix.RotateTrackball(Vector3(0,0,0), -in_objRotation.x, -in_objRotation.y);
    }

    // Disable depth test and culling
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE);

    glUseProgram(m_program);

    glBindVertexArray(m_vertexArray);

    // vertex attribute #0: vertex positions
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    // vertex position attributes: 3 floats not normalized, stride = "packed", no offset
    glVertexAttribPointer(m_posID, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // set the model/view/projection matrix
    Matrix4 worldViewProj = m_worldMatrix * m_viewMatrix * m_projectionMatrix;
    worldViewProj.Transpose();
    glUniformMatrix4fv(m_projectionMatrixID, 1, GL_FALSE, worldViewProj);

    // set the inverse view matrix, used to compute the billboard vertices
    Matrix4 inverseView;
    MatrixInverse(&inverseView, 0, &m_viewMatrix);
    inverseView.Transpose();
    glUniformMatrix4fv(m_invViewID, 1, GL_FALSE, inverseView);

    // bind the particle positions as a 1D texture
	glUniform1i(m_pointsID, 0);
	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_points_tb);

    // bind the diffuse texture
    glEnable(GL_TEXTURE_2D);
    glUniform1i(m_textureID, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glDrawElements(GL_TRIANGLES, m_numPoints*6, GL_UNSIGNED_INT, 0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);
    glBindVertexArray(0);
}

//-----------------------------------------------------------------------------
// window resize affects aspect ratio of projection matrix
//-----------------------------------------------------------------------------
void RenderGL::Resize(HWND in_hWnd)
{
    RECT rect;
    GetClientRect(in_hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    m_projectionMatrix.PerspectiveFovRH(FOV, float(width)/height, 0.1f, 10000.0f);

    glViewport(0, 0, width, height);
}
