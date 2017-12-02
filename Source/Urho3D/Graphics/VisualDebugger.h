#ifndef VISUAL_DEBUGGER_H
#define VISUAL_DEBUGGER_H



#include "../Core/Object.h"
#include "../Core/Timer.h"
#include "../Container/List.h"
#include "../Math/Sphere.h"
#include "../Math/Polyhedron.h"
#include "../Math/Frustum.h"
#include "../Scene/Node.h"

namespace Urho3D
{
	class DebugRenderer;
	class Camera;
	class Text;
	class Node;


	//Subsytem providing persistant visualization of debug geometry.
	class URHO3D_API VisualDebugger : public Object {
		URHO3D_OBJECT(VisualDebugger, Object);

	public:
		VisualDebugger(Context* context);

		class VisualDebuggerObject : public Object {
			URHO3D_OBJECT(VisualDebuggerObject, Object);

		public:
			friend class VisualDebugger;

			VisualDebuggerObject(VisualDebugger* visDebuger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			virtual void SetEnabled(bool enabled);
			void SetLifeTimeMs(unsigned int lifeTimeMs);
			void SetColor(Color color);

		protected:

			virtual void Setup();
			virtual void TearDown();

			Color mColor;
			unsigned int creationTimeMS = 0;
			unsigned int mLifetimeMS = 2000;
			bool depthTest = false;
			bool mEnabled = false;
			WeakPtr<VisualDebugger> mVisDebugger;
		};

		class VisualDebuggerCircle : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerCircle(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Vector3 mCenter;
			Vector3 mNormal;
			float mRadius;
			int mSteps = 64;
		};

		class VisualDebuggerLine : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerLine(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Vector3 mStart;
			Vector3 mEnd;
		};

		class VisualDebuggerBoundingBox : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerBoundingBox(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			BoundingBox mBox;
			bool mSolid = false;
		};

		class VisualDebuggerTriangle : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerTriangle(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Vector3 v1, v2, v3;
		};

		class VisualDebuggerCross : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerCross(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Vector3 mCenter;
			float mSize;
		};

		class VisualDebuggerPolygon : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerPolygon(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Vector3 v1, v2, v3, v4;
		};

		class VisualDebuggerPolyhedron : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerPolyhedron(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Polyhedron mPolyhedron;
		};

		class VisualDebuggerCylinder : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerCylinder(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Vector3 mPosition;
			float mRadius;
			float mHeight;
		};

		class VisualDebuggerFrustum : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerFrustum(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Frustum mFrustum;
		};

		class VisualDebuggerQuad : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerQuad(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Vector3 mCenter;
			float width;
			float height;
		};

		class VisualDebuggerSphere : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerSphere(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Sphere mSphere;
		};

		class VisualDebuggerSphereSector : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerSphereSector(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Sphere mSphere;
			Quaternion mRotation;
			float mAngle;
			bool mDrawLines;
		};

		class VisualDebuggerOrb : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerOrb(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			Vector3 mCenter;
			float mRadius;
			int mSteps = 32;
			int mNumCircles = 10;
		};

		class VisualDebuggerNode : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerNode(VisualDebugger* visDebugger, Context* context_);

			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);
			float mScale = 1.0f;
			WeakPtr<Node> mNode;
		};

		class VisualDebuggerUILabel : public VisualDebuggerObject {
			URHO3D_OBJECT(VisualDebuggerObject, VisualDebuggerObject);

		public:
			VisualDebuggerUILabel(VisualDebugger* visDebugger, Context* context_);
			~VisualDebuggerUILabel();
			virtual void DrawDebugGeometry(DebugRenderer* debugRenderer);

			virtual void Setup();
			virtual void TearDown();

			virtual void SetEnabled(bool enabled);

			void UpdatePosition();

			String mText;
			Vector3 mCenter;
			SharedPtr<Text> mUIText;
		};



		VisualDebuggerCircle* AddCircle(const Vector3& center, const Vector3& normal, float radius, const Color& color, int steps = 64, bool depthTest = true);

		VisualDebuggerLine* AddLine(const Vector3& start, const Vector3& end, const Color& color, bool depthTest = true);

		VisualDebuggerOrb* AddOrb(const Vector3& center, const float& radius, const Color& color, int circleSteps = 32, int numCircles = 10, bool depthTest = true);

		VisualDebuggerUILabel* AddLabel(const Vector3& center, String text, Color color = Color::WHITE);

		VisualDebuggerNode* AddNode(Node* node, const float& scale, bool depthTest);

		VisualDebuggerCross* AddCross(const Vector3& center, const float& size, Color color, bool depthTest);

		VisualDebuggerTriangle* AddTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color, bool depthTest);

		VisualDebuggerBoundingBox* AddBoundingBox(BoundingBox boundingBox, Color color, bool depthTest);

		VisualDebuggerPolygon* AddPolygon(Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Color color, bool depthTest);

		VisualDebuggerPolyhedron* AddPolyhedron(Polyhedron polyhedron, Color color, bool depthTest);

		VisualDebuggerCylinder* AddCylinder(Vector3 position, float radius, float height, Color color, bool depthTest);

		VisualDebuggerFrustum* AddFrustum(Frustum frustum, Color color, bool depthTest);

		VisualDebuggerQuad* AddQuad(Vector3 center, float width, float height, Color color, bool depthTest);

		VisualDebuggerSphere* AddSphere(Sphere sphere, Color color, bool depthTest);

		VisualDebuggerSphereSector* AddSphereSector(Sphere sphere, Quaternion rotation, float angle, bool drawLines, Color color, bool depthTest);

		//Draws all the debug geometry
		void DrawDebugGeometry(DebugRenderer* debugRenderer);

		//enables or disables all drawing
		void SetEnabled(bool enabled);

		//sets the default lifetime in milliseconds for all objects created in the future.
		void SetObjectLifeTimeMs(unsigned int lifeTimeMs = 2000);

		//sets which camera to use for world to screen cordinate mapping
		void SetPrimaryCamera(Camera* camera);

	protected:

		void SetupAndAddObjectToList(VisualDebuggerObject* object, bool depthTest, Color color);

		List<SharedPtr<VisualDebuggerObject>> mDebuggerObjects;
		Timer mTimer;
		unsigned int mDefaultLifetimeMs = 2000;
		WeakPtr<Camera> mCamera;
	};


}
#endif
