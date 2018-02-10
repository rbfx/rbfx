#include "ImGui/imgui.h"
#include "../src/ofbx.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cinttypes>
#include <memory>
#include <vector>


typedef char Path[255];
typedef unsigned int u32;
ofbx::IScene* g_scene = nullptr;
const ofbx::IElement* g_selected_element = nullptr;
const ofbx::Object* g_selected_object = nullptr;


template <int N>
void toString(ofbx::DataView view, char (&out)[N])
{
	int len = int(view.end - view.begin);
	if (len > sizeof(out) - 1) len = sizeof(out) - 1;
	strncpy(out, (const char*)view.begin, len);
	out[len] = 0;
}


int getPropertyCount(ofbx::IElementProperty* prop)
{
	return prop ? getPropertyCount(prop->getNext()) + 1 : 0;
}


template <int N>
void catProperty(char(&out)[N], const ofbx::IElementProperty& prop)
{
	char tmp[128];
	switch (prop.getType())
	{
		case ofbx::IElementProperty::DOUBLE: sprintf(tmp, "%f", prop.getValue().toDouble()); break;
		case ofbx::IElementProperty::LONG: sprintf(tmp, "%" PRId64, prop.getValue().toU64()); break;
		case ofbx::IElementProperty::INTEGER: sprintf(tmp, "%d", prop.getValue().toInt()); break;
		case ofbx::IElementProperty::STRING: prop.getValue().toString(tmp); break;
		default: sprintf(tmp, "Type: %c", (char)prop.getType()); break;
	}
	strcat(out, tmp);
}


void showGUI(const ofbx::IElement& parent)
{
	for (const ofbx::IElement* element = parent.getFirstChild(); element; element = element->getSibling())
	{
		auto id = element->getID();
		char label[128];
		id.toString(label);
		strcat(label, " (");
		ofbx::IElementProperty* prop = element->getFirstProperty();
		bool first = true;
		while (prop)
		{
			if(!first)
				strcat(label, ", ");
			first = false;
			catProperty(label, *prop);
			prop = prop->getNext();
		}
		strcat(label, ")");

		ImGui::PushID((const void*)id.begin);
		ImGuiTreeNodeFlags flags = g_selected_element == element ? ImGuiTreeNodeFlags_Selected : 0;
		if (!element->getFirstChild()) flags |= ImGuiTreeNodeFlags_Leaf;
		if (ImGui::TreeNodeEx(label, flags))
		{
			if(ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) g_selected_element = element;
			if (element->getFirstChild()) showGUI(*element);
			ImGui::TreePop();
		}
		else
		{
			if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) g_selected_element = element;
		}
		ImGui::PopID();
	}
}


template <typename T>
void showArray(const char* label, const char* format, ofbx::IElementProperty& prop)
{
	if (!ImGui::CollapsingHeader(label)) return;

	int count = prop.getCount();
	ImGui::Text("Count: %d", count);
	std::vector<T> tmp;
	tmp.resize(count);
	prop.getValues(&tmp[0], int(sizeof(tmp[0]) * tmp.size()));
	for (T v : tmp)
	{
		ImGui::Text(format, v);
	}
}


void showGUI(ofbx::IElementProperty& prop)
{
	ImGui::PushID((void*)&prop);
	char tmp[256];
	switch (prop.getType())
	{
		case ofbx::IElementProperty::LONG: ImGui::Text("Long: %" PRId64, prop.getValue().toU64()); break;
		case ofbx::IElementProperty::FLOAT: ImGui::Text("Float: %f", prop.getValue().toFloat()); break;
		case ofbx::IElementProperty::DOUBLE: ImGui::Text("Double: %f", prop.getValue().toDouble()); break;
		case ofbx::IElementProperty::INTEGER: ImGui::Text("Integer: %d", prop.getValue().toInt()); break;
		case ofbx::IElementProperty::ARRAY_FLOAT: showArray<float>("float array", "%f", prop); break;
		case ofbx::IElementProperty::ARRAY_DOUBLE: showArray<double>("double array", "%f", prop); break;
		case ofbx::IElementProperty::ARRAY_INT: showArray<int>("int array", "%d", prop); break;
		case ofbx::IElementProperty::ARRAY_LONG: showArray<ofbx::u64>("long array", "%" PRId64, prop); break;
		case ofbx::IElementProperty::STRING:
			toString(prop.getValue(), tmp);
			ImGui::Text("String: %s", tmp);
			break;
		default:
			ImGui::Text("Other: %c", (char)prop.getType());
			break;
	}

	ImGui::PopID();
	if (prop.getNext()) showGUI(*prop.getNext());
}


