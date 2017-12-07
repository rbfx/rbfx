//
// Copyright (c) 2008-2017 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Urho3DAll.h>
#include <Toolbox/SystemUI/Gizmo.h>
#include <Poco/Process.h>
#include <time.h>


using namespace std::placeholders;


class AssetViewer : public Application
{
    URHO3D_OBJECT(AssetViewer, Application);
public:
    SharedPtr<Scene> scene_;
    SharedPtr<Viewport> viewport_;
    SharedPtr<Light> light_;
    WeakPtr<Camera> camera_;
    WeakPtr<Node> node_;
    WeakPtr<Node> parentNode_;
    WeakPtr<AnimatedModel> model_;
    WeakPtr<AnimationController> animator_;
    float lookSensitivity_ = 1.0f;
    Gizmo gizmo_;
    bool showHelp_ = false;

    explicit AssetViewer(Context* context) : Application(context), gizmo_(context)
    {
    }

    void Setup() override
    {
        engineParameters_[EP_WINDOW_TITLE]   = GetTypeName();
        engineParameters_[EP_WINDOW_WIDTH]   = 1024;
        engineParameters_[EP_WINDOW_HEIGHT]  = 768;
        engineParameters_[EP_FULL_SCREEN]    = false;
        engineParameters_[EP_HEADLESS]       = false;
        engineParameters_[EP_SOUND]          = false;
        engineParameters_[EP_RESOURCE_PATHS] = "CoreData";
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] =
            context_->GetFileSystem()->GetProgramDir() + ";;..;../share/Urho3D/Resources";
        engineParameters_[EP_WINDOW_RESIZABLE] = true;

