


#include "../Precompiled.h"

#include "../Graphics/VisualDebugger.h"
#include "../Graphics/DebugRenderer.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Graphics.h"
#include "../Core/Context.h"

#include "../UI/UI.h"
#include "../UI/Text.h"


namespace Urho3D
{

VisualDebugger::VisualDebugger(Context* context) : Object(context)
{
	mTimer.Reset();
}

VisualDebugger::VisualDebuggerCircle* VisualDebugger::AddCircle(const Vector3& center, const Vector3& normal, float radius, const Color& color, int steps /*= 64*/, bool depthTest /*= true*/)
{
	VisualDebuggerCircle* newDbgObject = new VisualDebuggerCircle(this, context_);
	newDbgObject->mCenter = center;
	newDbgObject->mNormal = normal;
	newDbgObject->mRadius = radius;
	newDbgObject->mSteps = steps;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerLine* VisualDebugger::AddLine(const Vector3& start, const Vector3& end, const Color& color, bool depthTest /*= true*/)
{
	VisualDebuggerLine* newDbgObject = new VisualDebuggerLine(this, context_);
	newDbgObject->mStart = start;
	newDbgObject->mEnd = end;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerOrb* VisualDebugger::AddOrb(const Vector3& center, const float& radius, const Color& color, int circleSteps /*= 32*/, int numCircles /*= 10*/, bool depthTest /*= true*/)
{
	VisualDebuggerOrb* newDbgObject = new VisualDebuggerOrb(this, context_);
	newDbgObject->mCenter = center;
	newDbgObject->mRadius = radius;
	newDbgObject->mSteps = circleSteps;
	newDbgObject->mNumCircles = numCircles;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerUILabel* VisualDebugger::AddLabel(const Vector3& center, String text, Color color /*= Color::WHITE*/)
{
	VisualDebuggerUILabel* newDbgObject = new VisualDebuggerUILabel(this, context_);
	newDbgObject->mCenter = center;
	newDbgObject->mText = text;
	SetupAndAddObjectToList(newDbgObject, true, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerNode* VisualDebugger::AddNode(Node* node, const float& scale, bool depthTest)
{
	VisualDebuggerNode* newDbgObject = new VisualDebuggerNode(this, context_);
	newDbgObject->mNode = node;
	newDbgObject->mScale = scale;
	SetupAndAddObjectToList(newDbgObject, depthTest, Color::WHITE);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerCross* VisualDebugger::AddCross(const Vector3& center, const float& size, Color color, bool depthTest)
{
	VisualDebuggerCross* newDbgObject = new VisualDebuggerCross(this, context_);
	newDbgObject->mCenter = center;
	newDbgObject->mSize = size;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerTriangle* VisualDebugger::AddTriangle(Vector3 v1, Vector3 v2, Vector3 v3, Color color, bool depthTest)
{
	VisualDebuggerTriangle* newDbgObject = new VisualDebuggerTriangle(this, context_);
	newDbgObject->v1 = v1;
	newDbgObject->v2 = v2;
	newDbgObject->v3 = v3;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerBoundingBox* VisualDebugger::AddBoundingBox(BoundingBox boundingBox, Color color, bool depthTest)
{
	VisualDebuggerBoundingBox* newDbgObject = new VisualDebuggerBoundingBox(this, context_);
	newDbgObject->mBox = boundingBox;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerPolygon* VisualDebugger::AddPolygon(Vector3 v1, Vector3 v2, Vector3 v3, Vector3 v4, Color color, bool depthTest)
{
	VisualDebuggerPolygon* newDbgObject = new VisualDebuggerPolygon(this, context_);
	newDbgObject->v1 = v1;
	newDbgObject->v2 = v2;
	newDbgObject->v3 = v3;
	newDbgObject->v4 = v4;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerPolyhedron* VisualDebugger::AddPolyhedron(Polyhedron polyhedron, Color color, bool depthTest)
{
	VisualDebuggerPolyhedron* newDbgObject = new VisualDebuggerPolyhedron(this, context_);
	newDbgObject->mPolyhedron = polyhedron;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerCylinder* VisualDebugger::AddCylinder(Vector3 position, float radius, float height, Color color, bool depthTest)
{
	VisualDebuggerCylinder* newDbgObject = new VisualDebuggerCylinder(this, context_);
	newDbgObject->mPosition = position;
	newDbgObject->mRadius = radius;
	newDbgObject->mHeight = height;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerFrustum* VisualDebugger::AddFrustum(Frustum frustum, Color color, bool depthTest)
{
	VisualDebuggerFrustum* newDbgObject = new VisualDebuggerFrustum(this, context_);
	newDbgObject->mFrustum = frustum;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerQuad* VisualDebugger::AddQuad(Vector3 center, float width, float height, Color color, bool depthTest)
{
	VisualDebuggerQuad* newDbgObject = new VisualDebuggerQuad(this, context_);
	newDbgObject->mCenter = center;
	newDbgObject->width = width;
	newDbgObject->height = height;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerSphere* VisualDebugger::AddSphere(Sphere sphere, Color color, bool depthTest)
{
	VisualDebuggerSphere* newDbgObject = new VisualDebuggerSphere(this, context_);
	newDbgObject->mSphere = sphere;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

VisualDebugger::VisualDebuggerSphereSector* VisualDebugger::AddSphereSector(Sphere sphere, Quaternion rotation, float angle, bool drawLines, Color color, bool depthTest)
{
	VisualDebuggerSphereSector* newDbgObject = new VisualDebuggerSphereSector(this, context_);
	newDbgObject->mSphere = sphere;
	newDbgObject->mRotation = rotation;
	newDbgObject->mAngle = angle;
	newDbgObject->mDrawLines = drawLines;
	SetupAndAddObjectToList(newDbgObject, depthTest, color);
	return newDbgObject;
}

void VisualDebugger::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	auto i = mDebuggerObjects.Begin();
	while (i != mDebuggerObjects.End())
	{
		i->Get()->DrawDebugGeometry(debugRenderer);

		//check if the object has exceeded its lifetime and if so remove.
		if ((i->Get()->creationTimeMS + i->Get()->mLifetimeMS) <= mTimer.GetMSec(false)) {
			i->Get()->TearDown();
			mDebuggerObjects.Erase(i);
		}

		i++;
	}
}

void VisualDebugger::SetEnabled(bool enabled)
{
	auto i = mDebuggerObjects.Begin();
	while (i != mDebuggerObjects.End())
	{
		i->Get()->SetEnabled(enabled);
		i++;
	}
}

void VisualDebugger::SetObjectLifeTimeMs(unsigned int lifeTimeMs)
{
	mDefaultLifetimeMs = lifeTimeMs;
}

void VisualDebugger::SetPrimaryCamera(Camera* camera)
{
	mCamera = camera;
}

void VisualDebugger::SetupAndAddObjectToList(VisualDebuggerObject* object, bool depthTest, Color color)
{
	mDebuggerObjects.PushFront(SharedPtr<VisualDebuggerObject>(object));
	object->creationTimeMS = mTimer.GetMSec(false);
	object->mDepthTest = depthTest;
	object->mColor = color;
	object->mLifetimeMS = mDefaultLifetimeMs;
	object->Setup();
}



VisualDebugger::VisualDebuggerUILabel::VisualDebuggerUILabel(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

VisualDebugger::VisualDebuggerUILabel::~VisualDebuggerUILabel()
{

}

void VisualDebugger::VisualDebuggerUILabel::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	UpdatePosition();
}

void VisualDebugger::VisualDebuggerUILabel::Setup()
{
	VisualDebuggerObject::Setup();
	mUIText = context_->GetSubsystem<UI>()->GetRoot()->CreateChild<Text>();
	mUIText->SetText(mText);
	mUIText->SetFont("Fonts/Anonymous Pro.ttf");
	mUIText->SetColor(mColor);
	//mUIText->SetEnabled(mEnabled);
	mUIText->SetVisible(mEnabled);
	UpdatePosition();
}

void VisualDebugger::VisualDebuggerUILabel::TearDown()
{
	mUIText->Remove();
	mUIText.Detach();
}

void VisualDebugger::VisualDebuggerUILabel::SetEnabled(bool enabled)
{
	VisualDebuggerObject::SetEnabled(enabled);
	//mUIText->SetEnabled(mEnabled);
	mUIText->SetVisible(mEnabled);
}

void VisualDebugger::VisualDebuggerUILabel::UpdatePosition()
{
	//default to screen middle.
	Vector2 screenPoint = Vector2(GetSubsystem<Graphics>()->GetSize())*0.5f;
	
	if (mVisDebugger->mCamera.NotNull()) {
		screenPoint = mVisDebugger->mCamera->WorldToScreenPoint(mCenter);
		//screen point has range of 0-1. - convert back to pixels
		screenPoint *= Vector2(GetSubsystem<Graphics>()->GetSize());
	}
	

	mUIText->SetPosition(IntVector2(screenPoint.x_, screenPoint.y_));
}


VisualDebugger::VisualDebuggerNode::VisualDebuggerNode(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerNode::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddNode(mNode, mScale);
}

VisualDebugger::VisualDebuggerOrb::VisualDebuggerOrb(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerOrb::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	for (auto i = 0; i < mNumCircles; i++) {
		debugRenderer->AddCircle(mCenter, Vector3(Random(-1, 1), Random(-1, 1), Random(-1, 1)).Normalized(), mRadius, mColor, mSteps, mDepthTest);
	}
}


VisualDebugger::VisualDebuggerSphereSector::VisualDebuggerSphereSector(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerSphereSector::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddSphereSector(mSphere, mRotation, mAngle, mDrawLines, mColor, mDepthTest);
}

VisualDebugger::VisualDebuggerSphere::VisualDebuggerSphere(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerSphere::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddSphere(mSphere, mColor, mDepthTest);
}

VisualDebugger::VisualDebuggerQuad::VisualDebuggerQuad(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerQuad::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddQuad(mCenter, width, height, mColor, mDepthTest);
}

VisualDebugger::VisualDebuggerFrustum::VisualDebuggerFrustum(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerFrustum::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddFrustum(mFrustum, mColor, mDepthTest);
}

VisualDebugger::VisualDebuggerCylinder::VisualDebuggerCylinder(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerCylinder::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddCylinder(mPosition, mRadius, mHeight, mColor, mDepthTest);
}


VisualDebugger::VisualDebuggerPolyhedron::VisualDebuggerPolyhedron(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerPolyhedron::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddPolyhedron(mPolyhedron, mColor, mDepthTest);
}


VisualDebugger::VisualDebuggerPolygon::VisualDebuggerPolygon(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerPolygon::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddPolygon(v1, v2, v3, v4, mColor, mDepthTest);
}


VisualDebugger::VisualDebuggerCross::VisualDebuggerCross(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerCross::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddCross(mCenter, mSize, mColor, mDepthTest);
}


VisualDebugger::VisualDebuggerTriangle::VisualDebuggerTriangle(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerTriangle::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddTriangle(v1, v2, v3, mColor, mDepthTest);
}

VisualDebugger::VisualDebuggerBoundingBox::VisualDebuggerBoundingBox(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerBoundingBox::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddBoundingBox(mBox, mColor, mDepthTest, mSolid);
}

VisualDebugger::VisualDebuggerLine::VisualDebuggerLine(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerLine::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddLine(mStart, mEnd, mColor, mDepthTest);
}


VisualDebugger::VisualDebuggerCircle::VisualDebuggerCircle(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerCircle::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddCircle(mCenter, mNormal, mRadius, mColor, mSteps, mDepthTest);
}

VisualDebugger::VisualDebuggerObject::VisualDebuggerObject(VisualDebugger* visDebugger, Context* context_) : Object(context_), mVisDebugger(visDebugger)
{
}

void VisualDebugger::VisualDebuggerObject::DrawDebugGeometry(DebugRenderer* debugRenderer)
{

}

void VisualDebugger::VisualDebuggerObject::Setup()
{

}

void VisualDebugger::VisualDebuggerObject::TearDown()
{

}

void VisualDebugger::VisualDebuggerObject::SetEnabled(bool enabled)
{
	mEnabled = enabled;
}

void VisualDebugger::VisualDebuggerObject::SetLifeTimeMs(unsigned int lifeTimeMs)
{
	mLifetimeMS = lifeTimeMs;
}

void VisualDebugger::VisualDebuggerObject::SetColor(Color color)
{
	mColor = color;
}

VisualDebugger::VisualDebuggerRay::VisualDebuggerRay(VisualDebugger* visDebugger, Context* context_) : VisualDebuggerObject(visDebugger, context_)
{

}

void VisualDebugger::VisualDebuggerRay::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
	VisualDebuggerObject::DrawDebugGeometry(debugRenderer);
	debugRenderer->AddLine(mRay.origin_, mRay.origin_ + mRay.direction_, mColor, mDepthTest);
}

}