void showObjectGUI(const ofbx::Object& object)
{
	const char* label;
	switch (object.getType())
	{
		case ofbx::Object::Type::GEOMETRY: label = "geometry"; break;
		case ofbx::Object::Type::MESH: label = "mesh"; break;
		case ofbx::Object::Type::MATERIAL: label = "material"; break;
		case ofbx::Object::Type::ROOT: label = "root"; break;
		case ofbx::Object::Type::TEXTURE: label = "texture"; break;
		case ofbx::Object::Type::NULL_NODE: label = "null"; break;
		case ofbx::Object::Type::LIMB_NODE: label = "limb node"; break;
		case ofbx::Object::Type::NODE_ATTRIBUTE: label = "node attribute"; break;
		case ofbx::Object::Type::CLUSTER: label = "cluster"; break;
		case ofbx::Object::Type::SKIN: label = "skin"; break;
		case ofbx::Object::Type::ANIMATION_STACK: label = "animation stack"; break;
		case ofbx::Object::Type::ANIMATION_LAYER: label = "animation layer"; break;
		case ofbx::Object::Type::ANIMATION_CURVE: label = "animation curve"; break;
		case ofbx::Object::Type::ANIMATION_CURVE_NODE: label = "animation curve node"; break;
		default: assert(false); break;
	}

	ImGuiTreeNodeFlags flags = g_selected_object == &object ? ImGuiTreeNodeFlags_Selected : 0;
	char tmp[128];
	sprintf(tmp, "%" PRId64 " %s (%s)", object.id, object.name, label);
	if (ImGui::TreeNodeEx(tmp, flags))
	{
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) g_selected_object = &object;
		int i = 0;
		while (ofbx::Object* child = object.resolveObjectLink(i))
		{
			showObjectGUI(*child);
			++i;
		}
		ImGui::TreePop();
	}
	else
	{
		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) g_selected_object = &object;
	}
}


void showObjectsGUI(const ofbx::IScene& scene)
{
	if (!ImGui::Begin("Objects"))
	{
		ImGui::End();
		return;
	}
	const ofbx::Object* root = scene.getRoot();
	if (root) showObjectGUI(*root);

	int count = scene.getAnimationStackCount();
	for (int i = 0; i < count; ++i)
	{
		const ofbx::Object* stack = scene.getAnimationStack(i);
		showObjectGUI(*stack);
	}

	ImGui::End();
}