        ui::GetIO().IniFilename = nullptr;    // Disable saving of settings.
    }

    void Start() override
    {
        GetInput()->SetMouseVisible(true);
        GetInput()->SetMouseMode(MM_ABSOLUTE);

        scene_ = new Scene(context_);
        scene_->CreateComponent<Octree>();
        auto zone = scene_->CreateComponent<Zone>();
        zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
        camera_ = scene_->CreateChild("Camera")->CreateComponent<Camera>();
        light_ = camera_->GetNode()->CreateComponent<Light>();
        light_->SetColor(Color::WHITE);
        light_->SetLightType(LIGHT_DIRECTIONAL);

        viewport_ = new Viewport(context_, scene_, camera_);
        GetSubsystem<Renderer>()->SetViewport(0, viewport_);

        // Parent node used to rotate model around it's center (not origin).
        parentNode_ = scene_->CreateChild();

        GetSystemUI()->ApplyStyleDefault(true, 1);
        ui::GetStyle().WindowRounding = 0;

        SubscribeToEvent(E_UPDATE, std::bind(&AssetViewer::OnUpdate, this, _2));
        SubscribeToEvent(E_DROPFILE, std::bind(&AssetViewer::OnFileDrop, this, _2));

        for (const auto& arg: GetArguments())
            LoadFile(arg);
    }

    void OnUpdate(VariantMap& args)
    {
        if (node_.Null())
            return;

        if (!GetSystemUI()->IsAnyItemActive() && !GetSystemUI()->IsAnyItemHovered())
        {
            auto input = GetSubsystem<Input>();

            if (!input->GetKeyDown(KEY_SHIFT))
            {
                if (input->GetMouseButtonDown(MOUSEB_RIGHT))
                {
                    if (input->IsMouseVisible())
                        input->SetMouseVisible(false);

                    if (input->GetMouseMove() != IntVector2::ZERO)
                    {
                        camera_->GetNode()->RotateAround(Vector3::ZERO,
                            Quaternion(input->GetMouseMoveX() * 0.1f * lookSensitivity_, camera_->GetNode()->GetUp()) *
                                Quaternion(input->GetMouseMoveY() * 0.1f * lookSensitivity_,
                                    camera_->GetNode()->GetRight()),
                            TS_WORLD
                        );
                    }
                }
                else if (input->GetMouseButtonDown(MOUSEB_MIDDLE))
                {
                    if (input->IsMouseVisible())
                        input->SetMouseVisible(false);

                    node_->Translate2D(Vector2(input->GetMouseMoveX(), -input->GetMouseMoveY()) / 300.f, TS_WORLD);
                }
                else if (!input->IsMouseVisible())
                    input->SetMouseVisible(true);

                camera_->GetNode()->Translate(Vector3::FORWARD * input->GetMouseMoveWheel() * 0.2f);
            }
            else if (!input->IsMouseVisible())
                input->SetMouseVisible(true);
        }

        if (GetInput()->GetKeyPress(KEY_F1))
            showHelp_ = true;


        ui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
        if (ui::Begin("Settings", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
        {
            gizmo_.RenderUI();

            if (ui::Button("Reset"))
                ResetNode();

            // Window has to contain controls already in order for it's size to be set to match contents.
            ui::SetWindowSize({0, 0}, ImGuiCond_Always);
        }
        ui::End();

        if (showHelp_)
        {
            if (ui::Begin("Help", &showHelp_, ImGuiWindowFlags_NoSavedSettings))
            {
                ui::TextUnformatted("RMB: hold it rotates model around its center.");
                ui::TextUnformatted("Shift: holding it provides manipulation gizmo at model origin.");
            }
            ui::End();
        }

        if (node_ && GetInput()->GetKeyDown(KEY_SHIFT))
            gizmo_.Manipulate(camera_, parentNode_);
    }

    void OnFileDrop(VariantMap& args)
    {
        LoadFile(args[DropFile::P_FILENAME].GetString());
    }

    void LoadFile(const String& file_path)
    {
        if (file_path.EndsWith(".mdl"))
            LoadModel(file_path);
        else if (file_path.EndsWith(".ani"))
            LoadAnimation(file_path);
        else if (file_path.EndsWith(".fbx"))
            LoadFbx(file_path);
    }

    void LoadModel(const String& file_path, const Vector<String>& materials={})
    {
        if (node_.NotNull())
            node_->Remove();

        node_ = parentNode_->CreateChild("Node");
        model_ = node_->CreateComponent<AnimatedModel>();
        animator_ = node_->CreateComponent<AnimationController>();

        auto cache = GetSubsystem<ResourceCache>();
        model_->SetModel(cache->GetTempResource<Model>(file_path));

        ResetNode();

        for (auto i = 0; i < materials.Size(); i++)
            model_->SetMaterial(i, cache->GetTempResource<Material>(materials[i]));
    }

    void ResetNode()
    {
        if (node_.NotNull())
        {
            parentNode_->SetScale(1.f);
            parentNode_->SetPosition(Vector3::ZERO);
            parentNode_->SetRotation(Quaternion::IDENTITY);
            // Scale model to size of 1 unit
            auto size = model_->GetBoundingBox().Size();
            node_->SetScale(1.f / Max(size.x_, Max(size.y_, size.z_)));
            // Snap center of the model to {0,0,0}
            node_->SetWorldPosition(node_->GetWorldPosition() - model_->GetWorldBoundingBox().Center());
            node_->SetRotation(Quaternion::IDENTITY);

            camera_->GetNode()->LookAt(Vector3::ZERO);
            camera_->GetNode()->SetRotation(Quaternion::IDENTITY);
            camera_->GetNode()->SetPosition(Vector3::BACK * 1.5f);
        }
    }

    void LoadAnimation(const String& file_path)
    {
        if (animator_)
            animator_->PlayExclusive(file_path, 0, true);
    }

    void LoadFbx(const String& file_path)
    {
        auto fs = GetSubsystem<FileSystem>();
        String temp = fs->GetTemporaryPath();
        temp += "AssetViewer/";
        if (!fs->DirExists(temp))
        {
            fs->CreateDirs(temp, "mdl");
            fs->CreateDirs(temp, "ani");
        }
        auto animation_path = temp + "/ani";
        auto model_file = temp + "/mdl/out.mdl";
        auto material_list_file = temp + "/mdl/out.txt";
        fs->Delete(model_file);

        Poco::Process::launch((fs->GetProgramDir() + "AssetImporter").CString(),
            {"model", file_path.CString(), model_file.CString(), "-na", "-l"}, ".").wait();

        if (fs->FileExists(model_file))
        {
            Vector<String> materials;
            File fp(context_, material_list_file);
            if (fp.IsOpen())
            {
                while (!fp.IsEof())
                    materials.Push(temp + "/mdl/" + fp.ReadLine());
            }
            LoadModel(model_file, materials);
        }

        Vector<String> animations;
        fs->ScanDir(animations, animation_path, "*.ani", SCAN_FILES, false);
        for (const auto& filename : animations)
            fs->Delete(animation_path + "/" + filename);

        Poco::Process::launch("./AssetImporter", {"anim", file_path.CString(),
            (animation_path + "/out_" + ToString("%ld", time(nullptr))).CString()}, ".").wait();

        fs->ScanDir(animations, animation_path, "*.ani", SCAN_FILES, false);

        if (animations.Size())
            LoadAnimation(animation_path + "/" + animations[0]);
    }
};

URHO3D_DEFINE_APPLICATION_MAIN(AssetViewer);
