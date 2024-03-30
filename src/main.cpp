

#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl2.h"
#define GL_SILENCE_DEPRECATION
#include "application.h"
#include "renderer.h"

const char *APPLICATION_NAME = "Render Engine 2.0";

void glutDisplayFunc();
void process_normal_keys(unsigned char key, int x, int y);

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow){
    int argc;
    char *argv = (char*)malloc(sizeof(char));
    glutInit(&argc, &argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_MULTISAMPLE);
    glutInitWindowSize(1280, 720);
    glutInitWindowPosition(100, 50);
    glutCreateWindow(APPLICATION_NAME);

    glutDisplayFunc(glutDisplayFunc);

    glewInit();
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    io.FontGlobalScale = 1;

    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL2_Init();
    ImGui_ImplGLUT_InstallFuncs();

    //glutKeyboardFunc(process_normal_keys);

    RenderEngine::init();

    // Main loop
    glutMainLoop();

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGLUT_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
void process_normal_keys(unsigned char key, int x, int y){
    if(!viewport_focused){ return; }

    if(key ==  'w'){ camera = camera + look_dir * 0.5f; }
    if(key ==  's'){ camera = camera - look_dir * 0.5f; }
    if(key ==  'a'){ camera = camera + look_dir.cross_product(RenderEngine::vec3(0, 1, 0)) * 0.5f; }
    if(key ==  'd'){ camera = camera - look_dir.cross_product(RenderEngine::vec3(0, 1, 0)) * 0.5f; }
    if(key ==  ' '){ camera.y += 0.5f; }
    if(key ==  'r'){ camera.y -= 0.5f; }
    if(key ==  'o'){ fyaw += 0.08f; }
    if(key ==  'p'){ fyaw -= 0.08f; }
    if(key ==  'k'){ }
    if(key ==  'l'){ }
}

void glutDisplayFunc(){
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();


    render();

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (GLsizei)io.DisplaySize.x, (GLsizei)io.DisplaySize.y);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    glutSwapBuffers();
    glutPostRedisplay();
}

