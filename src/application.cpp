
#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl2.h"
#define GL_SILENCE_DEPRECATION

#include "application.h"
#include "renderer.h"

bool application_active     = true;
bool show_app_dockspace     = true;
bool window_settings        = false;
bool window_3dviewport      = true;
bool window_inspector       = true;

bool add_primitive          = false;
bool init = false;

using namespace ImGui;
void show_dock_space(bool* p_open);
void show_window_settings(bool* window_settings);
void show_window_3dviewport(bool* window_3dviewport);
void show_window_inspector(bool *window_inspector);

void render(){
    show_dock_space(&show_app_dockspace);

    if(window_settings      ){ show_window_settings     (&window_settings   ); }
    if(window_3dviewport    ){ show_window_3dviewport   (&window_3dviewport ); }
    if(window_inspector     ){ show_window_inspector    (&window_inspector  ); }

}
void show_dock_space(bool* p_open){
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBringToFrontOnFocus;
    const ImGuiViewport* viewport = GetMainViewport();
    SetNextWindowPos(viewport->WorkPos);
    SetNextWindowSize(viewport->WorkSize);
    SetNextWindowViewport(viewport->ID);
    PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    Begin("DockSpace", p_open, window_flags);
    PopStyleVar();
    PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = GetIO();
    if(io.ConfigFlags & ImGuiConfigFlags_DockingEnable){
        ImGuiID dockspace_id = GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    if(ImGui::BeginMenuBar()){
        if(ImGui::BeginMenu("File")){
            if(ImGui::MenuItem("Open", "Ctrl+O")){  }
            if(ImGui::MenuItem("New" , "Ctrl+N")){  }
            if(ImGui::MenuItem("Save", "Ctrl+S")){  }
            ImGui::Separator();
            if(ImGui::MenuItem("Exit", "Alt+F4")){ application_active = false; }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    if(ImGui::BeginMenuBar()){
        if(ImGui::BeginMenu("Edit")){
            if(ImGui::MenuItem("Settings")){ window_settings = true; }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();
}
void show_window_settings(bool* window_settings){
    Begin("Settings", window_settings, ImGuiWindowFlags_NoCollapse);
    BeginChild("ChildL", ImVec2(160, GetContentRegionAvail().y), false, ImGuiWindowFlags_HorizontalScrollbar);
    Button("Category 1", ImVec2(GetContentRegionAvail().x, 40));
    Button("Category 2", ImVec2(GetContentRegionAvail().x, 40));
    Button("Category 3", ImVec2(GetContentRegionAvail().x, 40));
    EndChild();
    SameLine();

    BeginChild("ChildR", ImVec2(0, GetContentRegionAvail().y), true, ImGuiWindowFlags_None);

    EndChild();

    End();
}

void RenderEngine::io(){
    if(viewport_focused){
        if(GetAsyncKeyState('W') & 0x8000){ camera = camera + look_dir * 0.5f; }
        if(GetAsyncKeyState('S') & 0x8000){ camera = camera - look_dir * 0.5f; }
        if(GetAsyncKeyState('A') & 0x8000){ camera = camera + look_dir.cross_product(RenderEngine::vec3(0, 1, 0)) * 0.5f; }
        if(GetAsyncKeyState('D') & 0x8000){ camera = camera - look_dir.cross_product(RenderEngine::vec3(0, 1, 0)) * 0.5f; }
        if(GetAsyncKeyState(' ') & 0x8000){ camera.y += 0.5f; }
        if(GetAsyncKeyState('R') & 0x8000){ camera.y -= 0.5f; }
        if(GetAsyncKeyState('O') & 0x8000){ fyaw += 0.08f; }
        if(GetAsyncKeyState('P') & 0x8000){ fyaw -= 0.08f; }
        if(GetAsyncKeyState('K') & 0x8000){ }
        if(GetAsyncKeyState('L') & 0x8000){ }

        if(GetAsyncKeyState(VK_ESCAPE) & 0x8000){ SetWindowFocus("inspector"); }
        ShowCursor(FALSE);
    }else{
        ShowCursor(TRUE);
    }
}
void show_window_3dviewport(bool* window_3dviewport){
    Begin("viewport", window_3dviewport, ImGuiWindowFlags_NoCollapse);
    viewport_focused = IsWindowFocused() ? true : false;
    Image((void *)RenderEngine::update(GetContentRegionAvail().x, GetContentRegionAvail().y), GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
    End();
}

void show_window_inspector(bool *window_inspector){
    Begin("inspector", window_inspector, ImGuiWindowFlags_NoCollapse);

    Dummy(ImVec2(10, 20));
    Dummy(ImVec2(15, 10));
    SameLine();
    Text("FPS: %d", frame_rate);

    Dummy(ImVec2(10, 20));
    Dummy(ImVec2(15, 10));
    SameLine();
    BeginChild("Light", ImVec2(GetContentRegionAvail().x, 150));{
        SeparatorText("Light");

        Dummy(ImVec2(10, 10));
        Text("Light vector: ");
        InputDouble("x1", &light.x, 0.1, 1, "%.3f");
        InputDouble("y1", &light.y, 0.1, 1, "%.3f");
        InputDouble("z1", &light.z, 0.1, 1, "%.3f");
        Dummy(ImVec2(10, 20));

    }EndChild();


    Dummy(ImVec2(10, 20));
    Dummy(ImVec2(15, 10));
    SameLine();
    BeginChild("Camera", ImVec2(GetContentRegionAvail().x, 500));{
        SeparatorText("Camera");

        Dummy(ImVec2(10, 10));
        Text("Camera vector: ");
        InputDouble("x2", &camera.x, 0.1, 1, "%.3f");
        InputDouble("y2", &camera.y, 0.1, 1, "%.3f");
        InputDouble("z2", &camera.z, 0.1, 1, "%.3f");

        Dummy(ImVec2(10, 10));
        Text("Yaw: ");
        InputFloat("x2.11", &fyaw, 0.1, 1, "%.3f");
        
        Dummy(ImVec2(10, 10));
        if(Button("Move forward ")){ move_forward  = true; }
        if(Button("Move backward")){ move_backward = true; }

        const char* items[] = { "Perspective", "Orthographic" };
        static const char* current_item = items[0];
        Dummy(ImVec2(10, 20));
        if(BeginCombo("##Type", current_item)){
            for(int n = 0; n < IM_ARRAYSIZE(items); n++){
                bool is_selected = (current_item == items[n]); 
                if(Selectable(items[n], is_selected)){
                    current_item = items[n];
                    if(is_selected){ SetItemDefaultFocus(); }
                    orthographic = current_item == items[0] ? false : true;
                }
            }
            EndCombo();
        }

        Dummy(ImVec2(10, 10));
        Text("Background color: ");
        InputDouble("x2.1", &background_color.x, 0.1, 1, "%.3f");
        InputDouble("y2.1", &background_color.y, 0.1, 1, "%.3f");
        InputDouble("z2.1", &background_color.z, 0.1, 1, "%.3f");

        Dummy(ImVec2(10, 10));
        Text("Resolution: ");
        Text("width : %d", fbo_width);
        Text("height: %d", fbo_height);

        Dummy(ImVec2(10, 10));
        Checkbox("Draw wireframe", &draw_wireframe_bool);

    }EndChild();

    Dummy(ImVec2(10, 20));
    Dummy(ImVec2(15, 10));
    SameLine();
    BeginChild("Transform", ImVec2(GetContentRegionAvail().x, 370));{
        SeparatorText("Transform");

        Dummy(ImVec2(10, 10));
        Text("position: ");
        InputDouble("x3", &default_mesh.position.x, 0.1, 1, "%.3f");
        InputDouble("y3", &default_mesh.position.y, 0.1, 1, "%.3f");
        InputDouble("z3", &default_mesh.position.z, 0.1, 1, "%.3f");

        Dummy(ImVec2(10, 10));
        Text("rotation: ");
        InputDouble("x4", &default_mesh.rotation.x, 0.1, 1, "%.3f");
        InputDouble("y4", &default_mesh.rotation.y, 0.1, 1, "%.3f");
        InputDouble("z4", &default_mesh.rotation.z, 0.1, 1, "%.3f");

        Dummy(ImVec2(10, 10));
        Text("scale: ");
        InputDouble("x5", &default_mesh.scale.x, 0.1, 1, "%.3f");
        InputDouble("y5", &default_mesh.scale.y, 0.1, 1, "%.3f");
        InputDouble("z5", &default_mesh.scale.z, 0.1, 1, "%.3f");
    }EndChild();


    End();
}