bool saveAsOBJ(ofbx::IScene& scene, const char* path)
{
	FILE* fp = fopen(path, "wb");
	if (!fp) return false;
	int obj_idx = 0;
	int indices_offset = 0;
	int normals_offset = 0;
	int mesh_count = scene.getMeshCount();
	for (int i = 0; i < mesh_count; ++i)
	{
		fprintf(fp, "o obj%d\ng grp%d\n", i, obj_idx);

		const ofbx::Mesh& mesh = *scene.getMesh(i);
		const ofbx::Geometry& geom = *mesh.getGeometry();
		int vertex_count = geom.getVertexCount();
		const ofbx::Vec3* vertices = geom.getVertices();
		for (int i = 0; i < vertex_count; ++i)
		{
			ofbx::Vec3 v = vertices[i];
			fprintf(fp, "v %f %f %f\n", v.x, v.y, v.z);
		}

		bool has_normals = geom.getNormals() != nullptr;
		if (has_normals)
		{
			const ofbx::Vec3* normals = geom.getNormals();
			int count = geom.getVertexCount();

			for (int i = 0; i < count; ++i)
			{
				ofbx::Vec3 n = normals[i];
				fprintf(fp, "vn %f %f %f\n", n.x, n.y, n.z);
			}
		}

		bool has_uvs = geom.getUVs() != nullptr;
		if (has_uvs)
		{
			const ofbx::Vec2* uvs = geom.getUVs();
			int count = geom.getVertexCount();

			for (int i = 0; i < count; ++i)
			{
				ofbx::Vec2 uv = uvs[i];
				fprintf(fp, "vt %f %f\n", uv.x, uv.y);
			}
		}

		bool new_face = true;
		int count = geom.getVertexCount();
		for (int i = 0; i < count; ++i)
		{
			if (new_face)
			{
				fputs("f ", fp);
				new_face = false;
			}
			int idx = i + 1;
			int vertex_idx = indices_offset + idx;
			fprintf(fp, "%d", vertex_idx);

			if (has_normals)
			{
				fprintf(fp, "/%d", idx);
			}
			else
			{
				fprintf(fp, "/");
			}

			if (has_uvs)
			{
				fprintf(fp, "/%d", idx);
			}
			else
			{
				fprintf(fp, "/");
			}

			new_face = idx < 0;
			fputc(new_face ? '\n' : ' ', fp);
		}

		indices_offset += vertex_count;
		++obj_idx;
	}
	fclose(fp);
	return true;
}

#include <Urho3D/Urho3DAll.h>

using namespace Urho3D;

class DemoApplication : public Application
{
	URHO3D_OBJECT(DemoApplication, Application);
public:
	explicit DemoApplication(Context* context)
		: Application(context)
	{
	}

	void Setup() override
	{
		engineParameters_[EP_WINDOW_TITLE]   = GetTypeName();
		engineParameters_[EP_WINDOW_WIDTH]   = 1024;
		engineParameters_[EP_WINDOW_HEIGHT]  = 768;
		engineParameters_[EP_FULL_SCREEN]	= false;
		engineParameters_[EP_HEADLESS]	   = false;
		engineParameters_[EP_SOUND]		  = false;
		engineParameters_[EP_RESOURCE_PATHS] = "CoreData";
		engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";..";
		engineParameters_[EP_WINDOW_RESIZABLE] = true;

		ui::GetIO().IniFilename = nullptr;	// Disable saving of settings.
	}

	void Start() override
	{
		GetInput()->SetMouseVisible(true);
		GetInput()->SetMouseMode(MM_ABSOLUTE);
		SubscribeToEvent(E_UPDATE, [&](StringHash, VariantMap&) {
			if (g_scene)
			{
				if (ImGui::Begin("Elements"))
				{
					const ofbx::IElement* root = g_scene->getRootElement();
					if (root && root->getFirstChild()) showGUI(*root->getFirstChild());
				}
				ImGui::End();

				if (ImGui::Begin("Properties") && g_selected_element)
				{
					ofbx::IElementProperty* prop = g_selected_element->getFirstProperty();
					if (prop) showGUI(*prop);
				}
				ImGui::End();

				showObjectsGUI(*g_scene);
			}
		});
		SubscribeToEvent(E_DROPFILE, [&](StringHash, VariantMap& args) {
			SharedPtr<File> file(new File(context_));
			if (!file->Open(args[DropFile::P_FILENAME].GetString()))
			{
				PrintLine("Failed to open input file.", true);
				return;
			}

			PODVector<uint8_t> buffer(file->GetSize());
			if (file->Read(&buffer.Front(), buffer.Size()) != file->GetSize())
			{
				PrintLine("Failed to read entire input file.", true);
				return;
			}

			g_scene = ofbx::load(&buffer.Front(), buffer.Size());
		});
	}
};

URHO3D_DEFINE_APPLICATION_MAIN(DemoApplication);
